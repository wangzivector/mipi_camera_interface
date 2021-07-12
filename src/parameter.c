#include "parameter.h"

int force_format = 0;

int frame_width = 1280;
int frame_height = 800;

// int frame_width = 640;
// int frame_height = 400;

int frame_count = 20;

int save_image_enable = 0;
int show_image_enable = 0;
int ext_trg_enable = 0;
int vid_stream_enable = 0;

char *save_folder = "./image/";
char *dev_name = "/dev/video0";

enum io_method io = IO_METHOD_MMAP;
// enum io_method io = IO_METHOD_READ;


void option_usage(FILE *fp, int argc, char **argv)
{
    fprintf(fp,
            "Usage: %s [options]\n\n"
            "Version 1.3\n"
            "Options:\n"
            "-s | --save          image folder path name: [%s]\n"
            "-h | --help          Print this message\n"
            "-m | --mmap          Use memory mapped buffers [default]\n"
            "-d | --display       iamge show_image_enable\n"
            "-u | --userp         Use application allocated buffers\n"
            "-e | --external      outer trigger mode \n"
            "-v | --video         video stream mode, not external trigger\n"
            "-f | --force         force set video format,like rgb and resolution\n"
            "-c | --count         Number of frames to grab [%i]\n"
            "\n",
            argv[0], dev_name, frame_count);
}

int set_option(int argc, char **argv)
{

const char short_options[] = "shmduevfc:";

const struct option
    long_options[] = {
        {"save", no_argument, NULL, 's'},
        {"help", no_argument, NULL, 'h'},
        {"mmap", no_argument, NULL, 'm'},
        {"disp", no_argument, NULL, 'd'},
        {"userp", no_argument, NULL, 'u'},
        {"external", no_argument, NULL, 'e'},
        {"video", no_argument, NULL, 'v'},
        {"force", no_argument, NULL, 'f'},
        {"count", required_argument, NULL, 'c'},    
        {0, 0, 0, 0}};

    for (;;)
    {
        int idx;
        int c;

        c = getopt_long(argc, argv,
                        short_options, long_options, &idx);

        if (-1 == c)
            break;

        switch (c)
        {
        case 0: /* getopt_long() flag */
            break;

        case 's':
            save_image_enable = 1;
            // save_folder = optarg;
            break;

        case 'h':
            option_usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);

        case 'm':
            io = IO_METHOD_MMAP;
            break;

        case 'd':
            show_image_enable = 1;
            break;

        case 'u':
            io = IO_METHOD_USERPTR;
            break;

        case 'e':
            ext_trg_enable = 1;
            break;

        case 'v':
            vid_stream_enable = 1;
            break;

        case 'f':
            force_format = 1;
            break;

        case 'c':
            errno = 0;
            frame_count = strtol(optarg, NULL, 0);
            if (errno)
            {
                fprintf(stderr, "%s error %d, %s\n", optarg, errno, strerror(errno));
                exit(EXIT_FAILURE);
            }
            break;

        default:
            option_usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }
}