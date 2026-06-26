/**
 * @file example_el_farol.c
 * @brief L6: El Farol Bar Problem (Arthur 1994)
 *
 * 100 agents decide whether to attend a bar each week.
 * If >60 attend, it's too crowded (negative utility).
 * Agents use inductive reasoning with bounded rationality.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define N_AGENTS 100
#define N_WEEKS 200
#define THRESHOLD 60
#define N_STRATEGIES 10
#define MEMORY 5

typedef struct {
    int strategies[N_STRATEGIES][1 << MEMORY];
    double scores[N_STRATEGIES];
} agent_t;

int main(void) {
    printf("=== El Farol Bar Problem ===\n\n");

    agent_t agents[N_AGENTS];
    int attendance_history[MEMORY] = {0};
    int attendance[N_WEEKS];
    int i, w, s, j;

    /* Initialize random strategies */
    srand(42);
    for (i = 0; i < N_AGENTS; i++) {
        for (s = 0; s < N_STRATEGIES; s++) {
            for (j = 0; j < (1 << MEMORY); j++) {
                agents[i].strategies[s][j] = rand() % 2;
            }
            agents[i].scores[s] = 0.0;
        }
    }

    /* Run simulation */
    for (w = 0; w < N_WEEKS; w++) {
        /* Build history index */
        int hist_idx = 0;
        for (j = 0; j < MEMORY; j++)
            hist_idx = (hist_idx << 1) | (attendance_history[j] >= THRESHOLD ? 1 : 0);

        int going = 0;
        for (i = 0; i < N_AGENTS; i++) {
            /* Pick best strategy */
            int best_s = 0;
            double best_score = -1e300;
            for (s = 0; s < N_STRATEGIES; s++) {
                if (agents[i].scores[s] > best_score) {
                    best_score = agents[i].scores[s];
                    best_s = s;
                }
            }
            if (agents[i].strategies[best_s][hist_idx] == 1) going++;
        }

        attendance[w] = going;

        /* Update scores: reward = go when not crowded, not go when crowded */
        int crowded = (going >= THRESHOLD) ? 1 : 0;
        for (i = 0; i < N_AGENTS; i++) {
            for (s = 0; s < N_STRATEGIES; s++) {
                int predicted = agents[i].strategies[s][hist_idx];
                int correct = (predicted == 1 && !crowded) || (predicted == 0 && crowded);
                agents[i].scores[s] += correct ? 1.0 : -1.0;
            }
        }

        /* Shift history */
        for (j = MEMORY - 1; j > 0; j--)
            attendance_history[j] = attendance_history[j - 1];
        attendance_history[0] = going;
    }

    /* Print results */
    printf("Week | Attendance | Mean=%d\n", THRESHOLD);
    printf("-----+------------+----------\n");
    double sum = 0.0;
    for (w = 0; w < N_WEEKS; w++) {
        sum += attendance[w];
        if (w % 20 == 0 || w < 5 || w >= N_WEEKS - 5) {
            printf("%4d | %10d | %s\n", w, attendance[w],
                   attendance[w] >= THRESHOLD ? "CROWDED" : "COMFORTABLE");
        }
    }

    double mean = sum / N_WEEKS;
    double var = 0.0;
    for (w = 0; w < N_WEEKS; w++)
        var += (attendance[w] - mean) * (attendance[w] - mean);
    var /= N_WEEKS;

    printf("\nMean attendance: %.1f (optimal: %d)\n", mean, THRESHOLD);
    printf("Std deviation: %.1f\n", sqrt(var));
    printf("Emergent property: agents self-organize around threshold ");
    printf("without coordination.\n");

    return 0;
}
