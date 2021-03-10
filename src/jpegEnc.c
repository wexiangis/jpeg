#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "jpegEnc.h"
#include "jpegFormat.h"

// 0xFFE0 APP0 图片信息
static const JF_APP0 app0_default = {
    .tag = "JFIF",
    .ver_h = 1,
    .ver_l = 1,
    .density = 0, // 这里往下,没有缩略图都为0
    .density_x = 0,
    .density_y = 0,
    .short_x = 0,
    .short_y = 0,
};

// 0xFFFE COM 注释信息
static const uint8_t com_default[] = "JPEG Encode Test";

// 0xFFDB DQT 量化表
static const JF_DQT dqt0_default = {0, 0};
static const JF_DQT dqt1_default = {1, 0};

// 0xFFC0 SOF0 帧图像开始(以下为YCrCb 4:1:1配置)
static JF_SOF0 sof0_default = {
    .bit = 8,
    .color = 3,
    .color_info = {
        {
            .id = 1,
            .factor_x = 2,
            .factor_y = 2,
            .dqt = 0,
        },
        {
            .id = 2,
            .factor_x = 1,
            .factor_y = 1,
            .dqt = 1,
        },
        {
            .id = 3,
            .factor_x = 1,
            .factor_y = 1,
            .dqt = 1,
        },
    },
};

// 0xFFDA SOS 开始扫描
static const JF_SOS sos_default = {
    .color = 3,
    .color_dcac = {
        {
            .id = 1,
            .dc = 0,
            .ac = 0,
        },
        {
            .id = 2,
            .dc = 1,
            .ac = 1,
        },
        {
            .id = 3,
            .dc = 1,
            .ac = 1,
        },
    },
    .begin = 0x00,
    .end = 0x3F,
    .select = 0x00,
};

static void write_head(int fd, JF_Type type, uint32_t len)
{
    uint8_t pkg[4] = {
        (uint8_t)((type >> 8) & 0xFF),
        (uint8_t)(type & 0xFF),
        (uint8_t)((len >> 8) & 0xFF),
        (uint8_t)(len & 0xFF),
    };
    write(fd, pkg, len > 0 ? 4 : 2);
}

/*
 *  jpeg编码文件初始化
 *  参数:
 *  返回: 0成功,其它失败
 */
int jpegEnc_create(JpegEnc_Struct *jes, char *file, int width, int height)
{
    uint8_t dht_id;

    // 创建文件
    if ((jes->fd = open(file, O_CREAT | O_TRUNC | O_RDWR, 0666)) < 1)
    {
        fprintf(stderr, "jpegEnc_create: open %s failed \r\n", file);
        return -1;
    }
    // 参数记录
    jes->width = width;
    jes->height = height;
    jes->frameCount = 0;
    jes->frameTotal = jes->width * jes->height / jes->frameNumY;
    // 每帧中需包含Y、Cr、Cb帧(8x8)数量
    jes->frameNumY =
        sof0_default.color_info[0].factor_x *
        sof0_default.color_info[0].factor_y;
    jes->frameNumCr =
        sof0_default.color_info[1].factor_x *
        sof0_default.color_info[1].factor_y;
    jes->frameNumCb =
        sof0_default.color_info[2].factor_x *
        sof0_default.color_info[2].factor_y;
    // 图片宽高配置
    sof0_default.width_h = (uint8_t)((width >> 8) & 0xFF);
    sof0_default.width_l = (uint8_t)(width & 0xFF);
    sof0_default.height_h = (uint8_t)((height >> 8) & 0xFF);
    sof0_default.height_l = (uint8_t)(height & 0xFF);

    write_head(jes->fd, JT_APP0, sizeof(app0_default) + 2);
    write(jes->fd, &app0_default, sizeof(app0_default));

    write_head(jes->fd, JT_COM, sizeof(com_default) + 2);
    write(jes->fd, &com_default, sizeof(com_default));

    write_head(jes->fd, JT_DQT, (1 + 64) * 2 + 2);
    write(jes->fd, &dqt0_default, 1);
    write(jes->fd, jf_qt_table_y, 64);
    write(jes->fd, &dqt1_default, 1);
    write(jes->fd, jf_qt_table_crcb, 64);

    write_head(jes->fd, JT_SOF0, sizeof(sof0_default) + 2);
    write(jes->fd, &sof0_default, sizeof(sof0_default));

    write_head(jes->fd, JT_DHT, 2 + (1 + 16 + 12 + 1 + 16 + 162) * 2);
    dht_id = 0x00;
    write(jes->fd, &dht_id, 1);
    write(jes->fd, jf_y_dc_codes, sizeof(jf_y_dc_codes));
    write(jes->fd, jf_y_dc_values, sizeof(jf_y_dc_values));
    dht_id = 0x01;
    write(jes->fd, &dht_id, 1);
    write(jes->fd, jf_crcb_dc_codes, sizeof(jf_crcb_dc_codes));
    write(jes->fd, jf_crcb_dc_values, sizeof(jf_crcb_dc_values));
    dht_id = 0x10;
    write(jes->fd, &dht_id, 1);
    write(jes->fd, jf_y_ac_codes, sizeof(jf_y_ac_codes));
    write(jes->fd, jf_y_ac_values, sizeof(jf_y_ac_values));
    dht_id = 0x11;
    write(jes->fd, &dht_id, 1);
    write(jes->fd, jf_crcb_ac_codes, sizeof(jf_crcb_ac_codes));
    write(jes->fd, jf_crcb_ac_values, sizeof(jf_crcb_ac_values));

    write_head(jes->fd, JT_SOS, sizeof(sos_default) + 2);
    write(jes->fd, &sos_default, sizeof(sos_default));

    return 0;
}

/*
 *  YCrCb格式的帧数据写入
 *  参数:
 *  返回: 实际写入帧数,小于frameSize时表示已满
 */
int jpegEnc_frame(
    JpegEnc_Struct *jes,
    uint8_t *Y[64],
    uint8_t *Cr[64],
    uint8_t *Cb[64],
    int frameSize)
{

    return 0;
}

/*
 *  jpeg文件编码结束
 */
void jpeg_close(int fd)
{
    ;
}