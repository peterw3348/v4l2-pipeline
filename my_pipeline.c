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
    printf("init captureDevice\n");
    int captureDevice = initDevice(argv[1]); // capture device
    printf("init outputDevice\n");
    int outputDevice = initDevice(argv[2]); // outputDevice
    
    if(!checkDeviceCap(captureDevice, V4L2_CAP_VIDEO_CAPTURE))
    {
        printf("Error: %s does not have video capture", argv[1]);
        return -1;
    }
    if(!checkDeviceCap(outputDevice, V4L2_CAP_VIDEO_OUTPUT))
    {
        printf("Error: %s does not have video output", argv[2]);
        return -1;
    }

    close(captureDevice);
    close(outputDevice);
    return 0;
}

