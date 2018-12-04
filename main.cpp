#include <GLFW/glfw3.h>
#include <GL/glut.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include "RgbImage.h"
#include <string.h>

#include <CL/cl.h>
#include <CL/cl_gl.h>

char* filename = "img.bmp";

///////////////////////////////////////////////////////////////////////////////
// Help macros for checking for errors
#define CHECK_NULL(p) \
    {\
        if (NULL == p)\
        {\
            printf("NULL pointer at line %d in file %s", __LINE__, __FILE__);\
            exit(EXIT_FAILURE);\
        }\
    }

#define CHECK_OCL_ERR(err) \
    {\
        if (CL_SUCCESS != err)\
        {\
            printf("OpenCL error %d at line %d in file %s", err, __LINE__, __FILE__);\
            exit(EXIT_FAILURE);\
        }\
    }

///////////////////////////////////////////////////////////////////////////////
// Prints out the name of the input platform.
void PrintPlatformName(cl_platform_id platform)
{
    cl_uint clError = 0;
    size_t nameSize = 0;
    char* platformName = NULL;
    
    clError = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, NULL, &nameSize);
    CHECK_OCL_ERR(clError);

    platformName = (char*)malloc(nameSize * sizeof(char));
    CHECK_NULL(platformName);

    clError = clGetPlatformInfo(platform, CL_PLATFORM_NAME, nameSize, platformName, NULL);
    CHECK_OCL_ERR(clError);

    printf("%s", platformName);

    free(platformName);
}

///////////////////////////////////////////////////////////////////////////////
// Prints out the name of the input device.
void PrintDeviceName(cl_device_id device)
{
    cl_uint clError = 0;
    size_t nameSize = 0;
    char* deviceName = NULL;

    clError = clGetDeviceInfo(device, CL_DEVICE_NAME, 0, NULL, &nameSize);
    CHECK_OCL_ERR(clError);

    deviceName = (char*)malloc(nameSize * sizeof(char));
    CHECK_NULL(deviceName);

    clError = clGetDeviceInfo(device, CL_DEVICE_NAME, nameSize, deviceName, NULL);
    CHECK_OCL_ERR(clError);

    printf("%s", deviceName);

    free(deviceName);
}

///////////////////////////////////////////////////////////////////////////////
// Prints out detailed information about OpenCL.
int PrintOpenCLInfo()
{    
    cl_uint numPlatforms = 0;
    cl_platform_id *platforms = 0;
    cl_int clError = 0;

    clError = clGetPlatformIDs(0, NULL, &numPlatforms);
    CHECK_OCL_ERR(clError);

    if (0 >= numPlatforms)
        return 0;

    printf("\nOpenCL platforms detected: %d", numPlatforms);

    platforms = (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));
    CHECK_NULL(platforms);

    clError = clGetPlatformIDs(numPlatforms, platforms, NULL);
    CHECK_OCL_ERR(clError);

    for (cl_uint i = 0; i < numPlatforms; i++)
    {       
        cl_uint numDevices = 0;
        cl_device_id *devices = NULL;               

        printf("\n%d. Platform: ", i + 1);
        PrintPlatformName(platforms[i]);
        
        clError = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
        CHECK_OCL_ERR(clError);
        
        printf("\n\tNumber of devices: %d", numDevices);

        devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
        CHECK_NULL(devices);

        clError = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, numDevices, devices, NULL);
        CHECK_OCL_ERR(clError);

        for (cl_uint j = 0; j < numDevices; j++)
        {           
            printf("\n\t%d. Device: ", j + 1);
            PrintDeviceName(devices[j]);

            cl_device_type deviceType;
            clError = clGetDeviceInfo(devices[j], CL_DEVICE_TYPE, sizeof(deviceType), &deviceType, NULL);
            CHECK_OCL_ERR(clError);

            switch (deviceType)
            {
            case CL_DEVICE_TYPE_CPU:                
                printf("\n\t\tType: CPU");
                break;
            case CL_DEVICE_TYPE_GPU:                
                printf("\n\t\tType: GPU");
                break;
            case CL_DEVICE_TYPE_ACCELERATOR:
                printf("\n\t\tType: ACCELERATOR");                
                break;
            default:            
                printf("\n\t\tType: Unknown");                
                break;
            }

            cl_uint cuCnt = 0;
            clError = clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cuCnt), &cuCnt, NULL);
            CHECK_OCL_ERR(clError);
            printf("\n\t\tNumber of CUs: %d", cuCnt);
        }

        if (devices)
            free(devices);
    }

    if (platforms)
        free(platforms);

    return numPlatforms;
}

