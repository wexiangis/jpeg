#include <stdio.h>
#include <stdint.h>

#include "fbmap.h"

typedef enum
{
    JT_SOI = 0xFFD8,
    JT_EOI = 0xFFD9,
    JT_SOS = 0xFFDA,
} J_Type;

typedef struct
{
    uint16_t type;   // 当前包类型
    uint8_t *pkg;    // 当前包数据区起始地址(不包含长度2字节)
    uint16_t pkgLen; // 数据区长度(已减去长度2字节)
    uint8_t *next;   // 下一包地址

    uint8_t *begin; // 整图内存起始
    uint8_t *end;   // 整图内存结束(注意 end - begin = 整图大小)
} Jpeg_Pkg;

/*
 *  获取下一帧
 */
int jpeg_getFrame(Jpeg_Pkg *jp)
{
    jp->type = *jp->next++;
    jp->type <<= 8;
    jp->type |= *jp->next++;

    if (jp->type < 0xFF01 || jp->type > 0xFFFE)
        return -1;

    if (jp->type == JT_SOI || jp->type == JT_EOI)
    {
        jp->pkgLen = 0;
        jp->pkg = NULL;
    }
    else
    {
        // 包长度截取
        jp->pkgLen = *jp->next++;
        jp->pkgLen <<= 8;
        jp->pkgLen += *jp->next++;
        // 减去长度2字节
        jp->pkgLen -= 2;
        // 包数据起始位置
        jp->pkg = jp->next;
        // 下一包起始位置
        jp->next += jp->pkgLen;

        // SOS包开始后直至结束
        // if (jp->type == JT_SOS)
        //     jp->next = jp->end - 2;
    }

    return 0;
}

void jpeg_info(char *file)
{
    Jpeg_Pkg jp;
    FbMap_Struct *fs = fbmap_open(file, FMT_R, 0);

    if (!fs)
        return;

    jp.begin = fs->mem;
    jp.end = fs->mem + fs->size;
    jp.next = fs->mem;

    while (jpeg_getFrame(&jp) == 0)
    {
        printf("type %04X, len %d \r\n", jp.type, jp.pkgLen);
        if (jp.type == JT_EOI)
            break;
    }

    fbmap_close(fs);
}
