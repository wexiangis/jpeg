#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "fbmap.h"

/*
 *  对文件进行内存映射
 *  参数:
 *      file: 目标文件
 *      type: 读写权限类型
 *      size: type=FMT_NEW 时传入创建文件内存大小,其它时候无效
 *  返回: 返回结构体指针,失败NULL
 */
FbMap_Struct *fbmap_open(char *file, FbMap_Type type, int size)
{
    FbMap_Struct *fs;
    unsigned char *mem;
    int fd;
    int prot;
    struct stat info;

    int bytesCount;
    unsigned char bytes = 0;

    // 参数检查
    if (!file)
    {
        fprintf(stderr, "fbmap_open: param error !! \r\n");
        return NULL;
    }

    // 打开文件
    switch (type)
    {
    case FMT_R:
        prot = PROT_READ;
        fd = open(file, O_RDONLY);
        break;
    case FMT_RW:
        prot = PROT_READ | PROT_WRITE;
        fd = open(file, O_RDWR);
        break;
    case FMT_NEW:
        prot = PROT_READ | PROT_WRITE;
        fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0666);
        // 创建文件时,指定文件大小
        if (size < 1)
            size = 1;
        // 拓宽文件到目标大小
        if (fd > 0)
        {
            for (bytesCount = 0; bytesCount < size; bytesCount++)
                write(fd, &bytes, 1);
            // 指针位置恢复到文件头
            lseek(fd, 0, SEEK_CUR);
        }
        break;
    default:
        fd = -1;
        break;
    }

    // 检查
    if (fd < 1)
    {
        fprintf(stderr, "fbmap_open: can't open %s \r\n", file);
        return NULL;
    }

    // 检查文件大小
    fstat(fd, &info);
    size = info.st_size;
    // printf("file size: %d \r\n", size);

    // 内存映射(第一个0表示不指定内存地址,最后的0表示从文件起始地址开始映射)
    mem = (unsigned char *)mmap(0, size, prot, MAP_SHARED, fd, 0);
    if (!mem)
    {
        fprintf(stderr, "fbmap_open: mmap failed \r\n");
        close(fd);
        return NULL;
    }

    // 返回参数
    fs = (FbMap_Struct *)calloc(1, sizeof(FbMap_Struct));
    fs->mem = mem;
    fs->fd = fd;
    fs->size = size;
    fs->type = type;

    return fs;
}

/*
 *  关闭内存映射和文件
 */
void fbmap_close(FbMap_Struct *fs)
{
    if (fs)
    {
        if (fs->mem)
            munmap(fs->mem, fs->size);
        if (fs->fd > 0)
            close(fs->fd);
        free(fs);
    }
}