///////////////////////////////////////////////////////////////////////////////
// Returns a platform and device id as selected by the user.
void SelectOpenCLPlatformAndDevice(cl_platform_id* pPlatform, cl_device_id* pDevice)
{
    cl_uint numPlatforms = 0;
    cl_uint numDevices = 0;
    cl_uint clError = 0;
    int platformIndex = 0;
    int deviceIndex = 0;
    cl_platform_id *platforms = 0;
    cl_device_id* devices = 0;

    CHECK_NULL(pPlatform);
    CHECK_NULL(pDevice);

    platforms = NULL;
    devices = NULL;

    clError = clGetPlatformIDs(0, NULL, &numPlatforms);
    CHECK_OCL_ERR(clError);

    platforms = (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));
    CHECK_NULL(platforms);

    clError = clGetPlatformIDs(numPlatforms, platforms, NULL);
    CHECK_OCL_ERR(clError);

    printf("\n\nSelect platform to use [%d-%d]:", 1, numPlatforms);
    scanf("%d", &platformIndex);
    platformIndex--;

    *pPlatform = platforms[platformIndex];

    clError = clGetDeviceIDs(platforms[platformIndex], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
    CHECK_OCL_ERR(clError);

    devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
    CHECK_NULL(devices);

    clError = clGetDeviceIDs(platforms[platformIndex], CL_DEVICE_TYPE_ALL, numDevices, devices, NULL);
    CHECK_OCL_ERR(clError);

    printf("Select device to use [%d-%d]:", 1, numDevices);
    scanf("%d", &deviceIndex);
    deviceIndex--;

    *pDevice = devices[deviceIndex];

    if (platforms)
        free(platforms);
    if (devices)
        free(devices);
}

///////////////////////////////////////////////////////////////////////////////
// Creates an OpenCL context for the given device and platform.
cl_context CreateOpenCLContext(cl_platform_id platform, cl_device_id device)
{
    cl_int clError;
    cl_context context;

    cl_context_properties contextProperties[] =
    {
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
		CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(), 
        CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(), 
        0
    };

    context = clCreateContext(contextProperties, 1, &device, NULL, NULL, &clError);
    CHECK_OCL_ERR(clError);

    return context;
}

