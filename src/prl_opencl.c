#if defined(__APPLE__)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <stdbool.h>
#include <inttypes.h>
#include "prl.h"

static const char *PRL_TARGET_DEVICE = "PRL_TARGET_DEVICE";

//static const char *PRL_PROFILING = "PRL_PROFILING";
//static const char *PRL_CPU_PROFILING = "PRL_CPU_PROFILING";
//static const char *PRL_GPU_PROFILING = "PRL_GPU_PROFILING";
//static const char *PRL_PROFILING_PREFIX = "PRL_PROFILING_PREFIX";
//static const char *PRL_BLOCKING = "PRL_BLOCKING";
//static const char *PRL_TIMINGS_RUNS = "PRL_TIMINGS_RUNS";
//static const char *PRL_TIMINGS_DRY_RUNS = "PRL_TIMINGS_DRY_RUNS";
//static const char *PRL_TIMINGS_PREFIX = "PRL_TIMINGS_PREFIX";
//static const char *PRL_PREFIX = "PRL_PREFIX";

enum prl_device_choice {
    PRL_TARGET_DEVICE_FIRST,
    PRL_TARGET_DEVICE_FIXED,
    PRL_TARGET_DEVICE_GPU_ONLY,
    PRL_TARGET_DEVICE_CPU_ONLY,
    PRL_TARGET_DEVICE_GPU_THEN_CPU,
    PRL_TARGET_DEVICE_CPU_THEN_GPU,
};

struct prl_global_config {
    enum prl_device_choice device_choice;
    int chosed_platform;
    int chosen_device;

    int cpu_profiling;
    int gpu_profiling;
    bool blocking;
};

struct prl_global_state {
    int initialized;
    struct prl_global_config config;

    cl_device_id device;
    cl_context context;

    // linked lists
    prl_program programs;
    prl_scop scops;
    prl_mem global_mems;
};

struct prl_stat {
	double gpu_transfer_to_device;
	double gpu_compute;
	double gpu_transfer_to_host;
};

struct prl_scop_struct {

    prl_scop next;
};

struct prl_scop_inst_struct {
    prl_scop scop;
    cl_command_queue queue;

    prl_mem local_mems;//linked list
};

struct prl_program_struct {
    cl_program program;

    prl_kernel kernels;
    prl_program next;
};

struct prl_kernel_struct {
    prl_scop scop;

    cl_kernel kernel;

    prl_kernel next;
};

enum prl_alloc_type {
	alloc_type_none,
	alloc_type_host_only,
	alloc_type_dev_only,

	// Use clEnqueueWriteBuffer/clEnqueueReadBuffer
	alloc_type_rwbuf,

	// Use clEnqueueMapBuffer/clEnqueueUnmapMemObject
	alloc_type_map,

	// clSVMAlloc
	alloc_type_svm,
};

enum prl_alloc_current_location {
	// buffer content is undefined
	loc_none,

	loc_host,
	loc_dev,

	loc_transferring_to_host,
	loc_transferring_to_dev
};

/* Describes a memory region */
// Possible scopes:
// 1. ad-hoc:   automatically allocated and released
// 2. manually: user-allocated and released
struct prl_mem_struct {
    // If this is a ad-hoc allocation, this is the scope it is used in (and can be freed when on leaving)
    prl_scop_instance scopinst;//RENAME: scopinst
	//bool is_global;
    size_t size;
    enum prl_alloc_type type;

    cl_mem clmem; //RENAME: dev_clmem
    bool dev_owning;
    bool dev_readable;
    bool dev_writable;

    void *host_mem; //RENAME: host_ptr
    bool host_owning;
    bool host_readable;
    bool host_writable;

    // Where the current buffer content resides
    enum prl_alloc_current_location loc;

    // On entering a SCoP:
    bool transfer_to_device;

    // On leaving a SCoP:
    bool transfer_to_host;
    bool free_on_leave;

    prl_mem next;
};

struct prl_scopinst_mem_struct {
	prl_mem mem_block;
	bool owning;
};


static struct prl_global_config global_config = {0};

static bool prl_initialized = false;
static struct prl_global_state global_state;

