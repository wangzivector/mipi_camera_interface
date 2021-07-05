#include "camera_control.h"

void enumerateMenu(unsigned long id)
{

    printf("Menu items, querymenu.name: \n");

    memset(&querymenu, 0, sizeof(querymenu));
    querymenu.id = id;

    for (querymenu.index = queryctrl.minimum;
         querymenu.index <= queryctrl.maximum;
         querymenu.index++)
    {
        if (0 == ioctl(fd, VIDIOC_QUERYMENU, &querymenu))
        {
            // printf("querymenu.name %s,  with querymenu.index: %d \n", querymenu.name, querymenu.index);
            printf(" %s ", querymenu.name);
        }
    }
}

/*
/// enumerationg all user function in old style
//  which implement all the possible user control function

memset(&queryctrl, 0, sizeof(queryctrl));

for (queryctrl.id = V4L2_CID_BASE;
     queryctrl.id < V4L2_CID_LASTP1;
     queryctrl.id++) {
    if (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) {
        if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            continue;

        printf("Control %s\\n", queryctrl.name);

        if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
            enumerateMenu(queryctrl.id);
    } else {
        if (errno == EINVAL)
            continue;

        perror("VIDIOC_QUERYCTRL");
        exit(EXIT_FAILURE);
    }
}

for (queryctrl.id = V4L2_CID_PRIVATE_BASE;;
     queryctrl.id++) {
    if (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) {
        if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            continue;

        printf("Control %s\\n", queryctrl.name);

        if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
            enumerateMenu(queryctrl.id);
    } else {
        if (errno == EINVAL)
            break;

        perror("VIDIOC_QUERYCTRL");
        exit(EXIT_FAILURE);
    }
}
*/

void enumerateMenuList(void)
{
    printf("\n   /////////////////\n    ENUMERATE MENU DEVICE\n");

    memset(&queryctrl, 0, sizeof(queryctrl));

    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    while (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
    {
        if (!(queryctrl.flags & V4L2_CTRL_FLAG_DISABLED))
        {
            printf("\n\n\n - : %s ", queryctrl.name);
            printf(" -----(min,max)=(%d:%d), step=%d, default= %d\n", 
                queryctrl.minimum, queryctrl.maximum, queryctrl.step, queryctrl.default_value);
            if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
                enumerateMenu(queryctrl.id); // print out all the possible type of queryctrl.name
        }

        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL; // get the next property
    }
    if (errno != EINVAL)
    {
        perror("VIDIOC_QUERYCTRL");
        exit(EXIT_FAILURE);
    }
}
/*
        ===== EXAMPLES OF STDOUT =====
        -----
        Control Horizontal Flip
        -----
        Control Vertical Flip
        -----
        Control Power Line Frequency
        Menu items:
        querymenu.name Disabled,  with querymenu.index: 0 
        querymenu.name 50 Hz,  with querymenu.index: 1 
        querymenu.name 60 Hz,  with querymenu.index: 2 
        querymenu.name Auto,  with querymenu.index: 3 
        -----
        Control Sharpness
        -----
        Control Color Effects
        Menu items:
        querymenu.name None,  with querymenu.index: 0 
        querymenu.name Black & White,  with querymenu.index: 1 
        querymenu.name Sepia,  with querymenu.index: 2 
        querymenu.name Negative,  with querymenu.index: 3 

        */

// different from enumerateMenu_request_all() :: VIDIOC_QUERY_EXT_CTRL vs. VIDIOC_QUERYCTRL
// the result contains contents of enumerateMenu_request_all()
// this function result have little more options.
void enumerateExtMenuList(void)
{
    printf("\n   /////////////////\n    ENUMERATE EXT MENU DEVICE\n");

    memset(&query_ext_ctrl, 0, sizeof(query_ext_ctrl));

    query_ext_ctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
    while (0 == ioctl(fd, VIDIOC_QUERY_EXT_CTRL, &query_ext_ctrl))
    {
        if (!(query_ext_ctrl.flags & V4L2_CTRL_FLAG_DISABLED))
        {
            printf("=====\nVIDIOC_QUERY_EXT_CTRL - Control %s\n", query_ext_ctrl.name);

            if (query_ext_ctrl.type == V4L2_CTRL_TYPE_MENU)
                enumerateMenu(query_ext_ctrl.id);
        }

        query_ext_ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND; // get the next property
    }
    if (errno != EINVAL)
    {
        perror("VIDIOC_QUERY_EXT_CTRL");
        exit(EXIT_FAILURE);
    }
}

void cameraFunctionsControlExample(void)
{

    printf("\n   /////////////////\n    CAMERA FUNCTION CONTROL\n");

    //
    // METHOD 1: a.check permission of function b.set the control id and value
    //
    memset(&queryctrl, 0, sizeof(queryctrl));
    queryctrl.id = V4L2_CID_BRIGHTNESS; // function is brightness

    if (-1 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) // if this function can not be read, then aborted.
    {
        if (errno != EINVAL)
        {
            perror("VIDIOC_QUERYCTRL");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("V4L2_CID_BRIGHTNESS is not supportedn");
        }
    }
    else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) // if have no permission to control, then aborted.
    {
        printf("V4L2_CID_BRIGHTNESS is not supportedn");
    }
    else // if everything is good to set,
    {
        printf(" - Controlling... %s ", queryctrl.name, "- min, max, step and default ... %d, %d: %d\n", 
        queryctrl.minimum, queryctrl.maximum, queryctrl.step, queryctrl.default_value);

        memset(&control, 0, sizeof(control));
        control.id = V4L2_CID_BRIGHTNESS;        // set function item
        control.value = queryctrl.default_value; // set item value

        if (-1 == ioctl(fd, VIDIOC_S_CTRL, &control)) // write and enforce control
        {
            perror("VIDIOC_S_CTRL");
            exit(EXIT_FAILURE);
        }
    }

    // //
    // // MEETHOD 2: just read, set the control id and value
    // //
    // memset(&control, 0, sizeof(control));
    // control.id = V4L2_CID_CONTRAST;

    // if (0 == ioctl(fd, VIDIOC_G_CTRL, &control)) // read and get control value.
    // {
    //     control.value += 1; // add value

    //     /* The driver may clamp the value or return ERANGE, ignored here */

    //     if (-1 == ioctl(fd, VIDIOC_S_CTRL, &control) && errno != ERANGE) // set and enforce control value
    //     {
    //         perror("VIDIOC_S_CTRL");
    //         exit(EXIT_FAILURE);
    //     }
    //     /* Ignore if V4L2_CID_CONTRAST is unsupported */
    // }
    // else if (errno != EINVAL)
    // {
    //     perror("VIDIOC_G_CTRL");
    //     exit(EXIT_FAILURE);
    // }

    // //
    // // METHOD 3: only set control id and value, without checking whether it is vaild
    // //
    // control.id = V4L2_CID_AUDIO_MUTE;
    // control.value = 1; /* silence */

    // /* Errors ignored */
    // ioctl(fd, VIDIOC_S_CTRL, &control);
}

