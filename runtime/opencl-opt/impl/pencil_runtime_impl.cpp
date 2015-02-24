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
#include <map>
#include <iostream>
#include <chrono>
#include <limits>
#include <tuple>
#include <algorithm>
#include <functional>

#ifdef THREAD_SAFE
#include<mutex>
#endif

#include "impl.h"

extern "C"
{
    const char *opencl_error_string(cl_int error);
}



#define UNUSED(exp) (void)(exp)
#define OPENCL_ASSERT(exp) do {if (exp != CL_SUCCESS) {std::cerr << "OpenCL error: " << opencl_error_string(exp) << std::endl;} assert (exp == CL_SUCCESS);} while (0)

typedef cl_int cl_error_code;

const int ERROR_CODE = 2;

void die (const char * error)
{
    std::cerr << "ERROR: " << error << std::endl;
    exit(ERROR_CODE);
}

// host_stopwatch
class stopwatch {
public:
	typedef typename std::chrono::high_resolution_clock::time_point::duration duration_t;

private:
	typedef typename std::chrono::high_resolution_clock::time_point::time_point timepoint_t;
	
	bool enabled;
	duration_t *counter;
	timepoint_t start;
	
	stopwatch(const stopwatch &that);
	
public:
	stopwatch() : enabled(false), counter(), start() {}
	
	stopwatch(duration_t &counter, bool enabled) : enabled(enabled), counter(&counter) {
		if (enabled)
			start = std::chrono::high_resolution_clock::now();
	}
	
	~stopwatch() {
		if (!enabled)
			return; 
		
		auto stop = std::chrono::high_resolution_clock::now();
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


bool __int_profiling_enabled();
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
        host = clEnqueueMapBuffer (queue, dev, CL_FALSE/*non-blocking*/,
                                   CL_MAP_READ | CL_MAP_WRITE, 0, size,
                                   0, NULL, __int_profiling_enabled() ? &event : NULL, &err);
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

        cl_event event = NULL;
        cl_error_code err = clEnqueueUnmapMemObject (queue, dev, host, 0, NULL,
        		__int_profiling_enabled() ? &event : NULL);
        OPENCL_ASSERT (err);

        //err = clWaitForEvents (1, &event);
        //OPENCL_ASSERT (err);
        //err = clReleaseEvent (event);
        //OPENCL_ASSERT (err);
        __int_add_event(event);

        host = NULL;
    }
};


class memory_manager
{

    std::map<void*, pencil_cl_mem> cache;
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
        pencil_cl_mem  buff = new __int_pencil_cl_mem (dev_buff, size, true);
        buff->map (queue, true);
        assert (buff->exposed_ptr);

