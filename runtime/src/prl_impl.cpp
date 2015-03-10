/*
 * Copyright (c) 2014, ARM Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cassert>
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <limits>
#include <tuple>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <unordered_map>
#include "prl.h"
#include "ocl_utilities.h"

#ifdef THREAD_SAFE
#include<mutex>
#endif

#include "prl_impl.h"



#define UNUSED(exp) (void)(exp)
#define OPENCL_ASSERT(exp) do {if (exp != CL_SUCCESS) {std::cerr << "OpenCL error: " << opencl_error_string(exp) << std::endl;} assert (exp == CL_SUCCESS);} while (0)

typedef cl_int cl_error_code;

const int ERROR_CODE = 2;

void die (const char * error)
{
    std::cerr << "ERROR: " << error << std::endl;
    exit(ERROR_CODE);
}


typedef typename std::chrono::steady_clock::time_point::duration cpu_duration_t;
typedef typename std::chrono::duration<cl_long, std::nano> gpu_duration_t;
typedef std::chrono::duration<double, std::milli> common_duration_t;

template<typename T, typename P>
std::ostream& operator<<(std::ostream& os, std::chrono::duration<T,P> dt) {
	auto ms = std::chrono::duration_cast<common_duration_t>(dt);
	os << std::setw(8) <<  std::fixed << std::setprecision(3) << ms.count() << " ms";
	return os;
}

class stopwatch {
public:
	typedef typename std::chrono::steady_clock::time_point::duration duration_t;
	typedef typename std::chrono::steady_clock::time_point::time_point timepoint_t;
	static_assert(std::chrono::steady_clock::is_steady, "Monotonic clock required");
	
private:
	bool enabled;
	duration_t *counter;
	timepoint_t start;
	
	stopwatch(const stopwatch &that);
	
public:
	stopwatch() : enabled(false), counter(), start() {}
	
	stopwatch(duration_t &counter, bool enabled) : enabled(enabled), counter(&counter) {
		if (enabled)
			start = std::chrono::steady_clock::now();
	}
	
	~stopwatch() {
		if (!enabled)
			return; 
		
		auto stop = std::chrono::steady_clock::now();
		auto duration = (stop - start);
		*counter += duration;
	}
	
	// move ctor
	stopwatch(stopwatch &&that) : enabled(true), counter(that.counter), start(that.start) {
		that.enabled = false;
	}
	
	const stopwatch &operator=(stopwatch &&that) { 
		if (this != &that) {
			enabled = that.enabled;
			counter = that.counter;
			start = that.start;
			that.enabled = false;
		}
		
		return *this;
	}
};

template<typename T>
static common_duration_t median_ms(const std::vector<T> &vec) {
	auto cpy = vec;
	std::sort(cpy.begin(), cpy.end());
	auto len = cpy.size();
	if (len % 2 == 1) {
		return std::chrono::duration_cast<common_duration_t>(cpy.at(len / 2));
	}

	return std::chrono::duration_cast<common_duration_t>(cpy.at(len/2-1))
			+ std::chrono::duration_cast<common_duration_t>(cpy.at(len/2)/2);
}





class variation_percent_t {
public:
	double ratio;
	variation_percent_t() : ratio(0) {} // No variation ctor
	variation_percent_t(double ratio) :ratio(ratio) {}
};

static std::ostream& operator<<(std::ostream& os, variation_percent_t var) {
	os << "\u00B1" << std::setw(5) << std::fixed << std::setprecision(2) << (100*var.ratio) << "%";
	return os;
}

static double sqr(double val) { return val*val; }

template<typename T>
static variation_percent_t variation_percent(const std::vector<T> &vec) {
	auto n = vec.size();
	if (n==0)
		return variation_percent_t();

	T sum = T::zero();
	double sqrsum = 0;
	for (auto x : vec) {
		sum += x;
		sqrsum += sqr(x.count());
	}

	auto avg = static_cast<double>(sum.count())/n;
	if (avg==0)
		return variation_percent_t();
	auto var = sqrsum/n - sqr(avg);
	return variation_percent_t(sqrt(var)/avg);
}



bool __int_gpu_profiling_enabled();
bool __int_blocking_enabled();
void __int_add_event(cl_event event);


struct __int_pencil_cl_mem
{
    cl_mem dev;
    void *host;
    void *exposed_ptr;
    size_t size;
    bool cached;

    __int_pencil_cl_mem (cl_mem dev, size_t size, bool cached):
        dev (dev), host (NULL), exposed_ptr (NULL), size (size), cached (cached)
    {
    }

    void map (cl_command_queue queue, bool reset)
    {
        if (host)
        {
            return;
        }
        cl_event event = NULL;
        cl_error_code err;
        host = clEnqueueMapBuffer (queue, dev, __int_blocking_enabled() ? CL_TRUE : CL_FALSE,
                                   CL_MAP_READ | CL_MAP_WRITE, 0, size,
                                   0, NULL, __int_gpu_profiling_enabled() ? &event : NULL, &err);
        OPENCL_ASSERT (err);
        __int_add_event(event);

        if (reset)
        {
            exposed_ptr = host;
        }
        assert (host);
        assert (exposed_ptr == host);
    }

    void unmap (cl_command_queue queue)
    {
        if (!host)
        {
            return;
        }
        assert (dev);

        auto profiling = __int_gpu_profiling_enabled();
        auto blocking = __int_blocking_enabled();

        cl_event event = NULL;
        cl_error_code err = clEnqueueUnmapMemObject (queue, dev, host, 0, NULL,
        		profiling || blocking ? &event : NULL);
        OPENCL_ASSERT (err);

        if (blocking) {
        	err = clWaitForEvents (1, &event);
        	OPENCL_ASSERT (err);
        }
        __int_add_event(event); // Will release if non-null

        host = NULL;
    }
};


class memory_manager
{

    std::unordered_map<void*, prl_cl_mem> cache;
    size_t unmanaged_buffer_count;

#ifdef THREAD_SAFE
    std::mutex lock;
#endif

    cl_mem alloc_dev_buffer (cl_context ctx, cl_mem_flags flags, size_t size,
                             void *host_ptr)
    {
        cl_error_code err;
        if (!(flags & (CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR)))
        {
            host_ptr = NULL;
        }
        cl_mem res = clCreateBuffer (ctx, flags, size, host_ptr, &err);
        OPENCL_ASSERT (err);
        assert (res);
        return res;
    }

public:
    void *alloc (cl_context ctx, cl_command_queue queue, size_t size)
    {
#ifdef THREAD_SAFE
        std::unique_lock<std::mutex> lck(lock);
#endif
        cl_mem dev_buff =
          alloc_dev_buffer (ctx, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                            size, NULL);
        prl_cl_mem  buff = new __int_pencil_cl_mem (dev_buff, size, true);
        buff->map (queue, true);
        assert (buff->exposed_ptr);

        cache[buff->exposed_ptr] = buff;
        return buff->exposed_ptr;
    }

    prl_cl_mem dev_alloc (cl_context ctx, cl_mem_flags flags, size_t size,
                             void *host_ptr, cl_command_queue queue)
    {
#ifdef THREAD_SAFE
        std::unique_lock<std::mutex> lck(lock);
#endif
        auto cached = cache.find (host_ptr);
        if (cached != cache.end ())
        {
            assert (cached->second->size == size);
            cached->second->unmap (queue);
            return cached->second;
        }
        cl_mem new_buffer = alloc_dev_buffer (ctx, flags, size, host_ptr);
        unmanaged_buffer_count++;
        return new __int_pencil_cl_mem (new_buffer, size, false);
    }

    void copy_to_device (cl_command_queue queue, prl_cl_mem dev, size_t size,
                         void *host)
    {
        if (dev->exposed_ptr != NULL)
        {
            assert (host == dev->exposed_ptr);
            assert (dev->cached);
            dev->unmap (queue);
            return;
        }
        cl_event event = NULL;
        cl_error_code err = clEnqueueWriteBuffer (queue, dev->dev, __int_blocking_enabled() ? CL_TRUE : CL_FALSE, 0,
                                                  size, host, 0, NULL, __int_gpu_profiling_enabled() ? &event : NULL);
        OPENCL_ASSERT (err);

        __int_add_event(event);
    }

    void copy_to_host (cl_command_queue queue, prl_cl_mem dev, size_t size,
                       void *host)
    {
        if (dev->exposed_ptr)
        {
            assert (host == dev->exposed_ptr);
            assert (dev->cached);
            dev->map (queue, false);
            return;
        }
        cl_event event = NULL;
        cl_error_code err = clEnqueueReadBuffer (queue, dev->dev, __int_blocking_enabled() ? CL_TRUE : CL_FALSE, 0,
                                                 size, host, 0, NULL, __int_gpu_profiling_enabled() ? &event : NULL);
        OPENCL_ASSERT (err);

        //err = clWaitForEvents (1, &event);
        //OPENCL_ASSERT (err);
        //err = clReleaseEvent (event);
        //OPENCL_ASSERT (err);
        __int_add_event(event);
    }

    void free (void *ptr, cl_command_queue queue)
    {
#ifdef THREAD_SAFE
        std::unique_lock<std::mutex> lck(lock);
#endif
        prl_cl_mem buff = cache[ptr];

        assert (buff->exposed_ptr == ptr);
        buff->unmap (queue);

        cl_error_code err = clReleaseMemObject (buff->dev);
        OPENCL_ASSERT (err);

        delete buff;
        cache.erase (ptr);
    }

    memory_manager (): unmanaged_buffer_count (0)
    {
    }

    ~memory_manager ()
    {
        assert (cache.size () == 0);
        assert (unmanaged_buffer_count == 0);
    }

    void dev_free (prl_cl_mem buff)
    {
        if (!buff->cached)
        {
            assert (unmanaged_buffer_count > 0);
            unmanaged_buffer_count--;
            cl_error_code err = clReleaseMemObject (buff->dev);
            assert (buff->exposed_ptr == NULL);
            OPENCL_ASSERT (err);
            delete buff;
        }
    }

};


struct __int_pencil_cl_kernel
{
    cl_kernel kernel;
    __int_pencil_cl_kernel (cl_kernel kernel): kernel (kernel) {}
};

struct __int_pencil_cl_program
{
private:
    std::string prog_file;
    std::string opts;
    cl_program prog;
    std::unordered_map<std::string, prl_cl_kernel> kernel_name_idx;

    const char * source;

public:

    __int_pencil_cl_program (const char *prog_file, const char *opts,
                             cl_context ctx, cl_device_id dev):
        prog_file (prog_file), opts (opts), source(NULL)
    {
        prog = opencl_build_program_from_file (ctx, dev, prog_file, opts);
    }

    __int_pencil_cl_program (const char *source, size_t size, const char *opts,
                             cl_context ctx, cl_device_id dev):
        opts (opts), source(source)
    {
        prog = opencl_build_program_from_string (ctx, dev, source, size, opts);
    }



    prl_cl_kernel get_kernel (const char *name)
    {
        assert (prog);
        std::string key (name);
        auto cached = kernel_name_idx.find (key);
        if (cached != kernel_name_idx.end ())
        {
            return cached->second;
        }
        cl_error_code err;
        cl_kernel kernel = clCreateKernel (prog, name, &err);
        OPENCL_ASSERT (err);
        assert (kernel);
        prl_cl_kernel res = new __int_pencil_cl_kernel (kernel);
        kernel_name_idx[key] = res;
        return res;
    }

    bool match (const char *prog_file, const char *opts)
    {
        return this->prog_file ==prog_file && this->opts == opts;
    }

    bool match_source (const char * source, const char * opts)
    {
        return source != NULL && source == this->source && opts == this->opts;
    }

    ~__int_pencil_cl_program ()
    {
        for (auto iter = kernel_name_idx.begin ();
             iter != kernel_name_idx.end (); ++iter)
        {
            cl_error_code err = clReleaseKernel (iter->second->kernel);
            OPENCL_ASSERT (err);
            delete iter->second;
        }
        cl_error_code err = clReleaseProgram (prog);
        OPENCL_ASSERT (err);
    }
};

class program_cache
{
#ifdef THREAD_SAFE
    std::mutex lock;
#endif
    std::vector<prl_cl_program> programs;
public:

    prl_cl_program get_program (const char *file, const char *opts,
                                   cl_context ctx, cl_device_id dev)
    {
#ifdef THREAD_SAFE
        std::unique_lock<std::mutex> lck(lock);
#endif
        for (auto iter = programs.begin ();
             iter != programs.end (); ++iter)
        {
            if ( (*iter)->match (file, opts))
            {
                return (*iter);
            }
        }
        programs.push_back (new __int_pencil_cl_program (file, opts, ctx, dev));
        return programs.back ();
    }

    prl_cl_program get_program (const char *program, size_t size,
                                   const char *opts,
                                   cl_context ctx, cl_device_id dev)
    {
#ifdef THREAD_SAFE
        std::unique_lock<std::mutex> lck(lock);
#endif
        for (auto iter = programs.begin ();
             iter != programs.end (); ++iter)
        {
            if ( (*iter)->match_source (program, opts))
            {
                return (*iter);
            }
        }
        programs.push_back (new __int_pencil_cl_program (program, size, opts, ctx, dev));
        return programs.back ();

    }

    void clear ()
    {
#ifdef THREAD_SAFE
        std::unique_lock<std::mutex> lck(lock);
#endif
        for (auto iter = programs.begin ();
             iter != programs.end (); ++iter)
        {
            delete *iter;
        }
        programs.clear ();
    }

    ~program_cache ()
    {
        assert (programs.size () == 0);
    }
};

void __ocl_report_error (const char *errinfo, const void *private_info,
                         size_t cb, void *user_data)
{
    UNUSED (private_info);
    UNUSED (cb);
    UNUSED (user_data);
    std::cerr << errinfo << std::endl;
}


class session
{
    cl_context context;
    cl_device_id device;
    cl_command_queue queue;

    memory_manager memory;
    program_cache programs;

    cl_device_type current_device_type;
    bool cpu_profiling_enabled;
    bool gpu_profiling_enabled;
    bool blocking;

    std::vector<cl_event> pending_events;

public: // Hack for the moment to avoid writing a lot of getters
    cpu_duration_t accumulated_init_time; // clGetPlatformIDs, clGetDeviceIDs, clCreateContext, clCreateCommandQueue
    cpu_duration_t accumulated_release_time; // clReleaseCommandQueue, clReleaseContext
    cpu_duration_t accumulated_alloc_time; // clCreateBuffer, clEnqueueMapBuffer, clEnqueueUnmapMemObject
    cpu_duration_t accumulated_free_time;  // clReleaseMemObject, clEnqueueUnmapMemObject
	cpu_duration_t accumulated_overhead_time; // clCreateKernel, clSetKernelArg
	cpu_duration_t accumulated_compilation_time; // clCreateProgramWithSource, clBuildProgram, clGetProgramBuildInfo
	cpu_duration_t accumulated_copy_to_device_time; // clEnqueueWriteBuffer, clEnqueueUnmapMemObject
	cpu_duration_t accumulated_copy_to_host_time; // clEnqueueReadBuffer, clEnqueueMapBuffer
	cpu_duration_t accumulated_compute_time; // clEnqueueNDRangeKernel
	cpu_duration_t accumulated_waiting_time; // clFinish

	gpu_duration_t accumulated_gpu_copy_to_device_time; // CL_COMMAND_WRITE_BUFFER, CL_COMMAND_WRITE_IMAGE, CL_COMMAND_MAP_BUFFER, CL_COMMAND_MAP_IMAGE
	gpu_duration_t accumulated_gpu_copy_to_host_time; // CL_COMMAND_READ_BUFFER, CL_COMMAND_READ_IMAGE, CL_COMMAND_UNMAP_BUFFER, CL_COMMAND_UNMAP_IMAGE
	gpu_duration_t accumulated_gpu_compute_time; // CL_COMMAND_NDRANGE_KERNEL, CL_COMMAND_TASK, CL_COMMAND_NATIVE_KERNEL
	gpu_duration_t accumulated_gpu_unused_time;
	gpu_duration_t accumulated_gpu_working_time;

public:
    session (int n_devices, const cl_device_type * devices, bool cpu_profiling_enabled, bool gpu_profiling_enabled, bool blocking)
		: cpu_profiling_enabled(cpu_profiling_enabled), gpu_profiling_enabled(gpu_profiling_enabled), blocking(blocking),
		  accumulated_init_time(0), accumulated_release_time(0), accumulated_alloc_time(0), accumulated_free_time(0),
		  accumulated_overhead_time(0), accumulated_copy_to_device_time(0), accumulated_copy_to_host_time(0), accumulated_compute_time(0),
		accumulated_gpu_copy_to_device_time(0), accumulated_gpu_copy_to_host_time(0), accumulated_gpu_compute_time(0),accumulated_gpu_unused_time(0),accumulated_gpu_working_time(0)
    {
		stopwatch wtch(accumulated_init_time, cpu_profiling_enabled);

        cl_uint num_devices;
        cl_error_code err;
        cl_platform_id platform;
        err = clGetPlatformIDs (1, &platform, NULL);
        OPENCL_ASSERT (err);
        for (int i = 0; i < n_devices; ++i)
        {
            cl_device_type device_type = devices[i];
            err = clGetDeviceIDs (platform, device_type, 1, &device,
                                  &num_devices);
            if (CL_SUCCESS != err)
            {
                continue;
            }
            assert (num_devices > 0);
            context = clCreateContext (NULL, 1, &device, __ocl_report_error,
                                       NULL, &err);
            current_device_type = device_type;
            break;
        }
        OPENCL_ASSERT (err);

        assert (context);
        queue = clCreateCommandQueue (context, device, gpu_profiling_enabled ? CL_QUEUE_PROFILING_ENABLE : 0, &err);
        OPENCL_ASSERT (err);
        assert (queue);
    }

    void release() {
		stopwatch wtch(accumulated_release_time, cpu_profiling_enabled);

        programs.clear ();

        if (queue) {
        	auto err = clReleaseCommandQueue (queue);
        	OPENCL_ASSERT (err);
        }

        if (context) {
        	auto err = clReleaseContext (context);
        	OPENCL_ASSERT (err);
        }

        queue = NULL;
        context = NULL;
    }

    ~session ()
    {
    	release();
    }

    stopwatch get_overhead_stopwatch() {
		return stopwatch(accumulated_overhead_time, cpu_profiling_enabled);
	}

    stopwatch get_compilation_stopwatch() {
		return stopwatch(accumulated_compilation_time, cpu_profiling_enabled);
	}
	
	stopwatch get_compute_stopwatch() {
		return stopwatch(accumulated_compute_time, cpu_profiling_enabled);
	}

	stopwatch get_alloc_stopwatch() {
		return stopwatch(accumulated_alloc_time, cpu_profiling_enabled);
	}
    
	stopwatch get_free_stopwatch() {
		return stopwatch(accumulated_free_time, cpu_profiling_enabled);
	}


    cl_device_type get_current_device_type () const
    {
        return current_device_type;
    }

    prl_cl_program create_or_get_program (const char *path, const char *opts)
    {
		stopwatch wtch(accumulated_compilation_time, cpu_profiling_enabled);
        return programs.get_program (path, opts, context, device);
    }

    prl_cl_program create_or_get_program (const char *program, size_t size, const char *opts)
    {
		stopwatch wtch(accumulated_compilation_time, cpu_profiling_enabled);
        return programs.get_program (program, size, opts, context, device);
    }

    void release_program (prl_cl_program prog)
    {
        /* All programs are cached by the runtime and released when the
           session is released.  */
        UNUSED (prog);
    }

    void release_kernel (prl_cl_kernel kernel)
    {
        /* All kernels are cached by the runtime and released when the
           session is released.  */
        UNUSED (kernel);
    }

    void *alloc_and_return_host_ptr (size_t size)
    {
    	stopwatch wtch(accumulated_alloc_time, cpu_profiling_enabled);
        return memory.alloc (context, queue, size);
    }

    prl_cl_mem alloc_and_return_dev_ptr (cl_mem_flags flags, size_t size,
                                            void *host_ptr)
    {
    	stopwatch wtch(accumulated_alloc_time, cpu_profiling_enabled);
        return memory.dev_alloc (context, flags, size, host_ptr, queue);
    }

    void free_host_ptr (void *ptr)
    {
    	stopwatch wtch(accumulated_free_time, cpu_profiling_enabled);
        memory.free (ptr, queue);
    }

    void free_dev_buffer (prl_cl_mem dev)
    {
		stopwatch wtch(accumulated_free_time, cpu_profiling_enabled);
        memory.dev_free (dev);
    }

    void copy_to_device (prl_cl_mem dev, size_t size, void *host)
    {
		stopwatch wtch(accumulated_copy_to_device_time, cpu_profiling_enabled);
        memory.copy_to_device (queue, dev, size, host);
    }

    void copy_to_host (prl_cl_mem dev, size_t size, void *host)
    {
		stopwatch wtch(accumulated_copy_to_host_time, cpu_profiling_enabled);
        memory.copy_to_host (queue, dev, size, host);
    }

    cl_command_queue get_command_queue ()
    {
        return queue;
    }
    
    bool is_cpu_profiling_enabled() const {
    	return cpu_profiling_enabled;
    }

    bool is_gpu_profiling_enabled() const {
    	return gpu_profiling_enabled;
    }

    bool is_blocking() const {
    	return blocking;
    }

    void add_event(cl_event event) {
    	assert(!gpu_profiling_enabled || event); // event must be non-null with gpu_profiling
    	if (!event)
    		return;

    	if (gpu_profiling_enabled) {
    		pending_events.push_back(event);
    	} else {
    		auto err = clReleaseEvent(event);
    		OPENCL_ASSERT(err);
    	}
    }

    typedef std::tuple<cl_ulong,cl_ulong,cl_command_type> tuple_t;
    void accumulate_durations(const std::vector<tuple_t> &exec_periods, const std::function<bool(const tuple_t&)> &filter, cl_ulong start_queing, gpu_duration_t &accumulated_working, gpu_duration_t &accumulated_idle) {
    	cl_ulong prev_end;
    	cl_ulong last_end;
    	cl_ulong last_filtered_end;
      		bool first = true;
        	for (auto const &p : exec_periods) {
        		auto start = std::get<0>(p);
        		auto end = std::get<1>(p);
        		auto cat = std::get<2>(p);
        		if (first) {
        			prev_end = start;
        			first = false;
        		}
        		last_end = end;

        		if (!filter(p))
        			continue;
        		last_filtered_end = end;

        		if (prev_end <= start) {
        			// Case 1: |<- prev ->|  |<- cur ->|
        			assert(start <= end);

        			auto cur_duration = gpu_duration_t(end) - gpu_duration_t(start);
        			auto unused_duration = gpu_duration_t(start) - gpu_duration_t(prev_end);

        			accumulated_working += cur_duration;
        			accumulated_idle += unused_duration;

        			prev_end = end;
        		} else if (end <= prev_end) {
            		// Case 2: |<-   prev   ->|
            		//           |<- cur ->|
        			assert(start <= end);
        		} else {
    				// Case 3: |<- prev ->|
    				//                |<- cur ->|
        			assert(start <= prev_end);
        			assert(prev_end <= end);

        			auto cur_duration = gpu_duration_t(end) - gpu_duration_t(prev_end);
        			accumulated_working += cur_duration;

        			prev_end = end;
        		}
        	}

        	accumulated_idle += gpu_duration_t(last_filtered_end) - gpu_duration_t(last_end);
    }

    void wait_pending() {
    	cl_int err;

    	{
    		stopwatch wtch(accumulated_waiting_time, cpu_profiling_enabled);
    		err = clFinish(queue);
    		OPENCL_ASSERT(err);
    	}

    	if (!gpu_profiling_enabled)
    		return;

    	{
    	stopwatch wtch(accumulated_overhead_time, cpu_profiling_enabled);

    	if (pending_events.empty())
    		return;

    	cl_ulong start_queing = std::numeric_limits<cl_ulong>::max();
    	std::vector<tuple_t> exec_periods;
    	exec_periods.reserve(pending_events.size());
    	for (auto e : pending_events) {
    		cl_ulong queued, submit, start, end;
    		cl_command_type cmdty;
    		cl_int status;

    		err = clGetEventProfilingInfo(e, CL_PROFILING_COMMAND_QUEUED, sizeof(queued), &queued, NULL);
    		OPENCL_ASSERT(err);
    		err = clGetEventProfilingInfo(e, CL_PROFILING_COMMAND_SUBMIT, sizeof(submit), &submit, NULL);
    		OPENCL_ASSERT(err);
    		err = clGetEventProfilingInfo(e, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
    		OPENCL_ASSERT(err);
    		err = clGetEventProfilingInfo(e, CL_PROFILING_COMMAND_END, sizeof(end), &end, NULL);
    		OPENCL_ASSERT(err);
    		err = clGetEventInfo(e, CL_EVENT_COMMAND_TYPE, sizeof(cmdty), &cmdty, NULL);
    		OPENCL_ASSERT(err);
    		err = clGetEventInfo(e, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(status), &status, NULL);
    		OPENCL_ASSERT(err);

    		assert(status == CL_COMPLETE);

    		// Ensure consistency
    		if (start < queued)
    			queued = start; // If this happens it might be because the clock used for host and device are not synchronized; we prioritize the device clock (START and END events)
    		if (end < start)
    			end = start; // Don't know how this could happen

    		// Check consistency
    		if (!(queued <= start && start <= end)) {
    			std::cout << "CL_PROFILING_COMMAND_QUEUED = " << queued << "\n";
    			std::cout << "CL_PROFILING_COMMAND_SUBMIT = " << submit << "\n";
    			std::cout << "CL_PROFILING_COMMAND_START  = " << start << "\n";
    			std::cout << "CL_PROFILING_COMMAND_END    = " << end << "\n";
    			std::cout << std::flush;
    		}
    		assert(queued <= start);
    		assert(start <= end);

    		start_queing = std::min(start_queing, queued);
    		exec_periods.emplace_back(start,end,cmdty);

    		err = clReleaseEvent(e);
    		OPENCL_ASSERT(err);
    	}
    	pending_events.clear();

    	// sort according start time
    	std::sort(exec_periods.begin(), exec_periods.end(), [](const tuple_t &x, const tuple_t &y) -> int { return std::get<0>(x) < std::get<0>(y); } );

    	accumulate_durations(exec_periods, [](const tuple_t &p)->bool { return true; }, start_queing, accumulated_gpu_working_time, accumulated_gpu_unused_time);
    	gpu_duration_t dummy;
       	accumulate_durations(exec_periods, [](const tuple_t &p)->bool { return std::get<2>(p) == CL_COMMAND_NDRANGE_KERNEL ||  std::get<2>(p) == CL_COMMAND_TASK || std::get<2>(p) == CL_COMMAND_NATIVE_KERNEL; }, start_queing, accumulated_gpu_compute_time, dummy);
       	accumulate_durations(exec_periods, [](const tuple_t &p)->bool { return std::get<2>(p) == CL_COMMAND_WRITE_BUFFER || std::get<2>(p) == CL_COMMAND_WRITE_IMAGE || std::get<2>(p) == CL_COMMAND_MAP_BUFFER|| std::get<2>(p) == CL_COMMAND_MAP_IMAGE; }, start_queing, accumulated_gpu_copy_to_device_time, dummy);
       	accumulate_durations(exec_periods, [](const tuple_t &p)->bool { return std::get<2>(p) == CL_COMMAND_READ_BUFFER || std::get<2>(p) == CL_COMMAND_READ_IMAGE || std::get<2>(p) == CL_COMMAND_UNMAP_MEM_OBJECT;  }, start_queing, accumulated_gpu_copy_to_host_time, dummy);
    	}
    }

protected:
    std::string get_device_string_property(cl_device_info prop) const {
    	// Get the length
    	size_t size=0;
    	auto err = clGetDeviceInfo(device, prop, 0, NULL, &size);
    	if (err!=CL_INVALID_VALUE) {
    		OPENCL_ASSERT(err);
    	}

    	// Get the data
    	char buf[size+1];
    	err = clGetDeviceInfo(device, prop,size,  &buf,NULL);
    	OPENCL_ASSERT(err);

    	return buf;
    }

    std::string get_platform_string_property(cl_platform_id platform, cl_platform_info prop) const {
    	// Get the length
    	size_t size = 0;
    	auto err = clGetPlatformInfo(platform, prop, 0, NULL, &size);
    	if (err!=CL_INVALID_VALUE) {
    		OPENCL_ASSERT(err);
    	}

    	// Get the data
    	char buf[size+1];
    	err =  clGetPlatformInfo(platform, prop, size, &buf, NULL);
    	OPENCL_ASSERT(err);

    	return buf;
    }

public:
    std::string get_device_name() const {
    	return get_device_string_property(CL_DEVICE_NAME);
    }

    std::string get_device_vendor() const {
    	return get_device_string_property(CL_DEVICE_VENDOR);
    }

    std::string get_device_version() const {
    	return get_device_string_property(CL_DEVICE_VERSION);
    }

    std::string get_driver_version() const {
    	return get_device_string_property(CL_DRIVER_VERSION);
    }

    cl_platform_id get_platform_id() const {
    	cl_platform_id platform;
    	auto err = clGetDeviceInfo(device, CL_DEVICE_PLATFORM,sizeof(platform),  &platform,NULL);
    	OPENCL_ASSERT(err);
    	return platform;
    }

    std::string get_platform_name() const {
    	auto platform = get_platform_id();
    	return get_platform_string_property(platform, CL_PLATFORM_NAME);
    }

    std::string get_platform_vendor() const {
    	auto platform = get_platform_id();
    	return get_platform_string_property(platform, CL_PLATFORM_VENDOR);
    }

    std::string get_platform_version() const {
    	auto platform = get_platform_id();
    	return get_platform_string_property(platform, CL_PLATFORM_VERSION);
    }


    void dump_device() {
    	std::cout << "Platform: " << get_platform_vendor() << " " << get_platform_name() << " (" << get_platform_version() << ")\n";
    	std::cout << "Device:   " << get_device_vendor() << " " << get_device_name() << " (Driver version " << get_driver_version() << ", " << get_device_version() << ")\n";
     }

    void dump_stats(const char *prefix) {
    	if (!prefix)
    		prefix = "";

    	auto cpu_total = accumulated_init_time + accumulated_release_time + accumulated_alloc_time + accumulated_free_time + accumulated_copy_to_device_time + accumulated_copy_to_host_time + accumulated_compute_time  + accumulated_compilation_time + accumulated_overhead_time+accumulated_waiting_time;
    	auto gpu_total = accumulated_gpu_working_time + accumulated_gpu_unused_time;

    	std::cout << "===============================================================================\n";
    	dump_device();
		if (blocking)
			std::cout << "Blocking implementation\n";
		else
			std::cout << "Non-blocking implementation\n";
    	if (cpu_profiling_enabled) {
			std::cout << prefix << "CPU_Copy-to-device: " << accumulated_copy_to_device_time << "\n";
			std::cout << prefix << "CPU_Compute:        " << accumulated_compute_time << "\n";
			std::cout << prefix << "CPU_Copy-to-host:   " << accumulated_copy_to_host_time << "\n";
			std::cout << prefix << "CPU_Waiting:        " << accumulated_waiting_time << "\n";
			std::cout << prefix << "CPU_Compilation:    " << accumulated_compilation_time << "\n";
			std::cout << prefix << "CPU_Init:           " << accumulated_init_time << "\n";
			std::cout << prefix << "CPU_Release:        " << accumulated_release_time << "\n";
			std::cout << prefix << "CPU_Alloc:          " << accumulated_alloc_time << "\n";
			std::cout << prefix << "CPU_Free:           " << accumulated_free_time << "\n";
			std::cout << prefix << "CPU_Overhead:       " << accumulated_overhead_time << "\n";
			std::cout << prefix << "CPU_Total:          " << cpu_total << "\n";
    	}
    	if (cpu_profiling_enabled && gpu_profiling_enabled) {
			std::cout << "\n";
    	}
    	if (gpu_profiling_enabled) {
			std::cout << prefix << "GPU_Copy-to-device: " << accumulated_gpu_copy_to_device_time << "\n";
			std::cout << prefix << "GPU_Compute:        " << accumulated_gpu_compute_time << "\n";
			std::cout << prefix << "GPU_Copy-to-host:   " << accumulated_gpu_copy_to_host_time << "\n";
			std::cout << prefix << "GPU_Idle:           " << accumulated_gpu_unused_time << "\n";
			std::cout << prefix << "GPU_Working:        " << accumulated_gpu_working_time << "\n";
			std::cout << prefix << "GPU_Total:          " << gpu_total << "\n";
		}
    	if (!cpu_profiling_enabled && !gpu_profiling_enabled) {
			std::cout << "Profiling disabled; cannot print profiling result\n";
			std::cout << "Set PRL_PROFILING=1 (PRL_CPU_PROFILING=1 or PRL_GPU_PROFILING=1) environment variable to enable.\n";
		}
		std::cout << "===============================================================================\n";
		std::cout << std::flush;
	}
	
	void reset_stats() {
		accumulated_init_time = cpu_duration_t::zero();
		accumulated_release_time = cpu_duration_t::zero();
		accumulated_alloc_time = cpu_duration_t::zero();
		accumulated_free_time = cpu_duration_t::zero();
		accumulated_copy_to_device_time = cpu_duration_t::zero();
		accumulated_copy_to_host_time = cpu_duration_t::zero();
		accumulated_compute_time = cpu_duration_t::zero();
		accumulated_waiting_time = cpu_duration_t::zero();
		accumulated_compilation_time = cpu_duration_t::zero();
		accumulated_overhead_time = cpu_duration_t::zero();

		accumulated_gpu_copy_to_device_time = gpu_duration_t::zero();
		accumulated_gpu_compute_time = gpu_duration_t::zero();
		accumulated_gpu_copy_to_host_time= gpu_duration_t::zero();
		accumulated_gpu_unused_time = gpu_duration_t::zero();
		accumulated_gpu_working_time = gpu_duration_t::zero();
	}
};


