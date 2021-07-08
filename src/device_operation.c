#include "device_operation.h"
#include "i2cbusses.h"
void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

int xioctl(int fh, int request, void *arg)
{
    int r;

    do
    {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

void init_read(unsigned int buffer_size)
{
    buffers = calloc(1, sizeof(*buffers));

    if (!buffers)
    {
        fprintf(stderr, "Out of memory\\n");
        exit(EXIT_FAILURE);
    }

    buffers[0].length = buffer_size;
    buffers[0].start = malloc(buffer_size);

    if (!buffers[0].start)
    {
        fprintf(stderr, "Out of memory\\n");
        exit(EXIT_FAILURE);
    }
}


void init_mmap(void)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4; // what is 4? the buffer size of frame to dynamically saved? do it related to n_buffers
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP; // set the video mode as memory address mapping

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) //
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s does not support "
                            "memory mapping",
                    dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) // request failure
    {
        fprintf(stderr, "Insufficient buffer memory on %s\\n",
                dev_name);
        exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers)); // create space in buffers point, ready to read

    if (!buffers)
    {
        fprintf(stderr, "Out of memory\\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) // for each buffer container in buffers point, store image!
    {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) // read the info. of iamge buffer, like buffer length of data
            errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =          // this is a memory mapping function and return address of data, avoiding hard copy
            mmap(NULL /* start anywhere */, // usually set to NULL
                 buf.length,
                 PROT_READ | PROT_WRITE /* required */,
                 MAP_SHARED /* recommended */,
                 fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
    }
}

void init_userp(unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s does not support "
                            "user pointer i/on",
                    dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    buffers = calloc(4, sizeof(*buffers));

    if (!buffers)
    {
        fprintf(stderr, "Out of memory\\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < 4; ++n_buffers)
    {
        buffers[n_buffers].length = buffer_size;
        buffers[n_buffers].start = malloc(buffer_size);

        if (!buffers[n_buffers].start)
        {
            fprintf(stderr, "Out of memory\\n");
            exit(EXIT_FAILURE);
        }
    }
}


void openDevice(void)
{
    printf("\n   /////////////////\n    OPEN DEVICE\n");

    struct stat st;

    if (-1 == stat(dev_name, &st))
    {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode))
    {
        fprintf(stderr, "%s is no devicen", dev_name);
        exit(EXIT_FAILURE);
    }

    fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd)
    {
        fprintf(stderr, "Cannot open '%s': %d, %s\n",
                dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (fd < 0)
    {
        perror("Cannot open device");
        exit(EXIT_FAILURE);
    }
}

int checkDeivce(void)
{
    printf("\n   /////////////////\n    CHECK DEVICE\n");
    struct v4l2_input input;
    int index;

    if (-1 == ioctl(fd, VIDIOC_G_INPUT, &index))
    {
        perror("VIDIOC_G_INPUT");
        exit(EXIT_FAILURE);
    }

    memset(&input, 0, sizeof(input));
    input.index = index;

    if (-1 == ioctl(fd, VIDIOC_ENUMINPUT, &input))
    {
        perror("VIDIOC_ENUMINPUT");
        exit(EXIT_FAILURE);
    }

    printf("Current input: %s\n", input.name); //Current input: Camera 0
    return input.index;
}

void selectDevice(int index)
{
    printf("\n   /////////////////\n    SELECT DEVICE\n");

    if (-1 == ioctl(fd, VIDIOC_S_INPUT, &index))
    {
        perror("VIDIOC_S_INPUT");
        exit(EXIT_FAILURE);
    }
}

void closeDevice(void) // close fd
{
    printf("\n   /////////////////\n    CLOSE DEVICE\n");

    if (-1 == close(fd))
        errno_exit("close");

    fd = -1;
}

void initDevice(void)
{
    printf("\n   /////////////////\n    INIT DEVICE\n");

    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) // check camera's property, read it
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s is no V4L2 device\\n",
                    dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) // check device is video device or not
    {
        fprintf(stderr, "%s is no video capture device\\n",
                dev_name);
        exit(EXIT_FAILURE);
    }

    switch (io) // different mode have different operation
    {
    case IO_METHOD_READ:
        if (!(cap.capabilities & V4L2_CAP_READWRITE)) // IO_Method read, check if read&write io is ok
        {
            fprintf(stderr, "%s does not support read i/o\\n",
                    dev_name);
            exit(EXIT_FAILURE);
        }
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        if (!(cap.capabilities & V4L2_CAP_STREAMING))
        {
            fprintf(stderr, "%s does not support streaming i/o\\n",
                    dev_name);
            exit(EXIT_FAILURE);
        }
        break;
    }

    /* Select video input, video standard and tune here. */

    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // set the type of buffer for transforming video

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */ // means setting the size crop of image?

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop))
        {
            switch (errno)
            {
            case EINVAL:
                /* Cropping not supported. */
                break;
            default:
                /* Errors ignored. */
                break;
            }
        }
    }
    else
    {
        /* Errors ignored. */
    }

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // format of image
    // printf("\n  force format...%d ", force_format);
    if (force_format)                       // default to 0
    {
        printf("\n  force format... ");
        fmt.fmt.pix.width = frame_width;
        fmt.fmt.pix.height = frame_height;

        // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // yuyv format
        // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24; // RGB format
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY; // GREY format
        fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

        if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
            errno_exit("VIDIOC_S_FMT");

        /* Note VIDIOC_S_FMT may change width and height. */
    }

    /* Preserve original settings as set by v4l2-ctl for example */
    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
        errno_exit("VIDIOC_G_FMT");
    frame_width = fmt.fmt.pix.width;
    frame_height = fmt.fmt.pix.height;

    char fmt_str[8];
    memset(fmt_str,0,8);
    memcpy(fmt_str, &fmt.fmt.pix.pixelformat, 4);
    printf("image format is : %d x %d %s \n ",frame_height, frame_width, fmt_str);

    /* Buggy driver paranoia. */ // wait to understand
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    switch (io) // create the buffer size to store video data
    {
    case IO_METHOD_READ:
        init_read(fmt.fmt.pix.sizeimage);
        break;

    case IO_METHOD_MMAP:
        init_mmap();
        break;

    case IO_METHOD_USERPTR:
        init_userp(fmt.fmt.pix.sizeimage);
        break;
    }
}

