#ifndef _JPEGENC_H_
#define _JPEGENC_H_

#include <stdint.h>

typedef struct {
    int fd;
    // 图片宽高
    int width, height;
    // 每帧YUV中包含的Y、U、V帧(8x8)数量
    int frameNumY;
    int frameNumU;
    int frameNumV;
    // 写入帧计数和总帧数
    int frameCount;
    int frameTotal;
} JpegEnc_Struct;

/*
 *  jpeg编码文件初始化
 *  参数:
 *  返回: 0成功,其它失败
 */
int jpegEnc_create(JpegEnc_Struct *jes, char *file, int width, int height);

/*
 *  YUV格式的帧数据写入
 *  参数:
 *      *Y[64]: 8x8块数组,至少jes->frameY块
 *      *U[64]: 8x8块数组,至少jes->frameU块
 *      *V[64]: 8x8块数组,至少jes->frameV块
 *      frameSize: 一次传入帧数,当为1时,至少传入上面
 *  返回: 实际写入帧数,小于frameSize时表示已满
 */
int jpegEnc_frame(
    JpegEnc_Struct *jes,
    uint8_t *Y[64],
    uint8_t *U[64],
    uint8_t *V[64],
    int frameSize);

/*
 *  jpeg文件编码结束
 */
void jpeg_close(int fd);

#endif