//http://stackoverflow.com/questions/24326432/convenient-way-to-show-opencl-error-codes
static const char *opencl_getErrorString(cl_int error) {
    switch (error) {
    // runtime
    case CL_SUCCESS:
        return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND:
        return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE:
        return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE:
        return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
        return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES:
        return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY:
        return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE:
        return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP:
        return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH:
        return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:
        return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE:
        return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE:
        return "CL_MAP_FAILURE";
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:
        return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
        return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case CL_COMPILE_PROGRAM_FAILURE:
        return "CL_COMPILE_PROGRAM_FAILURE";
    case CL_LINKER_NOT_AVAILABLE:
        return "CL_LINKER_NOT_AVAILABLE";
    case CL_LINK_PROGRAM_FAILURE:
        return "CL_LINK_PROGRAM_FAILURE";
    case CL_DEVICE_PARTITION_FAILED:
        return "CL_DEVICE_PARTITION_FAILED";
    case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
        return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

    // compilation
    case CL_INVALID_VALUE:
        return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE:
        return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM:
        return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE:
        return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT:
        return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES:
        return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE:
        return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR:
        return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT:
        return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
        return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE:
        return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER:
        return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY:
        return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS:
        return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM:
        return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE:
        return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME:
        return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION:
        return "CL_INVALID_KERNEL_DEFINITION";
    case CL_INVALID_KERNEL:
        return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX:
        return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE:
        return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE:
        return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS:
        return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION:
        return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE:
        return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE:
        return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET:
        return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST:
        return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT:
        return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION:
        return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT:
        return "CL_INVALID_GL_OBJECT";
    case CL_INVALID_BUFFER_SIZE:
        return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL:
        return "CL_INVALID_MIP_LEVEL";
    case CL_INVALID_GLOBAL_WORK_SIZE:
        return "CL_INVALID_GLOBAL_WORK_SIZE";
    case CL_INVALID_PROPERTY:
        return "CL_INVALID_PROPERTY";
    case CL_INVALID_IMAGE_DESCRIPTOR:
        return "CL_INVALID_IMAGE_DESCRIPTOR";
    case CL_INVALID_COMPILER_OPTIONS:
        return "CL_INVALID_COMPILER_OPTIONS";
    case CL_INVALID_LINKER_OPTIONS:
        return "CL_INVALID_LINKER_OPTIONS";
    case CL_INVALID_DEVICE_PARTITION_COUNT:
        return "CL_INVALID_DEVICE_PARTITION_COUNT";

    // extensions
    case CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR:
        return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
    case CL_PLATFORM_NOT_FOUND_KHR:
        return "CL_PLATFORM_NOT_FOUND_KHR";
    case CL_INT_MIN:
        return "CL_INT_MIN (err not set)";
    default:
        return NULL;
    }
}

static void opencl_error(cl_int err, const char *call) __attribute__((noreturn)) ;

static void opencl_error(cl_int err, const char *call)  {
    //TODO: Print chosen driver
	//TODO: Allow customization on what happens on error
    const char *desc = opencl_getErrorString(err);
    if (desc) {
        fprintf(stderr, "%s\nOpenCL error %s\n", call, desc);
    } else {
        fprintf(stderr, "%s\nOpenCL error %" PRIi32 "\n", call, err);
    }
    exit(1);
}

static void __ocl_report_error(const char *errinfo, const void *private_info, size_t cb, void *user_data) {
    fprintf(stderr, "OCL error: %s\n", errinfo);
    exit(1);
}



#define CL_BLOCKING_TRUE CL_TRUE
#define CL_BLOCKING_FALSE CL_FALSE

static cl_command_queue clCreateCommandQueue_checked(cl_context context, cl_device_id device, cl_command_queue_properties properties) {
    cl_int err = CL_INT_MIN;
    cl_command_queue result = clCreateCommandQueue(context, device, properties, &err);
    if (err!=CL_SUCCESS || !result)
        opencl_error(err, "clCreateCommandQueue");
    return result;
}


static void clGetPlatformIDs_checked(cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms) {
    cl_int err = clGetPlatformIDs(num_entries, platforms, num_platforms);
    if (err != CL_SUCCESS)
        opencl_error(err, "clGetPlatformIDs");
}


static void clGetDeviceIDs_checked(cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id *devices, cl_uint *num_devices) {
    cl_int err = clGetDeviceIDs(platform, device_type, num_entries, devices, num_devices);
    if (err != CL_SUCCESS)
        opencl_error(err, "clGetDeviceIDs");
}


