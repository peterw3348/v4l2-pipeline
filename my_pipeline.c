#include <stdio.h>
#include <fcntl.h>      // for open()
#include <unistd.h>     // for close()
#include <sys/ioctl.h>  // for ioctl()

#include <linux/videodev2.h>

#include "v4l2_helper.h"

int main(int argc,char* argv[])
{
    if(argc < 3)
    {
        fprintf(stderr, "%s <capture device> <outputdevice>\n", argv[0]);
        return -1;
    }
    
    int captureDevice = initDevice(argv[1]); // captureDevice
    int outputDevice = initDevice(argv[2]); // outputDevice
    struct v4l2_format captureFormat = {0};
    struct v4l2_format outputFormat = {0};
    
    // Verify device caps -> Negotiating formats between devices
    if(!checkDeviceCap(captureDevice, V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "Error: %s does not have video capture\n", argv[1]);
        return -1;
    }
    else
    {
        // Kernel rejects format with empty type. How do we know type?
        // Since we know device caps, and verify it supports video capture
        // We know it's buffer type as well
        captureFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(-1 == ioctl(captureDevice, VIDIOC_G_FMT, &captureFormat))
        {
            errno_exit("VIDIOC_G_FMT");
        }
    }
    if(!checkDeviceCap(outputDevice, V4L2_CAP_VIDEO_OUTPUT))
    {
        fprintf(stderr, "Error: %s does not have video output\n", argv[2]);
        return -1;
    }
    else
    {
        // Use same format as webcam for loopback
        outputFormat = captureFormat;
        outputFormat.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        if(-1 == ioctl(outputDevice, VIDIOC_S_FMT, &outputFormat))
        {
            errno_exit("VIDIOC_S_FMT");
        }
    }


    close(captureDevice);
    close(outputDevice);
    return 0;
}

