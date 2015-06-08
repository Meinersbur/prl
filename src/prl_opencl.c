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


enum prl_device_choice
{
  PRL_TARGET_DEVICE_FIRST,
    PRL_TARGET_DEVICE_DYNAMIC ,
    PRL_TARGET_DEVICE_GPU_ONLY,
    PRL_TARGET_DEVICE_CPU_ONLY,
    PRL_TARGET_DEVICE_GPU_THEN_CPU,
    PRL_TARGET_DEVICE_CPU_THEN_GPU,
};


struct prl_global_config {
  enum prl_device_choice device_choice;
  int cpu_profiling;
  int gpu_profiling;
  bool blocking;
  //bool global_queue;
};

struct prl_global_state {
  int initialized;
  struct prl_global_config config;

  cl_device_id device;
  cl_context context;
  //cl_command_queue queue;

  prl_program programs;
};

struct prl_scop_struct {

};

struct prl_scop_inst_struct {
	prl_scop scop;
	 cl_command_queue queue;
};

struct prl_program_struct {
  cl_program program;

  prl_program next;
};

struct prl_kernel_struct {
	prl_scop scop;

	cl_kernel kernel;
};


struct prl_mem_struct{
	prl_scop scop;

	cl_mem mem;
	void *host_mem;
	size_t size;
};




static struct prl_global_config global_config = {0};

static bool prl_initialized = false;
static struct prl_global_state global_state;


//http://stackoverflow.com/questions/24326432/convenient-way-to-show-opencl-error-codes
static const char *opencl_getErrorString(cl_int error) {
switch(error){
    // runtime
    case CL_SUCCESS: return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND: return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE: return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE: return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES: return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY: return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE: return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP: return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH: return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE: return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE: return "CL_MAP_FAILURE";
    case CL_MISALIGNED_SUB_BUFFER_OFFSET: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case CL_COMPILE_PROGRAM_FAILURE: return "CL_COMPILE_PROGRAM_FAILURE";
    case CL_LINKER_NOT_AVAILABLE: return "CL_LINKER_NOT_AVAILABLE";
    case CL_LINK_PROGRAM_FAILURE: return "CL_LINK_PROGRAM_FAILURE";
    case CL_DEVICE_PARTITION_FAILED: return "CL_DEVICE_PARTITION_FAILED";
    case CL_KERNEL_ARG_INFO_NOT_AVAILABLE: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

    // compilation
    case CL_INVALID_VALUE: return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE: return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM: return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE: return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT: return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES: return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE: return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR: return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT: return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE: return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER: return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY: return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS: return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM: return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE: return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME: return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION: return "CL_INVALID_KERNEL_DEFINITION";
    case CL_INVALID_KERNEL: return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX: return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE: return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE: return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS: return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION: return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE: return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE: return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET: return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST: return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT: return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION: return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT: return "CL_INVALID_GL_OBJECT";
    case CL_INVALID_BUFFER_SIZE: return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL: return "CL_INVALID_MIP_LEVEL";
    case CL_INVALID_GLOBAL_WORK_SIZE: return "CL_INVALID_GLOBAL_WORK_SIZE";
    case CL_INVALID_PROPERTY: return "CL_INVALID_PROPERTY";
    case CL_INVALID_IMAGE_DESCRIPTOR: return "CL_INVALID_IMAGE_DESCRIPTOR";
    case CL_INVALID_COMPILER_OPTIONS: return "CL_INVALID_COMPILER_OPTIONS";
    case CL_INVALID_LINKER_OPTIONS: return "CL_INVALID_LINKER_OPTIONS";
    case CL_INVALID_DEVICE_PARTITION_COUNT: return "CL_INVALID_DEVICE_PARTITION_COUNT";

    // extensions
    case CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
    case CL_PLATFORM_NOT_FOUND_KHR: return "CL_PLATFORM_NOT_FOUND_KHR";
    case CL_INT_MIN: return "CL_INT_MIN (err not set)";
    default: return NULL;
    }
}


