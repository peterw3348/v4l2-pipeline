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


    close(captureDevice);
    close(outputDevice);
    return 0;
}

