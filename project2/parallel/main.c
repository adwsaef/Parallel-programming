#include "common/io.h"
#include "common/sumset.h"
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Check system function for failure.
void SYS_OK(int res)
{
    if (res)
        exit(1);
}

void SYS_OK_ptr(void* res)
{
    if (res == NULL)
        exit(1);
}

#define stack_size 10000
#define max_thread 64

struct args {
    Sumset* a;
    Sumset* b;
    Sumset tmp; // Resrved memory to alloc next sumset.
};

int thread_cnt;
long long all_threads_free; // Constant which allows checking if all threads are free.

// Each thread has some private data, which can be accessed from any thread to set its initial data.
InputData input_data[max_thread];
struct args data[max_thread][stack_size];
int stack_lock[max_thread];
int range_to_process[max_thread][2];
int thread_arg[max_thread]; // every thread gets its numer as argument.

pthread_t mythreads[max_thread];
pthread_mutex_t mutex; // used for protection
pthread_cond_t wake_up_thread[max_thread];

// Mutex must be locked to mark thread as not free. Thread may be marked as free any time.
_Atomic unsigned long long thread_used; // i-th bit is on - ith thread is free


void init_state(int from, int to, int left_range, int right_range, int my_top_stack)
{

    stack_lock[to] = my_top_stack;

    for (int i = 0; i <= stack_lock[to]; ++i) {
        if (data[to][i].a->sum > data[to][i].b->sum) {
            Sumset* tmp = data[to][i].a;
            data[to][i].a = data[to][i].b;
            data[to][i].b = tmp;
        }

        data[to][i].tmp = data[from][i].tmp;
        data[to][i].tmp.prev = data[to][i].a;

        data[to][i + 1].a = &data[to][i].tmp;
        data[to][i + 1].b = data[to][i].b;
    }

    range_to_process[to][0] = left_range;
    range_to_process[to][1] = right_range;
}

// Thread private variables used in rec.
_Thread_local int thread_dlmit;
_Thread_local int thread_num;
_Thread_local int thread_operation_cnt;
_Thread_local int thread_stack_lock;
_Thread_local Solution thread_res;
_Thread_local InputData thread_input;

#define consider_giving_tasks(left, right, my_top_stack)                                            \
    {                                                                                               \
        if (atomic_load(&thread_used) != 0 && left != right) {                                      \
                                                                                                    \
            if (((my_top_stack <= 2) || (right - left > 20) ||                                      \
                                                                                                    \
                    ((thread_operation_cnt % 10000 == 0) && (right - left > 10))                    \
                    || (thread_operation_cnt % 1000000 == 0))                                       \
                && pthread_mutex_trylock(&mutex) == 0) {                                            \
                                                                                                    \
                int ile = __builtin_popcount(thread_used);                                          \
                                                                                                    \
                if (ile > 0) {                                                                      \
                    if (ile > right - left)                                                         \
                        ile = right - left;                                                         \
                    int dist = (right - left + 1) / (ile + 1);                                      \
                    for (int i = 0; i < ile; ++i) {                                                 \
                                                                                                    \
                        int next_thread = __builtin_ctzll(thread_used);                             \
                        int left_range = left;                                                      \
                        int right_range = left_range + dist - 1;                                    \
                        atomic_fetch_xor(&thread_used, (1ll << next_thread));                       \
                                                                                                    \
                        init_state(thread_num, next_thread, left_range, right_range, my_top_stack); \
                        left = right_range + 1;                                                     \
                        SYS_OK(pthread_cond_signal(&wake_up_thread[next_thread]));                  \
                    }                                                                               \
                }                                                                                   \
                SYS_OK(pthread_mutex_unlock(&mutex));                                               \
            }                                                                                       \
        }                                                                                           \
    }

void rec(int my_top_stack, struct args* stack)
{
    Sumset* a = stack->a;
    Sumset* b = stack->b;

    if (!is_sumset_intersection_trivial(a, b)) {
        if ((a->sum == b->sum)
            && (get_sumset_intersection_size(a, b) == 2)) { // s(a) ∩ s(b) = {0, ∑b}.

            if (b->sum > thread_res.sum) {
                solution_build(&thread_res, &thread_input, a, b);
            }
        }
        return;
    }
    int right = thread_dlmit;
    if (a->sum > b->sum) {
        int left = b->last - 1;
        while (++left <= right) {
            ++thread_operation_cnt;
            consider_giving_tasks(left, right, my_top_stack);

            if (!does_sumset_contain(a, left)) {
                sumset_add(&stack->tmp, b, left);
                ++stack;
                (stack)->b = &((stack - 1)->tmp);
                (stack)->a = a;
                rec(my_top_stack + 1, stack);
                --stack;
            }
        }
    } else {
        int left = a->last - 1;

        while (++left <= right) {
            ++thread_operation_cnt;
            consider_giving_tasks(left, right, my_top_stack);

            if (!does_sumset_contain(b, left)) {
                sumset_add(&stack->tmp, a, left);
                ++stack;
                (stack)->b = &((stack - 1)->tmp);
                (stack)->a = b;
                rec(my_top_stack + 1, stack);
                --stack;
            }
        }
    }
}

