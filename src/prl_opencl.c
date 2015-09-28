#if defined(__APPLE__)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include "prl.h"

static const char *PRL_TARGET_DEVICE = "PRL_TARGET_DEVICE";
static const char *PRL_BLOCKING = "PRL_BLOCKING";
//static const char *PRL_PREFERRED_TRANSFER = "PRL_TRANSFER"; // Select a preferred transfer mode (clEnqueueRead/WriteBuffer, clEnqueueMapBuffer, ...)

static const char *PRL_PREFIX = "PRL_PREFIX";

static const char *PRL_PROFILING_PREFIX = "PRL_PROFILING_PREFIX";
static const char *PRL_PROFILING = "PRL_PROFILING";
static const char *PRL_CPU_PROFILING = "PRL_CPU_PROFILING";
static const char *PRL_GPU_PROFILING = "PRL_GPU_PROFILING";
static const char *PRL_GPU_PROFILING_DETAILED = "PRL_GPU_PROFILING_DETAILED"; // Print duration of every queue item

static const char *PRL_TIMINGS_PREFIX = "PRL_TIMINGS_PREFIX";
static const char *PRL_TIMINGS_RUNS = "PRL_TIMINGS_RUNS";
static const char *PRL_TIMINGS_DRY_RUNS = "PRL_TIMINGS_DRY_RUNS";

typedef int64_t prl_time_t; // Enough for 292 years in nanosecond resolution

enum prl_device_choice {
    PRL_TARGET_DEVICE_FIRST,
    PRL_TARGET_DEVICE_FIXED,

    PRL_TARGET_DEVICE_CPU_ONLY,
    PRL_TARGET_DEVICE_CPU_THEN_GPU,
    PRL_TARGET_DEVICE_CPU_THEN_ACC,
    PRL_TARGET_DEVICE_CPU_THEN_GPU_THEN_ACC,
    PRL_TARGET_DEVICE_CPU_THEN_ACC_THEN_GPU,

    PRL_TARGET_DEVICE_GPU_ONLY,
    PRL_TARGET_DEVICE_GPU_THEN_CPU,
    PRL_TARGET_DEVICE_GPU_THEN_ACC,
    PRL_TARGET_DEVICE_GPU_THEN_CPU_THEN_ACC,
    PRL_TARGET_DEVICE_GPU_THEN_ACC_THEN_CPU,

    PRL_TARGET_DEVICE_ACC_ONLY,
    PRL_TARGET_DEVICE_ACC_THEN_CPU,
    PRL_TARGET_DEVICE_ACC_THEN_GPU,
    PRL_TARGET_DEVICE_ACC_THEN_CPU_THEN_GPU,
    PRL_TARGET_DEVICE_ACC_THEN_GPU_THEN_CPU,
};

struct prl_global_config {
    enum prl_device_choice device_choice;
    int chosed_platform;
    int chosen_device;

    bool blocking;

    const char *bench_prefix;
    bool cpu_profiling;
    bool gpu_profiling;
    bool gpu_detailed_profiling;
    bool profiling_dump_on_release;
    const char *profiling_prefix;

    int timing_runs;
    int timing_warmups;
};
static struct prl_global_config global_config = {
  .device_choice = PRL_TARGET_DEVICE_FIRST,
  .chosed_platform = 0,
  .chosen_device = 0,

  .blocking = false,

  .bench_prefix = "",
  .cpu_profiling = false,
  .gpu_profiling = false,
  .profiling_prefix = "",

  .timing_runs = 10,
  .timing_warmups = 1,
};

enum prl_stat_entry {
    stat_cpu_global, // CPU time since prl_init()
    stat_cpu_bench,  // CPU time between prl_prof_start() and prl_prof_stop()
    stat_cpu_scop,   // CPU time spent in scops

    // system calls
    stat_cpu_malloc,
    stat_cpu_realloc,
    stat_cpu_free,

    // OpenCL calls
    stat_cpu_clCreateContext,
    stat_cpu_clGetPlatformIDs,
    stat_cpu_clCreateCommandQueue,
    stat_cpu_clGetDeviceIDs,
    stat_cpu_clReleaseMemObject,
    stat_cpu_clEnqueueWriteBuffer,
    stat_cpu_clEnqueueReadBuffer,
    stat_cpu_clEnqueueNDRangeKernel,
    stat_cpu_clEnqueueMapBuffer,
    stat_cpu_clFinish,
    stat_cpu_clReleaseCommandQueue,
    stat_cpu_clCreateBuffer,
    stat_cpu_clEnqueueUnmapMemObject,
    stat_cpu_clReleaseEvent,
    stat_cpu_clWaitForEvents,
    stat_cpu_clGetEventProfilingInfo,
    stat_cpu_clGetEventInfo,
    stat_cpu_clGetPlatformInfo,
    stat_cpu_clCreateKernel,
    stat_cpu_clSetKernelArg,
    stat_cpu_clReleaseKernel,
    stat_cpu_clGetDeviceInfo,
    stat_cpu_clGetMemObjectInfo,

    // OpenCL profiling
    stat_gpu_total, // Time spent on GPU from start of first task to end of last task

    stat_gpu_working, // Time spent on GPU whith at least one task in progress
    stat_gpu_idle,    // Idle holes between tasks of one SCoP

    stat_gpu_transfer_to_device,
    stat_gpu_transfer_to_host,
    stat_gpu_compute,

    stat_gpu_NDRANGE_KERNEL,
    stat_gpu_TASK,
    stat_gpu_NATIVE_KERNEL,
    stat_gpu_READ_BUFFER,
    stat_gpu_WRITE_BUFFER,
    stat_gpu_COPY_BUFFER,
    stat_gpu_READ_IMAGE,
    stat_gpu_WRITE_IMAGE,
    stat_gpu_COPY_IMAGE,
    stat_gpu_COPY_IMAGE_TO_BUFFER,
    stat_gpu_COPY_BUFFER_TO_IMAGE,
    stat_gpu_MAP_BUFFER,
    stat_gpu_MAP_IMAGE,
    stat_gpu_UNMAP_MEM_OBJECT,
    stat_gpu_MARKER,
    stat_gpu_ACQUIRE_GL_OBJECTS,
    stat_gpu_RELEASE_GL_OBJECTS,
    stat_gpu_READ_BUFFER_RECT,
    stat_gpu_WRITE_BUFFER_RECT,
    stat_gpu_COPY_BUFFER_RECT,
    stat_gpu_USER,
    stat_gpu_BARRIER,
    stat_gpu_MIGRATE_MEM_OBJECTS,
    stat_gpu_FILL_BUFFER,
    stat_gpu_FILL_IMAGE,
    stat_gpu_other,
};
#define STAT_ENTRIES (stat_gpu_other + 1)
#define STAT_CPU_FIRST stat_cpu_scop
#define STAT_CPU_LAST stat_cpu_clGetDeviceInfo
#define STAT_GPU_FIRST stat_gpu_total
#define STAT_GPU_LAST stat_gpu_other

static enum prl_stat_entry clcommand_to_stat_entry(cl_int command) {
    switch (command) {
    case CL_COMMAND_NDRANGE_KERNEL:
        return stat_gpu_NDRANGE_KERNEL;
    case CL_COMMAND_TASK:
        return stat_gpu_TASK;
    case CL_COMMAND_NATIVE_KERNEL:
        return stat_gpu_NATIVE_KERNEL;
    case CL_COMMAND_READ_BUFFER:
        return stat_gpu_READ_BUFFER;
    case CL_COMMAND_WRITE_BUFFER:
        return stat_gpu_WRITE_BUFFER;
    case CL_COMMAND_COPY_BUFFER:
        return stat_gpu_COPY_BUFFER;
    case CL_COMMAND_READ_IMAGE:
        return stat_gpu_READ_IMAGE;
    case CL_COMMAND_WRITE_IMAGE:
        return stat_gpu_WRITE_IMAGE;
    case CL_COMMAND_COPY_IMAGE:
        return stat_gpu_COPY_IMAGE;
    case CL_COMMAND_COPY_IMAGE_TO_BUFFER:
        return stat_gpu_COPY_IMAGE_TO_BUFFER;
    case CL_COMMAND_COPY_BUFFER_TO_IMAGE:
        return stat_gpu_COPY_BUFFER_TO_IMAGE;
    case CL_COMMAND_MAP_BUFFER:
        return stat_gpu_MAP_BUFFER;
    case CL_COMMAND_MAP_IMAGE:
        return stat_gpu_MAP_IMAGE;
    case CL_COMMAND_UNMAP_MEM_OBJECT:
        return stat_gpu_UNMAP_MEM_OBJECT;
    case CL_COMMAND_MARKER:
        return stat_gpu_MARKER;
    case CL_COMMAND_ACQUIRE_GL_OBJECTS:
        return stat_gpu_ACQUIRE_GL_OBJECTS;
    case CL_COMMAND_RELEASE_GL_OBJECTS:
        return stat_gpu_RELEASE_GL_OBJECTS;
    case CL_COMMAND_READ_BUFFER_RECT:
        return stat_gpu_READ_BUFFER_RECT;
    case CL_COMMAND_WRITE_BUFFER_RECT:
        return stat_gpu_WRITE_BUFFER_RECT;
    case CL_COMMAND_COPY_BUFFER_RECT:
        return stat_gpu_COPY_BUFFER_RECT;
    case CL_COMMAND_USER:
        return stat_gpu_USER;
    case CL_COMMAND_BARRIER:
        return stat_gpu_BARRIER;
    case CL_COMMAND_MIGRATE_MEM_OBJECTS:
        return stat_gpu_MIGRATE_MEM_OBJECTS;
    case CL_COMMAND_FILL_BUFFER:
        return stat_gpu_FILL_BUFFER;
    case CL_COMMAND_FILL_IMAGE:
        return stat_gpu_FILL_IMAGE;
    default:
        return stat_gpu_other;
    }
}

static const char *statname[] = {
    [stat_cpu_scop] = "time in SCoPs",

    [stat_cpu_malloc] = "malloc",
    [stat_cpu_realloc] = "realloc",
    [stat_cpu_free] = "free",

    [stat_cpu_clCreateContext] = "clCreateContext",
    [stat_cpu_clGetPlatformIDs] = "clGetPlatformIDs",
    [stat_cpu_clCreateCommandQueue] = "clCreateCommandQueue",
    [stat_cpu_clGetDeviceIDs] = "clGetDeviceIDs",
    [stat_cpu_clReleaseMemObject] = "clReleaseMemObject",
    [stat_cpu_clEnqueueWriteBuffer] = "clEnqueueWriteBuffer",
    [stat_cpu_clEnqueueReadBuffer] = "clEnqueueReadBuffer",
    [stat_cpu_clEnqueueNDRangeKernel] = "clEnqueueNDRangeKernel",
    [stat_cpu_clEnqueueMapBuffer] = "clEnqueueMapBuffer",
    [stat_cpu_clFinish] = "clFinish",
    [stat_cpu_clReleaseCommandQueue] = "clReleaseCommandQueue",
    [stat_cpu_clCreateBuffer] = "clCreateBuffer",
    [stat_cpu_clEnqueueUnmapMemObject] = "clEnqueueUnmapMemObject",
    [stat_cpu_clReleaseEvent] = "clReleaseEvent",
    [stat_cpu_clWaitForEvents] = "clWaitForEvents",
    [stat_cpu_clGetEventProfilingInfo] = "clGetEventProfilingInfo",
    [stat_cpu_clGetEventInfo] = "clGetEventInfo",
    [stat_cpu_clGetPlatformInfo] = "clGetPlatformInfo",
    [stat_cpu_clCreateKernel] = "clCreateKernel",
    [stat_cpu_clSetKernelArg] = "clSetKernelArg",
    [stat_cpu_clReleaseKernel] = "clReleaseKernel",
    [stat_cpu_clGetDeviceInfo] = "clGetDeviceInfo",
    [stat_cpu_clGetMemObjectInfo] = "clGetMemObjectInfo",

    [stat_gpu_total] = "total",
    [stat_gpu_working] = "working",
    [stat_gpu_idle] = "idle",

    [stat_gpu_transfer_to_device] = "host->dev",
    [stat_gpu_transfer_to_host] = "dev->host",
    [stat_gpu_compute] = "compute",

    [stat_gpu_NDRANGE_KERNEL] = "CL_COMMAND_NDRANGE_KERNEL",
    [stat_gpu_TASK] = "CL_COMMAND_TASK",
    [stat_gpu_NATIVE_KERNEL] = "CL_COMMAND_NATIVE_KERNEL",
    [stat_gpu_READ_BUFFER] = "CL_COMMAND_READ_BUFFER",
    [stat_gpu_WRITE_BUFFER] = "CL_COMMAND_WRITE_BUFFER",
    [stat_gpu_COPY_BUFFER] = "CL_COMMAND_COPY_BUFFER",
    [stat_gpu_READ_IMAGE] = "CL_COMMAND_READ_IMAGE",
    [stat_gpu_WRITE_IMAGE] = "CL_COMMAND_WRITE_IMAGE",
    [stat_gpu_COPY_IMAGE] = "CL_COMMAND_COPY_IMAGE",
    [stat_gpu_COPY_IMAGE_TO_BUFFER] = "CL_COMMAND_COPY_IMAGE_TO_BUFFER",
    [stat_gpu_COPY_BUFFER_TO_IMAGE] = "CL_COMMAND_COPY_BUFFER_TO_IMAGE",
    [stat_gpu_MAP_BUFFER] = "CL_COMMAND_MAP_BUFFER",
    [stat_gpu_MAP_IMAGE] = "CL_COMMAND_MAP_IMAGE",
    [stat_gpu_UNMAP_MEM_OBJECT] = "CL_COMMAND_UNMAP_MEM_OBJECT",
    [stat_gpu_MARKER] = "CL_COMMAND_MARKER",
    [stat_gpu_ACQUIRE_GL_OBJECTS] = "CL_COMMAND_ACQUIRE_GL_OBJECTS",
    [stat_gpu_RELEASE_GL_OBJECTS] = "CL_COMMAND_RELEASE_GL_OBJECTS",
    [stat_gpu_READ_BUFFER_RECT] = "CL_COMMAND_READ_BUFFER_RECT",
    [stat_gpu_WRITE_BUFFER_RECT] = "CL_COMMAND_WRITE_BUFFER_RECT",
    [stat_gpu_COPY_BUFFER_RECT] = "CL_COMMAND_COPY_BUFFER_RECT",
    [stat_gpu_USER] = "CL_COMMAND_USER",
    [stat_gpu_BARRIER] = "CL_COMMAND_BARRIER",
    [stat_gpu_MIGRATE_MEM_OBJECTS] = "CL_COMMAND_MIGRATE_MEM_OBJECTS",
    [stat_gpu_FILL_BUFFER] = "CL_COMMAND_FILL_BUFFER",
    [stat_gpu_FILL_IMAGE] = "CL_COMMAND_FILL_BUFFER",
    [stat_gpu_other] = "other",
};

