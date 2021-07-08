#include "parameter.h"
#include "image_process.h"
#include "device_operation.h"
#include "camera_control.h"

#define UNCHATED_FILTER_VALUE 10000

struct buffer *buffers;
struct timeval tv;

enum process_mode{
    TEMP_COPY,
    ALL_PROCESS,
};

struct pic_temp_save{
    void *pic_buffer;
    int size;
}pic_temp_save;

int fd = -1;
unsigned int i, n_buffers;
char out_name[256];

struct pic_temp_save pic_buffs[300];

// static void process_image(const void *pic_buffer, int size, int index_image, enum process_mode mdoe)
static void process_image(const void *pic_buffer, int size, int index_image)
{

    // if (mdoe == TEMP_COPY)
    // {
    //     void *pic_buffer = 

    //     return;
    // }


    // printf("\r\n   /////////////////\n    PROCESSING IMAGE\n");
    printf("\rimage size is : %d  ", size);
    switch (size/(frame_width*frame_height))
    {
        case 3: 
            printf("image seems RGB format");
            if(save_image_enable)
            {
                char picname[100];
                sprintf(picname,"%sov5MP_%d*%d_%d.bmp",save_folder ,frame_width,frame_height, index_image);
                GenBmpFile(pic_buffer,24, frame_width,frame_height,picname); 
                printf("\rimage saved: %s", picname);
            }
            if(show_image_enable)
            {
                printf(", displaying image");
                SDL_display(pic_buffer,24);
            }
            break;

        case 1:
            printf("image seems GREY format");
            if (save_image_enable)
            {
                char picname[100];
                sprintf(picname,"%sov9281_%d*%d_%03d.bmp",save_folder ,frame_width,frame_height, index_image);
                GenBmpFile(pic_buffer, 8, frame_width, frame_height, picname); 
                printf("\nimage saved: %s", picname);
            }
            if(show_image_enable)
            {
                printf(", displaying image");
                SDL_display(pic_buffer,8);
            }
            break;

        default:
            printf("image seems not RGB mode, skip RGB process, if any.");
            break;
    }
}

static int readFrame(void)
{
    printf("\n   /////////////////\n    READ FRAME\n");
    
    struct v4l2_buffer buf;
    unsigned int i, count;
    struct timeval begin, end;
    int uncatched_last, uncatched = 0;
    gettimeofday(&begin, 0);
    uncatched = 0;
    uncatched_last = 0;

    for (count = 0; count < frame_count; count)
    {
        switch (io)
        {
        case IO_METHOD_READ:
            if (-1 == read(fd, buffers[0].start, buffers[0].length))
            {
                switch (errno)
                {
                case EAGAIN:
                    return 0;

                case EIO:
                    /* Could ignore EIO, see spec. */

                    /* fall through */

                default:
                    errno_exit("read");
                }
            }
                count ++;

            process_image(buffers[0].start, buffers[0].length, count);
            break;

        case IO_METHOD_MMAP: // default mode
            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            uncatched_last = uncatched;
            if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) // dqbuf means take out from memory space and to process
            {
                switch (errno)
                {
                case EAGAIN:
                    while (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) // dqbuf means take out from memory space and to process    
                    {
                        uncatched++;
                        // printf("there is no image availble yet, please wait .                  .. uncatched = %d \r", uncatched++);
                        // usleep(1);
                    }
                   break;
                    // return 0;
                case EIO:
                    printf("EIO error ..\r");
                    break;
                    /* Could ignore EIO, see spec. */
                    /* fall through */
                default:
                    printf("unknown error ..\r");
                    errno_exit("VIDIOC_DQBUF");
                }
            }

            if ((uncatched - uncatched_last) > UNCHATED_FILTER_VALUE) 
            {
                printf("\npriorly ignore the first frame after capture period with loop %d / %d -- %d \n", uncatched, uncatched_last, uncatched - uncatched_last);
            }
            else
            {
                printf("seem normal capture period with loop %d / %d -- %d \n", uncatched, uncatched_last, uncatched - uncatched_last);
                assert(buf.index < n_buffers);
                process_image(buffers[buf.index].start, buf.bytesused, count); // process each iamge right after getting it, then next., address of dqbuf_buff_address is related to buffers[].start
                
                count ++;
                gettimeofday(&end, 0);
                printf("num %d -- frame rate: %.02f   ---- time : %.02f ms\n", count, 1/((end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)*1e-6), 
                    ((end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)*1e-6)*1000.0);
                gettimeofday(&begin, 0);
            }
            
            if(buf.bytesused == 0) continue;
            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) // after processing the image data, put the buffer container back query to stream video
                errno_exit("VIDIOC_QBUF");
            break;

        case IO_METHOD_USERPTR:
            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;

            if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
            {
                switch (errno)
                {
                case EAGAIN:
                    return 0;

                case EIO:
                    /* Could ignore EIO, see spec. */

                    /* fall through */

                default:
                    errno_exit("VIDIOC_DQBUF");
                }
            }

            for (i = 0; i < n_buffers; ++i)
                if (buf.m.userptr == (unsigned long)buffers[i].start && buf.length == buffers[i].length)
                    break;

            assert(i < n_buffers);

            process_image((void *)buf.m.userptr, buf.bytesused, count);

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
            break;
        }
    }
    return 1;
}


int main(int argc, char **argv)
{
    set_option(argc, argv); 
    openDevice();
    initDevice();

    // selectDevice(checkDeivce());
    // enumerateMenuList();
    // enumerateExtMenuList();
    // no use
    // cameraFunctionsControl(V4L2_CID_EXPOSURE_AUTO, 1);
    // cameraFunctionsControl(V4L2_CID_EXPOSURE_ABSOLUTE, 50);
    // enumerateExtendedControl();
    startCapturing();
    setRegExtMode();
    
    if(show_image_enable)  SDL_display_init();

    readFrame();

    stopCapturing();
    uninitDevice();

    closeDevice();
    if(show_image_enable)  SDL_display_wait2close();
    return 0;
}