class runtime
{
#ifdef THREAD_SAFE
    std::mutex lock;
#endif
    unsigned int ref_counter;
    session *current_session;

    int current_n_devices;
    cl_device_type * current_devices;

    void delete_session ()
    {
        assert (current_session != NULL);
        delete current_session;
        current_session = NULL;
        current_devices = NULL;
    }

    void record_session_settings (int n_devices, const cl_device_type * devices)
    {
        current_n_devices = n_devices;
        assert (current_devices == NULL);
        if (n_devices != 0)
        {
            current_devices = new cl_device_type [n_devices];
            for (int i = 0; i < n_devices; ++i)
            {
                current_devices[i] = devices[i];
            }
        }
    }

    bool check_session_settings (int n_devices, const cl_device_type * devices)
    {
        if (n_devices != current_n_devices)
        {
            return false;
        }
        for (int i = 0; i < n_devices; ++i)
        {
            if (devices[i] != current_devices[i])
            {
                return false;
            }
        }
        return true;
    }

    void create_new_session (int n_devices, const cl_device_type * devices, bool cpu_profiling_enabled, bool gpu_profiling_enabled, bool blocking)
    {
        assert (current_session == NULL);
        assert (n_devices > 0);
        assert (devices != NULL);
        current_session = new session (n_devices, devices, cpu_profiling_enabled, gpu_profiling_enabled, blocking);

        assert (current_session != NULL);
    }

