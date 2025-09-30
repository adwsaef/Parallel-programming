#include "common/io.h"
#include "common/sumset.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#define stack_size 10000 // definitly will be enough and fits.
struct args {
    Sumset* a;
    Sumset* b;
    Sumset tmp; // Memory to save extra sumset (a_with_i or b_with i in ref)
    size_t next_call; // 0 means unprocessed
};

struct args stack[stack_size];
size_t top_stack;

int main()
{
    InputData input_data;
    input_data_read(&input_data);
    // input_data_init(&input_data, 8, 10, (int[]){0}, (int[]){1, 0});

    Solution best_solution;
    solution_init(&best_solution);

    stack[0].a = &input_data.a_start;
    stack[0].b = &input_data.b_start;

    stack[0].next_call = 0;
    ++top_stack;

    while (top_stack) {
        --top_stack;

        if (stack[top_stack].next_call == 0) {

#define posA stack[top_stack].a
#define posB stack[top_stack].b

            if (posA->sum > posB->sum) { // Swap sumsets.

                Sumset* tmp = posA;
                posA = posB;
                posB = tmp;
            }

            if (is_sumset_intersection_trivial(posA, posB)) {
                stack[top_stack].next_call = posA->last;
            } else {
                if ((posA->sum == posB->sum) && (get_sumset_intersection_size(posA, posB) == 2)) { // s(a) ∩ s(b) = {0, ∑b}.
                    if (posB->sum > best_solution.sum)
                        solution_build(&best_solution, &input_data, posA, posB);
                }
                continue;
            }
#undef posA
#undef posB
        }

        if (stack[top_stack].next_call > input_data.d)
            continue;

        if (!does_sumset_contain(stack[top_stack].b, stack[top_stack].next_call)) {
            ++top_stack;

            sumset_add(&stack[top_stack - 1].tmp, stack[top_stack - 1].a, stack[top_stack - 1].next_call);
            stack[top_stack].a = &stack[top_stack - 1].tmp;
            stack[top_stack].b = stack[top_stack - 1].b;
            stack[top_stack].next_call = 0;
            ++stack[top_stack - 1].next_call;
            ++top_stack;

        } else {

            ++stack[top_stack].next_call;
            ++top_stack;
        }
    }

    solution_print(&best_solution);
    return 0;
}