void uninitDevice(void) // release data buffer space
{
    printf("\n   /////////////////\n    UNINIT DEVICE\n");

    unsigned int i;

    switch (io)
    {
    case IO_METHOD_READ:
        free(buffers[0].start);
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i)
            if (-1 == munmap(buffers[i].start, buffers[i].length))
                errno_exit("munmap");
        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i)
            free(buffers[i].start);
        break;
    }

    free(buffers);
}


void startCapturing(void) // make it start stream iamge data
{
    printf("\n   /////////////////\n    START CAPTURING\n");

    unsigned int i;
    enum v4l2_buf_type type;

    switch (io)
    {
    case IO_METHOD_READ:
        break;

/////////////////////////////////////////////////////////////////////
    case IO_METHOD_MMAP:                // default mode
        for (i = 0; i < n_buffers; ++i) // for each buffer container
        {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) // VIDIOC_QBUF means create a buffer for driver to cache video data, with index i
                errno_exit("VIDIOC_QBUF");
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) // start to stream video?(start cache the image data into buffer)
            errno_exit("VIDIOC_STREAMON");
        break;
/////////////////////////////////////////////////////////////////////

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i)
        {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index = i;
            buf.m.userptr = (unsigned long)buffers[i].start;
            buf.length = buffers[i].length;

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");
        break;
    }
}


void setRegExtMode(void)
{
	struct reg regs_trigger[] = {
		{0x4F00, 0x01}, // low power mode, external mode needed
		{0x3030, 0x04}, // external mode
		{0x303F, 0x01}, // captured num frame(s) when trigger arrives
		{0x302C, 0x00}, // confusing sleep period nums
		{0x302F, 0x7F}, // confusing sleep period nums 2
		{0x3823, 0x30}, // external timing setting
		{0x0100, 0x00}, // PLL model 0:software standby or 1:streaming
	};
	struct reg regs_stream[] = {
		{0x4F00, 0x00},
		{0x3030, 0x00},
		{0x303F, 0x00},
		{0x302C, 0x00},
		{0x302F, 0x7F},
		// {0x302C, 0x00},
		// {0x302F, 0x00},
		{0x3823, 0x08},
		{0x0100, 0x01},
	};
	struct reg regs_GainExpose[] = {
		{0x3503, 0x03}, // manual control of exposure and gain -- 0x08
		{0x3509, 0x60}, // manual control of exposure and gain -- 0x08
        {0x350A, 0x04}, // digital gain num [7:0]-->[13:6]     -- 0x04
        {0x350B, 0x00}, // digital gain num [5:0]-->[5:0]      -- 0x00
        {0x3500, 0x00}, // long exposure [3:0]-->[19:16]       -- 0x00
        {0x3501, 0x2c}, // long exposure [7:0]-->[15:8]        -- 0x2c
        {0x3502, 0x10}, // long exposure [7:4]-->[3:0]  [7:4]-->fraction bits[3:0] -- 0x10   
	};

    if (ext_trg_enable)
    {
        printf("Trigger mode enabled, writing iic register.\n");
        i2cReadWrite(&regs_trigger, sizeof(regs_trigger)/sizeof(struct reg), I2C_WRITE_MODE);
        i2cReadWrite(&regs_trigger, sizeof(regs_trigger)/sizeof(struct reg), I2C_READ_MODE);
    }
    else if(vid_stream_enable)
    {
        printf("Stream mode enabled, writing iic register.\n");
        i2cReadWrite(&regs_stream, sizeof(regs_stream)/sizeof(struct reg), I2C_WRITE_MODE);
        i2cReadWrite(&regs_stream, sizeof(regs_stream)/sizeof(struct reg), I2C_READ_MODE);
    }
    
    i2cReadWrite(&regs_GainExpose, sizeof(regs_GainExpose)/sizeof(struct reg), I2C_WRITE_MODE);
    i2cReadWrite(&regs_GainExpose, sizeof(regs_GainExpose)/sizeof(struct reg), I2C_READ_MODE);
    usleep(10);
        // i2cReadWrite(&regs_trigger, sizeof(regs_trigger)/sizeof(struct reg), I2C_READ_MODE);

}

void stopCapturing(void) // stop the driver from streaming data
{

    printf("\n   /////////////////\n    STOP CAPTURING\n");

    enum v4l2_buf_type type;

    switch (io)
    {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
            errno_exit("VIDIOC_STREAMOFF");
        break;
    }
}