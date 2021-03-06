/*
 *  文件内存映射工具,创建或获取指定文件的内存块
 */
#ifndef _FBMAP_H_
#define _FBMAP_H_

typedef enum {
    FMT_R, // 只读模式得到的mem指针绝对不能写,否则段错误
    FMT_RW,
    FMT_NEW,
} FbMap_Type;

typedef struct {
    unsigned char *mem;
    int fd;
    int size;
    FbMap_Type type;
} FbMap_Struct;

/*
 *  对文件进行内存映射
 *  参数:
 *      file: 目标文件
 *      type: 读写权限类型
 *      size: type=FMT_NEW 时传入创建文件内存大小,其它时候无效
 *  返回: 返回结构体指针,失败NULL
 */
FbMap_Struct *fbmap_open(char *file, FbMap_Type type, int size);

/*
 *  关闭内存映射和文件
 */
void fbmap_close(FbMap_Struct *fs);

#endif
