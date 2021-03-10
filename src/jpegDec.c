#include <stdio.h>
#include <stdint.h>

#include "jpegFormat.h"
#include "fbmap.h"

typedef struct
{
    JF_APP0 *app0;
    JF_DQT *dqt;
    JF_SOF0 *sof0;
    JF_DHT_DC *dht_y_dc;
    JF_DHT_AC *dht_y_ac;
    JF_DHT_DC *dht_uv_dc;
    JF_DHT_AC *dht_uv_ac;
    JF_DRI *dri;
    JF_SOS *sos;
} JF_File;

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
    int i, sum;
    uint8_t *pTmp;

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
            case JT_SOI:
            {
                printf("type %04X SOI len %d\r\n", jp.type, jp.pkgLen);
                break;
            }
            case JT_APP0:
            {
                jff.app0 = (JF_APP0 *)jp.pkg;
                printf("type %04X APP0 len %d, tag %s, ver %d.%d, density %d %d %d, short_xy %dx%d\r\n",
                    jp.type, jp.pkgLen,
                    jff.app0->tag, 
                    jff.app0->ver_h, jff.app0->ver_l, 
                    jff.app0->density, jff.app0->density_x, jff.app0->density_y,
                    jff.app0->short_x, jff.app0->short_y);
                break;
            }
            case JT_DQT:
            {
                printf("type %04X DQT len %d -- 量化表ID-位数 ", jp.type, jp.pkgLen);
                pTmp = jp.pkg;
                do {
                    jff.dqt = (JF_DQT *)pTmp;
                    printf("%d-%d ", jff.dqt->id, jff.dqt->bit ? 16 : 8);
                    pTmp += 64 * (jff.dqt->bit + 1) + 1;
                } while(pTmp < jp.next);
                printf("\r\n");
                jff.dqt = (JF_DQT *)jp.pkg;
                break;
            }
            case JT_SOF0:
            {
                jff.sof0 = (JF_SOF0 *)jp.pkg;
#if 0
                jff.sof0->height_h = (uint8_t)(2160 >> 8);
                jff.sof0->height_l = (uint8_t)(2160 & 0xFF);
                jff.sof0->width_h = (uint8_t)(3840 >> 8);
                jff.sof0->width_l = (uint8_t)(3840 & 0xFF);
    
                jff.sof0->color_info[0].factor_x = 4;
                jff.sof0->color_info[0].factor_y = 4;
                jff.sof0->color_info[1].factor_x = 2;
                jff.sof0->color_info[1].factor_y = 2;
                jff.sof0->color_info[2].factor_x = 2;
                jff.sof0->color_info[2].factor_y = 2;
#elif 0
                jff.sof0->height_h = (uint8_t)(1080 >> 8);
                jff.sof0->height_l = (uint8_t)(1080 & 0xFF);
                jff.sof0->width_h = (uint8_t)(1920 >> 8);
                jff.sof0->width_l = (uint8_t)(1920 & 0xFF);
    
                jff.sof0->color_info[0].factor_x = 2;
                jff.sof0->color_info[0].factor_y = 2;
                jff.sof0->color_info[1].factor_x = 1;
                jff.sof0->color_info[1].factor_y = 1;
                jff.sof0->color_info[2].factor_x = 1;
                jff.sof0->color_info[2].factor_y = 1;
#elif 0
                jff.sof0->height_h = (uint8_t)(1080 >> 8);
                jff.sof0->height_l = (uint8_t)(1080 & 0xFF);
                jff.sof0->width_h = (uint8_t)(1920 >> 8);
                jff.sof0->width_l = (uint8_t)(1920 & 0xFF);
    
                jff.sof0->color_info[0].factor_x = 2;
                jff.sof0->color_info[0].factor_y = 2;
                jff.sof0->color_info[1].factor_x = 1;
                jff.sof0->color_info[1].factor_y = 1;
                jff.sof0->color_info[2].factor_x = 1;
                jff.sof0->color_info[2].factor_y = 1;
#endif

                printf("type %04X SOF0 len %d, bit %d, WxH %dx%d, color %d -- "
                    "YUV序号-XY采样比例-量化表ID: %d-%dx%d-%d, %d-%dx%d-%d, %d-%dx%d-%d\r\n",
                    jp.type, jp.pkgLen,
                    jff.sof0->bit,
                    jff.sof0->width_h * 256 + jff.sof0->width_l,
                    jff.sof0->height_h * 256 + jff.sof0->height_l,
                    jff.sof0->color,
                    jff.sof0->color_info[0].id,
                    jff.sof0->color_info[0].factor_x,
                    jff.sof0->color_info[0].factor_y,
                    jff.sof0->color_info[0].dqt,
                    jff.sof0->color_info[1].id,
                    jff.sof0->color_info[1].factor_x,
                    jff.sof0->color_info[1].factor_y,
                    jff.sof0->color_info[1].dqt,
                    jff.sof0->color_info[2].id,
                    jff.sof0->color_info[2].factor_x,
                    jff.sof0->color_info[2].factor_y,
                    jff.sof0->color_info[2].dqt);
                break;
            }
            case JT_DHT:
            {
                printf("type %04X DHT len %d -- 哈夫曼表DC/AC:ID-长度", jp.type, jp.pkgLen);
                pTmp = jp.pkg;
                do {
                    //对号入座
                    if (pTmp[0] == 0x00)
                        jff.dht_y_dc = (JF_DHT_DC *)pTmp;
                    else if (pTmp[0] == 0x10)
                        jff.dht_y_ac = (JF_DHT_AC *)pTmp;
                    else if (pTmp[0] == 0x01)
                        jff.dht_uv_dc = (JF_DHT_DC *)pTmp;
                    else if (pTmp[0] == 0x11)
                        jff.dht_uv_ac = (JF_DHT_AC *)pTmp;
                    //统计数据段长度
                    for (i = 1, sum = 0; i < 17; i++)
                        sum += pTmp[i];
                    //
                    printf("%02X-%d ", *pTmp, sum);
                    //下一包
                    pTmp += 1 + 16 + sum;
                } while(pTmp < jp.next);
                printf("\r\n");
                break;
            }
            case JT_DRI:
            {
                jff.dri = (JF_DRI *)jp.pkg;
                printf("type %04X DRI len %d, interval %d\r\n",
                    jp.type, jp.pkgLen,
                    jff.dri->interval);
                break;
            }
            case JT_SOS:
            {
                jff.sos = (JF_SOS *)jp.pkg;
                printf("type %04X SOS len %d, color %d -- "
                    "YUV序号-量化表DC-量化表AC: %d-%d-%d, %d-%d-%d, %d-%d-%d\r\n",
                    jp.type, jp.pkgLen,
                    jff.sos->color,
                    jff.sos->color_dcac[0].id,
                    jff.sos->color_dcac[0].dc,
                    jff.sos->color_dcac[0].ac,
                    jff.sos->color_dcac[1].id,
                    jff.sos->color_dcac[1].dc,
                    jff.sos->color_dcac[1].ac,
                    jff.sos->color_dcac[2].id,
                    jff.sos->color_dcac[2].dc,
                    jff.sos->color_dcac[2].ac);
                break;
            }
            case JT_COM:
            {
                printf("type %04X COM len %d %.*s\r\n", jp.type, jp.pkgLen, jp.pkgLen - 1, jp.pkg);
                break;
            }
            case JT_EOI:
            {
                printf("type %04X EOI len %d, data %d \r\n", jp.type, jp.pkgLen, jp.dataLen);
                break;
            }
            default:
            {
                if (jp.type > JT_APP0 && jp.type <= JT_APPn)
                    printf("type %04X APPn len %d %.*s\r\n", jp.type, jp.pkgLen, jp.pkgLen - 1, jp.pkg);
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