static cl_context clCreateContext_checked(const cl_context_properties *properties,
                                               cl_uint num_devices,
                                               const cl_device_id *devices,
                                               void(CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
                                               void *user_data) {
    cl_int err = CL_INT_MIN;
    cl_context result = clCreateContext(properties, num_devices, devices, pfn_notify, user_data, &err);
    if (err != CL_SUCCESS || !result)
        opencl_error(err, "clCreateContext");
    return result;
}

static void clEnqueueWriteBuffer_checked(cl_command_queue    command_queue ,
                     cl_mem              buffer ,
                     cl_bool             blocking_write ,
                     size_t              offset ,
                     size_t             size ,
                     const void *       ptr ,
                     cl_uint             num_events_in_wait_list ,
                     const cl_event *    event_wait_list ,
                     cl_event *          event ) {
	cl_int err = clEnqueueWriteBuffer(command_queue, buffer, blocking_write, offset, size, ptr, num_events_in_wait_list, event_wait_list, event);
	if (err!=CL_SUCCESS)
		opencl_error(err, "clEnqueueWriteBuffer");
}

static  void clEnqueueReadBuffer_checked(cl_command_queue     command_queue ,
                    cl_mem               buffer ,
                    cl_bool              blocking_read ,
                    size_t               offset ,
                    size_t               size ,
                    void *               ptr ,
                    cl_uint              num_events_in_wait_list ,
                    const cl_event *     event_wait_list ,
                    cl_event *           event ){
	cl_int err = clEnqueueReadBuffer(command_queue,buffer, blocking_read,offset,size,ptr,num_events_in_wait_list,event_wait_list,event);
	if (err!=CL_SUCCESS)
		opencl_error(err, "clEnqueueReadBuffer");
}


static void clEnqueueNDRangeKernel_checked(cl_command_queue command_queue,
                                                cl_kernel kernel,
                                                cl_uint work_dim,
                                                const size_t *global_work_offset,
                                                const size_t *global_work_size,
                                                const size_t *local_work_size,
                                                cl_uint num_events_in_wait_list,
                                                const cl_event *event_wait_list,
                                                cl_event *event) {
    cl_int err = clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event);
    if (err)
        opencl_error(err, "clEnqueueNDRangeKernel");
}

static void * clEnqueueMapBuffer_checked(cl_command_queue  command_queue ,
                   cl_mem            buffer ,
                   cl_bool           blocking_map ,
                   cl_map_flags      map_flags ,
                   size_t            offset ,
                   size_t            size ,
                   cl_uint           num_events_in_wait_list ,
                   const cl_event *  event_wait_list ,
                   cl_event *       event ) {
	cl_int err = CL_INT_MIN;
	void *result =  clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, size, num_events_in_wait_list,event_wait_list,event,&err);
	if(err!=CL_SUCCESS)
		opencl_error(err, "clEnqueueMapBuffer");
	return result;
}

static void clFinish_checked(cl_command_queue  command_queue) {
	assert(command_queue);
	cl_int err = clFinish(command_queue);
	if (err!=CL_SUCCESS)
		opencl_error(err, "clFinish");
}

static void clReleaseCommandQueue_checked(cl_command_queue command_queue ) {
	assert(command_queue);
	cl_int err =clReleaseCommandQueue(command_queue);
	if(err!=CL_SUCCESS)
		opencl_error(err, "clReleaseCommandQueue");
}

static cl_mem clCreateBuffer_checked(cl_context context, cl_mem_flags flags, size_t size, void *host_ptr) {
    cl_int err = CL_INT_MIN;
    cl_mem result = clCreateBuffer(context, flags, size, host_ptr, &err);
    if (err || !result)
        opencl_error(err, "clCreateBuffer");
    return result;
}

static void clEnqueueUnmapMemObject_checked(cl_command_queue  command_queue ,
                        cl_mem            memobj ,
                        void *            mapped_ptr ,
                        cl_uint           num_events_in_wait_list ,
                        const cl_event *   event_wait_list ,
                        cl_event *         event) {
	assert(command_queue);
	assert(memobj);
	assert(mapped_ptr);
	assert(num_events_in_wait_list==0 || event_wait_list);
	cl_int err = clEnqueueUnmapMemObject(command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event);
	if(err!=CL_SUCCESS)
		opencl_error(err,"clEnqueueUnmapMemObject");
}


static void clWaitForEvents_checked(cl_uint              num_events ,                const cl_event *     event_list ) {
	assert(num_events>=1);
	assert(event_list);
	cl_int err = clWaitForEvents(num_events, event_list);
	if (err!=CL_SUCCESS)
		opencl_error(err, "clWaitForEvents");
}

static void clWaitForEvent_checked(        cl_event  event ) {
	assert(event);
	clWaitForEvents_checked(1, &event);
}



