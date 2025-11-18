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
        printf("%s <capture device> <outputdevice>\n", argv[0]);
        return -1;
    }
    
    int captureDevice = initDevice(argv[1]); // capture device
    int outputDevice = initDevice(argv[2]); // outputDevice
    if(captureDevice == -1 || outputDevice == -1)
    {
        fprintf(stderr, "Error: A device was not init successfully\n");
        return -1;
    }

    if(!checkDeviceCap(captureDevice, V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "Error: %s does not have video capture", argv[1]);
        return -1;
    }
    if(!checkDeviceCap(outputDevice, V4L2_CAP_VIDEO_OUTPUT))
    {
        fprintf(stderr, "Error: %s does not have video output", argv[2]);
        return -1;
    }

    close(captureDevice);
    close(outputDevice);
    return 0;
}

