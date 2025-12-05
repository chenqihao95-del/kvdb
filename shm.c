#include "protocol.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHM_KEY 0x1234    // 共享内存 key, 自己约定一个即可

// 创建或获取共享内存, 返回表指针
KvTable* init_shm(int *shmid_out)
{
    // 创建或获取一段共享内存, 大小为 KvTable
    int shmid = shmget(SHM_KEY, sizeof(KvTable), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget 失败");
        exit(1);
    }

    // 将共享内存映射到本进程地址空间
    void *addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        perror("shmat 失败");
        exit(1);
    }

    KvTable *table = (KvTable*)addr;

    // 简单起见, 每次启动都清零, 然后再从文件加载
    memset(table, 0, sizeof(KvTable));

    if (shmid_out) {
        *shmid_out = shmid;
    }
    return table;
}

// 启动时从文件加载数据, 文本格式: 每行 "key value"
void load_data(KvTable *table, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        // 文件不存在就不处理
        return;
    }

    char key[KEY_SIZE];
    char val[VAL_SIZE];

    // 一行一行读, 填进共享内存
    while (fscanf(fp, "%31s %255s", key, val) == 2) {
        int i;
        for (i = 0; i < MAX_ITEMS; i++) {
            if (!table->items[i].used) {
                table->items[i].used = 1;
                strncpy(table->items[i].key, key, KEY_SIZE - 1);
                table->items[i].key[KEY_SIZE - 1] = '\0';
                strncpy(table->items[i].value, val, VAL_SIZE - 1);
                table->items[i].value[VAL_SIZE - 1] = '\0';
                break;
            }
        }
        // 空位用完就不再继续加载
        if (i == MAX_ITEMS) {
            break;
        }
    }

    fclose(fp);
}

// 退出时把共享内存里的数据写回文件
void save_data(KvTable *table, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("保存数据时 fopen 失败");
        return;
    }

    for (int i = 0; i < MAX_ITEMS; i++) {
        if (table->items[i].used) {
            // 一行一个键值对
            fprintf(fp, "%s %s\n",
                    table->items[i].key,
                    table->items[i].value);
        }
    }

    fclose(fp);
}
