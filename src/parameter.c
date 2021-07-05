#include "parameter.h"

void option_usage(FILE *fp, int argc, char **argv)
{
    fprintf(fp,
            "Usage: %s [options]\n\n"
            "Version 1.3\n"
            "Options:\n"
            "-s | --save image    image folder path name: [%s]\n"
            "-h | --help          Print this message\n"
            "-m | --mmap          Use memory mapped buffers [default]\n"
            "-r | --read          Use read() calls\n"
            "-u | --userp         Use application allocated buffers\n"
            "-o | --output        Outputs stream to stdout\n"
            "-f | --format        Force format to 640x480 YUYV\n"
            "-c | --count         Number of frames to grab [%i]\n"
            "\n",
            argv[0], dev_name, frame_count);
}

int set_option(int argc, char **argv)
{

const char short_options[] = "s:hmruofc:";

const struct option
    long_options[] = {
        {"save", required_argument, NULL, 's'},
        {"help", no_argument, NULL, 'h'},
        {"mmap", no_argument, NULL, 'm'},
        {"read", no_argument, NULL, 'r'},
        {"userp", no_argument, NULL, 'u'},
        {"output", no_argument, NULL, 'o'},
        {"format", no_argument, NULL, 'f'},
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
            save_iamge_enable = 1;
            save_folder = optarg;
            break;

        case 'h':
            option_usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);

        case 'm':
            io = IO_METHOD_MMAP;
            break;

        case 'r':
            io = IO_METHOD_READ;
            break;

        case 'u':
            io = IO_METHOD_USERPTR;
            break;

        case 'o':
            // out_buf++;
            break;

        case 'f':
            force_format++;
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