//TODO: It is not necessary to know size at creation-time
static prl_mem prl_mem_create_empty(size_t size, prl_scop_instance scopinst) {
	assert(size>0);

	prl_mem result = malloc(sizeof *result);
	assert(result);
	memset(result, 0, sizeof *result);

	result->size = size;
	result->transfer_to_device = true;
	result->transfer_to_host = true;

bool is_global = !scopinst;
	if (is_global) {
		// Global/user-managed memory
		// Responsibility to free is at user's
		result->next = global_state.global_mems;
		global_state.global_mems = result;
	} else {
		// Local to SCoP instance
		// prl_scop_leave will free this
		result->next = scopinst->local_mems;
		scopinst->local_mems = result;
	}

	return result;
}



static void prl_mem_alloc_host_only(prl_mem mem) {
	assert(mem);
	assert(mem->type==alloc_type_none);

	mem->type = alloc_type_host_only;
	mem->loc = loc_host;

	mem->host_mem = malloc(mem->size);
	mem->host_readable = true;
	mem->host_writable = true;
	mem->host_owning = true;


}

static void prl_mem_manage_host_only(prl_mem mem, void *host_mem, bool host_take_ownership) {
	assert(mem);
	assert(mem->type==alloc_type_none);
	assert(host_mem);

	mem->type = alloc_type_host_only;
	mem->host_mem =  host_mem;
	mem->host_readable = true;
	mem->host_writable = true;
	mem->host_owning = true;

	mem->loc = loc_host;
}

static cl_mem_flags dev_rw_flags[] = { 0, CL_MEM_READ_ONLY, CL_MEM_WRITE_ONLY, CL_MEM_READ_WRITE };
static void prl_mem_alloc_dev_only(prl_mem mem, bool readable, bool writable, void *initdata) {
	assert(mem);
	assert(mem->type==alloc_type_none);
	assert(readable || writable);

	mem->type = alloc_type_dev_only;
	mem->clmem = clCreateBuffer_checked(global_state.context, dev_rw_flags[(readable?1:0) + (writable?2:0)] | (initdata?CL_MEM_COPY_HOST_PTR:0), mem->size, initdata);
	mem->dev_owning = true;
	mem->dev_readable = readable;
	mem->dev_writable = writable;

	mem->loc = loc_dev;
}


static void prl_mem_manage_dev_only(prl_mem mem, cl_mem dev_mem, bool dev_take_ownership, bool readable, bool writable) {
		assert(mem);
	assert(mem->type==alloc_type_none);
	assert(dev_mem);
	assert(readable || writable);

	mem->type = alloc_type_dev_only;
	mem->clmem = dev_mem;
	mem->dev_owning = dev_take_ownership;
	mem->dev_readable = readable;
	mem->dev_writable = writable;

	mem->loc = loc_dev;
}



static void prl_mem_alloc_map(prl_mem mem, bool host_readable, bool host_writable, bool dev_readable, bool dev_writable, prl_scop_instance scopinst) {
	assert(mem);
	assert(mem->type==alloc_type_none);
	assert(host_readable | host_writable);
	assert(dev_readable | dev_writable);

	mem->type = alloc_type_map;
	mem->loc = loc_host;

	mem->clmem = clCreateBuffer_checked(global_state.context, dev_rw_flags[(dev_readable?1:0) + (dev_writable?2:0)] | CL_MEM_ALLOC_HOST_PTR, mem->size, NULL);
	mem->dev_owning = true;
	mem->dev_readable = dev_readable;
	mem->dev_writable = dev_writable;

	cl_command_queue clqueue;
	if (scopinst)
		clqueue = scopinst->queue;
	 else
	clqueue = clCreateCommandQueue_checked(global_state.context, global_state.device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);

	mem->host_mem = clEnqueueMapBuffer_checked(clqueue, mem->clmem, CL_BLOCKING_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, 0, mem->size, 0, NULL, NULL);
	clFinish(clqueue);

	if (!scopinst)
	clReleaseCommandQueue(clqueue);
	mem->host_owning = true;
	mem->host_readable = host_readable;
	mem->host_writable = host_writable;
}


static void prl_mem_manage_host_map(prl_mem mem, void *host_mem, bool host_take_ownership, bool host_readable, bool host_writable, bool dev_readable, bool dev_writable) {
	assert(mem);
	assert(mem->type==alloc_type_none);
	assert(host_mem);
	assert(host_readable | host_writable);
	assert(dev_readable | dev_writable);

	mem->type = alloc_type_map;
	mem->loc = loc_host;

	mem->clmem = clCreateBuffer_checked(global_state.context, dev_rw_flags[(dev_readable?1:0) + (dev_writable?2:0)] | CL_MEM_USE_HOST_PTR, mem->size, host_mem);
	mem->dev_owning = true;
	mem->dev_readable=dev_readable;
	mem->dev_writable = dev_writable;

	mem->host_mem = host_mem;
	mem->host_owning =host_take_ownership;
	mem->host_readable = host_readable;
	mem->host_writable = host_writable;
}



