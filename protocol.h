#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <sys/types.h>

#define KEY_SIZE  32    // key 最大长度
#define VAL_SIZE  256   // value 最大长度
#define MAX_ITEMS 128   // 支持的最大记录数

// 操作类型
#define OP_PUT  1
#define OP_GET  2
#define OP_DEL  3
#define OP_LIST 4

// 返回结果
#define RES_OK          0   // 成功
#define RES_NOT_FOUND   1   // 没找到
#define RES_FULL        2   // 共享内存已满
#define RES_ERROR       3   // 其他错误

// 单条键值对
typedef struct {
    int  used;                  // 1 表示这一格有数据, 0 表示空
    char key[KEY_SIZE];         // 键
    char value[VAL_SIZE];       // 值
} KvItem;

// 整个键值表
typedef struct {
    KvItem items[MAX_ITEMS];    // 顺序存放
} KvTable;

// 客户端请求消息
typedef struct {
    long mtype;                 // 消息类型, 必须放在第一个成员
    int  op;                    // 操作类型
    char key[KEY_SIZE];
    char value[VAL_SIZE];
} KvRequest;

// 服务器响应消息
typedef struct {
    long mtype;                 // 消息类型
    int  result;                // 返回结果
    char key[KEY_SIZE];
    char value[VAL_SIZE];
} KvResponse;

#endif