#define opencl_check(call) \
do {\
	cl_int err = (call); \
	if (err < 0) \
	opencl_error(err, #call, __FILE__, __LINE__); \
}while(0)

#define opencl_check_err(call) \
do {\
	cl_int err = CL_INT_MIN; \
	(call); \
	if (err < 0) \
	opencl_error(err, #call, __FILE__, __LINE__); \
}while(0)

static void opencl_error(cl_int err, const char *call, const char *file, int line) {
	//TODO: Print chosen driver
	const char *desc = opencl_getErrorString(err);
	if (desc) {
		fprintf(stderr, "%s:%d\t%s\nOpenCL error %s\n",         file, line, call, desc);
	} else {
		fprintf(stderr, "%s:%d\t%s\nOpenCL error %" PRIi32 "\n", file, line, call, err);
	}
	exit(1);
}





static void __ocl_report_error (const char *errinfo, const void *private_info, size_t cb, void *user_data)
{
  fprintf(stderr, "OCL error: %s", errinfo);
}


#define clCreateCommandQueue_checked(context, device, properties) clCreateCommandQueue_checked_impl(context, device, properties, __FILE__, __LINE__)
static  cl_command_queue clCreateCommandQueue_checked_impl(cl_context context,
                     cl_device_id device,
                     cl_command_queue_properties properties, const char *file, int line) {
	cl_int err = CL_INT_MIN;
	cl_command_queue result =clCreateCommandQueue(context, device, properties,&err);
	if (err || !result)
		opencl_error(err, "clCreateCommandQueue", file, line);
	return result;
}

static void prl_init() {
   cl_platform_id platform;

  if (prl_initialized)
    return;

  memset(&global_state, 0, sizeof(global_state));

  switch (global_config.device_choice) {
    case PRL_TARGET_DEVICE_FIRST:
     opencl_check(clGetPlatformIDs(1, &platform, NULL));
      opencl_check( clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &global_state.device, NULL));
    opencl_check_err(global_state.context = clCreateContext(NULL, 1, &global_state.device, __ocl_report_error,  NULL, &err));
      break;
  }

  prl_initialized = 1;
  global_state.initialized = 1;
}

static void prl_release() {
  if (!prl_initialized)
    return;

  prl_initialized = 0;
}






prl_scop_instance prl_scop_enter(prl_scop *scopref) {
   prl_init();

   prl_scop scop = *scopref;
   if (!scop) {
	   scop= malloc(sizeof *scop);
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

	free(scopinst);
}


void prl_scop_program_from_file(prl_scop_instance scopinst, prl_program *programref, const char *filename) {
assert(scopinst);
assert(programref);
assert(filename);

	if (*programref)
		return;

FILE *file = fopen(filename, "r");
assert(file);//TODO: Runtime checking
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	assert(size >= 0);
	rewind(file);

	char *str  = malloc(size + 1);
	size_t read = fread(str, sizeof(char), size, file);
	assert(read==size);
	fclose(file);

	str[size] = '\0';
prl_scop_program_from_str(scopinst, programref, str);
free(str);
}


#define clCreateProgramWithSource_checked(context, count, strings, lengths) clCreateProgramWithSource_checked_impl(context, count, strings, lengths, __FILE__, __LINE__)
static cl_program clCreateProgramWithSource_checked_impl(cl_context context, cl_uint count, const char **    strings, const size_t *lengths, const char *file, int line) {
	cl_int err = CL_INT_MIN;
	cl_program result = clCreateProgramWithSource(context, count, strings, lengths, &err);
if (err != 0)
	opencl_error(err, "clCreateProgramWithSource", file, line);
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
	cl_program clprogram = clCreateProgramWithSource_checked(global_state.context, 1,&str, &size);

	cl_int err = clBuildProgram(clprogram, 0, NULL, NULL, NULL, NULL);
	if (err< 0) { //TODO: Unified error handling
		fprintf(stderr, "Error during program build\n");
		size_t msgs_size;
		err = clGetProgramBuildInfo(clprogram, global_state.device, CL_PROGRAM_BUILD_LOG, 0,				NULL, &msgs_size);
		char *msgs = malloc(msgs_size + 1);
		err = clGetProgramBuildInfo(program->program, global_state.device, CL_PROGRAM_BUILD_LOG,				msgs_size, msgs, NULL);
		assert(err >= 0);
		msgs[msgs_size] = '\0';
		fputs(msgs, stderr);
		free(msgs);
	}
	assert(err>=0);

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


#define clEnqueueNDRangeKernel_checked(command_queue,kernel,work_dim,global_work_offset,global_work_size,local_work_size,num_events_in_wait_list,event_wait_list,event) clEnqueueNDRangeKernel_checked_impl(command_queue,kernel,work_dim,global_work_offset,global_work_size,local_work_size,num_events_in_wait_list,event_wait_list,event,__FILE__,__LINE__)
static void clEnqueueNDRangeKernel_checked_impl(cl_command_queue command_queue ,
                       cl_kernel         kernel,
                       cl_uint           work_dim ,
                       const size_t *    global_work_offset ,
                       const size_t *    global_work_size ,
                       const size_t *    local_work_size ,
                       cl_uint           num_events_in_wait_list ,
                       const cl_event *  event_wait_list ,
                       cl_event *        event , const char *file, int line) {
	cl_int err = clEnqueueNDRangeKernel(command_queue,kernel,work_dim,global_work_offset,global_work_size,local_work_size,num_events_in_wait_list,event_wait_list,event);
	if (err)
		opencl_error(err, "clEnqueueNDRangeKernel", file, line);
}



void prl_scop_call(prl_scop_instance scopinst, prl_kernel kernel, int dims, size_t grid_size[static const restrict dims], size_t block_size[static const restrict dims], size_t n_args, struct prl_kernel_call_arg args[static const restrict n_args]) {
assert(scopinst);
assert(kernel);
assert(dims>0);
assert(dims<=3);
assert(grid_size);
assert(block_size);

prl_scop scop = scopinst->scop;

	for (int i = 0; i < n_args; i+=1) {
		struct prl_kernel_call_arg *arg = &args[i];
		cl_int err = -1;
		switch (arg->type) {
			case prl_kernel_call_arg_value:
				 err = clSetKernelArg(kernel->kernel, i, arg->size, arg->data);
				break;
			case prl_kernel_call_arg_mem:
				err = clSetKernelArg(kernel->kernel, i, sizeof(cl_mem), &arg->mem->mem);
				break;
		}
		assert(err>=0);
	}

	size_t work_items[3];
	for (int i = 0; i<dims; i+=1) {
		work_items[i] = grid_size[i]*block_size[i];
	}

	cl_event event = NULL;
	clEnqueueNDRangeKernel_checked(scopinst->queue, kernel->kernel, dims, NULL, work_items, block_size, 0, NULL,
					     (global_config.gpu_profiling || global_config.blocking) ? &event : NULL);
	if (global_config. blocking) {
		cl_int err = clWaitForEvents(1, &event);
	}
}

#define clCreateBuffer_checked(context,flags,size,host_pre) clCreateBuffer_checked_impl(context,flags,size,host_pre,__FILE__,__LINE__)
static cl_mem clCreateBuffer_checked_impl(cl_context context, cl_mem_flags  flags, size_t size, void *host_ptr, const char *file, int line) {
	cl_int err = CL_INT_MIN;
cl_mem result =	clCreateBuffer(context, flags, size, host_ptr, &err);
if (err || !result)
	opencl_error(err, "clCreateBuffer", file, line);
return result;
}



void prl_scop_mem_init(prl_scop_instance scopinst, prl_mem *memref, void *host_mem, size_t size) {
assert(scopinst);
assert(memref);
assert(host_mem);

prl_scop scop = scopinst->scop;

	prl_mem mem = *memref;
	if (!mem) {

		cl_mem clmem = clCreateBuffer_checked(global_state.context, CL_MEM_READ_WRITE, size, NULL);
		assert(clmem);

		mem = malloc(sizeof *mem);
		mem->scop = scop;
		mem->mem = clmem;
		mem->host_mem = host_mem;
		mem->size = size;
		*memref = mem;
	}
	assert(mem->scop);
	assert(mem->mem);
}


void prl_scop_host_to_device(prl_scop_instance scopinst, prl_mem mem) {
	assert(scopinst);
	assert(mem);

	prl_scop scop = scopinst->scop;
	opencl_check(clEnqueueWriteBuffer(scopinst->queue, mem->mem, CL_TRUE, 0, mem->size, mem->host_mem, 0, NULL, NULL));
}


void prl_scop_device_to_host(prl_scop_instance scopinst, prl_mem mem) {
	assert(scopinst);
	assert(mem);

	prl_scop scop = scopinst->scop;
	cl_int err = clEnqueueReadBuffer(scopinst->queue, mem->mem, CL_TRUE, 0,mem->size, mem->host_mem, 0, NULL, NULL);
	assert(err>=0);
}


void prl_scop_host_wait(prl_scop_instance scop, prl_mem mem) {
	assert(scop);
	assert(mem);
}


