#include "vna_core.h"
#include "vna_cellular.h"
#include "vna_rule.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * Example: Traffic Flow Modeling with Cellular Automata
 *
 * The Nagel-Schreckenberg (1992) model simulates highway traffic using a 1D CA.
 * This is a canonical application of CA to real-world systems.
 *
 * Rule: 184 + velocity-dependent extensions (Nagel-Schreckenberg)
 * States: 0 (empty) to V_max (vehicle with velocity v)
 *
 * Key concepts:
 *   - Free flow → congested phase transition at critical density
 *   - Fundamental diagram: flux = density × velocity
 *   - Jam propagation (stop-and-go waves)
 *
 * Reference: Nagel & Schreckenberg (1992), J. Phys. I France 2, 2221.
 * ============================================================================ */

/* Nagel-Schreckenberg model parameters */
#define ROAD_LENGTH      100
#define MAX_VELOCITY     5
#define MAX_STEPS        200
#define P_SLOW           0.3   /* Randomization probability */

/* NS model state: 0=empty, 1-5=velocity (state = velocity) */
#define NS_STATES        6

typedef struct {
    int     length;
    int*    velocity;       /* velocity[i] for car at position i */
    int*    position;       /* Whether position i is occupied */
    double  density;
    double  avg_velocity;
    double  flux;
    int64_t total_steps;
} NSTrafficModel;

/* Initialize the traffic model */
NSTrafficModel* ns_traffic_create(int length, double density, unsigned int seed) {
    NSTrafficModel* model = (NSTrafficModel*)calloc(1, sizeof(NSTrafficModel));
    if (!model) return NULL;

    model->length = length;
    model->velocity = (int*)calloc(length, sizeof(int));
    model->position = (int*)calloc(length, sizeof(int));
    if (!model->velocity || !model->position) {
        free(model->velocity); free(model->position); free(model);
        return NULL;
    }

    /* Place vehicles randomly */
    unsigned int local_seed = seed ? seed : 777;
    for (int i = 0; i < length; i++) {
        double r = (double)((local_seed = local_seed * 1103515245 + 12345) &
                   0x7fffffff) / 0x7fffffff;
        if (r < density) {
            model->position[i] = 1;
            model->velocity[i] = (int)((local_seed = local_seed * 1103515245 + 12345) %
                                       (MAX_VELOCITY + 1)) & 0x7fffffff;
            model->velocity[i] = rand() % (MAX_VELOCITY + 1);
        }
    }
    model->density = density;
    return model;
}

void ns_traffic_free(NSTrafficModel* model) {
    if (!model) return;
    free(model->velocity);
    free(model->position);
    free(model);
}

/* Compute distance to next vehicle ahead */
static int distance_to_next(NSTrafficModel* model, int pos) {
    for (int d = 1; d <= model->length; d++) {
        int ahead = (pos + d) % model->length;
        if (model->position[ahead]) return d - 1;
    }
    return model->length; /* No car ahead (should not happen if density > 0) */
}

/* One step of Nagel-Schreckenberg dynamics */
void ns_traffic_step(NSTrafficModel* model, unsigned int seed) {
    unsigned int local_seed = seed;
    int* new_velocity = (int*)calloc(model->length, sizeof(int));
    int* new_position = (int*)calloc(model->length, sizeof(int));

    /* Step 1: Acceleration — increase velocity by 1 (up to V_max) */
    for (int i = 0; i < model->length; i++) {
        if (!model->position[i]) continue;
        int v = model->velocity[i];
        if (v < MAX_VELOCITY) v++;
        new_velocity[i] = v;
    }

    /* Step 2: Deceleration — avoid collision */
    for (int i = 0; i < model->length; i++) {
        if (!model->position[i]) continue;
        int gap = distance_to_next(model, i);
        if (new_velocity[i] > gap) new_velocity[i] = gap;
    }

    /* Step 3: Randomization — slow down with probability p */
    for (int i = 0; i < model->length; i++) {
        if (!model->position[i] || new_velocity[i] == 0) continue;
        double r = (double)((local_seed = local_seed * 1103515245 + 12345) &
                   0x7fffffff) / 0x7fffffff;
        if (r < P_SLOW && new_velocity[i] > 0) new_velocity[i]--;
    }

    /* Step 4: Move vehicles */
    for (int i = 0; i < model->length; i++) {
        if (!model->position[i]) continue;
        int new_pos = (i + new_velocity[i]) % model->length;
        new_position[new_pos] = 1;
        model->velocity[i] = new_velocity[i];
    }

    /* Update positions */
    memcpy(model->position, new_position, model->length * sizeof(int));

    /* Compute statistics */
    int total_velocity = 0, car_count = 0;
    model->flux = 0.0;
    for (int i = 0; i < model->length; i++) {
        if (model->position[i]) {
            total_velocity += model->velocity[i];
            car_count++;
            model->flux += model->velocity[i];
        }
    }
    model->avg_velocity = car_count > 0 ?
        (double)total_velocity / car_count : 0.0;
    model->flux = model->flux / model->length;
    model->density = (double)car_count / model->length;
    model->total_steps++;

    free(new_velocity);
    free(new_position);
}