        cache[buff->exposed_ptr] = buff;
        return buff->exposed_ptr;
    }

    pencil_cl_mem dev_alloc (cl_context ctx, cl_mem_flags flags, size_t size,
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

    void copy_to_device (cl_command_queue queue, pencil_cl_mem dev, size_t size,
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
        cl_error_code err = clEnqueueWriteBuffer (queue, dev->dev, CL_FALSE/*non-blocking*/, 0,
                                                  size, host, 0, NULL, __int_profiling_enabled() ? &event : NULL);
        OPENCL_ASSERT (err);

        __int_add_event(event);
    }

    void copy_to_host (cl_command_queue queue, pencil_cl_mem dev, size_t size,
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
        cl_error_code err = clEnqueueReadBuffer (queue, dev->dev, CL_FALSE/*non-blocking*/, 0,
                                                 size, host, 0, NULL, __int_profiling_enabled() ? &event : NULL);
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
        pencil_cl_mem buff = cache[ptr];

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

    void dev_free (pencil_cl_mem buff)
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

extern "C" {
cl_program opencl_build_program_from_file (cl_context ctx, cl_device_id dev,
                                           const char *filename,
                                           const char *opencl_options);
cl_program opencl_build_program_from_string (cl_context ctx, cl_device_id dev,
                                             const char *filename,
                                             size_t size,
                                             const char *opencl_options);
}

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
    std::map<std::string, pencil_cl_kernel> kernel_name_idx;

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



    pencil_cl_kernel get_kernel (const char *name)
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
        pencil_cl_kernel res = new __int_pencil_cl_kernel (kernel);
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
    std::vector<pencil_cl_program> programs;
public:

    pencil_cl_program get_program (const char *file, const char *opts,
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

    pencil_cl_program get_program (const char *program, size_t size,
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
    bool profiling_enabled;

	stopwatch::duration_t accumulated_overhead_time;
	stopwatch::duration_t accumulated_copy_to_device_time;
	stopwatch::duration_t accumulated_copy_to_host_time;
	stopwatch::duration_t accumulated_compute_time;
	stopwatch::duration_t accumulated_waiting_time;
	
	std::vector<cl_event> pending_events;
	uint64_t accumulated_gpu_copy_to_device_time;
	uint64_t accumulated_gpu_copy_to_host_time;
	uint64_t accumulated_gpu_compute_time;
	uint64_t accumulated_gpu_unused_time;
	uint64_t accumulated_gpu_working_time;

public:
    session (int n_devices, const cl_device_type * devices, bool profiling_enabled)
		: profiling_enabled(profiling_enabled),
			accumulated_overhead_time(0), accumulated_copy_to_device_time(0), accumulated_copy_to_host_time(0), accumulated_compute_time(0),
		accumulated_gpu_copy_to_device_time(0), accumulated_gpu_copy_to_host_time(0), accumulated_gpu_compute_time(0),accumulated_gpu_unused_time(0),accumulated_gpu_working_time(0)
    {
		stopwatch wtch(accumulated_overhead_time, profiling_enabled);

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
        queue = clCreateCommandQueue (context, device, profiling_enabled ? CL_QUEUE_PROFILING_ENABLE : 0, &err);
        OPENCL_ASSERT (err);
        assert (queue);
    }

    ~session ()
    {
		stopwatch wtch(accumulated_overhead_time, profiling_enabled);

        programs.clear ();
        cl_error_code err;
        err = clReleaseCommandQueue (queue);
        OPENCL_ASSERT (err);
        err = clReleaseContext (context);
        OPENCL_ASSERT (err);
    }

    stopwatch get_overhead_stopwatch() {
		return stopwatch(accumulated_overhead_time, profiling_enabled);
	}
	
	stopwatch get_compute_stopwatch() {
		return stopwatch(accumulated_compute_time, profiling_enabled);
	}
    
    cl_device_type get_current_device_type () const
    {
        return current_device_type;
    }

    pencil_cl_program create_or_get_program (const char *path, const char *opts)
    {
		stopwatch wtch(accumulated_overhead_time, profiling_enabled);

        return programs.get_program (path, opts, context, device);
    }

    pencil_cl_program create_or_get_program (const char *program, size_t size, const char *opts)
    {
		stopwatch wtch(accumulated_overhead_time, profiling_enabled);

        return programs.get_program (program, size, opts, context, device);
    }

    void release_program (pencil_cl_program prog)
    {
        /* All programs are cached by the runtime and released when the
           session is released.  */
        UNUSED (prog);
    }

    void release_kernel (pencil_cl_kernel kernel)
    {
        /* All kernels are cached by the runtime and released when the
           session is released.  */
        UNUSED (kernel);
    }

    void *alloc_and_return_host_ptr (size_t size)
    {
		stopwatch wtch(accumulated_overhead_time, profiling_enabled);

        return memory.alloc (context, queue, size);
    }

    pencil_cl_mem alloc_and_return_dev_ptr (cl_mem_flags flags, size_t size,
                                            void *host_ptr)
    {
		stopwatch wtch(accumulated_overhead_time, profiling_enabled);

        return memory.dev_alloc (context, flags, size, host_ptr, queue);
    }

    void free_host_ptr (void *ptr)
    {
		stopwatch wtch(accumulated_overhead_time, profiling_enabled);
		
        memory.free (ptr, queue);
    }

    void free_dev_buffer (pencil_cl_mem dev)
    {
		stopwatch wtch(accumulated_overhead_time, profiling_enabled);

        memory.dev_free (dev);
    }

    void copy_to_device (pencil_cl_mem dev, size_t size, void *host)
    {
		stopwatch wtch(accumulated_copy_to_device_time, profiling_enabled);

        memory.copy_to_device (queue, dev, size, host);
    }

    void copy_to_host (pencil_cl_mem dev, size_t size, void *host)
    {
		stopwatch wtch(accumulated_copy_to_host_time, profiling_enabled);

        memory.copy_to_host (queue, dev, size, host);
    }

    cl_command_queue get_command_queue ()
    {
        return queue;
    }
    
    bool is_profiling_enabled() {
    	return profiling_enabled;
    }

    void add_event(cl_event event) {
    	assert(profiling_enabled == !!event);
    	if (!event)
    		return;

    	if (profiling_enabled) {
    		pending_events.push_back(event);
    	} else {
    		auto err = clReleaseEvent(event);
    		OPENCL_ASSERT(err);
    	}
    }

    typedef std::tuple<cl_ulong,cl_ulong, 	cl_command_type> tuple_t;
    void accumulate_durations(const std::vector<tuple_t> &exec_periods, const std::function<bool(const tuple_t&)> &filter, cl_ulong start_queing, cl_ulong &accumulated_working, cl_ulong &accumulated_idle) {
      	ulong prev_end = start_queing;
        	for (auto const &p : exec_periods) {
        		if (!filter(p))
        			continue;

        		auto &start = std::get<0>(p);
        		auto &end = std::get<1>(p);
        		auto &cat = std::get<2>(p);

        		if (prev_end <= start) {
        			// Case 1: |<- prev ->|  |<- cur ->|
        			assert(start <= end);

        			auto cur_duration = end - start;
        			auto unused_duraction = start - prev_end;

        			accumulated_working += cur_duration;
        			accumulated_idle += unused_duraction;

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

        			auto cur_duration = end - prev_end;
        			accumulated_working += cur_duration;

        			prev_end = end;
        		}
        	}
    }

    void wait_pending() {
    	cl_int err;

    	{
    		stopwatch wtch(accumulated_waiting_time, profiling_enabled);
    		err = clFinish(queue);
    		OPENCL_ASSERT(err);
    	}

    	if (!profiling_enabled)
    		return;

    	{
    	stopwatch wtch(accumulated_overhead_time, profiling_enabled);

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
    			end = start; // Don't know hiow this could happen

    		// Check consistency
    		if (!(queued <= start && start <= end)) {
    			std::cerr << "CL_PROFILING_COMMAND_QUEUED = " << queued << "\n";
    			std::cerr << "CL_PROFILING_COMMAND_SUBMIT = " << submit << "\n";
    			std::cerr << "CL_PROFILING_COMMAND_START  = " << start << "\n";
    			std::cerr << "CL_PROFILING_COMMAND_END    = " << end << "\n";
    			std::cerr << std::flush;
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
    	cl_ulong dummy;
       	accumulate_durations(exec_periods, [](const tuple_t &p)->bool { return std::get<2>(p) == CL_COMMAND_NDRANGE_KERNEL ||  std::get<2>(p) == CL_COMMAND_TASK || std::get<2>(p) == CL_COMMAND_NATIVE_KERNEL; }, start_queing, accumulated_gpu_compute_time, dummy);
       	accumulate_durations(exec_periods, [](const tuple_t &p)->bool { return std::get<2>(p) == CL_COMMAND_WRITE_BUFFER || std::get<2>(p) == CL_COMMAND_WRITE_IMAGE || std::get<2>(p) == CL_COMMAND_MAP_BUFFER|| std::get<2>(p) == CL_COMMAND_MAP_IMAGE; }, start_queing, accumulated_gpu_copy_to_device_time, dummy);
       	accumulate_durations(exec_periods, [](const tuple_t &p)->bool { return std::get<2>(p) == CL_COMMAND_READ_BUFFER || std::get<2>(p) == CL_COMMAND_READ_IMAGE || std::get<2>(p) == CL_COMMAND_UNMAP_MEM_OBJECT;  }, start_queing, accumulated_gpu_copy_to_host_time, dummy);
    	}
    }

    void dump_stats() {
    	auto cpu_total = accumulated_copy_to_device_time + accumulated_copy_to_host_time + accumulated_compute_time + accumulated_overhead_time+accumulated_waiting_time;
    	auto gpu_total = accumulated_gpu_working_time + accumulated_gpu_unused_time;

    	std::cerr << "===============================================================================\n";
		if (profiling_enabled) {
    	std::cerr << "CPU:\n";
		std::cerr << "Copy-to-device: " << std::chrono::duration_cast<std::chrono::milliseconds>(accumulated_copy_to_device_time).count() << " ms\n";
		std::cerr << "Compute:        " << std::chrono::duration_cast<std::chrono::milliseconds>(accumulated_compute_time).count() << " ms\n";
		std::cerr << "Copy-to-host:   " << std::chrono::duration_cast<std::chrono::milliseconds>(accumulated_copy_to_host_time).count() << " ms\n";
		std::cerr << "Waiting:        " << std::chrono::duration_cast<std::chrono::milliseconds>(accumulated_waiting_time).count() << " ms\n";
		std::cerr << "Overhead:       " << std::chrono::duration_cast<std::chrono::milliseconds>(accumulated_overhead_time).count() << " ms\n";
		std::cerr << "Total:          " << std::chrono::duration_cast<std::chrono::milliseconds>(cpu_total).count() << " ms\n";
		std::cerr << "\n";
		std::cerr << "GPU:\n";
		std::cerr << "Copy-to-device: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(accumulated_gpu_copy_to_device_time)).count() << " ms\n";
		std::cerr << "Compute:        " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(accumulated_gpu_compute_time)).count() << " ms\n";
		std::cerr << "Copy-to-host:   " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(accumulated_gpu_copy_to_host_time)).count() << " ms\n";
		std::cerr << "Idle:           " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(accumulated_gpu_unused_time)).count() << " ms\n";
		std::cerr << "Working:        " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(accumulated_gpu_working_time)).count() << " ms\n";
		std::cerr << "Total:          " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(gpu_total)).count() << " ms\n";
		std::cerr << "===============================================================================\n";
		} else {
			std::cerr << "Profiling disabled; cannot print profiling result\n";
			std::cerr << "Set PENCIL_PROFILING environment variable to enable.\n";
		}
		std::cerr << std::flush;
	}
	
	void reset_stats() {
		accumulated_copy_to_device_time = accumulated_copy_to_device_time.zero();
		accumulated_copy_to_host_time = accumulated_copy_to_host_time.zero();
		accumulated_compute_time = accumulated_compute_time.zero();
		accumulated_waiting_time = accumulated_waiting_time.zero();
		accumulated_overhead_time = accumulated_overhead_time.zero();

		accumulated_gpu_copy_to_device_time = 0;
		accumulated_gpu_compute_time = 0;
		accumulated_gpu_copy_to_host_time= 0;
		accumulated_gpu_unused_time = 0;
		accumulated_gpu_working_time = 0;
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

    void create_new_session (int n_devices, const cl_device_type * devices, bool profiling_enabled)
    {
        assert (current_session == NULL);
        assert (n_devices > 0);
        assert (devices != NULL);
        current_session = new session (n_devices, devices, profiling_enabled);

        assert (current_session != NULL);
    }

    void create_new_dynamic_session (bool profiling_enabled)
    {
        int dyn_n_devices = 2;
        cl_device_type dyn_devices[] = {CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_CPU};

        assert (current_session == NULL);
        current_session = new session (dyn_n_devices, dyn_devices, profiling_enabled);
        assert (current_session != NULL);
    }

    void check_session (int n_devices, const cl_device_type * devices, bool profiling_enabled)
    {
        if ((n_devices != 0 && !check_session_settings(n_devices, devices)) || get_session()->is_profiling_enabled() != profiling_enabled)
        {
            std::cerr << "Cannot reinitialize existing session with different settings. "
              << "Use PENCIL_TARGET_DEVICE_DYNAMIC for consecutive pencil_init calls to use the existing settings."
              << std::endl;
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

    static void retain (int n_devices, const cl_device_type * devices, bool profiling_enabled)
    {
		auto start = std::chrono::high_resolution_clock::now();
        runtime& instance = get_instance ();
#ifdef THREAD_SAFE
        std::unique_lock<std::mutex> lck(instance.lock);
#endif
        if (instance.ref_counter++ == 0)
        {
            if (n_devices != 0)
            {
                instance.create_new_session (n_devices, devices, profiling_enabled);
            }
            else
            {
                instance.create_new_dynamic_session (profiling_enabled);
            }
            instance.record_session_settings (n_devices, devices);
        }
        else
        {
            instance.check_session (n_devices, devices, profiling_enabled);
        }
    }

    static void release (bool print_stats_on_release)
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
        	if (print_stats_on_release)
        		session->dump_stats();
            instance.delete_session ();
        }
    }
};