    void create_new_dynamic_session (bool cpu_profiling_enabled, bool gpu_profiling_enabled, bool blocking)
    {
        int dyn_n_devices = 2;
        cl_device_type dyn_devices[] = {CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_CPU};

        assert (current_session == NULL);
        current_session = new session (dyn_n_devices, dyn_devices, cpu_profiling_enabled, gpu_profiling_enabled, blocking);
        assert (current_session != NULL);
    }

    void check_session (int n_devices, const cl_device_type * devices, bool cpu_profiling_enabled, bool gpu_profiling_enabled, bool blocking)
    {
    	auto session = get_session();
        if ((n_devices != 0 && !check_session_settings(n_devices, devices))
        		//|| session->is_cpu_profiling_enabled() != cpu_profiling_enabled
				//|| session->is_gpu_profiling_enabled() != gpu_profiling_enabled
				//|| session->is_blocking() != blocking
				) {
            std::cerr << "Cannot reinitialize existing session with different settings. "
              << "Use PENCIL_TARGET_DEVICE_DYNAMIC for consecutive pencil_init calls to use the existing settings."
              << std::endl;
            die ("invalid session initialization");
        }

        if ((!session->is_cpu_profiling_enabled() && cpu_profiling_enabled)
         || (!session->is_gpu_profiling_enabled() && gpu_profiling_enabled)) {
        	std::cerr << "Profiling (CPU and/or GPU) requested but not specified in the first call of prl_init."
        			<< "Either add PRL_PROFILING_ENABLED to your call to prl_init() or re-run with the PRL_PROFILING environment variable set";
        	die ("invalid session initialization");
        }
    }