typedef prl_time_t prl_stat_list[STAT_ENTRIES];
typedef int prl_count_list[STAT_ENTRIES];

struct prl_stat {
    prl_stat_list entries;
    prl_count_list counts;

#if 0
	prl_time_t cpu_malloc_time;
	prl_time_t cpu_realloc_time;
	prl_time_t cpu_free_time;
	prl_time_t cpu_clCreateContext_time;
	prl_time_t cpu_clGetPlatformIDs_time;
	prl_time_t cpu_clCreateCommandQueue_time;
	prl_time_t cpu_clGetDeviceID_time;

	prl_time_t cpu_scop_time;

	prl_time_t gpu_working ;
	prl_time_t gpu_idle;

	prl_time_t gpu_transfer_to_device;
	prl_time_t gpu_compute;
	prl_time_t gpu_transfer_to_host;
#endif
};

static int cmp_time(const void *lhs_arg, const void *rhs_arg) {
    const prl_time_t *lhs = lhs_arg;
    const prl_time_t *rhs = rhs_arg;
    return (*lhs > *rhs) - (*lhs < *rhs);
}

static double sqrd(double val) {
    return val * val;
}

struct prl_global_state {
    prl_time_t prl_start;
    struct prl_global_config config;

    cl_platform_id platform;
    cl_device_id device;
    cl_context context;

    // Profiling
    struct prl_stat global_stat;
    struct prl_stat nonscop_stat;

    // Benchmarking
    struct prl_stat prev_global_stat;
    prl_time_t bench_start;
    size_t bench_stats_size;
    struct prl_stat *bench_stats;

    // linked lists
    prl_program programs;
    prl_scop scops;

    // doubly linked list (prl_mem->global_mem_next, prl_mem->global_mem_prev)
    // Needed to look up
    prl_mem global_mems;
};

struct prl_scop_struct {
    prl_scop next;
};

enum pending_event_type {
    pending_other,
    pending_transfer,
    pending_compute,
};

struct prl_pending_event {
    cl_event event;
    enum pending_event_type type;
    bool reported;
    union {
        prl_mem mem;
        prl_kernel kernel;
    };
};

struct prl_scop_inst_struct {
    prl_scop scop;
    prl_time_t scop_start;
    cl_command_queue queue;

    size_t event_size;
    struct prl_pending_event *pending_events;

    struct prl_stat stat;

    prl_mem local_mems; //linked list

    // global mems that are accessed in this scopinstance
    size_t mems_size;
    prl_mem *mems;
};

struct prl_program_struct {
    char *filename;

    cl_program program;

    prl_kernel kernels;
    prl_program next;
};

struct prl_kernel_struct {
    prl_scop scop;
    prl_program program;
    char *name;

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
    // Buffer is readable and contains the most recent data
    loc_bit_host_is_current = 1 << 0,
    loc_bit_dev_is_current = 1 << 1,

    // Buffer is ready to be written
    loc_bit_host_writable = 1 << 2,
    loc_bit_dev_writable = 1 << 3,

    loc_bit_transferring_dev_to_host = 1 << 4,
    loc_bit_transferring_host_to_dev = 2 << 4,

    // For alloc_type_map
    loc_bit_mapped = 1 << 6,

    // buffer content is undefined
    loc_none = 0,

    loc_host = loc_bit_host_is_current | loc_bit_host_writable,
    loc_dev = loc_bit_dev_is_current | loc_bit_dev_writable,

    loc_transferring_to_host = loc_bit_transferring_dev_to_host,
    loc_transferring_to_dev = loc_bit_transferring_host_to_dev,

    loc_map_host = loc_host | loc_bit_mapped,
    loc_map_dev = loc_dev,
    loc_map_mapping = loc_transferring_to_host | loc_bit_mapped,
    loc_map_unmapping = loc_bit_transferring_host_to_dev,
};

#define loc_mask_current (loc_bit_host_is_current | loc_bit_dev_is_current)
#define loc_mask_transferring (loc_bit_transferring_dev_to_host | loc_bit_transferring_host_to_dev)

/* Describes a memory region */
// Possible scopes:
// 1. ad-hoc:   automatically allocated and released
// 2. manually: user-allocated and released
struct prl_mem_struct {
    // If this is a ad-hoc allocation, this is the scope it is used in (and can be freed when on leaving)
    prl_scop_instance scopinst; //RENAME: scopinst
                                //bool is_global;
    size_t size;
    char *name;
    enum prl_alloc_type type;
    cl_event transferevent;

    void *host_mem;   //RENAME: host_ptr
    bool host_owning; // Whether to free(host_mem) when releasing this prl_mem
    bool host_exposed;
    bool host_readable;
    bool host_writable;
    //bool host_dirty;
    //bool host_current;

    cl_mem clmem; //RENAME: dev_clmem
    bool dev_owning;
    bool dev_exposed;
    bool dev_readable;
    bool dev_writable;
    //bool dev_dirty;
    //bool dev_current;

    // Where the current buffer content resides
    enum prl_alloc_current_location loc;


    bool transfer_to_device;     // On entering a SCoP:
    bool transfer_to_host;   // On leaving a SCoP:

    prl_mem mem_prev, mem_next; // of prl_scop_instance->local_mems OR global_state.global_mems
};

struct prl_scopinst_mem_struct {
    prl_mem mem_block;
    bool owning;
};

static bool prl_initialized = false;
static struct prl_global_state global_state;

static prl_time_t timestamp() {
    if (!global_state.config.cpu_profiling)
        return 0;

    struct timespec stamp;
    int err = clock_gettime(CLOCK_MONOTONIC_RAW, &stamp);
    assert(!err);
    prl_time_t result = stamp.tv_sec;
    result *= 1000000000L;
    result += stamp.tv_nsec;
    return result;
}

#define NOSCOPINST ((prl_scop_instance)NULL)

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

//static void opencl_error(cl_int err, const char *call) __attribute__((noreturn)) ;

static void opencl_error(cl_int err, const char *call) {
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

static struct prl_stat *scopstat(prl_scop_instance scopinst) {
    if (scopinst)
        return &scopinst->stat;
    return &global_state.nonscop_stat;
}

static void add_time(prl_scop_instance scopinst, enum prl_stat_entry entry, prl_time_t duration) {
    assert(duration >= 0);

    scopstat(scopinst)->entries[entry] += duration;
    scopstat(scopinst)->counts[entry] += 1;
    global_state.global_stat.entries[entry] += duration;
    global_state.global_stat.counts[entry] += 1;
}

static void clReleaseMemObject_checked(prl_scop_instance scopinst, cl_mem memobj) {
    assert(memobj);

    prl_time_t start = timestamp();
    cl_int err = clReleaseMemObject(memobj);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clReleaseMemObject, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clReleaseMemObject");
}

static cl_command_queue clCreateCommandQueue_checked(prl_scop_instance scopinst, cl_context context, cl_device_id device, cl_command_queue_properties properties) {
    cl_int err = CL_INT_MIN;

    prl_time_t start = timestamp();
    cl_command_queue result = clCreateCommandQueue(context, device, properties, &err);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clCreateCommandQueue, stop - start);

    if (err != CL_SUCCESS || !result)
        opencl_error(err, "clCreateCommandQueue");
    return result;
}

static void clGetPlatformIDs_checked(prl_scop_instance scopinst, cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms) {
    prl_time_t start = timestamp();
    cl_int err = clGetPlatformIDs(num_entries, platforms, num_platforms);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clGetPlatformIDs, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clGetPlatformIDs");
}

static void clGetDeviceIDs_checked(prl_scop_instance scopinst, cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id *devices, cl_uint *num_devices) {
    prl_time_t start = timestamp();
    cl_int err = clGetDeviceIDs(platform, device_type, num_entries, devices, num_devices);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clGetDeviceIDs, stop - start);

    if (err == CL_DEVICE_NOT_FOUND /*|| err==CL_INVALID_DEVICE_TYPE*/) {
        if (num_devices)
            *num_devices = 0;
        return;
    }

    if (err != CL_SUCCESS)
        opencl_error(err, "clGetDeviceIDs");
}

static cl_context clCreateContext_checked(prl_scop_instance scopinst, const cl_context_properties *properties,
                                          cl_uint num_devices,
                                          const cl_device_id *devices,
                                          void(CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
                                          void *user_data) {
    cl_int err = CL_INT_MIN;

    prl_time_t start = timestamp();
    cl_context result = clCreateContext(properties, num_devices, devices, pfn_notify, user_data, &err);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clCreateContext, stop - start);

    if (err != CL_SUCCESS || !result)
        opencl_error(err, "clCreateContext");
    return result;
}

static void clEnqueueWriteBuffer_checked(prl_scop_instance scopinst, cl_command_queue command_queue,
                                         cl_mem buffer,
                                         cl_bool blocking_write,
                                         size_t offset,
                                         size_t size,
                                         const void *ptr,
                                         cl_uint num_events_in_wait_list,
                                         const cl_event *event_wait_list,
                                         cl_event *event) {
    prl_time_t start = timestamp();
    cl_int err = clEnqueueWriteBuffer(command_queue, buffer, blocking_write, offset, size, ptr, num_events_in_wait_list, event_wait_list, event);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clEnqueueWriteBuffer, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clEnqueueWriteBuffer");
}

static void clEnqueueReadBuffer_checked(prl_scop_instance scopinst, cl_command_queue command_queue,
                                        cl_mem buffer,
                                        cl_bool blocking_read,
                                        size_t offset,
                                        size_t size,
                                        void *ptr,
                                        cl_uint num_events_in_wait_list,
                                        const cl_event *event_wait_list,
                                        cl_event *event) {
    prl_time_t start = timestamp();
    cl_int err = clEnqueueReadBuffer(command_queue, buffer, blocking_read, offset, size, ptr, num_events_in_wait_list, event_wait_list, event);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clEnqueueReadBuffer, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clEnqueueReadBuffer");
}

static void clEnqueueNDRangeKernel_checked(prl_scop_instance scopinst, cl_command_queue command_queue,
                                           cl_kernel kernel,
                                           cl_uint work_dim,
                                           const size_t *global_work_offset,
                                           const size_t *global_work_size,
                                           const size_t *local_work_size,
                                           cl_uint num_events_in_wait_list,
                                           const cl_event *event_wait_list,
                                           cl_event *event) {
    prl_time_t start = timestamp();
    cl_int err = clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clEnqueueNDRangeKernel, stop - start);

    if (err)
        opencl_error(err, "clEnqueueNDRangeKernel");
}

static void *clEnqueueMapBuffer_checked(prl_scop_instance scopinst, cl_command_queue command_queue,
                                        cl_mem buffer,
                                        cl_bool blocking_map,
                                        cl_map_flags map_flags,
                                        size_t offset,
                                        size_t size,
                                        cl_uint num_events_in_wait_list,
                                        const cl_event *event_wait_list,
                                        cl_event *event) {
    cl_int err = CL_INT_MIN;

    prl_time_t start = timestamp();
    void *result = clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, size, num_events_in_wait_list, event_wait_list, event, &err);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clEnqueueMapBuffer, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clEnqueueMapBuffer");
    return result;
}

