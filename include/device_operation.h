#ifndef DEVICE_OPERATION_H_
#define DEVICE_OPERATION_H_

#include "parameter.h"

struct v4l2_buffer buf;
enum v4l2_buf_type type;
struct v4l2_requestbuffers req;

void errno_exit(const char *s);
int xioctl(int fh, int request, void *arg);
void init_read(unsigned int buffer_size);
void init_mmap(void);
void init_userp(unsigned int buffer_size);
void uninitDevice(void); // release data buffer space
void initDevice(void);
void closeDevice(void); // close fd
void selectDevice(int index);
int checkDeivce(void);
void openDevice(void);
void startCapturing(void);
void stopCapturing(void); // stop the driver from streaming data



#endif /* PARAMETER_H_ */