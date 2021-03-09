#include <stdio.h>
#include "jpegFormat.h"

unsigned char test8x8[64] = {
    100, 100, 100, 100, 100, 100, 100, 101,
    100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100, 100, 100,
    102, 100, 100, 100, 100, 100, 100, 103};

/*
 *  作DCT和DQT处理
 *  DCT变换:
 *      设DCT矩阵为F,输入矩阵为IN,则OUT=F*IN*Ft,其中下标t表示转置
 *  DCT逆变换:
 *      设DCT矩阵为F,输入矩阵为IN(要解码的矩阵),则OUT=((IN*F)t*F)t
 */
void jpeg_dct_dqt(unsigned char in[64], unsigned char out[64])
{
    int x, y;
    int i;

    Map8x8_Float *A = (Map8x8_Float *)jf_dct_table;
    Map8x8_Uint8 *B = (Map8x8_Uint8 *)test8x8;
    Map8x8_Float C;
    Map8x8_Float D;

    for(y = 0; y < 8; ++y)
    {
        for(x = 0; x < 8; ++x)
        {
            C.m[y][x] = 0;
            for (i = 0; i < 8; i++)
                C.m[y][x] += A->m[y][i] * B->m[i][x];
        }
    }
    for(y = 0; y < 8; ++y)
    {
        for(x = 0; x < 8; ++x)
        {
            D.m[y][x] = 0;
            for (i = 0; i < 8; i++)
                D.m[y][x] += C.m[y][i] * A->m[x][i];
        }
    }

    printf("\r\n");
    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            printf("%8.2f, ", D.m[y][x]);
        }
        printf("\r\n");
    }

    for(y = 0; y < 8; ++y)
    {
        for(x = 0; x < 8; ++x)
        {
            C.m[y][x] = 0;
            for (i = 0; i < 8; i++)
                C.m[y][x] += D.m[y][i] * A->m[i][x];
        }
    }
    for(y = 0; y < 8; ++y)
    {
        for(x = 0; x < 8; ++x)
        {
            D.m[x][y] = 0;
            for (i = 0; i < 8; i++)
                D.m[x][y] += C.m[i][y] * A->m[i][x];
        }
    }

    printf("\r\n");
    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            printf("%8.2f, ", D.m[y][x]);
        }
        printf("\r\n");
    }
}