    static runtime& get_instance ()
    {
        static runtime instance;
        return instance;
    }

    runtime (): ref_counter (0), current_session (NULL), current_n_devices(0), current_devices (NULL) {}

    runtime (const runtime& ) = delete;
    void operator= (const runtime& ) = delete;
public:
    static session *get_session ()
    {
        runtime& instance = get_instance ();
        assert (instance.ref_counter > 0);
        assert (instance.current_session);
        return instance.current_session;
    }

    static void retain (int n_devices, const cl_device_type * devices, bool cpu_profiling_enabled, bool gpu_profiling_enabled, bool blocking)
    {
        runtime& instance = get_instance ();
#ifdef THREAD_SAFE
        std::unique_lock<std::mutex> lck(instance.lock);
#endif
        if (instance.ref_counter++ == 0)
        {
            if (n_devices != 0)
            {
                instance.create_new_session (n_devices, devices, cpu_profiling_enabled, gpu_profiling_enabled, blocking);
            }
            else
            {
                instance.create_new_dynamic_session (cpu_profiling_enabled, gpu_profiling_enabled, blocking);
            }
            instance.record_session_settings (n_devices, devices);
        }
        else
        {
            instance.check_session (n_devices, devices, cpu_profiling_enabled, gpu_profiling_enabled, blocking);
        }
    }