void* worker(void* args)
{
    Solution* current_thread_res = (Solution*)malloc(sizeof(Solution));
    SYS_OK_ptr(current_thread_res);

    solution_init(&thread_res);
    thread_input = input_data[thread_num];
    thread_num = *(int*)(args);
    thread_dlmit = input_data[thread_num].d;
    thread_operation_cnt = 0;

    struct args* const stack = data[thread_num];

    while (true) {

        SYS_OK(pthread_mutex_lock(&mutex));

        while (true) {

            // Check if job really exists, and it isn't random waking up.
            if ((thread_used & (1ll << thread_num)) == 0)
                break;

            if (thread_used == all_threads_free) {
                // No thread is working, wake up all and return result.
                for (int i = 0; i < thread_cnt; ++i) {
                    SYS_OK(pthread_cond_signal(&wake_up_thread[i]));
                }

                SYS_OK(pthread_mutex_unlock(&mutex));
                *current_thread_res = thread_res;
                return current_thread_res;
            }

            SYS_OK(pthread_cond_wait(&wake_up_thread[thread_num], &mutex));
        }
        SYS_OK(pthread_mutex_unlock(&mutex));

        thread_stack_lock = stack_lock[thread_num];
        if (range_to_process[thread_num][0] == 0) {
            rec(thread_stack_lock, &stack[thread_stack_lock]);
        } else {
            for (int i = range_to_process[thread_num][0]; i <= range_to_process[thread_num][1]; ++i) {
                consider_giving_tasks(i, range_to_process[thread_num][1], thread_stack_lock + 1);

                if (!does_sumset_contain(stack[thread_stack_lock].b, i)) {
                    sumset_add(&stack[thread_stack_lock].tmp, stack[thread_stack_lock].a, i);

                    stack[thread_stack_lock + 1].a = &stack[thread_stack_lock].tmp;
                    stack[thread_stack_lock + 1].b = stack[thread_stack_lock].b;
                    rec(thread_stack_lock + 1, &stack[thread_stack_lock + 1]);
                }
            }
        }

        // Mark thread as not working.
        atomic_fetch_xor(&thread_used, (1ll << thread_num));
    }
}

int main()
{
    input_data_read(&input_data[0]);
    thread_cnt = input_data[0].t;

    for (int i = 1; i < thread_cnt; ++i)
        input_data[i] = input_data[0];

    for (int i = 0; i < thread_cnt; ++i) {
        thread_arg[i] = i;
    }

    SYS_OK(pthread_mutex_init(&mutex, NULL));
    for (int i = 0; i < thread_cnt; ++i) {
        thread_used |= (1ll << i);
        SYS_OK(pthread_cond_init(&wake_up_thread[i], NULL));
    }

    all_threads_free = thread_used;

    for (int i = 0; i < thread_cnt; ++i) {
        data[i][0].a = &input_data[i].a_start;
        data[i][0].b = &input_data[i].b_start;
    }

    thread_used ^= 1;

    for (int i = 1; i < thread_cnt; ++i) {
        SYS_OK(pthread_create(&mythreads[i], NULL, &worker, &thread_arg[i]));
    }

    void* thread_res[thread_cnt];

    thread_res[0] = worker(&thread_arg[0]);
    for (int i = 1; i < thread_cnt; ++i)
        pthread_join(mythreads[i], &thread_res[i]);

    Solution* best_res = thread_res[0];

    for (int i = 1; i < thread_cnt; ++i) {
        if (((Solution*)thread_res[i])->sum > best_res[0].sum)
            best_res = thread_res[i];
    }

    solution_print(best_res);

    for (int i = 0; i < thread_cnt; ++i) {
        pthread_cond_destroy(&wake_up_thread[i]);
        free(thread_res[i]);
    }
    pthread_mutex_destroy(&mutex);
}
