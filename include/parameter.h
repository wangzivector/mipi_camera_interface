#ifndef PARAMETER_H_
#define PARAMETER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <math.h>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>


#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define BYTEPERPIX 1

struct buffer
{
    void *start;
    size_t length;
} buffer;

enum io_method
{
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
}io_method;

extern struct buffer *buffers;
extern struct timeval tv;


extern int fd;
extern unsigned int i, n_buffers;
extern char out_name[256];
extern int frame_width;
extern int frame_height;
extern int frame_count;

extern int save_image_enable;
extern int show_image_enable;
extern int ext_trg_enable;
extern int vid_stream_enable;
extern char *save_folder;
extern char *dev_name;

extern int force_format;
extern enum io_method io;


void option_usage(FILE *fp, int argc, char **argv);
int set_option(int argc, char **argv);


#endif /* PARAMETER_H_ */

// #ifndef PARAMETER_H_
// #define PARAMETER_H_

// // define here.


// #endif /* PARAMETER_H_ */