static void clFinish_checked(prl_scop_instance scopinst, cl_command_queue command_queue) {
    assert(command_queue);

    prl_time_t start = timestamp();
    cl_int err = clFinish(command_queue);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clFinish, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clFinish");
}

static void clReleaseCommandQueue_checked(prl_scop_instance scopinst, cl_command_queue command_queue) {
    assert(command_queue);

    prl_time_t start = timestamp();
    cl_int err = clReleaseCommandQueue(command_queue);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clReleaseCommandQueue, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clReleaseCommandQueue");
}

static cl_mem clCreateBuffer_checked(prl_scop_instance scopinst, cl_context context, cl_mem_flags flags, size_t size, void *host_ptr) {
    cl_int err = CL_INT_MIN;

    prl_time_t start = timestamp();
    cl_mem result = clCreateBuffer(context, flags, size, host_ptr, &err);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clCreateBuffer, stop - start);

    if (err || !result)
        opencl_error(err, "clCreateBuffer");
    return result;
}

static void clEnqueueUnmapMemObject_checked(prl_scop_instance scopinst, cl_command_queue command_queue,
                                            cl_mem memobj,
                                            void *mapped_ptr,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event *event_wait_list,
                                            cl_event *event) {
    assert(command_queue);
    assert(memobj);
    assert(mapped_ptr);
    assert(num_events_in_wait_list == 0 || event_wait_list);
    if (event)
        *event = NULL;

    prl_time_t start = timestamp();
    cl_int err = clEnqueueUnmapMemObject(command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clEnqueueUnmapMemObject, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clEnqueueUnmapMemObject");
    assert(!event || *event);
}

static void clReleaseEvent_checked(prl_scop_instance scopinst, cl_event event) {
    assert(event);

    prl_time_t start = timestamp();
    cl_int err = clReleaseEvent(event);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clReleaseEvent, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clReleaseEvent");
}

static void clWaitForEvents_checked(prl_scop_instance scopinst, cl_uint num_events, const cl_event *event_list) {
    assert(num_events >= 1);
    assert(event_list);

    prl_time_t start = timestamp();
    cl_int err = clWaitForEvents(num_events, event_list);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clWaitForEvents, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clWaitForEvents");
}

static void clWaitForEvent_checked(prl_scop_instance scopinst, cl_event event) {
    assert(event);
    clWaitForEvents_checked(scopinst, 1, &event);
}

static void clGetEventProfilingInfo_checked(prl_scop_instance scopinst, cl_event event,
                                            cl_profiling_info param_name,
                                            size_t param_value_size,
                                            void *param_value,
                                            size_t *param_value_size_ret) {
    assert(event);

    prl_time_t start = timestamp();
    cl_int err = clGetEventProfilingInfo(event, param_name, param_value_size, param_value, param_value_size_ret);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clGetEventProfilingInfo, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clGetEventProfilingInfo");
}

static void clGetEventInfo_checked(prl_scop_instance scopinst, cl_event event,
                                   cl_event_info param_name,
                                   size_t param_value_size,
                                   void *param_value,
                                   size_t *param_value_size_ret) {
    assert(event);
    assert(param_value);

    prl_time_t start = timestamp();
    cl_int err = clGetEventInfo(event, param_name, param_value_size, param_value, param_value_size_ret);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clGetEventInfo, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clGetEventInfo");
}

static void clGetPlatformInfo_checked(prl_scop_instance scopinst, cl_platform_id platform,
                                      cl_platform_info param_name,
                                      size_t param_value_size,
                                      void *param_value,
                                      size_t *param_value_size_ret) {
    prl_time_t start = timestamp();
    cl_int err = clGetPlatformInfo(platform, param_name, param_value_size, param_value, param_value_size_ret);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clGetPlatformInfo, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clGetPlatformInfo");
}

static cl_kernel clCreateKernel_checked(prl_scop_instance scopinst, cl_program program,
                                        const char *kernel_name) {
    assert(program);
    assert(kernel_name);

    cl_int err = CL_INT_MIN;

    prl_time_t start = timestamp();
    cl_kernel result = clCreateKernel(program, kernel_name, &err);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clCreateKernel, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clCreateKernel");
    return result;
}

 static void clSetKernelArg_checked( prl_scop_instance scopinst,	cl_kernel kernel,
  	cl_uint arg_index,
  	size_t arg_size,
  	const void *arg_value) {
	 assert(kernel);
	 assert(arg_size > 0);
	 assert(arg_value);

	     prl_time_t start = timestamp();
	 cl_int err = clSetKernelArg(kernel,arg_index, arg_size, arg_value);
	     prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clSetKernelArg, stop - start);

        if (err != CL_SUCCESS)
        opencl_error(err, "clSetKernelArg");
}

static void clReleaseKernel_checked(prl_scop_instance scopinst, cl_kernel kernel) {
    assert(kernel);

    prl_time_t start = timestamp();
    cl_int err = clReleaseKernel(kernel);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clReleaseKernel, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clReleaseKernel");
}

static void clGetDeviceInfo_checked(prl_scop_instance scopinst, cl_device_id device,
                                    cl_device_info param_name,
                                    size_t param_value_size,
                                    void *param_value,
                                    size_t *param_value_size_ret) {
    prl_time_t start = timestamp();
    cl_int err = clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clGetDeviceInfo, stop - start);

    if (err != CL_SUCCESS)
        opencl_error(err, "clGetDeviceInfo");
}

static cl_int clGetMemObjectInfo_checked(prl_scop_instance scopinst, cl_mem memobj,
                                         cl_mem_info param_name,
                                         size_t param_value_size,
                                         void *param_value,
                                         size_t *param_value_size_ret) {
    assert(memobj);

    prl_time_t start = timestamp();
    cl_int err = clGetMemObjectInfo(memobj, param_name, param_value_size, param_value, param_value_size_ret);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_clGetMemObjectInfo, stop - start);
    if (err != CL_SUCCESS)
        opencl_error(err, "clGetMemObjectInfo");
}

static void *malloc_checked(prl_scop_instance scopinst, size_t size) {
    prl_time_t start = timestamp();
    void *result = malloc(size);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_malloc, stop - start);

    assert(result);
    return result;
}

static void *realloc_checked(prl_scop_instance scopinst, void *ptr, size_t size) {
    prl_time_t start = timestamp();
    void *result = realloc(ptr, size);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_realloc, stop - start);

    assert(result);
    return result;
}

static void free_checked(prl_scop_instance scopinst, void *ptr) {
    prl_time_t start = timestamp();
    free(ptr);
    prl_time_t stop = timestamp();
    add_time(scopinst, stat_cpu_free, stop - start);
}

static bool any_profiling() {
    return global_state.config.cpu_profiling || global_state.config.gpu_profiling || global_state.config.gpu_detailed_profiling;
}

static bool any_gpu_profiling() {
    return global_state.config.gpu_profiling || global_state.config.gpu_detailed_profiling;
}

static bool need_events() {
    return any_gpu_profiling();
}

static bool need_store_events() {
    return global_state.config.gpu_profiling;
}

//RENAME: config_blocking
static bool is_blocking() {
    return global_state.config.blocking;
}

static void push_back_mem(prl_scop_instance scopinst, prl_mem mem) {
    assert(scopinst);
    assert(mem);

    size_t old_size = scopinst->mems_size;
    size_t new_size = old_size + 1;
    scopinst->mems = realloc_checked(scopinst, scopinst->mems, new_size * sizeof *scopinst->mems);
    scopinst->mems[old_size] = mem;
    scopinst->mems_size += 1;
}

static void stat_compute_medians(double medians[static const restrict STAT_ENTRIES], double relstddevs[static const restrict STAT_ENTRIES], size_t n, struct prl_stat data[static const restrict n]) {
    prl_time_t *sorted = malloc_checked(NOSCOPINST, n * sizeof *sorted);
    for (int j = 0; j < STAT_ENTRIES; j += 1) {
        if (n == 0) {
            medians[j] = NAN;
            relstddevs[j] = 0;
            continue;
        }

        prl_time_t sum = 0;
        double sqrsum = 0;
        for (int i = 0; i < n; i += 1) {
            prl_time_t val = data[i].entries[j];
            sorted[i] = val;
            sum += val;
            sqrsum += sqrd(val);
        }
        double avg = ((double)sum) / n;
        double variance = sqrsum / n - sqrd(avg);
        double stddev = sqrt(variance);
        double relstddev = (sum == sqrsum) ? 0 : stddev / avg;

        qsort(sorted, n, sizeof *sorted, &cmp_time);
        double median;
        if (n % 2 == 0)
            median = sorted[n / 2];
        else
            median = ((double)sorted[n / 2] + (double)sorted[(n + 1) / 2]) / 2;

        medians[j] = median;
        relstddevs[j] = relstddev;
    }
    free_checked(NOSCOPINST, sorted);
}

static bool is_valid_loc(prl_mem mem) {
	if (mem->loc & loc_bit_host_is_current)
		if (!mem->host_mem)
			return false;
	if (mem->loc & loc_bit_dev_is_current)
		if (!mem->clmem)
			return false;

    switch (mem->type) {
    case alloc_type_none:
        return mem->loc == loc_none;
    case alloc_type_host_only:
        return (mem->loc == loc_none) || (mem->loc == loc_host);
    case alloc_type_dev_only:
        return (mem->loc == loc_none) || (mem->loc == loc_dev);
    case alloc_type_rwbuf:
        return (mem->loc == loc_none) ||
               (mem->loc == loc_host) ||
               (mem->loc == loc_dev) ||
               (mem->loc == loc_transferring_to_dev) ||
               (mem->loc == loc_transferring_to_host);
    case alloc_type_map:
        return (mem->loc == loc_none) ||
               (mem->loc == loc_map_host) ||
               (mem->loc == loc_map_dev) ||
               (mem->loc == loc_map_mapping) ||
               (mem->loc == loc_map_unmapping);
    default:
        return false;
    }
}

static const char *cmdtype_to_str(cl_command_type cmdty) {
    return statname[clcommand_to_stat_entry(cmdty)];
}

static void dump_finished_kernel_event(cl_command_type cmdty, prl_kernel kernel, prl_time_t duration) {
    const char *srcfile = kernel->program->filename;
    if (!srcfile)
        srcfile = "";
    const char *kernelname = kernel->name;
    if (!kernelname)
        kernelname = "";
    printf("Compute %s %s: %fms (%s)\n", srcfile, kernelname, duration * 0.000001, cmdtype_to_str(cmdty));
}

static void dump_finished_transfer_event(cl_command_type cmdty, prl_mem mem, prl_time_t duration) {
    assert(is_valid_loc(mem));

    enum prl_alloc_current_location direction = mem->loc & loc_mask_transferring;
    assert(direction == loc_transferring_to_dev || direction == loc_transferring_to_host);
    const char *dirstr = (direction == loc_transferring_to_dev) ? "host->dev" : "dev->host";
    const char *cmdstr = statname[clcommand_to_stat_entry(cmdty)];

    if (mem->name)
        printf("Transfer %s of %s: %fms (%s)\n", dirstr, mem->name, duration * 0.000001, cmdstr);
    else
        printf("Transfer %s: %fms (%s)\n", dirstr, duration * 0.000001, cmdstr);
}

static void dump_finished_event(struct prl_pending_event *pendev, cl_command_type cmdty, prl_time_t duration) {
    switch (pendev->type) {
    case pending_compute:
        dump_finished_kernel_event(cmdty, pendev->kernel, duration);
        break;
    case pending_transfer:
        dump_finished_transfer_event(cmdty, pendev->mem, duration);
        break;
    case pending_other: {
        const char *cmdstr = statname[clcommand_to_stat_entry(cmdty)];
        printf("%s: %fms\n", cmdstr, duration * 0.000001d);
    } break;
    }
}

static bool has_event_completed(prl_scop_instance scopinst, cl_event event) {
    assert(event);

    cl_int status = 0;
    clGetEventInfo_checked(scopinst, event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(status), &status, NULL);
    return status == CL_COMPLETE;
}

static bool has_transfer_completed(prl_scop_instance scopinst, prl_mem mem) {
    assert(mem);
    return has_event_completed(scopinst, mem->transferevent);
}

