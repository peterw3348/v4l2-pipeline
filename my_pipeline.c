#include <stdio.h>
#include <fcntl.h>      // for open()
#include <unistd.h>     // for close()
#include <sys/ioctl.h>  // for ioctl()

#include <linux/videodev2.h>

#include "v4l2_helper.h"

// V4L2 demo using integrated webcam, v4l2 loopback and mmap() buffers

int main(int argc,char* argv[])
{
    if(argc < 3)
    {
        fprintf(stderr, "%s <capture device> <outputDevice.fd>\n", argv[0]);
        return -1;
    }
    
    Device captureDevice = {0};
    Device outputDevice = {0};
    int status;
    if((status = initDevice(argv[1], V4L2_CAP_VIDEO_CAPTURE, &captureDevice)) != INIT_SUCCESS)
    {
        init_err(status);
        return -1;
    }
    if((status = initDevice(argv[2], V4L2_CAP_VIDEO_OUTPUT, &outputDevice)) != INIT_SUCCESS)
    {
        init_err(status);
        return -1;
    }
    
    //  Negotiating formats between devices
    captureDevice.memtype = V4L2_MEMORY_MMAP;
    outputDevice.memtype = V4L2_MEMORY_MMAP;
    if(-1 == ioctl(captureDevice.fd, VIDIOC_G_FMT, &captureDevice.format))
    {
        errno_exit("VIDIOC_G_FMT");
    }
    outputDevice.format = captureDevice.format;
    outputDevice.format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT; // Change type
    if(-1 == ioctl(outputDevice.fd, VIDIOC_S_FMT, &outputDevice.format))
    {
        errno_exit("VIDIOC_S_FMT");
    }
    
    // Buffer negotiation -> mmap() -> VIDIOC_STREAMON
    // 2-4 buffers each for capture and output, at least 2 to stop hardware stalls
    // Size decided by format negotiation above e.g. 1280/720 'MJPG' ~ 1.2 MB 

    reqBuf(&captureDevice, 4);
    printf("capture buffers requested\n");


    reqBuf(&outputDevice, 4);
    printf("output buffers requested\n");


    // Cleanup opposite direction as above
    // VIDIOC_STREAMOFF -> mumap() -> buffer free -> close device
    reqBuf(&captureDevice, 0);
    printf("capture buffers freed\n");

    reqBuf(&outputDevice, 0);
    printf("output buffers freed\n");

    close(captureDevice.fd);
    close(outputDevice.fd);
    return 0;
}

