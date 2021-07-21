#include "parameter.h"
#include "image_process.h"
#include "device_operation.h"
#include "camera_control.h"
#include "i2cbusses.h"

struct buffer *buffers;
struct timeval tv;

struct pic_temp_save{
    void *pic_buffer;
    int size;
}pic_temp_save;

int fd = -1;
unsigned int i, n_buffers;

// static void process_image(const void *pic_buffer, int size, int index_image, enum process_mode mdoe)
static void process_image(const struct buffer *buf_img, int index_image)
{
//////////////////////////////////////////////////////////////////////////////////////
    // printf("\r\n   /////////////////\n    PROCESSING IMAGE\n");
    printf("\rimage size is : %d  ", buf_img->length);
    switch (buf_img->length/(frame_width*frame_height))
    {
        case 3: 
            printf("image seems RGB format");
            if(save_image_enable)
            {
                // char picname[100];
                // sprintf(picname,"%sov5MP_%d*%d_%d.bmp",save_folder ,frame_width,frame_height, index_image);
                // GenBmpFile(buf_img.start, 24, frame_width,frame_height,picname); 
                LoadJpgFile(buf_img);
                // printf("\rimage saved: %s", picname);
            }
            if(show_image_enable)
            {
                printf(", displaying image");
                SDL_display_process(buf_img);
            }
            break;

        case 1:
            printf("image seems GREY format");
            if (save_image_enable)
            {
                // char picname[100];
                // sprintf(picname,"%sov9281_%d*%d_%03d.bmp",save_folder ,frame_width,frame_height, index_image);
                // GenBmpFile(buf_img.start, 8, frame_width, frame_height, picname); 
                LoadJpgFile(buf_img);
                // printf("\nimage saved: %s", picname);
            }
            if(show_image_enable)
            {
                printf(", displaying image");
                SDL_display_process(buf_img);
            }
            break;

        default:
            printf("image seems not RGB mode, skip RGB process, if any.");
            break;
    }
}

static int readFrame(void)
{
    printf("\n\n   /////////////////\n    READ FRAME\n");
    
    struct v4l2_buffer buf;
    unsigned int i;
    struct timeval begin, end;
    int uncatched_last, uncatched = 0;
    gettimeofday(&begin, 0);
    uncatched = 0;
    uncatched_last = 0;
    sync_obj.img_count = 0;
    for (sync_obj.img_count = 0; sync_obj.img_count < frame_count; sync_obj.img_count)
    {
        switch (io)
        {
//////////////////////////////////////////////////////////////////////////////////////
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
            sync_obj.img_count ++;

            process_image(&buffers[0], sync_obj.img_count);
            break;

// IO_METHOD_MMAP
//////////////////////////////////////////////////////////////////////////////////////
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
                        uncatched++;
                   break;

                case EIO:
                    printf("EIO error ..\r");
                    break;
                default:
                    printf("unknown error ..\r");
                    errno_exit("VIDIOC_DQBUF");
                }
            }

            printf("normal capture period with loop %d / %d -- %d \n", uncatched, uncatched_last, uncatched - uncatched_last);
            assert(buf.index < n_buffers);
            process_image(&buffers[buf.index], sync_obj.img_count); // process each iamge right after getting it, then next., address of dqbuf_buff_address is related to buffers[].start
            // printf("start transfer ... \n");
            sync_obj.img_count ++;
            
            sync_obj.index_video = sync_obj.img_count;
            clock_gettime(CLOCK_REALTIME, &sync_obj.ts_video);
            if (sync_obj.img_count == frame_count) sync_obj.SPI_enable = 0;

            gettimeofday(&end, 0);
            float sec_loop = ((end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)*1e-6);
            printf("\nnum %d --- time : %.02f ms ---------------------- -- frame rate: -// %.02f // %s\n", sync_obj.img_count, sec_loop*1000.0, 1/sec_loop, 1/sec_loop < 8 ? "***********" : "");
            gettimeofday(&begin, 0);
            
            if(buf.bytesused == 0) continue;
            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) // after processing the image data, put the buffer container back query to stream video
                errno_exit("VIDIOC_QBUF");
            break;

//////////////////////////////////////////////////////////////////////////////////////

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

            struct buffer image_buf = {
                (void *)buf.m.userptr,
                 buf.bytesused
            };
            process_image(&image_buf, sync_obj.img_count);

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
            break;
        }
    }
    return 1;
}


int main(int argc, char **argv)
{
    pthread_t pt_thre;
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
    
    // if(show_image_enable)  SDL_display_init();
    if(show_image_enable) SDL_init();
    if(save_image_enable) StartJpgFile();
    if(spi_check_enable)  SPI_Init();
    if(spi_check_enable)  pthread_create(&pt_thre, NULL, Mlt_SPI_transfer, NULL); // spi thread here

    readFrame();
    stopCapturing();

    if(show_image_enable)  SDL_deinit();
    if(save_image_enable)  FinishJpgFile();
    if(spi_check_enable)   pthread_exit(NULL);
    if(spi_check_enable)   SPI_Close();

    uninitDevice();
    closeDevice();

    return 0;
}
