#include "ultrastability.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PI 3.14159265358979323846

static double urand_us(void) { return (double)rand() / (double)RAND_MAX; }

/* ===== ParameterField ===== */
ParameterField* param_field_create(const char* name, int n_params, int n_candidates) {
    ParameterField* pf = (ParameterField*)calloc(1, sizeof(ParameterField));
    pf->name = strdup(name);
    pf->n_params = n_params;
    pf->n_candidates = n_candidates;
    pf->values = (double*)calloc(n_params, sizeof(double));
    pf->candidates = (double**)malloc(n_candidates * sizeof(double*));
    for (int i = 0; i < n_candidates; i++)
        pf->candidates[i] = (double*)calloc(n_params, sizeof(double));
    pf->current_index = 0;
    pf->tried_indices = (int*)malloc(n_candidates * sizeof(int));
    pf->index_valid = (bool*)malloc(n_candidates * sizeof(bool));
    for (int i = 0; i < n_candidates; i++) { pf->tried_indices[i] = -1; pf->index_valid[i] = true; }
    pf->n_tried = 0; pf->step_cost = 1.0; pf->step_interval = 1.0;
    pf->use_random_search = true; pf->success_threshold = 0.1; pf->max_steps = 100;
    return pf;
}

void param_field_free(ParameterField* pf) {
    if (!pf) return;
    free(pf->name); free(pf->values);
    for (int i = 0; i < pf->n_candidates; i++) free(pf->candidates[i]);
    free(pf->candidates); free(pf->tried_indices); free(pf->index_valid); free(pf);
}

void param_field_set_candidate(ParameterField* pf, int idx, const double* values) {
    if (!pf || idx < 0 || idx >= pf->n_candidates) return;
    memcpy(pf->candidates[idx], values, pf->n_params * sizeof(double));
}

void param_field_initialize_random(ParameterField* pf, const double* mins, const double* maxs) {
    if (!pf) return;
    for (int i = 0; i < pf->n_candidates; i++)
        for (int j = 0; j < pf->n_params; j++)
            pf->candidates[i][j] = mins[j] + urand_us() * (maxs[j] - mins[j]);
    memcpy(pf->values, pf->candidates[0], pf->n_params * sizeof(double));
    pf->current_index = 0;
}

void param_field_step(ParameterField* pf) {
    if (!pf || pf->n_candidates == 0) return;
    pf->tried_indices[pf->n_tried] = pf->current_index;
    pf->n_tried++;
    if (pf->use_random_search) {
        int* untried = (int*)malloc(pf->n_candidates * sizeof(int));
        int n_untried = 0;
        for (int i = 0; i < pf->n_candidates; i++)
            if (pf->index_valid[i]) untried[n_untried++] = i;
        if (n_untried > 0) {
            int pick = rand() % n_untried;
            pf->current_index = untried[pick];
        } else {
            pf->current_index = (pf->current_index + 1) % pf->n_candidates;
        }
        free(untried);
    } else {
        /* Sequential scan with wrap-around */
        int start = pf->current_index;
        pf->current_index = (pf->current_index + 1) % pf->n_candidates;
        while (pf->current_index != start && !pf->index_valid[pf->current_index])
            pf->current_index = (pf->current_index + 1) % pf->n_candidates;
    }
    memcpy(pf->values, pf->candidates[pf->current_index], pf->n_params * sizeof(double));
}

void param_field_step_to(ParameterField* pf, int index) {
    if (!pf || index < 0 || index >= pf->n_candidates) return;
    pf->current_index = index;
    memcpy(pf->values, pf->candidates[index], pf->n_params * sizeof(double));
}

int param_field_best_untried(ParameterField* pf, double (*evaluate)(const double*, int)) {
    if (!pf || !evaluate) return -1;
    int best = -1;
    double best_score = 1e100;
    for (int i = 0; i < pf->n_candidates; i++) {
        if (!pf->index_valid[i]) continue;
        bool tried = false;
        for (int j = 0; j < pf->n_tried; j++)
            if (pf->tried_indices[j] == i) { tried = true; break; }
        if (tried) continue;
        double score = evaluate(pf->candidates[i], pf->n_params);
        if (score < best_score) { best_score = score; best = i; }
    }
    return best;
}

