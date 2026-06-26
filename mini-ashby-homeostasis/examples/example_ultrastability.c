#include "ultrastability.h"
#include "ashby_homeostasis.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* Simple evaluation function: how far parameters are from target [0.5, 0.3, 0.2] */
static double eval_params(const double* params, int n) {
    double target[] = {0.5, 0.3, 0.2};
    double dist = 0.0;
    for (int i = 0; i < n; i++) {
        double d = params[i] - target[i];
        dist += d * d;
    }
    return sqrt(dist);
}

int main(void) {
    printf("=== Ultrastability Demonstration ===\n\n");
    printf("Ashby's concept of ultrastability: a system with two\n");
    printf("feedback loops - a fast reacting part and a slow\n");
    printf("step-function part that changes parameters when the\n");
    printf("fast loop cannot maintain essential variables.\n\n");

    srand(12345);

    /* Create an ultrastable system with 3 parameters, 50 candidates */
    UltrastableSystem* us = ultrastable_system_create(ULTRASTABLE_SINGLE);
    us->field_primary = param_field_create("control_params", 3, 50);

    double mins[] = {0.0, 0.0, 0.0};
    double maxs[] = {1.0, 1.0, 1.0};
    param_field_initialize_random(us->field_primary, mins, maxs);

    ultrastable_system_set_trigger(us, 0.5, 0.15);
    ultrastable_system_set_search_params(us, 50.0, 1.0, 0.5);

    printf("Initial parameters: ");
    double* p = param_field_current(us->field_primary);
    for (int i = 0; i < 3; i++) printf("%.4f ", p[i]);
    printf("\nTarget parameters: %.4f %.4f %.4f\n\n", 0.5, 0.3, 0.2);

    printf("Searching for stable configuration...\n");
    int max_iterations = 50;
    for (int iter = 0; iter < max_iterations; iter++) {
        p = param_field_current(us->field_primary);
        double deviation = eval_params(p, 3);

        us->has_converged = ultrastable_system_should_stop(us, deviation);

        if (us->has_converged) {
            printf("\n[Iter %d] CONVERGED! deviation=%.4f\n", iter, deviation);
            printf("Final params: %.4f %.4f %.4f\n", p[0], p[1], p[2]);
            us->successful_configs++;
            break;
        }

        if (ultrastable_system_should_step(us, deviation)) {
            printf("[Iter %d] deviation=%.4f > trigger=%.2f -> STEP\n",
                   iter, deviation, us->trigger_threshold);
            ultrastable_system_execute_step(us);
            ultrastable_system_record_config(us, p, deviation);
            us->time_searching += 1.0;
        } else {
            printf("[Iter %d] deviation=%.4f (acceptable)\n", iter, deviation);
        }
    }

    printf("\nSearch statistics:\n");
    printf("  Steps performed: %d\n", us->steps_performed);
    printf("  Configurations tried: %d\n", us->n_configs_tried);
    printf("  Search space size: %d candidates\n", us->field_primary->n_candidates);
    printf("  Expected search time (1s/eval): %.1f s\n",
           ultrastable_expected_search_time(us->field_primary, 1.0));
    printf("  Viable configs (threshold=0.3): %d\n",
           ultrastable_count_viable_configs(us->field_primary, eval_params, 0.3));
    printf("  Search complexity (95%% success): %.0f trials\n",
           ultrastable_search_complexity(3, 50, 0.95));

    ultrastable_system_free(us);
    printf("\nDemonstration complete.\n");
    return 0;
}