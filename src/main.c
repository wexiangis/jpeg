#include <stdio.h>

#if 1
#include "jpeg.h"
#include "jpeg_format.h"

int main(int argc, char **argv)
{
    printf("JF_APP0: %ld \r\n", sizeof(JF_APP0));
    printf("JF_SOF0: %ld \r\n", sizeof(JF_SOF0));
    printf("JF_SOS: %ld \r\n", sizeof(JF_SOS));
    jpeg_info(argv[1]);
    return 0;
}

#else
#include "fbmap.h"

int main(int argc, char **argv)
{
    FbMap_Struct *fs;
    int c = 0;

#if 0
    if ((fs = fbmap_open("./t.txt", FMT_NEW, 4)))
    {
        fs->mem[c++] = '1';
        fs->mem[c++] = '2';
        fs->mem[c++] = '3';
        fs->mem[c++] = '4';
    }
#elif 1
    if ((fs = fbmap_open("./t.txt", FMT_RW, 0)))
    {
        fs->mem[c++] = 'a';
        fs->mem[c++] = 'b';
        fs->mem[c++] = 'c';
        fs->mem[c++] = 'd';
    }
#elif 1
    if ((fs = fbmap_open("./t.txt", FMT_NEW, 2)))
    {
        fs->mem[c++] = 't';
        fs->mem[c++] = 'g';
    }
#endif

    if (fs)
        fbmap_close(fs);

    return 0;
}
#endif