    static void release (bool print_stats_on_release, const char *prefix)
    {
        runtime& instance = get_instance ();
#ifdef THREAD_SAFE
        std::unique_lock<std::mutex> lck(instance.lock);
#endif

    	auto session = runtime::get_session();
    	session->wait_pending();

        assert (instance.ref_counter > 0);
        if (--instance.ref_counter == 0)
        {
        	session->release();
        	if (print_stats_on_release)
        		session->dump_stats(prefix);
            instance.delete_session ();
        }
    }
};

prl_cl_program __int_opencl_create_program_from_file (const char *filename,
                                                         const char *opts)
{
    return runtime::get_session ()->create_or_get_program (filename, opts);
}

prl_cl_program __int_opencl_create_program_from_string (const char *program,
                                                           size_t size,
                                                           const char *opts)
{
    return runtime::get_session ()->create_or_get_program (program, size, opts);
}

void __int_opencl_release_program (prl_cl_program program)
{
	 auto wtch = runtime::get_session ()->get_overhead_stopwatch();
    runtime::get_session ()->release_program (program);
}

prl_cl_kernel __int_opencl_create_kernel (prl_cl_program program,
                                             const char *name)
{
    auto wtch = runtime::get_session ()->get_overhead_stopwatch();
    return program->get_kernel (name);
}