static prl_mem prl_mem_lookup_global_ptr(void *host_ptr, size_t size) {
	assert(host_ptr);
	assert(size>0);

	char *ptr_begin= host_ptr;
	char *ptr_end = ptr_begin+size;

	prl_mem mem = global_state.global_mems;
	while (mem) {
		assert(!mem->scopinst);

		char *mem_begin = mem->host_mem;
		char *mem_end = mem_begin+mem->size;
		if (mem_begin <= ptr_begin && ptr_begin < mem_end) {
			assert(mem_begin <= ptr_end && ptr_end < mem_end);
			return mem;
		}

		assert(!(mem_begin <= ptr_end && ptr_end < mem_end));

		mem = mem->next;
	}

	return NULL; // Not found
}










static cl_device_type devtypes[] = {
    [PRL_TARGET_DEVICE_CPU_ONLY] CL_DEVICE_TYPE_CPU,
    [PRL_TARGET_DEVICE_GPU_ONLY] CL_DEVICE_TYPE_GPU,
    [PRL_TARGET_DEVICE_CPU_THEN_GPU] CL_DEVICE_TYPE_ALL,
    [PRL_TARGET_DEVICE_GPU_THEN_CPU] CL_DEVICE_TYPE_ALL};

static cl_device_type preftypes[] = {
    [PRL_TARGET_DEVICE_CPU_ONLY] CL_DEVICE_TYPE_CPU,
    [PRL_TARGET_DEVICE_GPU_ONLY] CL_DEVICE_TYPE_GPU,
    [PRL_TARGET_DEVICE_CPU_THEN_GPU] CL_DEVICE_TYPE_CPU,
    [PRL_TARGET_DEVICE_GPU_THEN_CPU] CL_DEVICE_TYPE_GPU};

static void clGetDeviceInfo_checked(cl_device_id device,
                                    cl_device_info param_name,
                                    size_t param_value_size,
                                    void *param_value,
                                    size_t *param_value_size_ret) {
    cl_int err = clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret);
    if (err != CL_SUCCESS)
        opencl_error(err, "clGetDeviceInfo");
}

static int is_preferable_device(cl_device_type old_type, cl_device_type alt_type) {
    assert(alt_type);
    if (!old_type)
        return 1;

    cl_device_type preferred_type = preftypes[global_config.device_choice];
if (!(old_type & preferred_type) && (alt_type & preferred_type))
	return 1;

if (!(alt_type & preferred_type) && (old_type & preferred_type))
	return -1;

    return 0;
}

static void prl_release();

