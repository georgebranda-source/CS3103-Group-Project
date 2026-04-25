#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "helpers.h"

#define MAX_NAME_LEN 128

typedef enum {
    ROLE_WORKER,
    ROLE_MANAGER,
    ROLE_SUPERVISOR
} Role;

typedef enum {
    OP_READ,
    OP_WRITE
} OpType;

typedef struct {
    Role role;
    int id;
    OpType op;
    char item[MAX_NAME_LEN];
    int seq;
} Operation;

typedef struct {
    char name[MAX_NAME_LEN];
    int size;
} DirectoryEntry;

static Operation *operations = NULL;
static int num_ops = 0;

static DirectoryEntry *directory_entries = NULL;
static int directory_count = 0;
static int directory_capacity = 0;

static pthread_mutex_t monitor_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ok_to_read = PTHREAD_COND_INITIALIZER;
static pthread_cond_t ok_to_manage = PTHREAD_COND_INITIALIZER;
static pthread_cond_t ok_to_supervise = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

static int active_readers = 0;
static int active_writer = 0;
static int waiting_managers = 0;
static int waiting_supervisors = 0;

static void safe_print(const char *msg) {
    pthread_mutex_lock(&print_mutex);
    printf("%s\n", msg);
    fflush(stdout);
    pthread_mutex_unlock(&print_mutex);
}

static int initial_size_for_item(const char *name) {
    int n = 0;
    const char *p = name;

    while (*p) {
        if (*p >= '0' && *p <= '9') {
            n = n * 10 + (*p - '0');
        }
        p++;
    }

    if (n <= 0) {
        n = 1;
    }

    return 512 + (n % 8) * 256;
}

