#ifndef _JPEG_FORMAT_H_
#define _JPEG_FORMAT_H_

#include <stdint.h>

typedef struct
{
    uint8_t tag[5];
    uint8_t ver_h;      // 高版本号
    uint8_t ver_l;      // 低版本号
    uint8_t density;    // x和y的密度单位 0:无单位 1:点/英寸 2:点/厘米
    uint16_t density_x; // xy的像素密度
    uint16_t density_y;
    uint8_t short_x; // 缩略图的xy像素
    uint8_t short_y;
    // 这里接上RGB格式的缩略图(或没有,则short_x,y为0)
    // ...
} JF_APP0; // 0xFFE0

typedef struct
{
} JF_APPn; // 0xFFE1 ~ 0xFFEF

typedef struct
{
    uint8_t bit : 4; // 高4位: 精度 0/8位,1/16位
    uint8_t id : 4;  // 低4位: 量化表ID 取值0~3
    // 这里接64x(精度+1)字节,如8位精度为64x(0+1)=64字节
    // ...
} JF_DQT; // 0xFFDB 量化表

typedef struct
{
    uint8_t bit;     // 每个数据样本位数,通常为8,一般软件不支持12、16
    uint16_t height; // 图像高度,像素,不支持DNL时必须>0
    uint16_t width;  // 图像宽度,像素,不支持DNL时必须>0
    uint8_t color;   // 颜色分量 1:灰度图 3:YCrCb(JFIF中恒为该值)或YIQ 4:CMYK
    // 以下循环x3字节,3来自上面的YCrCb
    struct ColorInfo
    {
        uint8_t id;      // 颜色分量ID
        uint8_t factors; // 水平/垂直采样因子
        uint8_t dqt;     // 当前分量使用的量化表ID
    } color_info[3];
} JF_SOF0; // 0xFFC0 帧图像开始

typedef struct
{
    uint8_t type : 4; // 高4位: 类型 0/DC直流 1/AC交流
    uint8_t id : 4;   // 低4位: 哈夫曼表ID,注意DC和AC表分开编码
    // 未知
    // ...
} JF_DHT; // 0xFFC4 哈夫曼表

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
    uint8_t color; // 颜色分量,同上 JF_SOF0 中的 color
    // 以下循环x3字节,3来自上面的YCrCb
    struct ColorDCAC
    {
        uint8_t id:8; // 颜色分量ID
        uint8_t dc:4; // 直流系数表号,用的哈夫曼树编号
        uint8_t ac:4; // 交流系数表号,用的哈夫曼树编号
    } color_dcac[3];
    uint8_t begin; // 谱选择开始,固定为0x00
    uint8_t end; // 谱选择结束,固定为0x3F
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

#endif