void __int_opencl_release_kernel (prl_cl_kernel kernel)
{
    runtime::get_session ()->release_kernel (kernel);
}

prl_cl_mem __int_opencl_create_device_buffer (cl_mem_flags flags, size_t size,
                                                 void *host_ptr)
{
    return
      runtime::get_session ()->alloc_and_return_dev_ptr (flags, size, host_ptr);
}

void __int_opencl_release_buffer (prl_cl_mem buffer)
{
    runtime::get_session ()->free_dev_buffer (buffer);
}

void __int_opencl_copy_to_device (prl_cl_mem dev, size_t size, void *host)
{
    runtime::get_session ()->copy_to_device (dev, size, host);
}

void __int_opencl_copy_to_host (prl_cl_mem dev, size_t size, void *host)
{
    runtime::get_session ()->copy_to_host (dev, size, host);
}

void *__int_pencil_alloc (size_t size)
{
#ifdef HOST_ALLOC
	auto wtch = runtime::get_session ()->get_alloc_stopwatch();
    return malloc (size);
#else
    return runtime::get_session ()->alloc_and_return_host_ptr (size);
#endif
}

void __int_pencil_free (void *ptr)
{
#ifdef HOST_ALLOC
	auto wtch = runtime::get_session ()->get_free_stopwatch();
    free (ptr);
#else
    runtime::get_session ()->free_host_ptr (ptr);
#endif
}

