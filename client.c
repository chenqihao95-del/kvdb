#include "protocol.h"
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MSG_KEY 0x9100   // 要和服务器端一致

int main(void)
{
    int msgid = msgget(MSG_KEY, 0666);
    if (msgid < 0) {
        perror("连接服务器失败");
        return 1;
    }

    printf("连接服务器成功, 可用命令: put/get/del/list/exit\n");

    while (1) {
        char cmd[16];
        char key[KEY_SIZE];
        char val[VAL_SIZE];

        KvRequest req;
        KvResponse res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));

        printf("> ");
        if (scanf("%15s", cmd) != 1) {
            // 读命令失败直接继续下一轮
            continue;
        }

        if (strcmp(cmd, "put") == 0) {
            if (scanf("%31s %255s", key, val) != 2) {
                printf("用法: put key value\n");
                // 把当前行剩余内容丢掉
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                continue;
            }
            req.mtype = 1;
            req.op    = OP_PUT;
            strncpy(req.key,   key, KEY_SIZE - 1);
            strncpy(req.value, val, VAL_SIZE - 1);
        }
        else if (strcmp(cmd, "get") == 0) {
            if (scanf("%31s", key) != 1) {
                printf("用法: get key\n");
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                continue;
            }
            req.mtype = 1;
            req.op    = OP_GET;
            strncpy(req.key, key, KEY_SIZE - 1);
        }
        else if (strcmp(cmd, "del") == 0) {
            if (scanf("%31s", key) != 1) {
                printf("用法: del key\n");
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                continue;
            }
            req.mtype = 1;
            req.op    = OP_DEL;
            strncpy(req.key, key, KEY_SIZE - 1);
        }
        else if (strcmp(cmd, "list") == 0) {
            req.mtype = 1;
            req.op    = OP_LIST;
        }
        else if (strcmp(cmd, "exit") == 0) {
            printf("客户端退出\n");
            break;
        }
        else {
            printf("未知命令: %s\n", cmd);
            // 把本行剩余内容丢掉
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            continue;
        }

        // 发送请求
        if (msgsnd(msgid,
                   &req,
                   sizeof(KvRequest) - sizeof(long),
                   0) < 0) {
            perror("msgsnd 失败");
            continue;
        }

        // 接收服务器响应, mtype = 2
        if (msgrcv(msgid,
                   &res,
                   sizeof(KvResponse) - sizeof(long),
                   2,
                   0) < 0) {
            perror("msgrcv 失败");
            continue;
        }

        // 根据返回值打印结果
        if (res.result == RES_OK) {
            if (req.op == OP_GET) {
                printf("结果: %s = %s\n", res.key, res.value);
            } else if (req.op == OP_LIST) {
                printf("list 命令已执行, 具体内容请看服务器终端\n");
            } else {
                printf("操作成功\n");
            }
        } else if (res.result == RES_NOT_FOUND) {
            printf("未找到对应的键\n");
        } else if (res.result == RES_FULL) {
            printf("共享内存已满, 无法插入\n");
        } else {
            printf("操作失败\n");
        }

        // 把这一行剩下的东西读掉, 防止影响下一次输入
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
    }

    return 0;
}