static void report_finished_event(prl_scop_instance scopinst, cl_event event, prl_mem mem, prl_kernel kernel) {
    if (!global_state.config.gpu_detailed_profiling)
        return;

    assert(event);
    assert(has_event_completed(scopinst, event));

    cl_ulong start = 0;
    cl_ulong stop = 0;
    cl_command_type cmdty;

    clGetEventProfilingInfo_checked(scopinst, event, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
    clGetEventProfilingInfo_checked(scopinst, event, CL_PROFILING_COMMAND_END, sizeof(stop), &stop, NULL);
    clGetEventInfo_checked(scopinst, event, CL_EVENT_COMMAND_TYPE, sizeof(cmdty), &cmdty, NULL);
    prl_time_t duration = stop - start;

    struct prl_pending_event pendev = {.event = event, .type = pending_other, .reported = false};
    assert(!(mem && kernel));
    if (mem) {
        pendev.type = pending_transfer;
        pendev.mem = mem;
    }
    if (kernel) {
        pendev.type = pending_compute;
        pendev.kernel = kernel;
    }
    dump_finished_event(&pendev, cmdty, duration);
}

// Takes responsibility to free event
static void push_back_event(prl_scop_instance scopinst, cl_event event, prl_mem mem, prl_kernel kernel, bool completed) {
    assert(scopinst);

    if (!event) {
        assert(!need_store_events());
        assert(!global_state.config.gpu_detailed_profiling);
        return;
    }

    bool reported = false;
    if (completed && global_state.config.gpu_detailed_profiling) {
        assert(has_event_completed(scopinst, event));
        report_finished_event(scopinst, event, mem, kernel);
        reported = true;
    }

    if (need_store_events()) {
        scopinst->event_size += 1;
        scopinst->pending_events = realloc_checked(scopinst, scopinst->pending_events, scopinst->event_size * sizeof *scopinst->pending_events);
        struct prl_pending_event *pende = &scopinst->pending_events[scopinst->event_size - 1];
        pende->event = event;
        pende->type = pending_other;
        pende->reported = reported;
        assert(!(mem && kernel));
        if (mem) {
            pende->type = pending_transfer;
            pende->mem = mem;
        }
        if (kernel) {
            pende->type = pending_compute;
            pende->kernel = kernel;
        }
    } else {
        clReleaseEvent_checked(scopinst, event);
    }
}

static void bench_push_back(struct prl_stat *elmt) {
    size_t old_size = global_state.bench_stats_size;
    size_t new_size = old_size + 1;

    global_state.bench_stats = realloc_checked(NOSCOPINST, global_state.bench_stats, new_size * sizeof *global_state.bench_stats);
    global_state.bench_stats[old_size] = *elmt;
    global_state.bench_stats_size = new_size;
}

static void memlist_push_front(prl_mem *first, prl_mem item) {
    assert(first);

    item->mem_prev = NULL;
    item->mem_next = *first;
    if (*first)
        (*first)->mem_prev = item;
    *first = item;
}

//TODO: It is not necessary to know size at creation-time
static prl_mem prl_mem_create_empty(size_t size, const char *name, prl_scop_instance scopinst) {
    assert(prl_initialized);
    assert(size > 0);

    prl_mem result = malloc_checked(scopinst, sizeof *result);
    memset(result, 0, sizeof *result);

    result->size = size;
    result->name = name ? strdup(name) : NULL;
    result->transfer_to_device = true;
    result->transfer_to_host = true;
    result->type = alloc_type_none;
    bool is_global = !scopinst;
    if (is_global) {
        // Global/user-managed memory
        // Responsibility to free is at user's
        memlist_push_front(&global_state.global_mems, result);
    } else {
        // Local to SCoP instance
        // prl_scop_leave will free this
        memlist_push_front(&scopinst->local_mems, result);
        result->scopinst = scopinst;
    }

    return result;
}


static void prl_mem_init_rwbuf( prl_mem mem,
			       void *host_mem, bool host_owning,  bool host_exposed , bool host_readable, bool host_writable,
			       cl_mem dev_mem,  bool dev_owning, bool dev_readable,  bool dev_exposed, bool dev_writable,
			       enum prl_alloc_current_location loc) {
	assert(mem);
	assert(host_mem);
	assert(dev_mem);
	assert(loc == loc_host || loc == loc_dev || loc == loc_none);

	mem->type = alloc_type_rwbuf;
	mem->loc = loc;
	mem->transfer_to_device = true;
	mem->transfer_to_host = true;

	// host side
	mem->host_mem = host_mem;
	mem->host_owning = host_owning;
	mem->host_exposed = host_exposed;
	mem->host_readable = host_readable;
	mem->host_writable = host_writable;

	// dev side
	mem->clmem = dev_mem;
	mem->dev_owning = dev_owning;
	mem->dev_exposed = dev_exposed;
	mem->dev_readable = dev_readable;
	mem->dev_writable = dev_writable;

	assert(is_valid_loc(mem));
}

static void prl_mem_init_rwbuf_host( prl_mem mem,
			       void *host_mem, bool host_owning, bool host_exposed, bool host_readable, bool host_writable,
			       bool dev_readable, bool dev_writable,
			       enum prl_alloc_current_location loc){
	assert(mem);
	assert(host_mem);
	assert(loc == loc_host || loc == loc_none);

	mem->type = alloc_type_rwbuf;
	mem->loc = loc;
	mem->transfer_to_device = true;
	mem->transfer_to_host = true;

	// host side
	mem->host_mem = host_mem;
	mem->host_owning = host_owning;
	mem->host_exposed = host_exposed;
	mem->host_readable = host_readable;
	mem->host_writable = host_writable;

	// dev side
	mem->clmem = NULL;
	mem->dev_owning = false;
	mem->dev_exposed = false;
	mem->dev_readable = dev_readable;
	mem->dev_writable = dev_writable;

	assert(is_valid_loc(mem));
}


static void prl_mem_init_rwbuf_dev( prl_mem mem,
			       bool host_readable, bool host_writable,
			       cl_mem dev_mem,  bool dev_owning, bool dev_readable,  bool dev_exposed, bool dev_writable,
			       enum prl_alloc_current_location loc) {
	assert(mem);
	assert(dev_mem);
	assert(loc == loc_dev || loc == loc_none);

	mem->type = alloc_type_rwbuf;
	mem->loc = loc;
	mem->transfer_to_device = true;
	mem->transfer_to_host = true;

	// host side
	mem->host_mem = NULL;
	mem->host_owning = false;
	mem->host_exposed = false;
	mem->host_readable = host_readable;
	mem->host_writable = host_writable;

	// dev side
	mem->clmem = dev_mem;
	mem->dev_owning = dev_owning;
	mem->dev_exposed = dev_exposed;
	mem->dev_readable = dev_readable;
	mem->dev_writable = dev_writable;

	assert(is_valid_loc(mem));
}

static void prl_mem_init_rwbuf_none( prl_mem mem,
			       bool host_readable, bool host_writable,
			       bool dev_readable, bool dev_writable){
	assert(mem);

	mem->type = alloc_type_rwbuf;
	mem->loc = loc_none;
	mem->transfer_to_device = true;
	mem->transfer_to_host = true;

	// host side
	mem->host_mem = NULL;
	mem->host_owning = false;
	mem->host_exposed = false;
	mem->host_readable = host_readable;
	mem->host_writable = host_writable;

	// dev side
	mem->clmem = NULL;
	mem->dev_owning = false;
	mem->dev_exposed = false;
	mem->dev_readable = dev_readable;
	mem->dev_writable = dev_writable;

	assert(is_valid_loc(mem));
}



static void prl_mem_alloc_host_only(prl_scop_instance scopinst, prl_mem mem) {
    assert(mem);
    assert(mem->type == alloc_type_none);

    mem->type = alloc_type_host_only;
    mem->loc = loc_host;

    mem->host_mem = malloc_checked(scopinst, mem->size);
    mem->host_exposed = false;
    mem->host_readable = true;
    mem->host_writable = true;
    mem->host_owning = true;
}

static void prl_mem_manage_host_only(prl_mem mem, void *host_mem, bool host_take_ownership) {
    assert(mem);
    assert(mem->type == alloc_type_none);
    assert(host_mem);

    mem->type = alloc_type_host_only;
    mem->host_mem = host_mem;
    mem->host_readable = true;
    mem->host_writable = true;
    mem->host_owning = host_take_ownership;

    mem->loc = loc_host;
}

static cl_mem_flags dev_rw_flags[] = {0, CL_MEM_READ_ONLY, CL_MEM_WRITE_ONLY, CL_MEM_READ_WRITE};
static void prl_mem_alloc_dev_only(prl_scop_instance scopinst, prl_mem mem, bool readable, bool writable, void *initdata) {
    assert(mem);
    assert(mem->type == alloc_type_none);
    assert(readable || writable);

    mem->type = alloc_type_dev_only;
    mem->clmem = clCreateBuffer_checked(scopinst, global_state.context, dev_rw_flags[(readable ? 1 : 0) + (writable ? 2 : 0)] | (initdata ? CL_MEM_COPY_HOST_PTR : 0), mem->size, initdata);
    mem->dev_owning = true;
    mem->dev_readable = readable;
    mem->dev_writable = writable;

    mem->loc = loc_dev;
}

static void prl_mem_manage_dev_only(prl_mem mem, cl_mem dev_mem, bool dev_take_ownership, bool readable, bool writable) {
    assert(mem);
    assert(mem->type == alloc_type_none);
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
    assert(mem->type == alloc_type_none);
    assert(host_readable | host_writable);
    assert(dev_readable | dev_writable);

    mem->type = alloc_type_map;

    mem->clmem = clCreateBuffer_checked(scopinst, global_state.context, dev_rw_flags[(dev_readable ? 1 : 0) + (dev_writable ? 2 : 0)] | CL_MEM_ALLOC_HOST_PTR, mem->size, NULL);
    mem->dev_owning = true;
    mem->dev_readable = dev_readable;
    mem->dev_writable = dev_writable;

    cl_command_queue clqueue;
    if (scopinst)
        clqueue = scopinst->queue;
    else
        clqueue = clCreateCommandQueue_checked(scopinst, global_state.context, global_state.device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);

    mem->host_mem = clEnqueueMapBuffer_checked(scopinst, clqueue, mem->clmem, CL_BLOCKING_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, 0, mem->size, 0, NULL, NULL);
    clFinish(clqueue);

    if (!scopinst)
        clReleaseCommandQueue(clqueue);
    mem->host_owning = true;
    mem->host_readable = host_readable;
    mem->host_writable = host_writable;

    mem->loc = loc_host;
}

static void prl_mem_manage_host_map(prl_scop_instance scopinst, prl_mem mem, void *host_mem, bool host_take_ownership, bool host_readable, bool host_writable, bool dev_readable, bool dev_writable) {
    assert(mem);
    assert(mem->type == alloc_type_none);
    assert(host_mem);
    assert(host_readable | host_writable);
    assert(dev_readable | dev_writable);

    mem->type = alloc_type_map;

    mem->clmem = clCreateBuffer_checked(scopinst, global_state.context, dev_rw_flags[(dev_readable ? 1 : 0) + (dev_writable ? 2 : 0)] | CL_MEM_USE_HOST_PTR, mem->size, host_mem);
    mem->dev_owning = true;
    mem->dev_readable = dev_readable;
    mem->dev_writable = dev_writable;

    mem->host_mem = host_mem;
    mem->host_owning = host_take_ownership;
    mem->host_readable = host_readable;
    mem->host_writable = host_writable;

    mem->loc = loc_map_dev;
}

static prl_mem prl_mem_lookup_global_ptr(void *host_ptr, size_t size) {
    assert(host_ptr);
    //assert(size>0);

    char *ptr_begin = host_ptr;
    char *ptr_end = ptr_begin + size;

    prl_mem mem = global_state.global_mems;
    while (mem) {
        assert(!mem->scopinst);

        char *mem_begin = mem->host_mem;
        char *mem_end = mem_begin + mem->size;
        if (mem_begin <= ptr_begin && ptr_begin < mem_end) {
            assert(mem_begin <= ptr_end && ptr_end <= mem_end);
            return mem;
        }

        assert(!(mem_begin <= ptr_end && ptr_end < mem_end));

        mem = mem->mem_next;
    }

    return NULL; // Not found
}

static cl_device_type devtypes[] = {
	[PRL_TARGET_DEVICE_FIRST] CL_DEVICE_TYPE_DEFAULT,
	[PRL_TARGET_DEVICE_FIXED]  CL_DEVICE_TYPE_ALL,

    [PRL_TARGET_DEVICE_GPU_ONLY] CL_DEVICE_TYPE_GPU,
    [PRL_TARGET_DEVICE_CPU_ONLY] CL_DEVICE_TYPE_CPU,
    [PRL_TARGET_DEVICE_ACC_ONLY] CL_DEVICE_TYPE_ACCELERATOR,

    [PRL_TARGET_DEVICE_GPU_THEN_CPU] CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU,
    [PRL_TARGET_DEVICE_CPU_THEN_GPU] CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU,
    [PRL_TARGET_DEVICE_ACC_THEN_CPU] CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_ACCELERATOR,
    [PRL_TARGET_DEVICE_ACC_THEN_GPU] CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR,
    [PRL_TARGET_DEVICE_GPU_THEN_ACC] CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR,
    [PRL_TARGET_DEVICE_CPU_THEN_ACC] CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_ACCELERATOR,

    [PRL_TARGET_DEVICE_GPU_THEN_CPU_THEN_ACC] CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR,
    [PRL_TARGET_DEVICE_CPU_THEN_GPU_THEN_ACC] CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR,
    [PRL_TARGET_DEVICE_GPU_THEN_ACC_THEN_CPU] CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR,
    [PRL_TARGET_DEVICE_CPU_THEN_ACC_THEN_GPU] CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR,
    [PRL_TARGET_DEVICE_ACC_THEN_GPU_THEN_CPU] CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR,
    [PRL_TARGET_DEVICE_ACC_THEN_CPU_THEN_GPU] CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR,
};

// { cpu, gpu, acc, other }
// 0 means never take
static unsigned char devtypes_rank[][4] = {
   [ PRL_TARGET_DEVICE_CPU_ONLY] ={ 4, 0,0,0 },
   [ PRL_TARGET_DEVICE_CPU_THEN_GPU] { 4, 3, 0,0 },
    [PRL_TARGET_DEVICE_CPU_THEN_ACC] { 4, 0, 3, 0 },
    [PRL_TARGET_DEVICE_CPU_THEN_GPU_THEN_ACC] { 4, 3, 2, 0 },
    [PRL_TARGET_DEVICE_CPU_THEN_ACC_THEN_GPU] { 4, 2, 3, 0 },

    [PRL_TARGET_DEVICE_GPU_ONLY]  { 0, 4, 0, 0 },
    [PRL_TARGET_DEVICE_GPU_THEN_CPU]  { 3, 4, 0, 0 },
    [PRL_TARGET_DEVICE_GPU_THEN_ACC]  { 0, 4, 3, 0 },
    [PRL_TARGET_DEVICE_GPU_THEN_CPU_THEN_ACC]  { 3, 4, 2, 0 },
    [PRL_TARGET_DEVICE_GPU_THEN_ACC_THEN_CPU]  { 2, 4, 3, 0 },

    [PRL_TARGET_DEVICE_ACC_ONLY]  { 0, 0, 4, 0 },
    [PRL_TARGET_DEVICE_ACC_THEN_CPU]  { 3, 0, 4, 0 },
    [PRL_TARGET_DEVICE_ACC_THEN_GPU]  { 0, 3, 4, 0 },
    [PRL_TARGET_DEVICE_ACC_THEN_CPU_THEN_GPU]  { 3, 2, 4, 0 },
    [PRL_TARGET_DEVICE_ACC_THEN_GPU_THEN_CPU]  { 2, 3, 4, 0 },
};


#if 0
static cl_device_type preftypes[] = {
    [PRL_TARGET_DEVICE_CPU_ONLY] CL_DEVICE_TYPE_CPU,
    [PRL_TARGET_DEVICE_GPU_ONLY] CL_DEVICE_TYPE_GPU,
    [PRL_TARGET_DEVICE_CPU_THEN_GPU] CL_DEVICE_TYPE_CPU,
    [PRL_TARGET_DEVICE_GPU_THEN_CPU] CL_DEVICE_TYPE_GPU};
#endif

static int extract_devtype(cl_device_type type) {
	if (type & CL_DEVICE_TYPE_CPU)
		return 0;
		if (type & CL_DEVICE_TYPE_GPU)
		return 1;
		if (type & CL_DEVICE_TYPE_ACCELERATOR)
		return 2;
return 3;
}

static int is_preferable_device(cl_device_type old_type, cl_device_type alt_type) {
    assert(alt_type);
    if (!old_type)
        return 1;

    int old_devtype = extract_devtype(old_type);
    int alt_devtype = extract_devtype(alt_type);

    int old_rank = devtypes_rank[global_config.device_choice][old_devtype];
    int alt_rank = devtypes_rank[global_config.device_choice][alt_devtype];

    return (alt_rank > old_rank) - (old_rank-alt_rank);
}

static bool get_bool(const char *str) {
    if (!str)
        return false;
    if (strcasecmp(str, "n") == 0 || strcasecmp(str, "false") == 0 || strcasecmp(str, "no") == 0)
        return false;
    char *end = NULL;
    long long res = strtoll(str, &end, 10);
    if (!end || *end != '\0')
        return true; // Assume true if something else than a number
    return res;
}

static int get_int(const char *str) {
    assert(str);
    char *end = NULL;
    long res = strtol(str, &end, 10);
    if (!end || *end != '\0') {
        fprintf(stderr, "Could not parse int: %s\n", str);
        exit(1);
    }
    assert(INT_MIN <= res && res <= INT_MAX);
    return res;
}

static const char *targetconfstr[] = {
	[PRL_TARGET_DEVICE_FIRST] "first",

    [PRL_TARGET_DEVICE_GPU_ONLY] "gpu",
    [PRL_TARGET_DEVICE_CPU_ONLY] "cpu",
    [PRL_TARGET_DEVICE_ACC_ONLY] "acc",

    [PRL_TARGET_DEVICE_GPU_THEN_CPU] "gpu_cpu",
    [PRL_TARGET_DEVICE_CPU_THEN_GPU] "cpu_gpu",
    [PRL_TARGET_DEVICE_ACC_THEN_CPU] "acc_cpu",
    [PRL_TARGET_DEVICE_ACC_THEN_GPU] "acc_gpu",
    [PRL_TARGET_DEVICE_GPU_THEN_ACC] "gpu_acc",
    [PRL_TARGET_DEVICE_CPU_THEN_ACC] "cpu_acc",

    [PRL_TARGET_DEVICE_CPU_THEN_GPU_THEN_ACC] "cpu_gpu_acc",
    [PRL_TARGET_DEVICE_GPU_THEN_CPU_THEN_ACC] "gpu_cpu_acc",
    [PRL_TARGET_DEVICE_GPU_THEN_ACC_THEN_CPU] "gpu_acc_cpu",
    [PRL_TARGET_DEVICE_CPU_THEN_ACC_THEN_GPU] "cpu_acc_gpu",
    [PRL_TARGET_DEVICE_ACC_THEN_GPU_THEN_CPU] "acc_gpu_cpu",
    [PRL_TARGET_DEVICE_ACC_THEN_CPU_THEN_GPU] "acc_cpu_gpu",
};

#define LENGTHOF(ARR) (sizeof(ARR)/sizeof(ARR[0]))

static size_t parse_targetconf(const char *targetdev) {
	for (size_t i = 0; i < LENGTHOF(targetconfstr); i+=1) {
	const char *confstr = targetconfstr[i];
	if (!confstr)
		continue;

	if (strcasecmp(targetdev, confstr)==0)
		return i;
	}
	return LENGTHOF(targetconfstr);
}

static void env_config(struct prl_global_config *config) {
    assert(config);
    const char *targetdev, *blocking, *profiling_str, *cpu_profiling_str, *gpu_profiling_str, *prefix, *str;

    if ((targetdev = getenv(PRL_TARGET_DEVICE))) {
        int env_platform, env_device;

	size_t preset = parse_targetconf(targetdev);
	if (preset < LENGTHOF(targetconfstr)) {
		config->device_choice = preset;
	} else if (sscanf(targetdev, "%d:%d", &env_platform, &env_device) == 2) {
            // reasonable limits
            assert(0 <= env_platform && env_platform <= 255);
            assert(0 <= env_device && env_device <= 255);
            config->device_choice = PRL_TARGET_DEVICE_FIXED;
            config->chosed_platform = env_platform;
            config->chosed_platform = env_device;
        } else {
            fputs("cannot read env PRL_TARGET_DEVICE\n", stderr); //TODO: Central error handling
            exit(1);
        }
    }

    if ((blocking = getenv(PRL_BLOCKING))) {
        config->blocking = get_bool(blocking);
    }
    if ((prefix = getenv(PRL_PREFIX))) {
        config->profiling_prefix = prefix;
        config->bench_prefix = prefix;
    }

    if ((profiling_str = getenv(PRL_PROFILING))) {
        bool profiling = get_bool(profiling_str);
        config->cpu_profiling = profiling;
        config->gpu_profiling = profiling;
        config->profiling_dump_on_release |= profiling;
    }
    if ((cpu_profiling_str = getenv(PRL_CPU_PROFILING))) {
        bool profiling = get_bool(cpu_profiling_str);
        config->cpu_profiling = profiling;
        config->profiling_dump_on_release |= profiling;
    }
    if ((gpu_profiling_str = getenv(PRL_GPU_PROFILING))) {
        bool profiling = get_bool(gpu_profiling_str);
        config->gpu_profiling = profiling;
        config->profiling_dump_on_release |= profiling;
    }
    if ((str = getenv(PRL_GPU_PROFILING_DETAILED))) {
        bool detailed_gpu_profiling = get_bool(str);
        config->gpu_detailed_profiling = detailed_gpu_profiling;
    }

    if ((prefix = getenv(PRL_PROFILING_PREFIX))) {
        config->profiling_prefix = prefix;
    }

    if ((prefix = getenv(PRL_TIMINGS_PREFIX))) {
        config->bench_prefix = prefix;
    }
    if ((str = getenv(PRL_TIMINGS_DRY_RUNS))) {
        config->timing_warmups = get_int(str);
        assert(config->timing_warmups >= 0);
    }
    if ((str = getenv(PRL_TIMINGS_RUNS))) {
        config->timing_runs = get_int(str);
        assert(config->timing_runs >= 1);
    }
}

static void print_stat_entry(const char *name, const int *count, double duration, const double *relstddev, const char *prefix) {
    assert(name);
    if (!prefix)
        prefix = "";

    if (duration == 0 && (!count || !*count))
        return;

    if (relstddev) {
        // Report relative standard error
        if (*relstddev != 0)
            printf("%s%-25s:%8.3fms (\u00B1%5.1f%%)\n", prefix, name, duration * 0.000001d, 100 * *relstddev);
        else
            printf("%s%-25s:%8.3fms\n", prefix, name, duration * 0.000001d);
    } else {
        // Report times
        if (count) {
            assert(*count > 0);
            printf("%s%-25s:%8.3fms (%3d time%1s)\n", prefix, name, duration * 0.000001d, *count, (*count == 1) ? "" : "s");
        } else
            printf("%s%-25s:%8.3fms\n", prefix, name, duration * 0.000001d);
    }
}

static void print_stat(double durations[static const restrict STAT_ENTRIES], int counts[const restrict STAT_ENTRIES], double relstddevs[const restrict STAT_ENTRIES], const char *prefix) {
    assert(durations);

    //puts("===============================================================================");
    if (global_state.config.cpu_profiling) {
        puts("                           CPU accumulated wall clock");
        for (int i = STAT_CPU_FIRST; i <= STAT_CPU_LAST; i += 1) {
            print_stat_entry(statname[i], counts ? &counts[i] : NULL, durations[i], relstddevs ? &relstddevs[i] : NULL, prefix);
        }
    }

    if (global_state.config.cpu_profiling && global_state.config.gpu_profiling)
        puts("");
    if (global_state.config.gpu_profiling) {
        puts("                           GPU accumulated wall clock");
        for (int i = STAT_GPU_FIRST; i <= STAT_GPU_LAST; i += 1) {
            print_stat_entry(statname[i], counts ? &counts[i] : NULL, durations[i], relstddevs ? &relstddevs[i] : NULL, prefix);
        }
    }
    //puts("===============================================================================");
}

#if 0
static void dump_entry(const char *name, double median, double relstddev) {
	if (global_state.config.bench_prefix)
		puts(global_state.config.bench_prefix);
	if (relstddev == 0)
		printf("%12s %.3f ms\n", name, median);
	else
		printf("%12s %.3f ms (\u00B1 %.1f)\n", name, median, relstddev*100);
}
#endif

void prl_prof_dump() {
    size_t n = global_state.bench_stats_size;
    int intn = n;

    double medians[STAT_ENTRIES];
    double relstddevs[STAT_ENTRIES];
    stat_compute_medians(medians, relstddevs, n, global_state.bench_stats);

    puts("===============================================================================");
    printf("Profiling median (relative standard deviation) results after %zu runs\n", n);
    puts("");
    print_stat_entry("Duration", &intn, medians[stat_cpu_bench], &relstddevs[stat_cpu_bench], global_state.config.bench_prefix);
    puts("");
    print_stat(medians, NULL, relstddevs, global_state.config.bench_prefix);
    puts("===============================================================================");
}

static void mem_free(prl_scop_instance scopinst, prl_mem mem) {
    assert(mem);
    assert(is_valid_loc(mem));

    switch (mem->type) {
    case alloc_type_none:
        break;
    case alloc_type_host_only:
    case alloc_type_dev_only:
    case alloc_type_rwbuf:
    case alloc_type_map: //TODO: Ensure that memory is unmapped?
        if (mem->host_mem && mem->host_owning)
            free_checked(scopinst, mem->host_mem);
        mem->host_mem = NULL;
        if (mem->clmem && mem->dev_owning)
            clReleaseMemObject_checked(scopinst, mem->clmem);
        mem->clmem = NULL;
        break;
    default:
        assert(false);
    }

    free_checked(scopinst, mem->name);

    if (!mem->scopinst) {
        // Global mem, remove from list
        prl_mem prev = mem->mem_prev;
        prl_mem next = mem->mem_next;

        assert(!prev || prev->mem_next == mem);
        assert(!next || next->mem_prev == mem);

        if (prev == mem)
            prev = NULL;
        if (next == mem)
            next = NULL;

        if (prev)
            prev->mem_next = next;
        if (next)
            next->mem_prev = prev;

        if (global_state.global_mems == mem) {
            if (prev)
                global_state.global_mems = prev;
            else
                global_state.global_mems = next;
        }
    }

    free_checked(scopinst, mem);
}

void prl_release() {
    if (!prl_initialized)
        return;

    bool dumping = global_state.config.profiling_dump_on_release;
    if (dumping) {
        puts("===============================================================================");
        puts("Shutting down PRL...");
    }

    prl_program program = global_state.programs;
    while (program) {

        prl_kernel kernel = program->kernels;
        while (kernel) {
            assert(kernel);
            prl_kernel nextkernel = kernel->next;
            if (kernel->kernel)
                clReleaseKernel_checked(NOSCOPINST, kernel->kernel);
            free_checked(NOSCOPINST, kernel->name);
            free_checked(NOSCOPINST, kernel);

            kernel = nextkernel;
        }

        prl_program nextprogram = program->next;
        if (program->program)
            clReleaseProgram(program->program);
        free_checked(NOSCOPINST, program->filename);
        free_checked(NOSCOPINST, program);

        program = nextprogram;
    }

    prl_scop scop = global_state.scops;
    while (scop) {
        prl_scop nextscop = scop->next;
        free_checked(NOSCOPINST, scop);
        scop = nextscop;
    }

// Global mems are to be freed by user
#if 0
    prl_mem mem = global_state.global_mems;
    while (mem) {
        prl_mem nextmem = mem->next;
	mem_free(NOSCOPINST, mem);
        mem = nextmem;
    }
#endif
#ifndef NDEBUG
	if (global_state.global_mems) {
		fputs("\nMemory leak! Some PRL global memory has not been freed using prl_free or prl_mem_free\n", stderr);
	}
#endif


    if (dumping) {
        if (global_state.config.cpu_profiling) {
            const int one = 1;
            print_stat_entry("PRL active for", &one, timestamp() - global_state.prl_start, NULL, global_state.config.profiling_prefix);
        }

        double durations[STAT_ENTRIES];
        for (int i = 0; i < STAT_ENTRIES; i += 1) {
            durations[i] = global_state.global_stat.entries[i]; // Type conversion to double
        }
        puts("");
        print_stat(durations, global_state.global_stat.counts, NULL, global_state.config.profiling_prefix);
        puts("===============================================================================");
    }

    prl_initialized = 0;
}

static char *get_platform_string_property(prl_scop_instance scopinst, cl_platform_id platform, cl_platform_info prop) {
    size_t size = 0;
    clGetPlatformInfo_checked(scopinst, platform, prop, 0, NULL, &size);

    char *buf = malloc_checked(scopinst, size + 1);
    clGetPlatformInfo_checked(scopinst, platform, prop, size, buf, NULL);
    buf[size] = '\0';
    return buf;
}

static char *get_device_string_property(prl_scop_instance scopinst, cl_device_id device, cl_platform_info prop) {
    size_t size = 0;
    clGetDeviceInfo_checked(scopinst, device, prop, 0, NULL, &size);

    char *buf = malloc_checked(scopinst, size + 1);
    clGetDeviceInfo_checked(scopinst, device, prop, size, buf, NULL);
    buf[size] = '\0';
    return buf;
}

static void dump_device() {
    char *platform_vendor = get_platform_string_property(NULL, global_state.platform, CL_PLATFORM_VENDOR);
    char *platform_name = get_platform_string_property(NULL, global_state.platform, CL_PLATFORM_NAME);
    char *platform_version = get_platform_string_property(NULL, global_state.platform, CL_PLATFORM_VERSION);
    char *device_vendor = get_device_string_property(NULL, global_state.device, CL_DEVICE_VENDOR);
    char *device_name = get_device_string_property(NULL, global_state.device, CL_DEVICE_NAME);
    char *device_version = get_device_string_property(NULL, global_state.device, CL_DEVICE_VERSION);
    char *driver_version = get_device_string_property(NULL, global_state.device, CL_DRIVER_VERSION);

    printf("Platform:  %s %s (%s)\n", platform_vendor, platform_name, platform_version);
    printf("Device:    %s %s (Driver version %s, %s)\n", device_vendor, device_name, driver_version, device_version);

    free_checked(NOSCOPINST, platform_vendor);
    free_checked(NOSCOPINST, platform_name);
    free_checked(NOSCOPINST, platform_version);
    free_checked(NOSCOPINST, device_vendor);
    free_checked(NOSCOPINST, device_name);
    free_checked(NOSCOPINST, device_version);
    free_checked(NOSCOPINST, driver_version);
}

void prl_init() {
    if (prl_initialized)
        return;

    memset(&global_state, 0, sizeof global_state);

    global_state.config = global_config;
    env_config(&global_state.config);

    bool dumping = global_state.config.profiling_dump_on_release;
    if (dumping) {
        fputs("===============================================================================\n", stdout);
        puts("Initializing PRL...");
        fputs("Profiling: ", stdout);
        if (global_state.config.cpu_profiling)
            fputs("CPU", stdout);
        if (global_state.config.cpu_profiling && global_state.config.gpu_profiling)
            fputs(", ", stdout);
        if (global_state.config.gpu_profiling)
            fputs("GPU", stdout);
        puts("");
    }

    global_state.prl_start = timestamp();
    enum prl_device_choice effective_device_choice = global_state.config.device_choice;
    int effective_platform = global_state.config.chosed_platform;
    int effective_device = global_state.config.chosen_device;

    cl_device_id best_device = NULL;
    cl_platform_id best_platform = NULL;
    switch (effective_device_choice) {
    case PRL_TARGET_DEVICE_FIRST: {
        clGetPlatformIDs_checked(NOSCOPINST, 1, &best_platform, NULL);
        assert(best_platform);
        clGetDeviceIDs_checked(NOSCOPINST, best_platform, CL_DEVICE_TYPE_DEFAULT, 1, &best_device, NULL);
    } break;
    case PRL_TARGET_DEVICE_FIXED: {
        cl_platform_id *platforms = malloc_checked(NOSCOPINST, (effective_platform + 1) * sizeof *platforms);
        cl_uint received_platforms = 0;
        clGetPlatformIDs_checked(NOSCOPINST, effective_platform + 1, platforms, &received_platforms);
        assert(received_platforms == effective_platform + 1);
        best_platform = platforms[effective_platform];
        free_checked(NOSCOPINST, best_platform);

        assert(best_platform);
        cl_device_id *devices = malloc_checked(NOSCOPINST, (effective_device + 1) * sizeof *devices);
        cl_uint received_devices = 0;
        clGetDeviceIDs_checked(NOSCOPINST, best_platform, CL_DEVICE_TYPE_ALL, effective_device + 1, devices, &received_devices);
        assert(received_devices == effective_device + 1);
        best_device = devices[effective_device];
        free_checked(NOSCOPINST, devices);
    } break;
    case  PRL_TARGET_DEVICE_CPU_ONLY:
    case   PRL_TARGET_DEVICE_CPU_THEN_GPU:
    case   PRL_TARGET_DEVICE_CPU_THEN_ACC:
    case  PRL_TARGET_DEVICE_CPU_THEN_GPU_THEN_ACC:
    case  PRL_TARGET_DEVICE_CPU_THEN_ACC_THEN_GPU:

    case   PRL_TARGET_DEVICE_GPU_ONLY:
    case   PRL_TARGET_DEVICE_GPU_THEN_CPU:
    case   PRL_TARGET_DEVICE_GPU_THEN_ACC:
    case  PRL_TARGET_DEVICE_GPU_THEN_CPU_THEN_ACC:
    case   PRL_TARGET_DEVICE_GPU_THEN_ACC_THEN_CPU:

    case  PRL_TARGET_DEVICE_ACC_ONLY:
    case   PRL_TARGET_DEVICE_ACC_THEN_CPU:
    case   PRL_TARGET_DEVICE_ACC_THEN_GPU:
    case    PRL_TARGET_DEVICE_ACC_THEN_CPU_THEN_GPU:
    case   PRL_TARGET_DEVICE_ACC_THEN_GPU_THEN_CPU:
    {
        cl_device_type best_type = 0;

        cl_uint num_platforms = 0;
        clGetPlatformIDs_checked(NOSCOPINST, 0, NULL, &num_platforms);
        cl_platform_id *platforms = malloc_checked(NOSCOPINST, num_platforms * sizeof *platforms);
        clGetPlatformIDs_checked(NOSCOPINST, num_platforms, platforms, NULL);

        for (int i = 0; i < num_platforms; i += 1) {
            cl_uint num_devices = 0;
            clGetDeviceIDs_checked(NOSCOPINST, platforms[i], devtypes[effective_device_choice], 0, NULL, &num_devices);
            if (num_devices == 0)
                continue;

            cl_device_id *devices = malloc_checked(NOSCOPINST, num_devices * sizeof *devices);
            clGetDeviceIDs_checked(NOSCOPINST, platforms[i], devtypes[effective_device_choice], num_devices, devices, NULL);

            for (int j = 0; j < num_devices; j += 1) {
                cl_device_type devtype;
                clGetDeviceInfo_checked(NOSCOPINST, devices[j], CL_DEVICE_TYPE, sizeof devtype, &devtype, NULL);

		if (devtypes_rank[effective_device_choice][extract_devtype(devtype)] <= 0)
			continue;

                if (!best_device || is_preferable_device(best_type, devtype)) {
                    best_platform = platforms[i];
                    best_device = devices[j];
                    best_type = devtype;
                }
            }
            free_checked(NOSCOPINST, devices);
        }
        free_checked(NOSCOPINST, platforms);

        assert(best_platform);
        assert(best_type);
    } break;
    default:
        assert(false);
    }
    assert(best_device);

    global_state.platform = best_platform;
    global_state.device = best_device;

    if (dumping) {
        dump_device();
    }

    global_state.context = clCreateContext_checked(NOSCOPINST, NULL, 1, &best_device, __ocl_report_error, NULL);
    atexit(prl_release);

    if (dumping) {
        fputs("===============================================================================\n", stdout);
    }

    prl_initialized = 1;
}

prl_scop_instance prl_scop_enter(prl_scop *scopref) {
    assert(scopref);
    prl_init();

    prl_scop scop = *scopref;
    if (!scop) {
        scop = malloc_checked(NOSCOPINST, sizeof *scop);
        memset(scop, 0, sizeof *scop);
        *scopref = scop;
    }

    prl_time_t scop_start = timestamp();
    struct prl_scop_inst_struct dummystat = {0};
    prl_scop_instance scopinst = malloc_checked(&dummystat, sizeof *scopinst);
    memset(scopinst, 0, sizeof *scopinst);
    memset(scopinst, 0, sizeof *scopinst);
    scopinst->stat.entries[stat_cpu_malloc] += dummystat.stat.entries[stat_cpu_malloc];

    cl_command_queue clqueue = clCreateCommandQueue_checked(scopinst, global_state.context, global_state.device, any_gpu_profiling() ? CL_QUEUE_PROFILING_ENABLE : 0);

    scopinst->scop = scop;
    scopinst->queue = clqueue;
    scopinst->scop_start = scop_start;
    return scopinst;
}

static void free_events(prl_scop_instance scopinst) {
    assert(scopinst);

    for (int i = 0; i < scopinst->event_size; i += 1) {
        clReleaseEvent_checked(scopinst, scopinst->pending_events[i].event);
    }
    free_checked(scopinst, scopinst->pending_events);
    scopinst->pending_events = NULL;
    scopinst->event_size = 0;
}

struct event_prof {
    cl_event event;
    prl_time_t start, stop;
    cl_command_type cmdty;
    cl_int status;
};

static int cmp_prof(const void *lhs_ptr, const void *rhs_ptr) {
    const struct event_prof *lhs = lhs_ptr;
    const struct event_prof *rhs = rhs_ptr;

    return (lhs->start > rhs->start) - (lhs->start < rhs->start);
    //return  lhs->start - rhs->start;
}

enum prof_kind {
    prof_all,
    prof_to_device,
    prof_compute,
    prof_to_host
};

static void accumulate_gpu_durations(struct event_prof *profs, size_t n_profs, enum prof_kind prof_kind, prl_time_t *working_result, prl_time_t *idle_result) {
    bool first = true;
    prl_time_t prev_stop;
    //prl_time_t prev_match_stop;
    //bool first_match = true;
    prl_time_t accumulated_working = 0;
    prl_time_t accumulated_idle = 0;

    if (working_result)
        accumulated_working = *working_result;
    if (accumulated_idle)
        accumulated_idle = *idle_result;

    for (int i = 0; i < n_profs; i += 1) {
        struct event_prof *prof = &profs[i];
        //cl_event event = prof->event;
        prl_time_t start = prof->start;
        prl_time_t stop = prof->stop;
        bool match;
        cl_command_type cmdty = prof->cmdty;

        switch (prof_kind) {
        case prof_all:
            match = true;
            break;
        case prof_to_device:
            match = (cmdty == CL_COMMAND_WRITE_BUFFER) || (cmdty == CL_COMMAND_WRITE_IMAGE) || (cmdty == CL_COMMAND_UNMAP_MEM_OBJECT);
            break;
        case prof_compute:
            match = (cmdty == CL_COMMAND_NDRANGE_KERNEL) || (cmdty == CL_COMMAND_TASK) || (cmdty == CL_COMMAND_NATIVE_KERNEL);
            break;
        case prof_to_host:
            match = (cmdty == CL_COMMAND_READ_BUFFER) || (cmdty == CL_COMMAND_READ_IMAGE) || (cmdty == CL_COMMAND_MAP_BUFFER) || (cmdty == CL_COMMAND_MAP_IMAGE);
            break;
        }
        if (!match)
            continue;

        if (first) {
            prev_stop = start;
            first = false;
        }

        if (prev_stop <= start) {
            // Case 1: |<- prev ->|  |<- cur ->|
            assert(start <= stop);

            prl_time_t cur_duration = stop - start;
            prl_time_t unused_duration = start - prev_stop;

            accumulated_working += cur_duration;
            accumulated_idle += unused_duration;

            prev_stop = stop;
        } else if (stop <= prev_stop) {
            // Case 2: |<-   prev   ->|
            //           |<- cur ->|
            assert(start <= stop);
        } else {
            // Case 3: |<- prev ->|
            //                |<- cur ->|
            assert(start <= prev_stop);
            assert(prev_stop <= stop);

            prl_time_t cur_duration = stop - prev_stop;

            accumulated_working += cur_duration;

            prev_stop = stop;
        }
    }

    if (working_result)
        *working_result = accumulated_working;
    if (idle_result)
        *idle_result = accumulated_idle;
}

static void eval_events(prl_scop_instance scopinst) {
    assert(scopinst);

    if (!global_state.config.gpu_profiling)
        return;

    size_t n_events = scopinst->event_size;
    struct event_prof *profs = malloc_checked(scopinst, n_events * sizeof *profs);

    for (int i = 0; i < n_events; i += 1) {
        cl_ulong start = 0;
        cl_ulong stop = 0;
        struct prl_pending_event *pendev = &scopinst->pending_events[i];
        cl_event event = pendev->event;
        cl_command_type cmdty;
        cl_int status;

        clGetEventProfilingInfo_checked(scopinst, event, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
        clGetEventProfilingInfo_checked(scopinst, event, CL_PROFILING_COMMAND_END, sizeof(stop), &stop, NULL);
        clGetEventInfo_checked(scopinst, event, CL_EVENT_COMMAND_TYPE, sizeof(cmdty), &cmdty, NULL);
        clGetEventInfo_checked(scopinst, event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(status), &status, NULL);

        assert(status == CL_COMPLETE);
        assert(start <= stop);
        add_time(scopinst, clcommand_to_stat_entry(cmdty), stop - start);
        if (global_state.config.gpu_detailed_profiling)
            dump_finished_event(pendev, cmdty, stop - start);

        profs[i].event = event;
        profs[i].start = start;
        profs[i].stop = stop;
        profs[i].cmdty = cmdty;
        profs[i].status = status;
    }

    qsort(profs, scopinst->event_size, sizeof *profs, &cmp_prof);

    prl_time_t gpu_working = 0;
    prl_time_t gpu_idle = 0;
    prl_time_t gpu_transfer_to_device = 0;
    prl_time_t gpu_compute = 0;
    prl_time_t gpu_transfer_to_host = 0;
    accumulate_gpu_durations(profs, scopinst->event_size, prof_all, &gpu_working, &gpu_idle);
    add_time(scopinst, stat_gpu_working, gpu_working);
    add_time(scopinst, stat_gpu_idle, gpu_idle);

    accumulate_gpu_durations(profs, scopinst->event_size, prof_to_device, &gpu_transfer_to_device, NULL);
    add_time(scopinst, stat_gpu_transfer_to_device, gpu_transfer_to_device);

    accumulate_gpu_durations(profs, scopinst->event_size, prof_compute, &gpu_compute, NULL);
    add_time(scopinst, stat_gpu_compute, gpu_compute);

    accumulate_gpu_durations(profs, scopinst->event_size, prof_to_host, &gpu_transfer_to_host, NULL);
    add_time(scopinst, stat_gpu_transfer_to_host, gpu_transfer_to_host);
}

// Change location of buffer without necessarily preserving its contents
static void ensure_on_host(prl_scop_instance scopinst, prl_mem mem) {
    assert(scopinst);
    assert(mem);

    if (mem->loc == loc_host || mem->loc == loc_transferring_to_host) {
        // Nothing to do
        return;
    }

    switch (mem->type) {
    case alloc_type_rwbuf:
	    if (mem->host_mem)
		mem->loc = loc_host;
	    else
		    mem->loc = loc_none; // There is no host memory to be updated, data is just lost.
        break;

    case alloc_type_map: {
        cl_event event = NULL;
        void *host_ptr = clEnqueueMapBuffer_checked(scopinst, scopinst->queue, mem->clmem, is_blocking(), mem->host_writable ? CL_MAP_WRITE : 0, 0, mem->size, 0, NULL, need_events() ? &event : NULL);
        assert(host_ptr == mem->host_mem);
        mem->loc = is_blocking() ? loc_host : loc_transferring_to_host;
        mem->transferevent = event;
        push_back_event(scopinst, event, mem, NULL, is_blocking());
    } break;
    default:
        assert(false);
    }
}

// Call when we know that a transfer has finished; this function updates its status
static void mem_event_finished(prl_scop_instance scopinst, prl_mem mem) {
    assert(is_valid_loc(mem));

    if (mem->loc & loc_mask_transferring) {
        assert(!mem->transferevent || has_transfer_completed(scopinst, mem));
    }

    //TODO: buffer transferred from is not yet obsolete; we could still use it for reading.
    switch (mem->loc) {
    case loc_transferring_to_dev:
        mem->transferevent = NULL;
        mem->loc = loc_dev;
        break;

    case loc_transferring_to_host:
        mem->transferevent = NULL;
        mem->loc = loc_host;
        break;

    default:
        break;
    }

    assert(is_valid_loc(mem));
}

void prl_scop_leave(prl_scop_instance scopinst) {
    assert(scopinst);
    assert(prl_initialized);

    prl_time_t scop_start = scopinst->scop_start;

    prl_mem lmem = scopinst->local_mems;
    while (lmem) {
        assert(lmem->scopinst);
        ensure_on_host(scopinst, lmem);
        lmem = lmem->mem_next;
    }
    for (int i = 0; i < scopinst->mems_size; i += 1) {
        prl_mem gmem = scopinst->mems[i];
        ensure_on_host(scopinst, gmem);
    }

    clFinish_checked(scopinst, scopinst->queue);
    eval_events(scopinst);

    for (int i = 0; i < scopinst->mems_size; i += 1) {
        prl_mem gmem = scopinst->mems[i];
        mem_event_finished(scopinst, gmem);
    }

    lmem = scopinst->local_mems;
    while (lmem) {
        prl_mem next = lmem->mem_next;
        mem_event_finished(scopinst, lmem);
        mem_free(scopinst, lmem);
        lmem = next;
    }

    free_events(scopinst);
    clReleaseCommandQueue_checked(scopinst, scopinst->queue);
    free_checked(scopinst, scopinst->mems);
    free_checked(NOSCOPINST, scopinst);

    prl_time_t scop_stop = timestamp();
    add_time(NOSCOPINST, stat_cpu_scop, scop_stop - scop_start);
}

void prl_scop_program_from_file(prl_scop_instance scopinst, prl_program *programref, const char *filename, const char *compiler_options) {
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

    char *str = malloc_checked(scopinst, size + 1);
    size_t read = fread(str, sizeof(char), size, file);
    assert(read == size);
    fclose(file);

    str[size] = '\0';
    prl_scop_program_from_str(scopinst, programref, str, size + 1, compiler_options);
    free_checked(scopinst, str);

    prl_program program = *programref;
    assert(!program->filename);
    program->filename = strdup(filename);
}

#define clCreateProgramWithSource_checked(context, count, strings, lengths) clCreateProgramWithSource_checked_impl(context, count, strings, lengths, __FILE__, __LINE__)
static cl_program clCreateProgramWithSource_checked_impl(cl_context context, cl_uint count, const char **strings, const size_t *lengths, const char *file, int line) {
    cl_int err = CL_INT_MIN;
    cl_program result = clCreateProgramWithSource(context, count, strings, lengths, &err);
    if (err != 0)
        opencl_error(err, "clCreateProgramWithSource");
    return result;
}

// str_size including NULL character
void prl_scop_program_from_str(prl_scop_instance scopinst, prl_program *programref, const char *str, size_t str_size, const char *build_options) {
    assert(scopinst);
    assert(programref);
    assert(str);
    assert(prl_initialized);

    prl_program program = *programref;
    if (!program) {
        if (!str_size)
            str_size = strlen(str);
        else
            str_size -= 1;
        cl_program clprogram = clCreateProgramWithSource_checked(global_state.context, 1, &str, &str_size);

        cl_int err = clBuildProgram(clprogram, 0, NULL, build_options, NULL, NULL);
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

        program = malloc_checked(scopinst, sizeof *program);
        memset(program, 0, sizeof *program);
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
    assert(program);

    prl_kernel kernel = *kernelref;
    if (!kernel) {
        cl_kernel clkernel = clCreateKernel_checked(NOSCOPINST, program->program, kernelname);

        kernel = malloc_checked(NOSCOPINST, sizeof *kernel);
        memset(kernel, 0, sizeof *kernel);
        kernel->scop = scop->scop;
        kernel->name = strdup(kernelname);
        kernel->kernel = clkernel;
        kernel->program = program;
        kernel->next = program->kernels;
        program->kernels = kernel;
        *kernelref = kernel;
    }
    assert(kernel->kernel);
}

static bool is_mem_registered(prl_scop_instance scopinst, prl_mem gmem) {
	assert(gmem);

	for (int i = 0; i < scopinst->mems_size; i+=1) {
			if (scopinst->mems[i] == gmem)
				return true;
		}
		return false;
}

prl_mem prl_scop_get_mem(prl_scop_instance scopinst, void *host_mem, size_t size, const char *name) {
    assert(scopinst);
    assert(size > 0);

    if (host_mem) {
	prl_mem gmem = prl_mem_lookup_global_ptr(host_mem, size);
	if (gmem) {
		if (!gmem->name && name) {
			gmem->name = strdup(name);
		}
		if (!is_mem_registered(scopinst, gmem))
			push_back_mem(scopinst, gmem);
		assert(is_valid_loc(gmem));
		return gmem;
	}
    }

    // If it is not a user-allocated memory location, create a temporary local one
    prl_mem lmem = prl_mem_create_empty(size, name, scopinst);
    if (host_mem) {
	     prl_mem_init_rwbuf_host( lmem,
		       host_mem, false, true, true, true,
		       true, true, loc_host);
    } else {
	    // No host memory available
	         prl_mem_init_rwbuf_none( lmem,
		       true, true,
		       true, true);
    }

    return lmem;

}

static void ensure_dev_allocated(prl_scop_instance scopinst, prl_mem mem) {
	assert(mem);

	if (mem->clmem)
		return;

	switch(mem->type) {
		case alloc_type_rwbuf:
			mem->clmem = clCreateBuffer_checked(scopinst, global_state.context, CL_MEM_READ_WRITE /*| CL_MEM_COPY_HOST_PTR*/, mem->size, NULL /*mem->host_mem*/);
			  mem->dev_owning = true;
			  mem->dev_exposed = false;
			break;
		default:
			assert(!"No device allocation for this type");
	}

    assert(mem->clmem);
}

static void ensure_host_allocated(prl_scop_instance scopinst, prl_mem mem) {
	assert(mem);

	if (mem->host_mem)
		return;

	switch(mem->type) {
		case alloc_type_rwbuf:
			mem->host_mem = malloc_checked(scopinst, mem->size);
			mem->host_owning  = true;
			mem->host_exposed = false;
			break;
		default:
			assert(!"No host allocation for this type");
	}

	assert(mem->host_mem);
}

static void *get_exposed_host(prl_scop_instance scopinst, prl_mem mem) {
	ensure_host_allocated(scopinst, mem);
	mem->host_exposed = true;
	return mem->host_mem;
}

void *prl_mem_get_host_mem(prl_mem mem) {
    assert(mem);

    return get_exposed_host( NOSCOPINST, mem);
}

cl_mem prl_mem_get_dev_mem(prl_mem mem) {
    assert(mem);

    mem->dev_exposed = true;
    return mem->clmem;
}

// Change location of buffer without necessarily preserving its contents; if content preservation is required, prl_scop_host_to_device must have been called first
static void ensure_to_device(prl_scop_instance scopinst, prl_mem mem) {
    assert(scopinst);
    assert(mem);

    if (mem->loc == loc_dev || mem->loc == loc_transferring_to_dev) {
        // Nothing to do
        return;
    }

    ensure_dev_allocated(scopinst, mem);
    assert(is_valid_loc(mem));

    switch (mem->type) {
    case alloc_type_rwbuf:
        switch (mem->loc) {
        case loc_dev:
        case loc_transferring_to_dev:
            // synchronous command queue; data will be available
            break;
        case loc_host:
        case loc_none:
            // no transfer needed
            mem->loc = loc_dev;
            break;
        default:
            // Unhandled, should never happen
            assert(false);
        }
        break;

    case alloc_type_map: {
        switch (mem->loc) {
        case loc_dev:                 // Nothing to do
        case loc_transferring_to_dev: // Synchronous command queues
            break;
        case loc_host: { //TODO: Transfer not necessary; data not needed to be preserved
            cl_event event = NULL;
            clEnqueueUnmapMemObject_checked(scopinst, scopinst->queue, mem->clmem, mem->host_mem, 0, NULL, (need_events() || is_blocking()) ? &event : NULL);
            if (is_blocking()) {
                clWaitForEvent_checked(scopinst, event);
                mem->loc = loc_dev;
                push_back_event(scopinst, event, mem, NULL, true);
            } else {
                mem->loc = loc_transferring_to_dev;
                mem->transferevent = event;
                push_back_event(scopinst, event, mem, NULL, false);
            }

        } break;
        default:
            assert(false);
        }

        //mem->loc = loc_dev;
    } break;

    default:
        assert(false);
    }

    assert(mem->loc == loc_dev || mem->loc == loc_transferring_to_dev);
    assert(is_valid_loc(mem));
}

static bool is_mem_available_on_dev(prl_mem mem) {
    return (mem->loc & loc_bit_dev_is_current) || (mem->loc & loc_bit_transferring_host_to_dev);
}

static bool is_mem_available_on_host(prl_mem mem) {
    return (mem->loc & loc_bit_host_is_current);
}

void prl_scop_host_to_device(prl_scop_instance scopinst, prl_mem mem) {
    assert(scopinst);
    assert(mem);
    assert(is_valid_loc(mem));

    if (is_mem_available_on_dev(mem)) {
        // Nothing to do
        return;
    }

    // If we did not allocate a device-side buffer yet, do it now
    ensure_dev_allocated(scopinst, mem);
    assert(is_valid_loc(mem));

    switch (mem->type) {
    case alloc_type_rwbuf: {
        if (mem->loc & loc_bit_host_is_current) {
            cl_event event = NULL;
            clEnqueueWriteBuffer_checked(scopinst, scopinst->queue, mem->clmem, is_blocking(), 0, mem->size, mem->host_mem, 0, NULL, need_events() ? &event : NULL);
            if (is_blocking()) {
                mem->loc = loc_dev;
                push_back_event(scopinst, event, mem, NULL, true);
            } else {
                mem->transferevent = event;
                mem->loc = loc_transferring_to_dev;
                push_back_event(scopinst, event, mem, NULL, false);
            }
        } else {
            mem->loc = loc_dev;
        }
    } break;

    case alloc_type_map: {
        cl_event event = NULL;
        clEnqueueUnmapMemObject_checked(scopinst, scopinst->queue, mem->clmem, mem->host_mem, 0, NULL, (need_events() || is_blocking()) ? &event : NULL);
        if (is_blocking()) {
            clWaitForEvent_checked(scopinst, event);
            mem->loc = loc_dev;
        } else {
            mem->transferevent = event;
            mem->loc = loc_transferring_to_dev;
            push_back_event(scopinst, event, mem, NULL, false);
        }
    } break;

    case alloc_type_dev_only:
        // Nothing to do
        break;

    case alloc_type_host_only:
    case alloc_type_none:
    case alloc_type_svm:
        assert(false);
    }

    assert(is_valid_loc(mem));
    assert(mem->loc == loc_dev || mem->loc == loc_transferring_to_dev);
}

void prl_scop_device_to_host(prl_scop_instance scopinst, prl_mem mem) {
    assert(scopinst);
    assert(mem);
    assert(is_valid_loc(mem));

    if (mem->loc == loc_host || mem->loc == loc_transferring_to_host) {
        // Already on host
        return;
    }

    switch (mem->type) {
    case alloc_type_rwbuf: {
        cl_event event = NULL;
        clEnqueueReadBuffer_checked(scopinst, scopinst->queue, mem->clmem, is_blocking(), 0, mem->size, mem->host_mem, 0, NULL, need_events() ? &event : NULL);
        if (is_blocking()) {
            assert(event);
            mem->loc = loc_host;
            push_back_event(scopinst, event, mem, NULL, true);
        } else {
            mem->loc = loc_transferring_to_host;
            mem->transferevent = event;
            push_back_event(scopinst, event, mem, NULL, false);
        }

    } break;
    case alloc_type_map: {
        cl_event event = NULL;
        void *mappedptr = clEnqueueMapBuffer_checked(scopinst, scopinst->queue, mem->clmem, is_blocking(), (mem->dev_readable ? CL_MAP_READ : 0) | (mem->dev_writable ? CL_MAP_WRITE : 0), 0, mem->size, 0, NULL, need_events() ? &event : NULL);
        assert(mappedptr == mem->host_mem && "clEnqueueMapBuffer should always return the same pointer");
        if (is_blocking()) {
            assert(event);
            mem->loc = loc_host;
            push_back_event(scopinst, event, mem, NULL, true);
        } else {
            mem->loc = loc_transferring_to_host;
            mem->transferevent = event;
            push_back_event(scopinst, event, mem, NULL, false);
        }
    } break;
    case alloc_type_host_only:
        // Nothing to do
        break;

    case alloc_type_dev_only:
    case alloc_type_none:
    case alloc_type_svm:
        assert(false);
    }
    assert(mem->loc == loc_host || mem->loc == loc_transferring_to_host);
    assert(is_valid_loc(mem));
}

void prl_scop_call(prl_scop_instance scopinst, prl_kernel kernel, int grid_dims, size_t grid_size[static const restrict grid_dims], int block_dims, size_t block_size[static const restrict block_dims], size_t n_args, struct prl_kernel_call_arg args[static const restrict n_args]) {
    assert(scopinst);
    assert(kernel);
    assert(grid_dims > 0);
    assert(grid_dims <= 3);
    assert(block_dims > 0);
    assert(block_dims <= 3);
    assert(grid_size);
    assert(block_size);
    assert(grid_dims == block_dims);

    for (int i = 0; i < n_args; i += 1) {
        struct prl_kernel_call_arg *arg = &args[i];

        switch (arg->type) {
        case prl_kernel_call_arg_value:
            clSetKernelArg_checked (scopinst, kernel->kernel, i, arg->size, arg->data);
            break;
        case prl_kernel_call_arg_mem: {
            assert(arg->mem);
            ensure_to_device(scopinst, arg->mem);
           clSetKernelArg_checked(scopinst, kernel->kernel, i, sizeof(cl_mem), &arg->mem->clmem);
        } break;
        }

    }

    int dims = (grid_dims < block_dims) ? grid_dims : block_dims;
    size_t work_items[3];
    for (int i = 0; i < dims; i += 1) {
        work_items[i] = grid_size[i] * block_size[i];
    }

    cl_event event = NULL;
    clEnqueueNDRangeKernel_checked(scopinst, scopinst->queue, kernel->kernel, dims, NULL, work_items, block_size, 0, NULL,
                                   (need_events() || is_blocking()) ? &event : NULL);
    if (is_blocking()) {
        clWaitForEvent_checked(scopinst, event);
        push_back_event(scopinst, event, NULL, kernel, true);
    } else {
        push_back_event(scopinst, event, NULL, kernel, false);
    }
}

void prl_prof_reset() {
    prl_init();

    free_checked(NOSCOPINST, global_state.bench_stats);
    global_state.bench_stats = NULL;
    global_state.bench_stats_size = 0;
}

void prl_prof_start() {
    global_state.prev_global_stat = global_state.global_stat;
    global_state.bench_start = timestamp();
}

void prl_prof_stop() {
    prl_time_t bench_stop = timestamp();
    struct prl_stat diff_stat;
    for (int i = 0; i < STAT_ENTRIES; i += 1) {
        diff_stat.entries[i] = global_state.global_stat.entries[i] - global_state.prev_global_stat.entries[i];
        assert(diff_stat.entries[i] >= 0);
        diff_stat.counts[i] = global_state.global_stat.counts[i] - global_state.prev_global_stat.counts[i];
        assert(diff_stat.counts[i] >= 0);
    }
    diff_stat.entries[stat_cpu_bench] = bench_stop - global_state.bench_start;
    diff_stat.counts[stat_cpu_bench] = 1;

    bench_push_back(&diff_stat);
}

void prl_prof_benchmark(timing_callback bench_func, void *user, timing_callback init_callback, void *init_user, timing_callback finit_callback, void *finit_user) {
    assert(bench_func);

    //FIXME: What is supposed to happen if prl is already initialized?
    assert(!prl_initialized);
    //TODO: Pass some other way, independent of user settings ,like by argument
    global_config.cpu_profiling = true;
    global_config.gpu_profiling = true;

    prl_init();
    assert(any_profiling());

    int warmups = global_state.config.timing_warmups;
    assert(warmups >= 0);
    int runs = global_state.config.timing_runs;
    assert(runs >= 1);

    prl_prof_reset();

    for (int i = 0; i < warmups; i += 1) {
        if (init_callback)
            (*init_callback)(init_user);
        (*bench_func)(user);
        if (finit_callback)
            (*finit_callback)(finit_user);
    }

    for (int i = 0; i < runs; i += 1) {
        if (init_callback)
            (*init_callback)(init_user);
        prl_prof_start();
        (*bench_func)(user);
        prl_prof_stop();
        if (finit_callback)
            (*finit_callback)(finit_user);
    }

    prl_prof_dump();
}

void prl_mem_free(prl_mem mem) {
    mem_free(NOSCOPINST, mem);
}

void *prl_alloc(size_t size) {
    prl_init(); // TODO: Lazy initialization at first scop enter? Requires global_state.global_mems to become separate and device memory to be allocated lazily

    prl_mem mem = prl_mem_create_empty(size, NULL, NOSCOPINST);
    prl_mem_init_rwbuf_none(mem, true, true, true, true);
    return get_exposed_host(NOSCOPINST, mem);
}

prl_mem prl_mem_alloc(size_t size, enum prl_mem_flags flags) {
	prl_init();

	prl_mem mem = prl_mem_create_empty(size, NULL, NOSCOPINST);
	prl_mem_init_rwbuf_none(mem, true, true, true, true);
	return mem;
}

void prl_free(void *ptr) {
    assert(prl_initialized);

    prl_mem mem = prl_mem_lookup_global_ptr(ptr, 0);
    assert(mem);
    assert(is_valid_loc(mem));

    mem_free(NOSCOPINST, mem);
}

prl_mem prl_mem_manage_host(size_t size, void *host_ptr, enum prl_mem_flags flags) {
    prl_init();
    assert(size > 0);
    assert(host_ptr);

    prl_mem gmem = prl_mem_create_empty(size, NULL, NOSCOPINST);
    prl_mem_init_rwbuf_host( gmem, host_ptr, false, true, true, true, true, true, loc_host);
	assert(is_valid_loc(gmem));
    return gmem;
}

cl_context prl_opencl_get_context() {
    prl_init();
    assert(global_state.context);
    return global_state.context;
}

static size_t get_clmem_size(cl_mem dev_ptr) {
    size_t size = -1ll;
    size_t size_size = 0;
    clGetMemObjectInfo_checked(NOSCOPINST, dev_ptr, CL_MEM_SIZE, sizeof(size_t), &size, &size_size);
    assert(size_size == sizeof(size));
    assert(size != -1ll && size > 0);

    return size;
}

prl_mem prl_opencl_mem_manage_dev(cl_mem dev_ptr, enum prl_mem_flags flags) {
    assert(dev_ptr);

    size_t size = get_clmem_size(dev_ptr);

    prl_mem gmem = prl_mem_create_empty(size, NULL, NOSCOPINST);
    prl_mem_init_rwbuf_dev( gmem, true, true, dev_ptr, false, true, true, true,  loc_dev );
    assert(is_valid_loc(gmem));
    return gmem;
}

prl_mem prl_opencl_mem_manage(void *host_ptr, cl_mem dev_ptr, enum prl_mem_flags flags) {
    assert(host_ptr);
    assert(dev_ptr);

    size_t size = get_clmem_size(dev_ptr);

    prl_mem gmem = prl_mem_create_empty(size, NULL, NOSCOPINST);
    gmem->type = alloc_type_rwbuf;
    gmem->loc = loc_none;

    gmem->host_mem = host_ptr;
    gmem->host_readable = !(flags & prl_mem_host_noread);
    gmem->host_writable = !(flags & prl_mem_host_nowrite);
    gmem->host_exposed = true;
    gmem->host_owning = flags & prl_mem_host_take;

    gmem->clmem = dev_ptr;
    gmem->dev_readable = !(flags & prl_mem_dev_noread);
    gmem->dev_writable = !(flags & prl_mem_dev_nowrite);
    gmem->dev_exposed = true;
    gmem->dev_owning = flags & prl_mem_dev_take;

    return gmem;
}