///////////////////////////////////////////////////////////////////////////////
// Releases the input OpenCL context.
void ReleaseOpenCLContext(cl_context *pContext)
{
    cl_int clError;

    CHECK_NULL(pContext);

    if (*pContext)
    {
        clError = clReleaseContext(*pContext);
        CHECK_OCL_ERR(clError);

        *pContext = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Creates an OpenCL queue for the given device and context.
cl_command_queue CreateOpenCLQueue(cl_device_id device, cl_context context)
{
    cl_int clError;
    cl_command_queue queue;

    queue = clCreateCommandQueue(context, device, 0, &clError);
    CHECK_OCL_ERR(clError);

    return queue;
}

///////////////////////////////////////////////////////////////////////////////
// Releases the input OpenCL queue.
void ReleaseOpenCLQueue(cl_command_queue *pQueue)
{
    cl_int clError;

    CHECK_NULL(pQueue);

    if (*pQueue)
    {
        clError = clReleaseCommandQueue(*pQueue);
        CHECK_OCL_ERR(clError);

        *pQueue = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Creates an OpenCL buffer in the given context.
cl_mem CreateDeviceBuffer(cl_context context, size_t sizeInBytes)
{
    cl_int clError;

    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeInBytes, NULL, &clError);
    CHECK_OCL_ERR(clError);

    return buffer;
}

///////////////////////////////////////////////////////////////////////////////
// Releases the input OpenCL memory buffer.
void ReleaseDeviceBuffer(cl_mem *pDeviceBuffer)
{
    cl_int clError;

    CHECK_NULL(pDeviceBuffer);

    if (*pDeviceBuffer)
    {
        clError = clReleaseMemObject(*pDeviceBuffer);
        CHECK_OCL_ERR(clError);

        *pDeviceBuffer = 0;
    }      
}

///////////////////////////////////////////////////////////////////////////////
// Creates an OpenCL image for the given device and platform.
/*cl_mem CreateDeviceImage(cl_context context, size_t width, size_t height)
{
    cl_int clError;
    cl_image_format imageFormat;
    cl_image_desc imageDesc;

    imageFormat.image_channel_order = CL_LUMINANCE;
        //CL_R;
    imageFormat.image_channel_data_type = CL_FLOAT;

    memset(&imageDesc, 0, sizeof(imageDesc));
    imageDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
    imageDesc.image_width = width;
    imageDesc.image_height = height;

    cl_mem buffer = clCreateImage(context, CL_MEM_READ_ONLY, &imageFormat, &imageDesc, NULL, &clError);
    CHECK_OCL_ERR(clError);

    return buffer;
}*/

///////////////////////////////////////////////////////////////////////////////
// Copies data from a host buffer to an OpenCL image.
void CopyImageHostToDevice(void* hostBuffer, cl_mem deviceBuffer, size_t width, size_t height, cl_command_queue queue, cl_bool blocking)

{
    cl_int clError;
    size_t origin[] = { 0, 0, 0 };
    size_t region[] = {width, height, 1};

    clError = clEnqueueWriteImage(queue, deviceBuffer, blocking, origin, region, 0, 0, hostBuffer, 0, NULL, NULL);

    CHECK_OCL_ERR(clError);
}

///////////////////////////////////////////////////////////////////////////////
// Copies data from a host buffer to an OpenCL device buffer.
void CopyHostToDevice(void* hostBuffer, cl_mem deviceBuffer, size_t sizeInBytes, cl_command_queue queue, cl_bool blocking)
{
    cl_int clError;

    clError = clEnqueueWriteBuffer(queue, deviceBuffer, blocking, 0, sizeInBytes, hostBuffer, 0, NULL, NULL);

    CHECK_OCL_ERR(clError);
}

///////////////////////////////////////////////////////////////////////////////
// Copies data from a device buffer back to host.
void CopyDeviceToHost(cl_mem deviceBuffer, void* hostBuffer, size_t sizeInBytes, cl_command_queue queue, cl_bool blocking)
{
    cl_int clError;

    clError = clEnqueueReadBuffer(queue, deviceBuffer, blocking, 0, sizeInBytes, hostBuffer, 0, NULL, NULL);

    CHECK_OCL_ERR(clError);
}

///////////////////////////////////////////////////////////////////////////////
// Loads the OpenCL code from the input file.
char* LoadOpenCLSourceFromFile(const char* filePath, size_t *pSourceLength)
{
    FILE* fileHandle;
    char* sourceCode;

    fileHandle = fopen(filePath, "rb");
    CHECK_NULL(fileHandle);
    fseek(fileHandle, 0, SEEK_END);

    *pSourceLength = ftell(fileHandle);
    sourceCode = (char*)malloc((*pSourceLength) + 1);
    CHECK_NULL(sourceCode);

    fseek(fileHandle, 0, SEEK_SET);
    fread(sourceCode, *pSourceLength, 1, fileHandle);
    sourceCode[(*pSourceLength)] = 0;
    *pSourceLength = (*pSourceLength) + 1;

    fclose(fileHandle);

    return sourceCode;
}

///////////////////////////////////////////////////////////////////////////////
// Builds an OpenCL program for the specified device
void BuildProgram(cl_program program, cl_device_id device)
{
    cl_int clError;
    char *buildLog;
    size_t buildLogSize;
    //char buildOptions[256];
    
    //sprintf(buildOptions, "-DVALS_PER_WI=%d", VALS_PER_WI);
    clError = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (CL_SUCCESS != clError)
    {
        printf("\nOpenCL error %d at line %d in file %s", clError, __LINE__, __FILE__);

        buildLog = NULL;
        buildLogSize = 0;

        clError = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &buildLogSize);
        CHECK_OCL_ERR(clError);

        if (buildLogSize)
        {
            // Allocate memory to fit the build log - it can be very large in case of errors
            buildLog = (char*)malloc(buildLogSize);
            if (buildLog)
            {
                clError = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, NULL);
                CHECK_OCL_ERR(clError);

                printf("\nOpenCL program build info: \n%s\n", buildLog);

                // Free buildLog buffer
                free(buildLog);
            }
        }

        exit(EXIT_FAILURE);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Creates and builds an OpenCL program with the input source code for
// the given context and source code string.
cl_program CreateAndBuildProgramFromSource(cl_context context, char* sourceCode, size_t sourceCodeLength)
{
    cl_program program;
    cl_int clError;
    cl_device_id device;

    clError = clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(cl_device_id), &device, NULL);
    CHECK_OCL_ERR(clError);

    program = clCreateProgramWithSource(context, 1, (const char**)(&sourceCode), &sourceCodeLength, &clError);
    CHECK_OCL_ERR(clError);

    BuildProgram(program, device);

    return program;
}

///////////////////////////////////////////////////////////////////////////////
// Releases an OpenCL program object.
void ReleaseProgram(cl_program *pProgram)
{
    cl_int clError;

    CHECK_NULL(pProgram);

    if (*pProgram)
    {
        clError = clReleaseProgram(*pProgram);
        CHECK_OCL_ERR(clError);

        *pProgram = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Creates an OpenCL kernel object for the kernel with the given name.
cl_kernel CreateKernel(cl_program program, const char* kernelName)
{
    cl_int clError;
    cl_kernel kernel;

    kernel = clCreateKernel(program, kernelName, &clError);
    CHECK_OCL_ERR(clError);

    return kernel;
}

///////////////////////////////////////////////////////////////////////////////
// Releases an OpenCL kernel object.
void ReleaseKernel(cl_kernel *pKernel)
{
    cl_int clError;

    CHECK_NULL(pKernel);

    if (*pKernel)
    {
        clError = clReleaseKernel(*pKernel);
        CHECK_OCL_ERR(clError);

        *pKernel = 0;
    }
}

GLuint loadTextureFromFile(RgbImage theTexMap, int id)
{   
	GLuint texture;
	glGenTextures(id, &texture); // Get the First Free Name to use for the Font Texture
	glBindTexture(GL_TEXTURE_2D, texture); // Actually create the texture object
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);

   // Pixel alignment: each row is word aligned (aligned to a 4 byte boundary)
   //    Therefore, no need to call glPixelStore( GL_UNPACK_ALIGNMENT, ... );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, theTexMap.GetNumCols(), theTexMap.GetNumRows();

    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, theTexMap.GetNumCols(), theTexMap.GetNumRows(), GL_RGB, GL_UNSIGNED_BYTE, theTexMap.ImageData() );
	
	return texture;
}

GLuint loadTexture(int id, int width, int height)
{   
	GLuint texture;
	glGenTextures(id, &texture); // Get the First Free Name to use for the Font Texture
	glBindTexture(GL_TEXTURE_2D, texture); // Actually create the texture object
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);

   // Pixel alignment: each row is word aligned (aligned to a 4 byte boundary)
   //    Therefore, no need to call glPixelStore( GL_UNPACK_ALIGNMENT, ... );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

	return texture;
}

void runKernel(cl_command_queue queue, cl_kernel kernel, cl_mem image, cl_mem filterWeightsBuffer, cl_mem buffer, int width, int height)
{
	cl_int clError = 0;
	
	glFinish();
	clEnqueueAcquireGLObjects(queue, 1,  &image, 0, 0, NULL);
	clEnqueueAcquireGLObjects(queue, 1,  &buffer, 0, 0, NULL);
	clFinish(queue);
	clError |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &image);
	clError |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &filterWeightsBuffer);
	clError |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &buffer);
	CHECK_OCL_ERR(clError);

	int workDim = 2;
	size_t globalWorkSize[2] = {width, height};
	// Launch the kernel
	clError = clEnqueueNDRangeKernel(queue, kernel, workDim, NULL, globalWorkSize, NULL, 0, NULL, NULL);
	CHECK_OCL_ERR(clError);
	clFinish(queue);
	
	clEnqueueReleaseGLObjects(queue, 1,  &image, 0, 0, NULL);
	clEnqueueReleaseGLObjects(queue, 1,  &buffer, 0, 0, NULL);
	clFinish(queue);
}

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(void)
{
	float filter [] = {
		1, 2, 1,
		2, 4, 2,
		1, 2, 1
	};

	// Normalize the filter
	for (int i = 0; i < 9; ++i) {
		filter [i] /= 16.0f;
	}
	
	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	
	cl_platform_id platform = 0;
    cl_device_id device = 0;
    cl_context context = 0;
    cl_command_queue queue = 0;

    char* sourceCode = NULL;
    size_t sourceCodeLength = 0;

    cl_program program = 0;
    cl_kernel filterKernel = 0;

    cl_int clError = 0;

    // Check if OpenCL is supported and print info
    if (!PrintOpenCLInfo())
    {
		printf("\nNo OpenCL platform or device detected.");
		exit(EXIT_FAILURE);
	}

    // OpenCL initializations
    // Select an OpenCL platform and device
    SelectOpenCLPlatformAndDevice(&platform, &device);

    // Print the names of the selected platform and device
    printf("\nUsing platform "); PrintPlatformName(platform);
    printf(" and device "); PrintDeviceName(device);
    printf("\n");
	
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	window = glfwCreateWindow(512, 512, "Simple example", NULL, NULL);
	
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	
	context = CreateOpenCLContext(platform, device);
    queue = CreateOpenCLQueue(device, context);
	
	sourceCode = LoadOpenCLSourceFromFile("OpenCLKernels.cl", &sourceCodeLength);
    program = CreateAndBuildProgramFromSource(context, sourceCode, sourceCodeLength);
	filterKernel = CreateKernel(program, "Filter");
	
	GLuint texture;
	GLuint texture2;
	RgbImage theTexMap1(filename);
	RgbImage theTexMap2(filename);
    texture = loadTextureFromFile(theTexMap1, 1);
	texture2 = loadTexture(1, width, height);
	
	// Create OpenCL buffers on device
	cl_mem image = clCreateFromGLTexture2D(context, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, texture, &clError);
	CHECK_OCL_ERR(clError);
	
	cl_mem filterWeightsBuffer = clCreateBuffer (context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof (float) * 9, filter, &clError);
	CHECK_OCL_ERR(clError);
		
    cl_mem buffer = clCreateFromGLTexture2D(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, texture2, &clError);
	CHECK_OCL_ERR(clError);
	
	runKernel(queue, filterKernel, image, filterWeightsBuffer, buffer, width, height);

	while (!glfwWindowShouldClose(window))
	{
		float ratio;
		ratio = width / (float) height;
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(0.f, 0.f, 0.f, 1.f);
		
		//runKernel(queue, filterKernel, image, filterWeightsBuffer, buffer, width, height);
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	   	glEnable(GL_TEXTURE_2D);
	   	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	   	glBegin(GL_QUADS);

	   	glTexCoord2f(0.0, 0.0);
	   	glVertex3f(-1.0, -1.0, 0.0);

	   	glTexCoord2f(0.0, 1.0);
	   	glVertex3f(-1.0, 1.0, 0.0);

	   	glTexCoord2f(1.0, 1.0);
	   	glVertex3f(1.0, 1.0, 0.0);

	   	glTexCoord2f(1.0, 0.0);
	   	glVertex3f(1.0, -1.0, 0.0);

	   	glEnd();

	   	glFlush();
	   	glDisable(GL_TEXTURE_2D);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	
	clReleaseMemObject(image);
	clReleaseMemObject(filterWeightsBuffer);
	clReleaseMemObject(buffer);
	
	if (sourceCode)
        free(sourceCode);
	ReleaseKernel(&filterKernel);
    ReleaseProgram(&program);
	ReleaseOpenCLQueue(&queue);
    ReleaseOpenCLContext(&context);

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}