static void prl_init() {
    if (prl_initialized)
        return;

    enum prl_device_choice effective_device_choice = global_config.device_choice;
    int effective_platform = global_config.chosed_platform;
    int effective_device = global_config.chosen_device;
    const char *targetdev = getenv(PRL_TARGET_DEVICE);
    if (targetdev) {
        int env_platform, env_device;
        if (!strcmp(targetdev, "first"))
            effective_device_choice = PRL_TARGET_DEVICE_FIRST;
        else if (!strcmp(targetdev, "cpu"))
            effective_device_choice = PRL_TARGET_DEVICE_CPU_ONLY;
        else if (!strcmp(targetdev, "cpu_gpu"))
            effective_device_choice = PRL_TARGET_DEVICE_CPU_THEN_GPU;
        else if (!strcmp(targetdev, "gpu"))
            effective_device_choice = PRL_TARGET_DEVICE_GPU_ONLY;
        else if (!strcmp(targetdev, "gpu_cpu"))
            effective_device_choice = PRL_TARGET_DEVICE_GPU_THEN_CPU;
        else if (sscanf(targetdev, "%d:%d", &env_platform, &env_device) == 2) {
            // reasonable limits
            assert(0 <= env_platform && env_platform <= 255);
            assert(0 <= env_device && env_device <= 255);
            effective_device_choice = PRL_TARGET_DEVICE_FIXED;
            effective_platform = env_platform;
            effective_device = env_device;
        } else {
            fputs("cannot read env PRL_TARGET_DEVICE\n", stderr); //TODO: Central error handling
            exit(1);
        }
    }

    cl_device_id best_device = NULL;
    switch (effective_device_choice) {
    case PRL_TARGET_DEVICE_FIRST: {
        cl_platform_id platform = NULL;
        clGetPlatformIDs_checked(1, &platform, NULL);
        assert(platform);
        clGetDeviceIDs_checked(platform, CL_DEVICE_TYPE_DEFAULT, 1, &best_device, NULL);
    } break;
    case PRL_TARGET_DEVICE_FIXED: {
        cl_platform_id *platforms = malloc((effective_platform + 1) * sizeof *platforms);
        cl_uint received_platforms = 0;
        clGetPlatformIDs_checked(effective_platform + 1, platforms, &received_platforms);
        assert(received_platforms == effective_platform + 1);
        cl_platform_id platform = platforms[effective_platform];
        free(platform);

        assert(platform);
        cl_device_id *devices = malloc((effective_device + 1) * sizeof *devices);
        cl_uint received_devices = 0;
        clGetDeviceIDs_checked(platform, CL_DEVICE_TYPE_ALL, effective_device + 1, devices, &received_devices);
        assert(received_devices == effective_device + 1);
        best_device = devices[effective_device];
        free(devices);
    } break;
    case PRL_TARGET_DEVICE_CPU_ONLY:
    case PRL_TARGET_DEVICE_CPU_THEN_GPU:
    case PRL_TARGET_DEVICE_GPU_ONLY:
    case PRL_TARGET_DEVICE_GPU_THEN_CPU: {
        cl_platform_id best_platform = NULL;
        cl_device_type best_type = 0;

        cl_uint num_platforms = 0;
        clGetPlatformIDs_checked(0, NULL, &num_platforms);
        cl_platform_id *platforms = malloc(num_platforms * sizeof *platforms);
        clGetPlatformIDs_checked(num_platforms, platforms, NULL);

        for (int i = 0; i < num_platforms; i += 1) {
            cl_uint num_devices = 0;
            clGetDeviceIDs_checked(platforms[i], devtypes[global_config.device_choice], 0, NULL, &num_devices);
            cl_device_id *devices = malloc(num_devices * sizeof *devices);
            clGetDeviceIDs_checked(platforms[i], devtypes[global_config.device_choice], num_devices, devices, NULL);

            for (int j = 0; j < num_devices; j += 1) {
                cl_device_type devtype;
                clGetDeviceInfo_checked(devices[j], CL_DEVICE_TYPE, sizeof devtype, &devtype, NULL);

                if (!best_device || is_preferable_device(best_type, devtype)) {
                    best_platform = platforms[i];
                    best_device = devices[j];
                    best_type = devtype;
                }
            }
            free(devices);
        }
        free(platforms);

        assert(best_platform);
        assert(best_type);
    } break;
    default:
        assert(false);
    }
    assert(best_device);

    global_state.device = best_device;
    global_state.context = clCreateContext_checked(NULL, 1, &best_device, __ocl_report_error, NULL);
	atexit(prl_release);

    prl_initialized = 1;
    global_state.initialized = 1;
}

static void prl_release() {
    if (!prl_initialized)
        return;

    prl_program program = global_state.programs;
    while (program) {

        prl_kernel kernel = program->kernels;
        while (kernel) {
            prl_kernel nextkernel = kernel->next;

            clReleaseKernel(kernel->kernel);
            free(kernel);

            kernel = nextkernel;
        }

        prl_program nextprogram = program->next;
        clReleaseProgram(program->program);
        free(program);

        program = nextprogram;
    }

    prl_scop scop = global_state.scops;
    while (scop) {
        prl_scop nextscop = scop->next;
        free(scop);
        scop = nextscop;
    }

    prl_mem mem = global_state.global_mems;
    while (mem) {
        prl_mem nextmem = mem->next;
        clReleaseMemObject(mem->clmem);
        free(nextmem);
        mem = nextmem;
    }

    prl_initialized = 0;
}

prl_scop_instance prl_scop_enter(prl_scop *scopref) {
    prl_init();

    prl_scop scop = *scopref;
    if (!scop) {
        scop = malloc(sizeof *scop);
        *scopref = scop;
    }

    cl_command_queue clqueue = clCreateCommandQueue_checked(global_state.context, global_state.device, global_config.gpu_profiling ? CL_QUEUE_PROFILING_ENABLE : 0);

    prl_scop_instance scopinst = malloc(sizeof *scopinst);
    scopinst->scop = scop;
    scopinst->queue = clqueue;
    return scopinst;
}

void prl_scop_leave(prl_scop_instance scopinst) {
    assert(scopinst);
    assert(prl_initialized);

    clFinish(scopinst->queue);
    clReleaseCommandQueue(scopinst->queue);
    free(scopinst);
}