void __int_pencil_init (int n_devices, const cl_device_type * devices, bool cpu_profiling_enabled, bool gpu_profiling_enabled, bool blocking)
{
    runtime::retain (n_devices, devices, cpu_profiling_enabled, gpu_profiling_enabled, blocking);
}

void __int_pencil_shutdown (bool print_stats_on_release, const char *prefix)
{
    runtime::release (print_stats_on_release, prefix);
}


void __int_opencl_set_kernel_arg (prl_cl_kernel kernel, cl_uint idx,
                                  size_t size, const void *value, int buffer)
{
	auto session = runtime::get_session ();
	auto wtch = session->get_overhead_stopwatch();
	//auto wtch = session->get_compute_stopwatch();

    if (!buffer)
    {
        cl_error_code err = clSetKernelArg (kernel->kernel, idx, size, value);
        OPENCL_ASSERT (err);
    }
    else
    {
        assert (size == sizeof (prl_cl_mem));
        prl_cl_mem arg = *((prl_cl_mem*)value);
        cl_error_code err = clSetKernelArg (kernel->kernel, idx,
                                            sizeof (cl_mem), &(arg->dev));
        OPENCL_ASSERT (err);
    }
}

void __int_opencl_launch_kernel (prl_cl_kernel kernel, cl_uint work_dim,
                                 const size_t *goffset, const size_t *gws,
                                 const size_t *lws)
{
	auto session = runtime::get_session ();
	auto wtch = session->get_compute_stopwatch();
	auto profiling = session->is_gpu_profiling_enabled();
	auto blocking = session->is_blocking();

    cl_command_queue queue = runtime::get_session ()->get_command_queue ();
	cl_event event = NULL;
    cl_error_code err = clEnqueueNDRangeKernel (queue, kernel->kernel, work_dim,
                                                goffset, gws, lws, 0, NULL,
                                                profiling || blocking ? &event : NULL);
	OPENCL_ASSERT (err);
	if (blocking) {
		err = clWaitForEvents(1, &event);
		OPENCL_ASSERT (err);
	}
	session->add_event(event); // Will release event if non-null
}

void __int_pencil_dump_stats(const char *prefix) {
	runtime::get_session ()->dump_stats(prefix);
}

void __int_pencil_reset_stats() {
	runtime::get_session ()->reset_stats();
}

void __int_add_event(cl_event event) {
	auto session = runtime::get_session();
	session->add_event(event);
}

bool __int_gpu_profiling_enabled() {
	auto session = runtime::get_session();
	return session->is_gpu_profiling_enabled();
}

bool __int_blocking_enabled() {
	auto session = runtime::get_session();
	return session->is_blocking();
}



static bool active=false;
static stopwatch::timepoint_t start;

static std::vector<cpu_duration_t> durations;
static std::vector<cpu_duration_t> accumulated_compilation_time;
static std::vector<cpu_duration_t> accumulated_copy_to_device_time;
static std::vector<cpu_duration_t> accumulated_copy_to_host_time;
static std::vector<cpu_duration_t> accumulated_compute_time;
static std::vector<cpu_duration_t> accumulated_waiting_time;
static std::vector<cpu_duration_t> accumulated_init_time;
static std::vector<cpu_duration_t> accumulated_release_time;
static std::vector<cpu_duration_t> accumulated_alloc_time;
static std::vector<cpu_duration_t> accumulated_free_time;
static std::vector<cpu_duration_t> accumulated_overhead_time;

static std::vector<gpu_duration_t> accumulated_gpu_copy_to_device_time;
static std::vector<gpu_duration_t> accumulated_gpu_copy_to_host_time;
static std::vector<gpu_duration_t> accumulated_gpu_compute_time;
static std::vector<gpu_duration_t> accumulated_gpu_unused_time;
static std::vector<gpu_duration_t> accumulated_gpu_working_time;

void __int_print_timings(const char *prefix) {
	if (!prefix)
		prefix = "";

	auto session = runtime::get_session();
	auto blocking = session->is_blocking();
	auto cpu_profiling_enabled = session->is_cpu_profiling_enabled();
	auto gpu_profiling_enabled = session->is_gpu_profiling_enabled();

	auto n = durations.size();
	std::vector<stopwatch::duration_t> cpu_totals;
	std::vector<stopwatch::duration_t> duration_wo_compilation;
	std::vector<gpu_duration_t> gpu_totals;
	for (auto i = 0; i<n; ++i) {
		cpu_totals.push_back(accumulated_copy_to_device_time.at(i) +  accumulated_copy_to_host_time.at(i) + accumulated_compute_time.at(i) + accumulated_compilation_time.at(i)  + accumulated_overhead_time.at(i) +accumulated_waiting_time.at(i)
				+ accumulated_init_time.at(0) +  accumulated_release_time.at(0) +  accumulated_alloc_time.at(0) +  accumulated_free_time.at(0)
		);
		duration_wo_compilation.push_back(durations.at(i) - accumulated_compilation_time.at(i));
		gpu_totals.push_back(accumulated_gpu_working_time.at(i)+accumulated_gpu_unused_time.at(i));
	}

	std::cout << "===============================================================================\n";
	session->dump_device();
	if (blocking)
		std::cout << "Blocking implementation\n";
	else
		std::cout << "Non-blocking implementation\n";
	{
		std::cout << "\n";
		std::cout << "                    " << "     median (relative standard deviation) of " << n << " samples\n";
		std::cout << prefix << "Duration:           " << median_ms(durations) << " (" << variation_percent(durations) <<  ")\n";
	}
	if (cpu_profiling_enabled) {
		std::cout << prefix << "w/o compilation:    " << median_ms(duration_wo_compilation) << " (" << variation_percent(duration_wo_compilation) <<  ")\n";
		std::cout << "\n";
		std::cout << prefix << "CPU_Copy-to-device: " << median_ms(accumulated_copy_to_device_time) << " (" << variation_percent(accumulated_copy_to_device_time) <<  ")\n";
		std::cout << prefix << "CPU_Compute:        " << median_ms(accumulated_compute_time)  << " (" << variation_percent(accumulated_compute_time) <<  ")\n";
		std::cout << prefix << "CPU_Copy-to-host:   " << median_ms(accumulated_copy_to_host_time)  << " (" << variation_percent(accumulated_copy_to_host_time) <<  ")\n";
		std::cout << prefix << "CPU_Waiting:        " << median_ms(accumulated_waiting_time)  << " (" << variation_percent(accumulated_waiting_time) <<  ")\n";
		std::cout << prefix << "CPU_Compilation:    " << median_ms(accumulated_compilation_time) << " (" << variation_percent(accumulated_compilation_time) <<  ")\n";
		std::cout << prefix << "CPU_Init:           " << median_ms(accumulated_init_time) << " (" << variation_percent(accumulated_init_time) <<  ")\n";
		std::cout << prefix << "CPU_Release:        " << median_ms(accumulated_release_time) << " (" << variation_percent(accumulated_release_time) <<  ")\n";
		std::cout << prefix << "CPU_Alloc:          " << median_ms(accumulated_alloc_time) << " (" << variation_percent(accumulated_alloc_time) <<  ")\n";
		std::cout << prefix << "CPU_Free:           " << median_ms(accumulated_free_time) << " (" << variation_percent(accumulated_free_time) <<  ")\n";
		std::cout << prefix << "CPU_Overhead:       " << median_ms(accumulated_overhead_time) << " (" << variation_percent(accumulated_overhead_time) <<  ")\n";
		std::cout << prefix << "CPU_Total:          " << median_ms(cpu_totals) << " (" << variation_percent(cpu_totals) <<  ")\n";
	}
	std::cout << "\n";
	if (gpu_profiling_enabled) {
		std::cout << prefix << "GPU_Copy-to-device: " << median_ms(accumulated_gpu_copy_to_device_time) << " (" << variation_percent(accumulated_gpu_copy_to_device_time) <<  ")\n";;
		std::cout << prefix << "GPU_Compute:        " << median_ms(accumulated_gpu_compute_time) << " (" << variation_percent(accumulated_gpu_compute_time) <<  ")\n";
		std::cout << prefix << "GPU_Copy-to-host:   " << median_ms(accumulated_gpu_copy_to_host_time) << " (" << variation_percent(accumulated_gpu_copy_to_host_time) <<  ")\n";
		std::cout << prefix << "GPU_Idle:           " << median_ms(accumulated_gpu_unused_time) << " (" << variation_percent(accumulated_gpu_unused_time) <<  ")\n";
		std::cout << prefix << "GPU_Working:        " << median_ms(accumulated_gpu_working_time) << " (" << variation_percent(accumulated_gpu_working_time) <<  ")\n";
		std::cout << prefix << "GPU_Total:          " << median_ms(gpu_totals) << " (" << variation_percent(gpu_totals) <<  ")\n";
	}
	std::cout << "===============================================================================\n";
	std::cout << std::flush;
}