int cameraFunctionsControl(unsigned int fun_id, signed int fun_value)
{

    printf("\n   /////////////////\n    CAMERA FUNCTION CONTROL\n");

    //
    // METHOD 1: a.check permission of function b.set the control id and value
    //
    memset(&queryctrl, 0, sizeof(queryctrl));
    queryctrl.id = fun_id; // function is brightness

    if (-1 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) // if this function can not be read, then aborted.
    {
        if (errno != EINVAL)
        {
            perror("VIDIOC_QUERYCTRL");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("%s", queryctrl.name, " is not supportedn");
            return -1;
        }
    }
    else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) // if have no permission to control, then aborted.
    {
        printf("%s", queryctrl.name," is not supportedn");
        return -1;

    }
    else // if everything is good to set,
    {
        printf(" - Controlling: %s ", queryctrl.name, "- min, max, step, default: %d, %d: %d\n", 
        queryctrl.minimum, queryctrl.maximum, queryctrl.step, queryctrl.default_value);

        memset(&control, 0, sizeof(control));
        control.id = fun_id;        // set function item
        if (fun_value == -1)
            control.value = queryctrl.default_value;
        else
            control.value = fun_value; // set item value

        if (-1 == ioctl(fd, VIDIOC_S_CTRL, &control)) // write and enforce control
        {
            perror("VIDIOC_S_CTRL");
            exit(EXIT_FAILURE);
        }
        printf("set to %d", control.value);
    }

    return 0;
}

// Extended Control API
void enumerateExtendedControl()
{
    printf("\n   /////////////////\n    ENUMERATE EXT CONTROL\n");

    // exactly the same code as enumerateMenuList
    // wait for implementation
    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    while (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
    {
        /* ... */
        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    // // control class with difference
    queryctrl.id = V4L2_CTRL_CLASS_MPEG | V4L2_CTRL_FLAG_NEXT_CTRL;
    while (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
    {
        if (V4L2_CTRL_ID2CLASS(queryctrl.id) != V4L2_CTRL_CLASS_MPEG)
            break;
        /* ... */
        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }
}