void prl_scop_program_from_file(prl_scop_instance scopinst, prl_program *programref, const char *filename) {
    assert(scopinst);
    assert(programref);
    assert(filename);

    if (*programref)
        return;

    FILE *file = fopen(filename, "r");
    assert(file); //TODO: Runtime checking
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    assert(size >= 0);
    rewind(file);

    char *str = malloc(size + 1);
    size_t read = fread(str, sizeof(char), size, file);
    assert(read == size);
    fclose(file);

    str[size] = '\0';
    prl_scop_program_from_str(scopinst, programref, str);
    free(str);
}

#define clCreateProgramWithSource_checked(context, count, strings, lengths) clCreateProgramWithSource_checked_impl(context, count, strings, lengths, __FILE__, __LINE__)
static cl_program clCreateProgramWithSource_checked_impl(cl_context context, cl_uint count, const char **strings, const size_t *lengths, const char *file, int line) {
    cl_int err = CL_INT_MIN;
    cl_program result = clCreateProgramWithSource(context, count, strings, lengths, &err);
    if (err != 0)
        opencl_error(err, "clCreateProgramWithSource");
    return result;
}

void prl_scop_program_from_str(prl_scop_instance scopinst, prl_program *programref, const char *str) {
    assert(scopinst);
    assert(programref);
    assert(str);
    assert(prl_initialized);

    prl_program program = *programref;
    if (!program) {
        size_t size = strlen(str);
        cl_program clprogram = clCreateProgramWithSource_checked(global_state.context, 1, &str, &size);

        cl_int err = clBuildProgram(clprogram, 0, NULL, NULL, NULL, NULL);
        if (err < 0) { //TODO: Unified error handling
            fprintf(stderr, "Error during program build\n");
            size_t msgs_size;
            err = clGetProgramBuildInfo(clprogram, global_state.device, CL_PROGRAM_BUILD_LOG, 0, NULL, &msgs_size);
            char *msgs = malloc(msgs_size + 1);
            err = clGetProgramBuildInfo(clprogram, global_state.device, CL_PROGRAM_BUILD_LOG, msgs_size, msgs, NULL);
            assert(err >= 0);
            msgs[msgs_size] = '\0';
            fputs(msgs, stderr);
            free(msgs);
        }
        assert(err >= 0);

        program = malloc(sizeof *program);
        program->program = clprogram;
        program->next = global_state.programs;
        global_state.programs = program;
        *programref = program;
    }
    assert(program->program);
}

void prl_scop_init_kernel(prl_scop_instance scop, prl_kernel *kernelref, prl_program program, const char *kernelname) {
    assert(scop);
    assert(kernelref);
    assert(kernelname);

    prl_kernel kernel = *kernelref;
    if (!kernel) {
        cl_int err;
        cl_kernel clkernel = clCreateKernel(program->program, kernelname, &err);
        assert(err >= 0);

        kernel = malloc(sizeof *kernel);
        kernel->kernel = clkernel;
        *kernelref = kernel;
    }
    assert(kernel->kernel);
}






prl_mem prl_scop_get_mem(prl_scop_instance scopinst, void *host_mem, size_t size) {
	assert(scopinst);
	assert(host_mem);
	assert(size>0);

	prl_mem gmem = prl_mem_lookup_global_ptr(host_mem, size);
	if (gmem)
		return gmem;

	// If it is not a user-allocated memory location, create a temporary local one
	prl_mem lmem = prl_mem_create_empty(size, scopinst);
	prl_mem_manage_host_map(lmem, host_mem, false, true, true, true, true);
	return lmem;
}


// Change location of buffer without necessarily preserving its contents
static void ensure_on_device(prl_scop_instance scopinst, prl_mem mem) {
   assert(scopinst);
    assert(mem);

    if (mem->loc==loc_dev) {
	    // Nothing to do
	    return;
    }

    switch(mem->type) {
case alloc_type_rwbuf:
	mem->loc = loc_dev;
	break;

case alloc_type_map: {
	//TODO: Some method to tell to NOT transfer any data?
	cl_event event = NULL;
clEnqueueUnmapMemObject_checked(scopinst->queue, mem->clmem, mem->host_mem, 0, NULL, &event);
		clWaitForEvent_checked(event);
mem->loc = loc_dev;
} break;

	default:
		assert(false);
    }
}