void __int_reset_timings() {
	//auto session = runtime::get_session();
	//session->reset_stats();

	durations.clear();
	accumulated_compilation_time.clear();
	accumulated_copy_to_device_time.clear();
	accumulated_copy_to_host_time.clear();
	accumulated_compute_time.clear();
	accumulated_waiting_time.clear();
	accumulated_init_time.clear();
	accumulated_release_time.clear();
	accumulated_alloc_time.clear();
	accumulated_free_time.clear();
	accumulated_overhead_time.clear();

	accumulated_gpu_copy_to_device_time.clear();
	accumulated_gpu_copy_to_host_time.clear();
	accumulated_gpu_compute_time.clear();
	accumulated_gpu_unused_time.clear();
	accumulated_gpu_working_time.clear();
}


void __int_pencil_timing_start() {
	assert(!active);
	auto session = runtime::get_session();
	session->reset_stats();

	active = true;
	start = std::chrono::steady_clock::now();
}

void __int_pencil_timing_stop() {
	assert(active);
	auto stop = std::chrono::steady_clock::now();
	active = false;

	auto duration = stop - start;
	assert(duration>=duration.zero());
	if (duration<duration.zero())
		duration = duration.zero();
	auto session = runtime::get_session();

	durations.push_back(duration);
	accumulated_compilation_time.push_back(session->accumulated_compilation_time);
	accumulated_copy_to_device_time.push_back(session->accumulated_copy_to_device_time);
	accumulated_copy_to_host_time.push_back(session->accumulated_copy_to_host_time);
	accumulated_compute_time.push_back(session->accumulated_compute_time);
	accumulated_waiting_time.push_back(session->accumulated_waiting_time);
	accumulated_init_time.push_back(session->accumulated_init_time);
	accumulated_release_time.push_back(session->accumulated_release_time);
	accumulated_alloc_time.push_back(session->accumulated_alloc_time);
	accumulated_free_time.push_back(session->accumulated_free_time);
	accumulated_overhead_time.push_back(session->accumulated_overhead_time);

	accumulated_gpu_copy_to_device_time.push_back(session->accumulated_gpu_copy_to_device_time);
	accumulated_gpu_copy_to_host_time.push_back(session->accumulated_gpu_copy_to_host_time);
	accumulated_gpu_compute_time.push_back(session->accumulated_gpu_compute_time);
	accumulated_gpu_unused_time.push_back(session->accumulated_gpu_unused_time);
	accumulated_gpu_working_time.push_back(session->accumulated_gpu_working_time);

	session->reset_stats();
}



void __int_pencil_timing(timing_callback timed_func, void *user, timing_callback init_callback, void *init_user, timing_callback finit_callback, void *finit_user, enum prl_init_flags flags, int dryruns, int runs, const char *prefix) {
	assert(timed_func);

	prl_init(static_cast<prl_init_flags>(flags | PRL_PROFILING_ENABLED));
	__int_reset_timings();

	// Warmup runs
	for (auto i = 0; i < dryruns; ++i) {
		if (init_callback) (*init_callback)(init_user);
		(*timed_func)(user);
		if (finit_callback) (*finit_callback)(finit_user);
	}

	assert(runs >= 1);
	for (auto i = 0; i < runs; ++i) {
		if (init_callback) (*init_callback)(init_user);
		__int_pencil_timing_start();
		(*timed_func)(user);
		__int_pencil_timing_stop();
		if (finit_callback) (*finit_callback)(finit_user);
	}

	__int_print_timings(prefix);
	prl_shutdown();
}

#if 0

prl_alloc -> __int_pencil_alloc -> alloc:alloc_and_return_host_ptr -> memory.alloc -> alloc_dev_buffer -> clCreateBuffer
                                                                                      -> map -> clEnqueueMapBuffer

prl_free -> __int_pencil_free -> free:free_host_ptr -> free_host_ptr -> memory.free -> unmap -> clEnqueueUnmapMemObject
                                                                                    -> clReleaseMemObject

prl_create_device_buffer -> __int_opencl_create_device_buffer -> alloc:alloc_and_return_dev_ptr -> memory.dev_alloc -> unmap -> clEnqueueUnmapMemObject
                                                                                                                    -> alloc_dev_buffer -> clCreateBuffer

prl_release_buffer-> __int_opencl_release_buffer -> free:free_dev_buffer -> memory.dev_free -> clReleaseMemObject

#endif
