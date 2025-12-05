#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>

#define SEM_KEY 0x5678    // 信号量 key, 自己约定一个

// 有的系统头文件里没有定义 union semun, 这里自己补一个
union semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
};

// 初始化一个二元信号量, 初值为 1
int init_sem()
{
    // 只要 1 个信号量
    int semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid < 0) {
        perror("semget 失败");
        exit(1);
    }

    union semun arg;
    arg.val = 1;  // 初始为 1, 表示资源空闲
    if (semctl(semid, 0, SETVAL, arg) < 0) {
        perror("semctl 设置初值失败");
        exit(1);
    }

    return semid;
}

// P 操作, 加锁
void sem_lock(int semid)
{
    struct sembuf sb;
    sb.sem_num = 0;    // 只有一个信号量, 下标为 0
    sb.sem_op  = -1;   // -1 表示申请资源
    sb.sem_flg = 0;

    if (semop(semid, &sb, 1) < 0) {
        perror("semop P 失败");
    }
}

// V 操作, 解锁
void sem_unlock(int semid)
{
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op  = 1;    // +1 表示释放资源
    sb.sem_flg = 0;

    if (semop(semid, &sb, 1) < 0) {
        perror("semop V 失败");
    }
}
