// gcc -g -o spmc spmc.c -pthread -Wall
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>
#include <unistd.h>

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})

typedef struct {
    pthread_rwlock_t lock;
    int size;
    int capacity;
    uint8_t buf[0];
} spmc_entry_t;

typedef struct {
    int size;
    int capacity;
    spmc_entry_t entries[0];
} spmc_t;

void *spmc_init(int entry_num, int buf_size) {
    spmc_t *spmc = (spmc_t *)malloc(sizeof(spmc_t)+entry_num*(sizeof(spmc_entry_t)+buf_size));
    if (spmc == NULL) {
        printf("spmc_init: malloc error\n");
        return NULL;
    }
    spmc->size = 0;
    spmc->capacity = entry_num;

    for (int i = 0; i < spmc->capacity; i++) {
        int ret = pthread_rwlock_init(&spmc->entries[i].lock, NULL);
        if (ret != 0) {
            printf("pthread_rwlock_init error\n");
            free(spmc);
            return NULL;
        }

        spmc->entries[i].size = 0;
        spmc->entries[i].capacity = buf_size;
    }

    return (void *)spmc;
}

void spmc_free(void *s) {
    spmc_t *spmc = (spmc_t *)s;
    if (spmc == NULL) {
        return;
    }

    for (int i = 0; i < spmc->capacity; i++) {
        pthread_rwlock_destroy(&spmc->entries[i].lock);
    }

    free(spmc);
}

// 生产者

// 生产者申请第一个能够写入的entry
uint8_t *producer_alloc(void *s) {
    spmc_t *spmc = (spmc_t *)s;
    int ret;
    for (int i = 0; i < spmc->capacity; i++) {
        ret = pthread_rwlock_trywrlock(&spmc->entries[i].lock);
        if (ret == 0) {
            // 更新size，使用的entry数量。只增不减
            if (spmc->size <= i) {
                spmc->size = i+1;
            }
            printf("producer_alloc index: %d\n", i);
            return spmc->entries[i].buf;
        } else {
            printf("producer_alloc trywrlock error: %d\n", i);
        }
    }

    return NULL;
}

// 生产者写入后释放写锁
void producer_alloc_end(const uint8_t *buf) {
    spmc_entry_t *entry = container_of(buf, spmc_entry_t, buf[0]);
    pthread_rwlock_unlock(&entry->lock);
}

// 消费者

// 对申请的entry的buf只读
uint8_t *consumer_alloc(void *s) {
    spmc_t *spmc = (spmc_t *)s;
    int ret;
    for (int i = 0; i < spmc->size; i++) {
        ret = pthread_rwlock_tryrdlock(&spmc->entries[i].lock);
        if (ret == 0) {
            printf("consumer_alloc index: %d\n", i);
            return spmc->entries[i].buf;
        } else {
            printf("consumer_alloc tryrdlock error: %d\n", i);
        }
    }

    return NULL;
}

// 读取完成后解锁
void consumer_alloc_end(const uint8_t *buf) {
    spmc_entry_t *entry = container_of(buf, spmc_entry_t, buf[0]);
    pthread_rwlock_unlock(&entry->lock);
}


/////////////////////////// 测试 ////////////////////////////////////////////////////////
spmc_t *spmc;
const int count = 2000;

void* consume(void *not_used) {
    int i = 0;
    for (; i < count; ) {
        uint8_t *buf = consumer_alloc(spmc);
        if (buf == NULL) {
            continue;
        }

        printf("consume: %d %p\n", buf[0], &buf[0]);
        consumer_alloc_end(buf);
        i++;
    }

    return NULL;
}

void* produce(void *not_used) {
    for (int i = 0; i < count; i++) {
        uint8_t *buf = producer_alloc(spmc);
        if (buf == NULL) {
            printf("ERROR producer_alloc NULL!!!\n");
            return NULL;
        }

        buf[0] = (uint8_t)i;
        printf("produce: %d %p\n", buf[0], &buf[0]);
        consumer_alloc_end(buf);
    }

    return NULL;
}

int main() {
    spmc = spmc_init(4, 10);
    printf("%p\n", spmc->entries[0].buf);

    pthread_t threads[3];
    pthread_create(&threads[0], NULL, consume, NULL);
    pthread_create(&threads[1], NULL, consume, NULL);
    pthread_create(&threads[2], NULL, consume, NULL);

    produce(NULL);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);

    return 0;
}
