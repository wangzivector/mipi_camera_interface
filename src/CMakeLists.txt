
# Declare the executable target built from your sources
add_executable(camera_image_grab camera_image_grab.c image_process.c device_operation.c camera_control.c parameter.c i2cbusses.c )
 
# Link your application with OpenCV libraries
target_link_libraries(camera_image_grab /usr/lib/arm-linux-gnueabihf/libSDL.so)
