#ifndef _JPEGFORMAT_H_
#define _JPEGFORMAT_H_

#include <stdint.h>

typedef union {
    uint8_t a[64];   // array数组存储
    uint8_t m[8][8]; // map二维矩阵存储
} Map8x8_Uint8;

typedef union {
    float a[64];   // array数组存储
    float m[8][8]; // map二维矩阵存储
} Map8x8_Float;

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
} JF_Type;

typedef struct
{
    // 一般为"JFIF"
    uint8_t tag[5];
    // 高版本号
    uint8_t ver_h;
    // 低版本号
    uint8_t ver_l;
    // x和y的密度单位 0:无单位 1:点/英寸 2:点/厘米
    uint8_t density;
    // xy的像素密度
    uint16_t density_x;
    uint16_t density_y;
    // 缩略图的xy像素
    uint8_t short_x;
    uint8_t short_y;
    // 这里接上RGB格式的缩略图(或没有,则short_x,y为0)
    // ...
} JF_APP0; // 0xFFE0

typedef struct
{
    // 高4位: 量化表ID 取值0~3
    uint8_t id : 4;
    // 低4位: 精度 0/8位,1/16位
    uint8_t bit : 4;
    // 这里接64x(精度+1)字节,如8位精度为64x(0+1)=64字节
    uint8_t dqt[64];
} JF_DQT; // 0xFFDB 量化表(一般重复出现2次id=0~1,最多id=0~3)

typedef struct
{
    // 每个数据样本位数,通常为8,一般软件不支持12、16
    uint8_t bit;
    // 图像高度,像素,不支持DNL时必须>0
    uint8_t height_h;
    uint8_t height_l;
    // 图像宽度,像素,不支持DNL时必须>0
    uint8_t width_h;
    uint8_t width_l;
    // 颜色分量 1:灰度图 3:YUV(JFIF中恒为该值)或YIQ 4:CMYK
    uint8_t color;
    // 以下循环x3字节,3来自上面的YUV
    struct ColorInfo
    {
        // 颜色分量ID
        uint8_t id : 8;
        // 高4位: 水平采样因子
        uint8_t factor_x : 4;
        // 低4位: 垂直采样因子
        uint8_t factor_y : 4;
        // 当前分量使用的量化表ID
        uint8_t dqt : 8;
    } color_info[3];
} JF_SOF0; // 0xFFC0 帧图像开始

typedef struct
{
    // 高4位: 类型 0/DC直流 1/AC交流
    uint8_t type : 4;
    // 低4位: 哈夫曼表ID,注意DC和AC表分开编码
    uint8_t id : 4;
    // 接下来固定16字节的codes
    uint8_t codes[16];
    // 根据上面codes数组元素累加之和,为该段数据的长度
    uint8_t values[12];
} JF_DHT_DC; // 0xFFC4 哈夫曼表(该结构一般循环出现4次)

typedef struct
{
    // 高4位: 类型 0/DC直流 1/AC交流
    uint8_t type : 4;
    // 低4位: 哈夫曼表ID,注意DC和AC表分开编码
    uint8_t id : 4;
    // 接下来固定16字节的codes
    uint8_t codes[16];
    // 根据上面codes数组元素累加之和,为该段数据的长度
    uint8_t values[162];
} JF_DHT_AC; // 0xFFC4 哈夫曼表(该结构一般循环出现4次)

typedef struct
{
    /*
     * 设其值为n,则表示每n个MCU块就有一个RSTn标记.
     * 第一个标记是RST0,第二个是RST1等,RST7后再从RST0重复.
     */
    uint16_t interval;
} JF_DRI; // 0xFFDD 差分编码累计复位的间隔

typedef struct
{
    // 颜色分量,同上 JF_SOF0 中的 color(JFIF格式固定为3)
    uint8_t color;
    // 以下循环x3字节,3来自上面的YUV
    struct ColorDCAC
    {
        // 颜色分量ID
        uint8_t id : 8;
        // 直流系数表号,用的哈夫曼树编号
        uint8_t dc : 4;
        // 交流系数表号,用的哈夫曼树编号
        uint8_t ac : 4;
    } color_dcac[3];
    uint8_t begin;  // 谱选择开始,固定为0x00
    uint8_t end;    // 谱选择结束,固定为0x3F
    uint8_t select; // 谱选择,在基本JPEG中总为0x00
    /*
     *  后面紧接压缩数据
     *  当数据中遇到0xFF,需判断后一字节,并作如下处理:
     *      0x00: 去除该0x00并保留0xFF作为译码数据
     *      0xD9: 文件结束
     *      0xD0~D7: 判断为RSTn包,解析包并调整译码变量,然后跳过该包
     *      0xFF: 忽视该值继续判断下一个值
     *      其它: 直接忽视原来的0xFF,并接着正常译码
     */
    // ...
} JF_SOS; // 0xFFDA 开始扫描

// DCT列表
extern const float jf_dct_table[64];
// YUV中Y帧的8x8采样图像使用的量化表
extern const uint8_t jf_qt_table_y[64];
// YUV中CrCb帧的8x8采样图像使用的量化表
extern const uint8_t jf_qt_table_uv[64];
// Z路径排序取值,以下为数组元素序号
extern const int jf_z_loop[64];

extern const uint8_t jf_y_dc_codes[16];
extern const uint8_t jf_y_dc_values[12];
extern const uint8_t jf_uv_dc_codes[16];
extern const uint8_t jf_uv_dc_values[12];
extern const uint8_t jf_y_ac_codes[16];
extern const uint8_t jf_y_ac_values[162];
extern const uint8_t jf_uv_ac_codes[16];
extern const uint8_t jf_uv_ac_values[162];

#endif
