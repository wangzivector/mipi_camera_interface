# cmake needs this line
cmake_minimum_required(VERSION 2.8)
 
# Define project name
project(v4l2Test)
 
# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# via the command line or GUI

find_package(SDL REQUIRED)

include_directories(include)
add_subdirectory(src)
# If the package has been found, several variables will
# be set, you can find the full list with descriptions
# in the OpenCVConfig.cmake file.
# Print some message showing some of them
# message(STATUS "    SDL library status:")
# message(STATUS "    version: ${SDL_VERSION}")
# message(STATUS "    libraries: ${SDL_LIBS}")
# message(STATUS "    include path: ${SDL_INCLUDE_DIRS}")
 
# Add OpenCV headers location to your include paths
# include_directories(${OpenCV_INCLUDE_DIRS})
 
# Declare the executable target built from your sources
# add_executable(camera_capture_example camera_capture_example.c)
 
# Link your application with OpenCV libraries
# target_link_libraries(camera_capture_example)

# # Declare the executable target built from your sources
# add_executable(camera_image_grab camera_image_grab.c image_process.c device_operation.c camera_control.c parameter.c)
 
# # Link your application with OpenCV libraries
# target_link_libraries(camera_image_grab /usr/lib/arm-linux-gnueabihf/libSDL.so)


 
# Declare the executable target built from your sources
# add_executable(copy copy.cpp)
 
# Link your application with OpenCV libraries
# target_link_libraries(copy /usr/lib/arm-linux-gnueabihf/libSDL.so)