pencil_cl_program __int_opencl_create_program_from_file (const char *filename,
                                                         const char *opts)
{
    return runtime::get_session ()->create_or_get_program (filename, opts);
}

pencil_cl_program __int_opencl_create_program_from_string (const char *program,
                                                           size_t size,
                                                           const char *opts)
{
    return runtime::get_session ()->create_or_get_program (program, size, opts);
}

void __int_opencl_release_program (pencil_cl_program program)
{
    runtime::get_session ()->release_program (program);
}

pencil_cl_kernel __int_opencl_create_kernel (pencil_cl_program program,
                                             const char *name)
{
    auto wtch = runtime::get_session ()->get_overhead_stopwatch();
    return program->get_kernel (name);
}

void __int_opencl_release_kernel (pencil_cl_kernel kernel)
{
    runtime::get_session ()->release_kernel (kernel);
}

pencil_cl_mem __int_opencl_create_device_buffer (cl_mem_flags flags, size_t size,
                                                 void *host_ptr)
{
    return
      runtime::get_session ()->alloc_and_return_dev_ptr (flags, size, host_ptr);
}

void __int_opencl_release_buffer (pencil_cl_mem buffer)
{
    runtime::get_session ()->free_dev_buffer (buffer);
}

void __int_opencl_copy_to_device (pencil_cl_mem dev, size_t size, void *host)
{
    runtime::get_session ()->copy_to_device (dev, size, host);
}