// Change location of buffer without necessarily preserving its contents
static void ensure_on_host(prl_scop_instance scopinst, prl_mem mem) {
	 assert(scopinst);
    assert(mem);

        if (mem->loc==loc_host) {
	    // Nothing to do
	    return;
    }

        switch(mem->type) {
case alloc_type_rwbuf:
	mem->loc = loc_host;
	break;

case alloc_type_map:{
	void *host_ptr = clEnqueueMapBuffer_checked(scopinst->queue, mem->clmem, CL_BLOCKING_TRUE, (mem->host_writable ? CL_MAP_WRITE : 0), 0, mem->size, 0, NULL, NULL);
	assert(host_ptr == mem->host_mem); mem->loc = loc_host;
}break;

	default:
		assert(false);
    }
}


void prl_scop_host_to_device(prl_scop_instance scopinst, prl_mem mem) {
    assert(scopinst);
    assert(mem);

    if (mem->loc==loc_dev) {
	    // Nothing to do
	    return;
    }

    switch(mem->type) {
	case alloc_type_rwbuf:
		clEnqueueWriteBuffer_checked(scopinst->queue, mem->clmem, CL_BLOCKING_TRUE, 0, mem->size, mem->host_mem, 0, NULL, NULL);
		mem->loc = loc_dev;
		break;

	case alloc_type_map:{
		cl_event event = NULL;
		clEnqueueUnmapMemObject_checked(scopinst->queue, mem->clmem, mem->host_mem, 0, NULL, &event);
		clWaitForEvent_checked(event);
		mem->loc = loc_dev;
	}break;

	case alloc_type_dev_only:
		// Nothing to do
		break;

	case alloc_type_host_only:
	case alloc_type_none:
	case alloc_type_svm:
		assert(false);
    }

    assert(mem->loc==loc_dev || mem->loc==loc_transferring_to_dev);
}

void prl_scop_device_to_host(prl_scop_instance scopinst, prl_mem mem) {
    assert(scopinst);
    assert(mem);

    if (mem->loc==loc_host) {
    // Already on host
	    return;
}

    switch(mem->type) {
	case alloc_type_rwbuf:
		  clEnqueueReadBuffer_checked(scopinst->queue, mem->clmem, CL_BLOCKING_TRUE, 0, mem->size, mem->host_mem, 0, NULL, NULL);
		  mem->loc = loc_host;
		  break;

	case alloc_type_map: {
		void *mappedptr = clEnqueueMapBuffer_checked(scopinst->queue, mem->clmem,  CL_BLOCKING_TRUE, (mem->dev_readable ? CL_MAP_READ : 0) | (mem->dev_writable ? CL_MAP_WRITE : 0), 0, mem->size, 0, NULL, NULL);
		assert(mappedptr == mem->host_mem && "clEnqueueMapBuffer should always return the same pointer");
		mem->loc = loc_host;
	} break;

		case alloc_type_host_only:
		// Nothing to do
		break;

	case alloc_type_dev_only:
	case alloc_type_none:
		case alloc_type_svm:
		assert(false);
    }
assert(mem->loc==loc_host|| mem->loc==loc_transferring_to_host);
}


    void prl_scop_call(prl_scop_instance scopinst, prl_kernel kernel, int dims, size_t grid_size[static const restrict dims], size_t block_size[static const restrict dims], size_t n_args, struct prl_kernel_call_arg args[static const restrict n_args]) {
    assert(scopinst);
    assert(kernel);
    assert(dims > 0);
    assert(dims <= 3);
    assert(grid_size);
    assert(block_size);

    for (int i = 0; i < n_args; i += 1) {
        struct prl_kernel_call_arg *arg = &args[i];
        cl_int err = -1;
        switch (arg->type) {
        case prl_kernel_call_arg_value:
            err = clSetKernelArg(kernel->kernel, i, arg->size, arg->data);
            break;
        case prl_kernel_call_arg_mem: {
		assert(arg->mem);
		assert(arg->mem->clmem);
		ensure_on_device(scopinst, arg->mem);
            err = clSetKernelArg(kernel->kernel, i, sizeof(cl_mem), &arg->mem->clmem);
        } break;
        }
        assert(err >= 0);
    }

    size_t work_items[3];
    for (int i = 0; i < dims; i += 1) {
        work_items[i] = grid_size[i] * block_size[i];
    }

    cl_event event = NULL;
    clEnqueueNDRangeKernel_checked(scopinst->queue, kernel->kernel, dims, NULL, work_items, block_size, 0, NULL,
                                   (global_config.gpu_profiling || global_config.blocking) ? &event : NULL);
    if (global_config.blocking) {
        cl_int err = clWaitForEvents(1, &event);
	assert(err==CL_SUCCESS);
    }
}