static int find_entry(const char *name) {
    for (int i = 0; i < directory_count; i++) {
        if (strcmp(directory_entries[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int get_or_add_entry(const char *name) {
    int idx = find_entry(name);

    if (idx >= 0) {
        return idx;
    }

    if (directory_count >= directory_capacity) {
        fprintf(stderr, "Directory capacity exceeded.\n");
        exit(1);
    }

    strcpy(directory_entries[directory_count].name, name);
    directory_entries[directory_count].size = initial_size_for_item(name);
    directory_count++;

    return directory_count - 1;
}

static void enter_worker(int id) {
    char buffer[256];

    pthread_mutex_lock(&monitor_mutex);

    if (active_writer || waiting_supervisors > 0 || waiting_managers > 0) {
        if (waiting_supervisors > 0) {
            snprintf(buffer, sizeof(buffer),
                     "[Worker-%d] [worker blocked: supervisor pending] waiting...",
                     id);
        } else if (waiting_managers > 0) {
            snprintf(buffer, sizeof(buffer),
                     "[Worker-%d] [worker blocked: manager pending] waiting...",
                     id);
        } else {
            snprintf(buffer, sizeof(buffer),
                     "[Worker-%d] waiting for read access...",
                     id);
        }

        safe_print(buffer);
    }

    while (active_writer || waiting_supervisors > 0 || waiting_managers > 0) {
        pthread_cond_wait(&ok_to_read, &monitor_mutex);
    }

    active_readers++;

    pthread_mutex_unlock(&monitor_mutex);
}

static void exit_worker(void) {
    pthread_mutex_lock(&monitor_mutex);

    active_readers--;

    if (active_readers == 0) {
        if (waiting_supervisors > 0) {
            pthread_cond_signal(&ok_to_supervise);
        } else if (waiting_managers > 0) {
            pthread_cond_signal(&ok_to_manage);
        } else {
            pthread_cond_broadcast(&ok_to_read);
        }
    }

    pthread_mutex_unlock(&monitor_mutex);
}

static void enter_manager(int id) {
    char buffer[256];

    pthread_mutex_lock(&monitor_mutex);

    waiting_managers++;

    if (active_writer || active_readers > 0 || waiting_supervisors > 0) {
        snprintf(buffer, sizeof(buffer),
                 "[Manager-%d] waiting for write lock", id);
        safe_print(buffer);
    }

    while (active_writer || active_readers > 0 || waiting_supervisors > 0) {
        pthread_cond_wait(&ok_to_manage, &monitor_mutex);
    }

    waiting_managers--;
    active_writer = 1;

    snprintf(buffer, sizeof(buffer),
             "[Manager-%d] acquired write lock", id);
    safe_print(buffer);

    pthread_mutex_unlock(&monitor_mutex);
}

static void enter_supervisor(int id) {
    char buffer[256];

    pthread_mutex_lock(&monitor_mutex);

    waiting_supervisors++;

    if (active_writer || active_readers > 0) {
        snprintf(buffer, sizeof(buffer),
                 "[Supervisor-%d] waiting for write lock", id);
        safe_print(buffer);
    }

    while (active_writer || active_readers > 0) {
        pthread_cond_wait(&ok_to_supervise, &monitor_mutex);
    }

    waiting_supervisors--;
    active_writer = 1;

    if (waiting_managers > 0) {
        snprintf(buffer, sizeof(buffer),
                 "[Supervisor-%d] [supervisor preempts manager] acquired write lock",
                 id);
    } else {
        snprintf(buffer, sizeof(buffer),
                 "[Supervisor-%d] acquired write lock", id);
    }

    safe_print(buffer);

    pthread_mutex_unlock(&monitor_mutex);
}

static void exit_writer(void) {
    pthread_mutex_lock(&monitor_mutex);

    active_writer = 0;

    if (waiting_supervisors > 0) {
        pthread_cond_signal(&ok_to_supervise);
    } else if (waiting_managers > 0) {
        pthread_cond_signal(&ok_to_manage);
    } else {
        pthread_cond_broadcast(&ok_to_read);
    }

    pthread_mutex_unlock(&monitor_mutex);
}

static void perform_read(Operation *op) {
    char buffer[256];

    enter_worker(op->id);

    int idx = find_entry(op->item);
    int size = 0;

    if (idx >= 0) {
        size = directory_entries[idx].size;
    }

    pthread_mutex_lock(&monitor_mutex);
    int concurrent = active_readers > 1;
    pthread_mutex_unlock(&monitor_mutex);

    if (concurrent) {
        snprintf(buffer, sizeof(buffer),
                 "[Worker-%d] [concurrent read] FILE: %s SIZE: %d bytes",
                 op->id, op->item, size);
    } else {
        snprintf(buffer, sizeof(buffer),
                 "[Worker-%d] FILE: %s SIZE: %d bytes",
                 op->id, op->item, size);
    }

    safe_print(buffer);

    simulate_work(OP_Q2_WORKER_READ);

    exit_worker();
}

static void perform_manager_write(Operation *op) {
    char buffer[256];

    enter_manager(op->id);

    int idx = find_entry(op->item);
    int old_size = directory_entries[idx].size;
    int new_size = old_size + 512;

    directory_entries[idx].size = new_size;

    snprintf(buffer, sizeof(buffer),
             "[Manager-%d] updated %s: %d -> %d bytes",
             op->id, op->item, old_size, new_size);
    safe_print(buffer);

    simulate_work(OP_Q2_MANAGER_HANDLE);

    exit_writer();
}

static void perform_supervisor_write(Operation *op) {
    char buffer[256];

    enter_supervisor(op->id);

    int idx = find_entry(op->item);
    int old_size = directory_entries[idx].size;
    int new_size = old_size + 512;

    directory_entries[idx].size = new_size;

    snprintf(buffer, sizeof(buffer),
             "[Supervisor-%d] updated %s: %d -> %d bytes",
             op->id, op->item, old_size, new_size);
    safe_print(buffer);

    simulate_work(OP_Q2_SUPERVISOR_UPDATE);

    exit_writer();
}

static void *thread_main(void *arg) {
    Operation *op = (Operation *)arg;

    if (op->role == ROLE_WORKER && op->op == OP_READ) {
        perform_read(op);
    } else if (op->role == ROLE_MANAGER && op->op == OP_WRITE) {
        perform_manager_write(op);
    } else if (op->role == ROLE_SUPERVISOR && op->op == OP_WRITE) {
        perform_supervisor_write(op);
    } else {
        safe_print("[Invalid operation] role/op combination is not allowed.");
    }

    return NULL;
}

static Role parse_role(char c) {
    if (c == 'W') {
        return ROLE_WORKER;
    }

    if (c == 'M') {
        return ROLE_MANAGER;
    }

    return ROLE_SUPERVISOR;
}

static OpType parse_op(const char *s) {
    if (strcmp(s, "READ") == 0) {
        return OP_READ;
    }

    return OP_WRITE;
}

int main(void) {
    int num_workers;
    int num_managers;
    int num_supervisors;

    if (scanf("%d %d %d %d",
              &num_workers,
              &num_managers,
              &num_supervisors,
              &num_ops) != 4) {
        fprintf(stderr,
                "Usage: <num_workers> <num_managers> <num_supervisors> <num_ops>\n");
        return 1;
    }

    operations = (Operation *)malloc(sizeof(Operation) * num_ops);
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * num_ops);

    directory_capacity = num_ops + 10;
    directory_entries = (DirectoryEntry *)malloc(sizeof(DirectoryEntry) * directory_capacity);

    if (!operations || !threads || !directory_entries) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(operations);
        free(threads);
        free(directory_entries);
        return 1;
    }

    for (int i = 0; i < num_ops; i++) {
        char role_char;
        char op_str[16];
        char item[MAX_NAME_LEN];

        if (scanf(" %c %d %15s %127s",
                  &role_char,
                  &operations[i].id,
                  op_str,
                  item) != 4) {
            fprintf(stderr, "Invalid operation line at index %d.\n", i);
            free(operations);
            free(threads);
            free(directory_entries);
            return 1;
        }

        operations[i].role = parse_role(role_char);
        operations[i].op = parse_op(op_str);
        strcpy(operations[i].item, item);
        operations[i].seq = i;

        get_or_add_entry(item);
    }

    printf("=== Q2 Warehouse Directory Concurrency Control ===\n");
    printf("Workers=%d Managers=%d Supervisors=%d Operations=%d\n",
           num_workers,
           num_managers,
           num_supervisors,
           num_ops);
    fflush(stdout);

    for (int i = 0; i < num_ops; i++) {
        if (pthread_create(&threads[i], NULL, thread_main, &operations[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d.\n", i);
            free(operations);
            free(threads);
            free(directory_entries);
            return 1;
        }
    }

    for (int i = 0; i < num_ops; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("=== Final Directory State ===\n");

    for (int i = 0; i < directory_count; i++) {
        printf("%s %d bytes\n",
               directory_entries[i].name,
               directory_entries[i].size);
    }

    free(operations);
    free(threads);
    free(directory_entries);

    return 0;
}
