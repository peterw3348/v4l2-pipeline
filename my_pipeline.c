#include <stdio.h>

int main(int argc,char* argv[])
{
    if(argc < 3)
    {
        printf("%s <capture device> <outputdevice>\n", argv[0]);
    }
    return 0;
}
