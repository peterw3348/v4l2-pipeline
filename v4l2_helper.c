#include <stdio.h>
#include <fcntl.h>      // for open()
#include <sys/ioctl.h>  // for ioctl()
#include <inttypes.h>   // for uint32_t
#include <errno.h>      // for errno
#include <stdlib.h>     // for EXIT_FAILURE
#include <string.h>     // for strerror(), memset()
#include <stdbool.h>    // for bool

#include <linux/videodev2.h>

#include "v4l2_helper.h"

struct BitToCapName
{
    uint32_t bit;
    const char * name;
};

static const struct BitToCapName capTable[] = 
{
    {V4L2_CAP_VIDEO_CAPTURE, "VIDEO CAPTURE"},
    {V4L2_CAP_VIDEO_OUTPUT, "VIDEO OUTPUT"},
    {V4L2_CAP_VIDEO_OVERLAY, "VIDEO OVERLAY"},
    {V4L2_CAP_VBI_CAPTURE, "VBI CAPTURE"},
    {V4L2_CAP_VBI_OUTPUT, "VBI OUTPUT"},
    {V4L2_CAP_SLICED_VBI_CAPTURE, "SLICED VBI CAPTURE"},
    {V4L2_CAP_SLICED_VBI_OUTPUT, "SLICED VBI OUTPUT"},
    {V4L2_CAP_RDS_CAPTURE, "RDS CAPTURE"},
    {V4L2_CAP_VIDEO_OUTPUT_OVERLAY, "VIDEO OUTPUT OVERLAY"},
    {V4L2_CAP_HW_FREQ_SEEK, "HW FREQ SEEK"},
    {V4L2_CAP_RDS_OUTPUT, "RDS OUTPUT"},
    {V4L2_CAP_VIDEO_CAPTURE_MPLANE, "VIDEO_CAPTURE_MPLANE"},
    {V4L2_CAP_VIDEO_OUTPUT_MPLANE, "VIDEO_OUTPUT_MPLANE"},
    {V4L2_CAP_VIDEO_M2M_MPLANE, "VIDEO_M2M_MPLANE"},
    {V4L2_CAP_VIDEO_M2M, "VIDEO_M2M"}
};

void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

// Open device, Verify device caps
int initDevice(char* dev_node, uint32_t device_cap, Device * device)
{
    // fd, buftype
    printf("init %s\n", dev_node);
    int initStatus = INIT_SUCCESS;
    *device = (typeof(*device)){0};
    
    device->fd = open(dev_node, O_RDWR /* required */ | O_NONBLOCK, 0);
    if(device->fd == -1)
    {
        errno_exit("OPEN");
    }
    printf("%s opened as %d\n", dev_node, device->fd);

    struct v4l2_capability caps;
    ioctl(device->fd, VIDIOC_QUERYCAP, &caps);
    
    printf("Device Caps:\n");
    initStatus = INIT_UNSUPPORTED;
    for(int i = 0; i < sizeof(capTable)/sizeof(struct BitToCapName); ++i)
    {
        if(capTable[i].bit & caps.device_caps) // check supported device_caps
        {
            // dev_cap matches
            printf("%s\n", capTable[i].name);
            if(capTable[i].bit == device_cap) // check if we can configure device
            {
                switch(device_cap)
                {
                    case V4L2_CAP_VIDEO_CAPTURE:
                        device->buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        initStatus = INIT_SUCCESS;
                        break;
                    case V4L2_CAP_VIDEO_OUTPUT:
                        device->buftype = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                        initStatus = INIT_SUCCESS;
                        break;
                    default: // unimplemented
                        initStatus = INIT_FAILURE;
                        break;
                }
            }
        }
    }
    if(device->buftype)
    {
        // Kernel rejects format with empty type. How do we know type?
        // If we know the device caps, we will also know supported
        // buf types
        device->format.type = device->buftype;
    }
    return initStatus;
}

bool checkDeviceCap(int fd, uint32_t device_caps)
{
    struct v4l2_capability caps;
    if(-1 == ioctl(fd, VIDIOC_QUERYCAP, &caps))
    {
        errno_exit("VIDIOC_QUERYCAP");
    }
    return device_caps & caps.device_caps;
}

void reqBuf(Device * dev, int count)
{
    struct v4l2_requestbuffers buf = {0};
    buf.count = count;
    buf.type = dev->buftype;
    buf.memory = dev->memtype; // We will mmap() the memory later
    if(-1 == ioctl(dev->fd, VIDIOC_REQBUFS, &buf))
    {
        errno_exit("VIDIOC_REQBUFS");
    }
}

void init_err(int status)
{
    switch(status)
    {
        case INIT_UNSUPPORTED:
            fprintf(stderr, "ERROR: Device does not have provided capability");
        case INIT_FAILURE:
            fprintf(stderr, "ERROR: Init");
    }
}