void __int_opencl_copy_to_host (pencil_cl_mem dev, size_t size, void *host)
{
    runtime::get_session ()->copy_to_host (dev, size, host);
}

void *__int_pencil_alloc (size_t size)
{
#ifdef HOST_ALLOC
	auto wtch = runtime::get_session ()->get_overhead_stopwatch();
    return malloc (size);
#else
    return runtime::get_session ()->alloc_and_return_host_ptr (size);
#endif
}

void __int_pencil_free (void *ptr)
{
#ifdef HOST_ALLOC
	auto wtch = runtime::get_session ()->get_overhead_stopwatch();
    free (ptr);
#else
    runtime::get_session ()->free_host_ptr (ptr);
#endif
}

void __int_pencil_init (int n_devices, const cl_device_type * devices, bool profiling_enabled)
{
    runtime::retain (n_devices, devices, profiling_enabled);
}

void __int_pencil_shutdown (bool print_stats_on_release)
{
    runtime::release (print_stats_on_release);
}


void __int_opencl_set_kernel_arg (pencil_cl_kernel kernel, cl_uint idx,
                                  size_t size, const void *value, int buffer)
{
	auto wtch = runtime::get_session ()->get_overhead_stopwatch();

    if (!buffer)
    {
        cl_error_code err = clSetKernelArg (kernel->kernel, idx, size, value);
        OPENCL_ASSERT (err);
    }
    else
    {
        assert (size == sizeof (pencil_cl_mem));
        pencil_cl_mem arg = *((pencil_cl_mem*)value);
        cl_error_code err = clSetKernelArg (kernel->kernel, idx,
                                            sizeof (cl_mem), &(arg->dev));
        OPENCL_ASSERT (err);
    }
}

void __int_opencl_launch_kernel (pencil_cl_kernel kernel, cl_uint work_dim,
                                 const size_t *goffset, const size_t *gws,
                                 const size_t *lws)
{
	auto session = runtime::get_session ();
	auto wtch = session->get_compute_stopwatch();

    cl_command_queue queue = runtime::get_session ()->get_command_queue ();
	cl_event event = NULL;
    cl_error_code err = clEnqueueNDRangeKernel (queue, kernel->kernel, work_dim,
                                                goffset, gws, lws, 0, NULL,
                                                session->is_profiling_enabled() ? &event : NULL);
	OPENCL_ASSERT (err);
	//clWaitForEvents(1, &event);
	session->add_event(event);
}

void __int_pencil_dump_stats() {
	runtime::get_session ()->dump_stats();
}

void __int_pencil_reset_stats() {
	runtime::get_session ()->reset_stats();
}

void __int_add_event(cl_event event) {
	auto session = runtime::get_session();
	session->add_event(event);
}

bool __int_profiling_enabled() {
	auto session = runtime::get_session();
	return session->is_profiling_enabled();
}
