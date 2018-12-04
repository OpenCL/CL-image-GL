cmake_minimum_required(VERSION 2.8)

project(SC_OpenGL)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# find OpenCL
find_package(OpenCL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLFW REQUIRED)

include_directories(SYSTEM ${OpenCL_INCLUDE_DIRS})
include_directories(SYSTEM ${OPENGL_INCLUDE_DIR})
include_directories(SYSTEM ${GLFW_INCLUDE_DIR})

link_directories(${OpenCL_LIBRARIES}) 
link_directories(${OPENGL_glu_LIBRARY}) 
link_directories(${OPENGL_gl_LIBRARY}) 
link_directories(${GLFW_LIBRARIES})

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/RgbImage.cpp)
target_link_libraries(${PROJECT_NAME} ${OpenCL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${OPENGL_glu_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${OPENGL_gl_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARIES})

# If no build type specified, configure for Release
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    # Disable "no-unused-result" warning
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-unused-result")
endif()

# Custom post-build step: copy the file with the OpenCL kernels and
# all the input files from SOURCE_DIR to BINARY_DIR
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/OpenCLKernels.cl ${CMAKE_CURRENT_BINARY_DIR}/OpenCLKernels.cl)
	
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/img.bmp ${CMAKE_CURRENT_BINARY_DIR}/img.bmp)

