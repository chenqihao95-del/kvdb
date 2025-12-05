#include "protocol.h"
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// 声明 shm.c / sem.c 里提供的函数
KvTable* init_shm(int *shmid_out);
void load_data(KvTable *table, const char *filename);
void save_data(KvTable *table, const char *filename);

int  init_sem();
void sem_lock(int semid);
void sem_unlock(int semid);

#define MSG_KEY   0x9100        // 消息队列 key
#define DATA_FILE "kvdb.data"   // 持久化文件名

static int msgid = -1;
static int semid = -1;
static int shmid = -1;
static KvTable *table = NULL;

// 处理单次客户端请求
static void handle_request(const KvRequest *req, KvResponse *res)
{
    // 响应类型统一用 2
    res->mtype  = 2;
    res->result = RES_ERROR;
    memset(res->key,   0, KEY_SIZE);
    memset(res->value, 0, VAL_SIZE);

    // 对共享内存加锁
    sem_lock(semid);

    if (req->op == OP_PUT) {
        // 先查有没有同名 key, 有则覆盖
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (table->items[i].used &&
                strcmp(table->items[i].key, req->key) == 0) {

                strncpy(table->items[i].value, req->value, VAL_SIZE - 1);
                table->items[i].value[VAL_SIZE - 1] = '\0';
                res->result = RES_OK;
                sem_unlock(semid);
                return;
            }
        }
        // 没有同名 key, 找一个空位
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (!table->items[i].used) {
                table->items[i].used = 1;
                strncpy(table->items[i].key, req->key, KEY_SIZE - 1);
                table->items[i].key[KEY_SIZE - 1] = '\0';
                strncpy(table->items[i].value, req->value, VAL_SIZE - 1);
                table->items[i].value[VAL_SIZE - 1] = '\0';
                res->result = RES_OK;
                sem_unlock(semid);
                return;
            }
        }
        // 走到这里说明共享内存满了
        res->result = RES_FULL;
    }
    else if (req->op == OP_GET) {
        // 遍历查找 key
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (table->items[i].used &&
                strcmp(table->items[i].key, req->key) == 0) {

                strncpy(res->key, table->items[i].key, KEY_SIZE - 1);
                res->key[KEY_SIZE - 1] = '\0';
                strncpy(res->value, table->items[i].value, VAL_SIZE - 1);
                res->value[VAL_SIZE - 1] = '\0';
                res->result = RES_OK;
                sem_unlock(semid);
                return;
            }
        }
        res->result = RES_NOT_FOUND;
    }
    else if (req->op == OP_DEL) {
        // 删除对应 key
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (table->items[i].used &&
                strcmp(table->items[i].key, req->key) == 0) {

                table->items[i].used = 0;
                res->result = RES_OK;
                sem_unlock(semid);
                return;
            }
        }
        res->result = RES_NOT_FOUND;
    }
    else if (req->op == OP_LIST) {
        // 在服务端打印当前所有键值对
        printf("当前键值对列表:\n");
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (table->items[i].used) {
                printf(" [%d] %s = %s\n",
                       i,
                       table->items[i].key,
                       table->items[i].value);
            }
        }
        res->result = RES_OK;
    }

    sem_unlock(semid);
}

// Ctrl+C 信号处理, 做收尾
static void cleanup(int signo)
{
    (void)signo;  // 避免未使用参数的编译警告

    printf("\n接收到信号, 保存数据并退出...\n");

    if (table) {
        save_data(table, DATA_FILE);
        shmdt(table);
        table = NULL;
    }
    if (shmid >= 0) {
        shmctl(shmid, IPC_RMID, NULL);
    }
    if (msgid >= 0) {
        msgctl(msgid, IPC_RMID, NULL);
    }
    if (semid >= 0) {
        semctl(semid, 0, IPC_RMID);  // 删除信号量集合
    }

    exit(0);
}

int main(void)
{
    // 捕获 Ctrl+C
    signal(SIGINT, cleanup);

    // 创建或获取消息队列
    msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid < 0) {
        perror("msgget 失败");
        return 1;
    }

    // 初始化信号量
    semid = init_sem();

    // 初始化共享内存, 并从文件加载历史数据
    table = init_shm(&shmid);
    load_data(table, DATA_FILE);

    printf("服务器已启动, 等待客户端请求...\n");

    while (1) {
        KvRequest req;
        KvResponse res;

        // 先把结构体清零
        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));

        // 接收来自客户端的请求, mtype = 1
        ssize_t n = msgrcv(msgid,
                           &req,
                           sizeof(KvRequest) - sizeof(long),
                           1,
                           0);
        if (n < 0) {
            perror("msgrcv 失败");
            continue;
        }

        // 处理请求并填充响应
        handle_request(&req, &res);

        // 回复客户端, mtype = 2
        if (msgsnd(msgid,
                   &res,
                   sizeof(KvResponse) - sizeof(long),
                   0) < 0) {
            perror("msgsnd 失败");
        }
    }

    return 0;
}