double* param_field_current(const ParameterField* pf) { return pf ? pf->values : NULL; }

void param_field_mark_success(ParameterField* pf) {
    if (!pf) return;
    pf->index_valid[pf->current_index] = true;
}

void param_field_mark_failure(ParameterField* pf) {
    if (!pf) return;
    pf->index_valid[pf->current_index] = false;
}

bool param_field_exhausted(const ParameterField* pf) {
    if (!pf) return true;
    for (int i = 0; i < pf->n_candidates; i++)
        if (pf->index_valid[i]) return false;
    return true;
}

void param_field_reset(ParameterField* pf) {
    if (!pf) return;
    for (int i = 0; i < pf->n_candidates; i++) pf->index_valid[i] = true;
    pf->n_tried = 0; pf->current_index = 0;
    memcpy(pf->values, pf->candidates[0], pf->n_params * sizeof(double));
}

void param_field_print(const ParameterField* pf) {
    if (!pf) return;
    printf("ParameterField '%s': %d params, %d candidates, current=%d\n",
           pf->name, pf->n_params, pf->n_candidates, pf->current_index);
    printf("  Values: ");
    for (int j = 0; j < pf->n_params; j++) printf("%.4f ", pf->values[j]);
    printf("\n  Tried: %d/%d, exhausted=%s\n", pf->n_tried, pf->n_candidates,
           param_field_exhausted(pf) ? "yes" : "no");
}

/* ===== UltrastableSystem ===== */
UltrastableSystem* ultrastable_system_create(UltrastabilityLevel level) {
    UltrastableSystem* us = (UltrastableSystem*)calloc(1, sizeof(UltrastableSystem));
    us->level = level;
    us->field_primary = NULL; us->field_secondary = NULL;
    us->trigger_threshold = 0.5; us->recovery_threshold = 0.2;
    us->max_search_time = 100.0; us->min_settle_time = 2.0;
    us->is_searching = false; us->has_converged = false;
    us->time_searching = 0.0; us->time_stable = 0.0;
    us->steps_performed = 0; us->successful_configs = 0;
    us->config_history_capacity = 100;
    us->config_history = (double**)malloc(us->config_history_capacity * sizeof(double*));
    us->config_score = (double*)malloc(us->config_history_capacity * sizeof(double));
    us->n_configs_tried = 0;
    us->search_aggressiveness = 0.5; us->hysteresis = 0.1;
    return us;
}

void ultrastable_system_free(UltrastableSystem* us) {
    if (!us) return;
    if (us->field_primary) param_field_free(us->field_primary);
    if (us->field_secondary) param_field_free(us->field_secondary);
    for (int i = 0; i < us->n_configs_tried; i++) free(us->config_history[i]);
    free(us->config_history); free(us->config_score); free(us);
}

void ultrastable_system_set_trigger(UltrastableSystem* us, double trigger, double recovery) {
    if (!us) return;
    us->trigger_threshold = trigger; us->recovery_threshold = recovery;
}

void ultrastable_system_set_search_params(UltrastableSystem* us, double max_time, double settle_time, double aggressiveness) {
    if (!us) return;
    us->max_search_time = max_time; us->min_settle_time = settle_time;
    us->search_aggressiveness = aggressiveness;
}

bool ultrastable_system_should_step(const UltrastableSystem* us, double current_deviation) {
    if (!us) return false;
    if (param_field_exhausted(us->field_primary)) return false;
    if (us->time_searching >= us->max_search_time) return false;
    double effective_trigger = us->trigger_threshold;
    if (us->has_converged) effective_trigger += us->hysteresis;
    return current_deviation > effective_trigger;
}

void ultrastable_system_execute_step(UltrastableSystem* us) {
    if (!us) return;
    us->is_searching = true;
    us->steps_performed++;
    if (us->field_primary) param_field_step(us->field_primary);
    if (us->level >= ULTRASTABLE_DUAL && us->field_secondary) {
        /* Secondary field steps only when primary is stuck */
        if (param_field_exhausted(us->field_primary))
            param_field_step(us->field_secondary);
    }
}