int main(void) {
    printf("=== Nagel-Schreckenberg Traffic Flow Model ===\n\n");

    /* Fundamental diagram: scan densities from 0.05 to 0.95 */
    printf("1. Fundamental Diagram (flux vs. density)\n");
    printf("   %-8s  %-12s  %-12s\n", "Density", "Avg Velocity", "Flux");
    printf("   %-8s  %-12s  %-12s\n", "-------", "-----------", "----------");

    for (int d = 1; d <= 19; d += 2) {
        double density = d * 0.05;
        NSTrafficModel* model = ns_traffic_create(ROAD_LENGTH, density,
                                                   (unsigned int)(d * 100));

        /* Warm up */
        for (int t = 0; t < 100; t++)
            ns_traffic_step(model, (unsigned int)(t + 1000));

        /* Measure steady state */
        double avg_v = 0.0, avg_flux = 0.0;
        int measure_steps = 100;
        for (int t = 0; t < measure_steps; t++) {
            ns_traffic_step(model, (unsigned int)(t + 2000));
            avg_v += model->avg_velocity;
            avg_flux += model->flux;
        }
        avg_v /= measure_steps;
        avg_flux /= measure_steps;

        printf("   %.2f      %-12.3f  %-12.6f\n", density, avg_v, avg_flux);
        ns_traffic_free(model);
    }

    /* Time series at critical density */
    printf("\n2. Time Series at Critical Density (ρ = 0.20)\n");
    NSTrafficModel* crit = ns_traffic_create(ROAD_LENGTH, 0.20, 555);

    printf("   Warm-up phase...\n");
    for (int t = 0; t < 50; t++)
        ns_traffic_step(crit, (unsigned int)(t + 3000));

    printf("   %-6s  %-12s  %-12s  %-30s\n",
           "Step", "Velocity", "Flux", "Road (0=empty, 1-5=speed)");
    for (int t = 0; t < 20; t++) {
        ns_traffic_step(crit, (unsigned int)(t + 4000));
        printf("   %-6lld  %-12.3f  %-12.6f  ",
               (long long)crit->total_steps,
               crit->avg_velocity, crit->flux);

        /* Print road visualization */
        for (int i = 0; i < ROAD_LENGTH; i++) {
            if (crit->position[i])
                putchar('0' + crit->velocity[i]);
            else
                putchar('.');
        }
        putchar('\n');
    }
    ns_traffic_free(crit);

    /* Jam detection at high density */
    printf("\n3. Congestion Analysis (ρ = 0.50)\n");
    NSTrafficModel* jam = ns_traffic_create(ROAD_LENGTH, 0.50, 999);

    for (int t = 0; t < 100; t++)
        ns_traffic_step(jam, (unsigned int)(t + 5000));

    /* Count stopped vehicles */
    int stopped = 0;
    for (int i = 0; i < ROAD_LENGTH; i++)
        if (jam->position[i] && jam->velocity[i] == 0) stopped++;

    printf("   Stopped vehicles: %d/%d (%.1f%%)\n",
           stopped, (int)(ROAD_LENGTH * jam->density),
           100.0 * stopped / (ROAD_LENGTH * jam->density));
    printf("   Jam indicator (velocity < 1): %d vehicles\n", stopped);

    /* Detect jam waves: regions with ≥3 consecutive stopped cars */
    int jam_waves = 0;
    for (int i = 0; i < ROAD_LENGTH; i++) {
        if (jam->position[i] && jam->velocity[i] == 0) {
            int run = 1;
            for (int j = 1; j < 5 && i + j < ROAD_LENGTH; j++) {
                if (jam->position[(i + j) % ROAD_LENGTH] &&
                    jam->velocity[(i + j) % ROAD_LENGTH] == 0)
                    run++;
                else break;
            }
            if (run >= 3) {
                jam_waves++;
                i += run; /* Skip past this jam */
            }
        }
    }
    printf("   Detected %d jam waves (≥3 consecutive stopped cars)\n", jam_waves);
    ns_traffic_free(jam);

    printf("\n[Traffic flow model demonstration complete]\n");
    return 0;
}
