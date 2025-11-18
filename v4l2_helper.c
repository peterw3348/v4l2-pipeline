#include <stdio.h>
#include <fcntl.h>      // for open()
#include <sys/ioctl.h>  // for ioctl()
#include <inttypes.h>   // for uint32_t
#include <errno.h>      // for errno
#include <stdlib.h>     // for EXIT_FAILURE
#include <string.h>     // for strerror()

#include <linux/videodev2.h>

#include "v4l2_helper.h"

struct bitToCapName
{
    uint32_t bit;
    const char * name;
};


static const struct bitToCapName capTable[] = 
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

int initDevice(char* dev)
{
    printf("init %s\n", dev);
    int fd = open(dev, O_RDWR /* required */ | O_NONBLOCK, 0);
    if(fd == -1)
    {
        errno_exit("OPEN");
    }
    printf("%s opened as %d\n", dev, fd);

    struct v4l2_capability caps;
    ioctl(fd, VIDIOC_QUERYCAP, &caps);

    printf("Device Caps:\n");
    for(int i = 0; i < sizeof(capTable)/sizeof(struct bitToCapName); ++i)
    {
        if(capTable[i].bit & caps.device_caps)
        {
            printf("%s\n", capTable[i].name);
        }
    }
    return fd;
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
