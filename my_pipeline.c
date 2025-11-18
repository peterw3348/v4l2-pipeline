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

    // Buffer negotiation -> mmap() -> VIDIOC_STREAMON
    // 2-4 buffers each for capture and output, at least 2 to stop hardware stalls
    // Size decided by format negotiation above e.g. 1280/720 'MJPG' ~ 1.2 MB 
    struct v4l2_requestbuffers captureBuffers = {0};
    captureBuffers.count = 4;
    captureBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    captureBuffers.memory = V4L2_MEMORY_MMAP; // We will mmap() the memory later
    if(-1 == ioctl(captureDevice, VIDIOC_REQBUFS, &captureBuffers))
    {
        errno_exit("VIDIOC_REQBUFS");
    }
    printf("capture buffers requested\n");


    struct v4l2_requestbuffers outputBuffers = {0};
    outputBuffers.count = 4;
    outputBuffers.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    outputBuffers.memory = V4L2_MEMORY_MMAP;
    if(-1 == ioctl(outputDevice, VIDIOC_REQBUFS, &outputBuffers))
    {
        errno_exit("VIDIOC_REQBUFS");
    }
    printf("output buffers requested\n");


    // Cleanup opposite direction as above
    // VIDIOC_STREAMOFF -> mumap() -> buffer free -> close device
    captureBuffers.count = 0;
    captureBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    captureBuffers.memory = V4L2_MEMORY_MMAP; // We will mmap() the memory later
    if(-1 == ioctl(captureDevice, VIDIOC_REQBUFS, &captureBuffers))
    {
        errno_exit("VIDIOC_REQBUFS");
    }
    printf("capture buffers freed\n");

    outputBuffers.count = 0;
    outputBuffers.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    outputBuffers.memory = V4L2_MEMORY_MMAP; // We will mmap() the memory later
    if(-1 == ioctl(outputDevice, VIDIOC_REQBUFS, &outputBuffers))
    {
        errno_exit("VIDIOC_REQBUFS");
    }
    printf("output buffers freed\n");

    close(captureDevice);
    close(outputDevice);
    return 0;
}

