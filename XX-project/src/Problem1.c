#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include "helpers.h"


typedef struct {
    int order_id;     
    int raw_value;
    int encoded_value;
} Packet;

typedef struct {
    Packet* items;
    int capacity;
    int head;
    int tail;
    pthread_mutex_t mutex;
    sem_t empty_slots;
    sem_t filled_slots;
} BoundedBuffer;

typedef struct {
    int* available_tokens;
    int num_types;
    pthread_mutex_t pool_mutex;
    pthread_cond_t pool_cv;
} TokenPool;

typedef struct {
    int tA;
    int tB;
} EncoderReq;

BoundedBuffer BufferA;
BoundedBuffer BufferB;
TokenPool token_pool;
EncoderReq* encoder_reqs;

int P, M, N, num_orders, T;


int current_order_id = 0;
pthread_mutex_t order_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_buffer(BoundedBuffer* buf, int capacity) {
    buf->capacity = capacity;
    buf->items = (Packet*)malloc(sizeof(Packet) * capacity);
    buf->head = 0;
    buf->tail = 0;
    pthread_mutex_init(&buf->mutex, NULL);
    sem_init(&buf->empty_slots, 0, capacity);
    sem_init(&buf->filled_slots, 0, 0);
}

void destroy_buffer(BoundedBuffer* buf) {
    free(buf->items);
    pthread_mutex_destroy(&buf->mutex);
    sem_destroy(&buf->empty_slots);
    sem_destroy(&buf->filled_slots);
}

void buf_put(BoundedBuffer* buf, Packet p) {
    sem_wait(&buf->empty_slots);
    pthread_mutex_lock(&buf->mutex);

    buf->items[buf->tail] = p;
    buf->tail = (buf->tail + 1) % buf->capacity;

    pthread_mutex_unlock(&buf->mutex);
    sem_post(&buf->filled_slots);
}

Packet buf_get(BoundedBuffer* buf) {
    sem_wait(&buf->filled_slots);
    pthread_mutex_lock(&buf->mutex);

    Packet p = buf->items[buf->head];
    buf->head = (buf->head + 1) % buf->capacity;

    pthread_mutex_unlock(&buf->mutex);
    sem_post(&buf->empty_slots);
    return p;
}
void init_token_pool(int num_types) {
    token_pool.num_types = num_types;
    token_pool.available_tokens = (int*)malloc(sizeof(int) * num_types);
    pthread_mutex_init(&token_pool.pool_mutex, NULL);
    pthread_cond_init(&token_pool.pool_cv, NULL);
}

void destroy_token_pool() {
    free(token_pool.available_tokens);
    pthread_mutex_destroy(&token_pool.pool_mutex);
    pthread_cond_destroy(&token_pool.pool_cv);
}

void acquire_two_tokens(int tA, int tB) {
    pthread_mutex_lock(&token_pool.pool_mutex);

    while (token_pool.available_tokens[tA] == 0 || token_pool.available_tokens[tB] == 0) {
        pthread_cond_wait(&token_pool.pool_cv, &token_pool.pool_mutex);
    }

    token_pool.available_tokens[tA]--;
    token_pool.available_tokens[tB]--;

    pthread_mutex_unlock(&token_pool.pool_mutex);
}

void release_two_tokens(int tA, int tB) {
    pthread_mutex_lock(&token_pool.pool_mutex);

    token_pool.available_tokens[tA]++;
    token_pool.available_tokens[tB]++;

    pthread_cond_broadcast(&token_pool.pool_cv);

    pthread_mutex_unlock(&token_pool.pool_mutex);
}

void* quantizer_thread(void* arg) {
    while (true) {
        int my_id;
        pthread_mutex_lock(&order_mutex);
        if (current_order_id >= num_orders) {
            pthread_mutex_unlock(&order_mutex);
            break;
        }
        my_id = current_order_id++;
        pthread_mutex_unlock(&order_mutex);

        simulate_work(OP_Q1_QUANTIZE);

        Packet p;
        p.order_id = my_id;
        p.raw_value = my_id;

        buf_put(&BufferA, p);
    }

    Packet sentinel;
    sentinel.order_id = -1;
    buf_put(&BufferA, sentinel);

    pthread_exit(NULL);
}

void* encoder_thread(void* arg) {
    int id = *(int*)arg;
    int tA = encoder_reqs[id].tA;
    int tB = encoder_reqs[id].tB;

    while (true) {
        Packet p = buf_get(&BufferA);

        if (p.order_id == -1) {
            buf_put(&BufferB, p);
            break;
        }

        acquire_two_tokens(tA, tB);

        simulate_work(OP_Q1_ENCODE);
        p.encoded_value = p.raw_value * 2 + tA + tB;

        release_two_tokens(tA, tB);

        buf_put(&BufferB, p);
    }

    pthread_exit(NULL);
}

void* logger_thread(void* arg) {
    int sentinels_received = 0;
    while (sentinels_received < P) {
        Packet p = buf_get(&BufferB);

        if (p.order_id == -1) {
            sentinels_received++;
        }
        else {
            simulate_work(OP_Q1_LOG);
            printf("[Logger] order_id=%d encoded=%d\n", p.order_id, p.encoded_value);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        fprintf(stderr, "Usage: %s <P> <M> <N> <num_orders> <T> <cnt_0> ... <cnt_T-1> <tA_0> <tB_0> ... <tA_P-1> <tB_P-1>\n", argv[0]);
        return 1;
    }

    P = atoi(argv[1]);
    M = atoi(argv[2]);
    N = atoi(argv[3]);
    num_orders = atoi(argv[4]);
    T = atoi(argv[5]);
    if (argc != 1 + 5 + T + 2 * P) {
        fprintf(stderr, "Error: Incorrect number of arguments provided.\n");
        return 1;
    }

    init_buffer(&BufferA, M);
    init_buffer(&BufferB, N);
    init_token_pool(T);
    encoder_reqs = (EncoderReq*)malloc(sizeof(EncoderReq) * P);

    int arg_idx = 6;
    for (int i = 0; i < T; i++) {
        token_pool.available_tokens[i] = atoi(argv[arg_idx++]);
    }
    for (int i = 0; i < P; i++) {
        encoder_reqs[i].tA = atoi(argv[arg_idx++]);
        encoder_reqs[i].tB = atoi(argv[arg_idx++]);
    }

    pthread_t* quantizers = (pthread_t*)malloc(sizeof(pthread_t) * P);
    pthread_t* encoders = (pthread_t*)malloc(sizeof(pthread_t) * P);
    pthread_t logger;

    int* encoder_ids = (int*)malloc(sizeof(int) * P);

    pthread_create(&logger, NULL, logger_thread, NULL);

    for (int i = 0; i < P; i++) {
        encoder_ids[i] = i;
        pthread_create(&encoders[i], NULL, encoder_thread, &encoder_ids[i]);
        pthread_create(&quantizers[i], NULL, quantizer_thread, NULL);
    }

    for (int i = 0; i < P; i++) {
        pthread_join(quantizers[i], NULL);
        pthread_join(encoders[i], NULL);
    }
    pthread_join(logger, NULL);
    destroy_buffer(&BufferA);
    destroy_buffer(&BufferB);
    destroy_token_pool();
    free(encoder_reqs);
    free(quantizers);
    free(encoders);
    free(encoder_ids);

    return 0;
}