bool ultrastable_system_should_stop(const UltrastableSystem* us, double current_deviation) {
    if (!us) return true;
    return current_deviation < us->recovery_threshold;
}

void ultrastable_system_record_config(UltrastableSystem* us, const double* params, double score) {
    if (!us || !params) return;
    if (us->n_configs_tried >= us->config_history_capacity) {
        us->config_history_capacity *= 2;
        us->config_history = (double**)realloc(us->config_history, us->config_history_capacity * sizeof(double*));
        us->config_score = (double*)realloc(us->config_score, us->config_history_capacity * sizeof(double));
    }
    /* Find n_params from primary field */
    int np = us->field_primary ? us->field_primary->n_params : 4;
    us->config_history[us->n_configs_tried] = (double*)malloc(np * sizeof(double));
    memcpy(us->config_history[us->n_configs_tried], params, np * sizeof(double));
    us->config_score[us->n_configs_tried] = score;
    us->n_configs_tried++;
}

bool ultrastable_system_is_stable(const UltrastableSystem* us) {
    if (!us) return false;
    return us->has_converged;
}

double ultrastable_system_adaptation_efficiency(const UltrastableSystem* us) {
    if (!us || us->steps_performed == 0) return 0.0;
    return (double)us->successful_configs / us->steps_performed;
}

void ultrastable_system_print(const UltrastableSystem* us) {
    if (!us) return;
    printf("=== Ultrastable System (level=%d) ===\n", us->level);
    printf("Searching: %s, Converged: %s\n",
           us->is_searching ? "yes" : "no", us->has_converged ? "yes" : "no");
    printf("Time: search=%.2f, stable=%.2f\n", us->time_searching, us->time_stable);
    printf("Steps: %d, Configs tried: %d, successful: %d\n",
           us->steps_performed, us->n_configs_tried, us->successful_configs);
    printf("Trigger=%.3f, Recovery=%.3f, Efficiency=%.3f\n",
           us->trigger_threshold, us->recovery_threshold,
           ultrastable_system_adaptation_efficiency(us));
    if (us->field_primary) param_field_print(us->field_primary);
}

/* ===== Analysis ===== */
double ultrastable_search_complexity(int n_params, int values_per_param, double target_probability) {
    /* Number of configurations in the search space */
    double total = pow((double)values_per_param, (double)n_params);
    /* Expected trials for target probability of success */
    if (target_probability >= 1.0) return total;
    if (target_probability <= 0.0) return total;
    return log(1.0 - target_probability) / log(1.0 - 1.0 / total);
}

double ultrastable_expected_search_time(const ParameterField* pf, double eval_time_per_step) {
    if (!pf) return 0.0;
    int valid = 0;
    for (int i = 0; i < pf->n_candidates; i++)
        if (pf->index_valid[i]) valid++;
    if (valid == 0) return 1e100;
    /* Expected steps = sum of geometric distribution mean */
    double expected_steps = 0.0;
    int remaining = valid;
    for (int i = 0; i < pf->n_candidates && remaining > 0; i++) {
        expected_steps += (double)pf->n_candidates / remaining;
        remaining--;
    }
    return expected_steps * eval_time_per_step;
}

int ultrastable_count_viable_configs(const ParameterField* pf, double (*check)(const double*, int), double threshold) {
    if (!pf || !check) return 0;
    int count = 0;
    for (int i = 0; i < pf->n_candidates; i++) {
        double score = check(pf->candidates[i], pf->n_params);
        if (score <= threshold) count++;
    }
    return count;
}

void ultrastable_parameter_sensitivity(const ParameterField* pf, double (*evaluate)(const double*, int), double* sensitivity) {
    if (!pf || !evaluate || !sensitivity) return;
    double base_score = evaluate(pf->values, pf->n_params);
    double* perturbed = (double*)malloc(pf->n_params * sizeof(double));
    double h = 1e-4;
    for (int j = 0; j < pf->n_params; j++) {
        memcpy(perturbed, pf->values, pf->n_params * sizeof(double));
        perturbed[j] += h;
        double score_up = evaluate(perturbed, pf->n_params);
        sensitivity[j] = (score_up - base_score) / h;
    }
    free(perturbed);
}