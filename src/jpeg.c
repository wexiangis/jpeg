#include <stdio.h>
#include <stdint.h>

#include "jpeg_format.h"
#include "fbmap.h"

// 常用判断标志
typedef enum
{
    // 起始和结束
    JT_SOI = 0xFFD8,
    JT_EOI = 0xFFD9,
    // 扫描开始
    JT_SOS = 0xFFDA,
    // 未知
    JT_RST0 = 0xFFD0,
    JT_RSTn = 0xFFD7,
    // 文件信息, 注意 0xFFE1~0xFFEF 包格式和 0xFFE0 的不同
    JT_APP0 = 0xFFE0,
    JT_APPn = 0xFFEF,
    // 量化表
    JT_DQT = 0xFFDB,
    // 帧图像开始
    JT_SOF0 = 0xFFC0,
    // 哈夫曼表
    JT_DHT = 0xFFC4,
    // 差分编码累计复位的间隔
    JT_DRI = 0xFFDD,
    // 注释
    JT_COM = 0xFFFE,
} J_Type;

typedef struct
{
    uint16_t type;   // 当前包类型
    uint8_t *pkg;    // 当前包数据区起始地址(不包含长度2字节)
    uint16_t pkgLen; // 数据区长度(已减去长度2字节)
    uint8_t *next;   // 下一包地址

    uint8_t *begin; // 整图内存起始
    uint8_t *end;   // 整图内存结束(注意 end - begin = 整图大小)

    uint32_t dataLen;
} Jpeg_Pkg;

/*
 *  从压缩数据中找包
 *  返回: 0/有效 其它/异常
 */
int jpeg_getFrameFromData(Jpeg_Pkg *jp)
{
    // 特殊数据 0xFF 处理
    // 找第一个 0xFF
    while (jp->next < jp->end)
    {
        jp->dataLen++;
        if (*jp->next++ == 0xFF)
        {
            // printf("0xFF%02X \r\n", *jp->next);

            // 判断第二个数据
            while (jp->next < jp->end)
            {
                // 去除该0x00并保留0xFF作为译码数据
                if (*jp->next == 0x00)
                {
                    jp->next++;
                    break;
                }
                // 忽略该字节继续判断下一个
                else if (*jp->next == 0xFF)
                {
                    jp->next++;
                    continue;
                }
                // 包 JT_EOI, JT_RST0~n
                else if (*jp->next == 0xD9 ||
                         (*jp->next > 0xCF &&
                          *jp->next < 0xD8))
                {
                    jp->dataLen--;
                    jp->next -= 1;
                    return 0;
                }
                // 丢弃前面的0xFF继续译码
                else
                {
                    jp->next++;
                    break;
                }
            }
        }
    }
    // 直至遍历完文件,都没有有效包(这是异常的,至少应有"结束包EOI")
    return -1;
}

/*
 *  获取下一帧
 *  返回: 0/有效 其它/异常
 */
int jpeg_getFrame(Jpeg_Pkg *jp)
{
    // 上一包为 JT_SOS 或者 JT_RST0~n 时作特殊判断
    if (jp->type != JT_SOS && (jp->type < JT_RST0 || jp->type > JT_RSTn))
        ;
    else if (jpeg_getFrameFromData(jp))
        return -1;

    // 取得包类型2字节
    jp->type = *jp->next++;
    jp->type <<= 8;
    jp->type |= *jp->next++;

    // 无效包检查
    if (jp->type < 0xFF01 || jp->type > 0xFFFE)
        return -1;

    // 头尾包无长度和内容信息
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
    }

    return 0;
}

void jpeg_info(char *file)
{
    Jpeg_Pkg jp = {.dataLen = 0};
    JF_File jff;
    FbMap_Struct *fs = fbmap_open(file, FMT_R, 0);

    if (!fs)
        return;

    jp.begin = fs->mem;
    jp.end = fs->mem + fs->size;
    jp.next = fs->mem;

    while (jpeg_getFrame(&jp) == 0)
    {
        switch (jp.type)
        {
            case JT_APP0:
            {
                jff.app0 = (JF_APP0 *)jp.pkg;
                printf("type %04X, len %d, tag %s, ver %d.%d, density %d %d %d, short_xy %dx%d\r\n",
                    jp.type, jp.pkgLen,
                    jff.app0->tag, 
                    jff.app0->ver_h, jff.app0->ver_l, 
                    jff.app0->density, jff.app0->density_x, jff.app0->density_y,
                    jff.app0->short_x, jff.app0->short_y);
                break;
            }
            case JT_DQT:
            {
                jff.dqt = (JF_DQT *)jp.pkg;
                printf("type %04X, len %d, bit %d, id %d\r\n",
                    jp.type, jp.pkgLen,
                    jff.dqt->bit, jff.dqt->id);
                break;
            }
            case JT_SOF0:
            {
                jff.sof0 = (JF_SOF0 *)jp.pkg;
                printf("type %04X, len %d, bit %d, WxH %dx%d, color %d, %d %d %d, %d %d %d, %d %d %d\r\n",
                    jp.type, jp.pkgLen,
                    jff.sof0->bit,
                    jff.sof0->width_h * 256 + jff.sof0->width_l,
                    jff.sof0->height_h * 256 + jff.sof0->height_l,
                    jff.sof0->color,
                    jff.sof0->color_info[0].id,
                    jff.sof0->color_info[0].factors,
                    jff.sof0->color_info[0].dqt,
                    jff.sof0->color_info[1].id,
                    jff.sof0->color_info[1].factors,
                    jff.sof0->color_info[1].dqt,
                    jff.sof0->color_info[2].id,
                    jff.sof0->color_info[2].factors,
                    jff.sof0->color_info[2].dqt);
                break;
            }
            case JT_DHT:
            {
                jff.dht = (JF_DHT *)jp.pkg;
                printf("type %04X, len %d, type %d, id %d\r\n",
                    jp.type, jp.pkgLen,
                    jff.dht->type, jff.dht->id);
                break;
            }
            case JT_DRI:
            {
                jff.dri = (JF_DRI *)jp.pkg;
                printf("type %04X, len %d, interval %d\r\n",
                    jp.type, jp.pkgLen,
                    jff.dri->interval);
                break;
            }
            case JT_SOS:
            {
                jff.sos = (JF_SOS *)jp.pkg;
                printf("type %04X, len %d, color %d\r\n",
                    jp.type, jp.pkgLen,
                    jff.sos->color);
                break;
            }
            case JT_COM:
            {
                printf("type %04X, len %d %.*s\r\n", jp.type, jp.pkgLen, jp.pkgLen - 1, jp.pkg);
                break;
            }
            case JT_EOI:
            {
                printf("type %04X, len %d, data %d \r\n", jp.type, jp.pkgLen, jp.dataLen);
                break;
            }
            default:
            {
                if (jp.type > JT_APP0 && jp.type <= JT_APPn)
                    printf("type %04X, len %d %.*s\r\n", jp.type, jp.pkgLen, jp.pkgLen - 1, jp.pkg);
                else
                    printf("type %04X, len %d \r\n", jp.type, jp.pkgLen);
                break;
            }
        }
        if (jp.type == JT_EOI)
            break;
    }

    fbmap_close(fs);
}
