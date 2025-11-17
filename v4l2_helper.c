#include <stdio.h>
#include <fcntl.h>      // for open()
#include <sys/ioctl.h>  // for ioctl()
#include <inttypes.h>   // for uint32_t

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

int initDevice(char* dev)
{
    int fd = open(dev, O_RDWR /* required */ | O_NONBLOCK, 0);
    if(fd == -1)
    {
        return -1;
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

    // device_caps
    printf("\n");
    return fd;
}
