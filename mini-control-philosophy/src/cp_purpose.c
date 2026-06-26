#include "cp_purpose.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CP_EPS 1e-12

/* ============================================================================
 * Goal Specification
 * ============================================================================ */

GoalSpec* cp_goal_create(const char* description, int n_dims,
                          const double* targets, const double* tolerances) {
    GoalSpec* goal = (GoalSpec*)calloc(1, sizeof(GoalSpec));
    if (!goal) return NULL;
    goal->description = (description && description[0]) ? strdup(description) : strdup("goal");
    goal->n_dimensions = n_dims;
    goal->priority = 1.0;
    goal->is_active = true;
    goal->is_achieved = false;
    goal->achievement_ratio = 0.0;
    if (n_dims > 0) {
        goal->target_values = (double*)malloc((size_t)n_dims * sizeof(double));
        goal->tolerances    = (double*)malloc((size_t)n_dims * sizeof(double));
        if (goal->target_values && targets)
            memcpy(goal->target_values, targets, (size_t)n_dims * sizeof(double));
        if (goal->tolerances && tolerances)
            memcpy(goal->tolerances, tolerances, (size_t)n_dims * sizeof(double));
    }
    return goal;
}

void cp_goal_free(GoalSpec* goal) {
    if (!goal) return;
    free(goal->description);
    free(goal->target_values);
    free(goal->tolerances);
    free(goal);
}

void cp_goal_set_priority(GoalSpec* goal, double priority) {
    if (goal) goal->priority = (priority < 0.0) ? 0.0 : ((priority > 1.0) ? 1.0 : priority);
}

bool cp_goal_check_satisfaction(const GoalSpec* goal, const double* current_output) {
    if (!goal || !current_output) return false;
    for (int i = 0; i < goal->n_dimensions; i++) {
        double error = fabs(current_output[i] - goal->target_values[i]);
        if (error > goal->tolerances[i]) return false;
    }
    return true;
}

double cp_goal_achievement_ratio(const GoalSpec* goal, const double* current_output) {
    if (!goal || !current_output || goal->n_dimensions == 0) return 0.0;
    double total = 0.0;
    for (int i = 0; i < goal->n_dimensions; i++) {
        double error = fabs(current_output[i] - goal->target_values[i]);
        double tol = goal->tolerances[i];
        if (tol < CP_EPS) tol = 0.01;
        double ratio = (error < tol) ? 1.0 : exp(-error / tol);
        total += ratio;
    }
    return total / goal->n_dimensions;
}

/* ============================================================================
 * Utility Function
 * ============================================================================ */

UtilityFunction* cp_utility_create_quadratic(int n_dims, const double* weights) {
    UtilityFunction* uf = (UtilityFunction*)calloc(1, sizeof(UtilityFunction));
    if (!uf) return NULL;
    uf->n_dimensions = n_dims;
    uf->is_quadratic = true;
    uf->risk_aversion = 1.0;
    uf->discount_factor = 1.0;
    if (n_dims > 0) {
        uf->weights = (double*)malloc((size_t)n_dims * sizeof(double));
        if (uf->weights && weights)
            memcpy(uf->weights, weights, (size_t)n_dims * sizeof(double));
    }
    return uf;
}

UtilityFunction* cp_utility_create_linear(int n_dims, const double* weights) {
    UtilityFunction* uf = cp_utility_create_quadratic(n_dims, weights);
    if (uf) {
        uf->is_quadratic = false;
        uf->is_linear = true;
    }
    return uf;
}

UtilityFunction* cp_utility_create_saturating(int n_dims, const double* weights) {
    UtilityFunction* uf = cp_utility_create_quadratic(n_dims, weights);
    if (uf) {
        uf->is_quadratic = false;
        uf->is_saturating = true;
    }
    return uf;
}

void cp_utility_free(UtilityFunction* uf) {
    if (!uf) return;
    free(uf->weights);
    free(uf);
}

double cp_utility_evaluate(const UtilityFunction* uf, const double* y, const double* r) {
    if (!uf || !y || !r) return 0.0;
    double u = 0.0;
    if (uf->is_quadratic) {
        for (int i = 0; i < uf->n_dimensions; i++) {
            double e = y[i] - r[i];
            u -= uf->weights[i] * e * e;
        }
    } else if (uf->is_linear) {
        for (int i = 0; i < uf->n_dimensions; i++) {
            u += uf->weights[i] * y[i];
        }
    } else if (uf->is_saturating) {
        for (int i = 0; i < uf->n_dimensions; i++) {
            u += uf->weights[i] * tanh(y[i] - r[i]);
        }
    }
    return u;
}

double cp_utility_certainty_equivalent(const UtilityFunction* uf,
                                        const double* y, const double* r) {
    if (!uf) return 0.0;
    double base = cp_utility_evaluate(uf, y, r);
    /* Risk adjustment: CE = E[U] - 0.5 * rho * Var[U] */
    double var_estimate = 0.0;
    for (int i = 0; i < uf->n_dimensions; i++) {
        double e = y[i] - r[i];
        var_estimate += uf->weights[i] * e * e;
    }
    return base - 0.5 * uf->risk_aversion * var_estimate;
}

void cp_utility_set_risk_aversion(UtilityFunction* uf, double gamma) {
    if (uf && gamma > 0) uf->risk_aversion = gamma;
}

/* ============================================================================
 * Teleological System
 * ============================================================================ */

TeleologicalSystem* cp_teleo_create(ControlSystem* sys, int max_goals) {
    TeleologicalSystem* ts = (TeleologicalSystem*)calloc(1, sizeof(TeleologicalSystem));
    if (!ts) return NULL;
    ts->sys = sys;
    if (max_goals > 0)
        ts->goals = (GoalSpec*)calloc((size_t)max_goals, sizeof(GoalSpec));
    ts->n_goals = 0;
    ts->cumulative_utility = 0.0;
    ts->goal_switches = 0;
    ts->history_cap = 1000;
    ts->history_len = 0;
    ts->outcome_history = (double*)calloc((size_t)(ts->history_cap * sys->ny), sizeof(double));
    return ts;
}

void cp_teleo_free(TeleologicalSystem* ts) {
    if (!ts) return;
    /* GoalSpec array is a flat array of structs with owned pointers */
    for (int i = 0; i < ts->n_goals; i++) {
        free(ts->goals[i].description);
        free(ts->goals[i].target_values);
        free(ts->goals[i].tolerances);
    }
    free(ts->goals);
    free(ts->outcome_history);
    free(ts);
}

void cp_teleo_add_goal(TeleologicalSystem* ts, const char* desc,
                        const double* targets, const double* tolerances, int n_dims) {
    if (!ts) return;
    GoalSpec* gs = cp_goal_create(desc, n_dims, targets, tolerances);
    if (!gs) return;
    /* Use placement via struct copy */
    if (ts->n_goals > 0) {
        /* Copy old heap strings to temp before overwrite */
        GoalSpec tmp = ts->goals[ts->n_goals];
        (void)tmp;
    }
    memcpy(&ts->goals[ts->n_goals], gs, sizeof(GoalSpec));
    free(gs); /* free the wrapper but keep the heap strings (now owned by array) */
    ts->n_goals++;
    if (ts->n_goals == 1) ts->active_goal = &ts->goals[0];
}

int cp_teleo_select_goal(TeleologicalSystem* ts, int goal_idx) {
    if (!ts || goal_idx < 0 || goal_idx >= ts->n_goals) return -1;
    ts->active_goal = &ts->goals[goal_idx];
    ts->goal_switches++;
    return goal_idx;
}

void cp_teleo_arbitrate(TeleologicalSystem* ts) {
    if (!ts || ts->n_goals == 0) return;
    /* Select highest-priority unsatisfied goal */
    double best_priority = -1.0;
    int best_idx = -1;
    for (int i = 0; i < ts->n_goals; i++) {
        if (!ts->goals[i].is_active) continue;
        ts->goals[i].is_achieved = cp_goal_check_satisfaction(&ts->goals[i], ts->sys->y);
        if (!ts->goals[i].is_achieved && ts->goals[i].priority > best_priority) {
            best_priority = ts->goals[i].priority;
            best_idx = i;
        }
    }
    if (best_idx >= 0) {
        if (ts->active_goal != &ts->goals[best_idx]) {
            ts->active_goal = &ts->goals[best_idx];
            ts->goal_switches++;
        }
        /* Update reference to match active goal */
        cp_system_set_reference(ts->sys, ts->active_goal->target_values,
                                 ts->active_goal->tolerances);
    }
}

void cp_teleo_step(TeleologicalSystem* ts, double dt) {
    if (!ts || !ts->sys) return;
    cp_teleo_arbitrate(ts);
    cp_system_step(ts->sys, dt);
    cp_teleo_record_outcome(ts, ts->sys->y);
    if (ts->active_goal) {
        ts->active_goal->achievement_ratio =
            cp_goal_achievement_ratio(ts->active_goal, ts->sys->y);
    }
}

void cp_teleo_record_outcome(TeleologicalSystem* ts, const double* outcome) {
    if (!ts || !outcome) return;
    int offset = ts->history_len * ts->sys->ny;
    if (offset + ts->sys->ny > ts->history_cap * ts->sys->ny) {
        /* shift left */
        int shift = ts->sys->ny;
        int total = ts->history_cap * ts->sys->ny;
        memmove(ts->outcome_history, ts->outcome_history + shift,
                (size_t)(total - shift) * sizeof(double));
        offset = (ts->history_cap - 1) * ts->sys->ny;
    } else {
        ts->history_len++;
    }
    for (int i = 0; i < ts->sys->ny; i++)
        ts->outcome_history[offset + i] = outcome[i];
}

void cp_teleo_print(const TeleologicalSystem* ts) {
    if (!ts) { printf("TeleologicalSystem: NULL\n"); return; }
    printf("=== Teleological System ===\n");
    printf("Goals: %d, Active: %d, Switches: %d\n",
           ts->n_goals,
           ts->active_goal ? (int)(ts->active_goal - ts->goals) : -1,
           ts->goal_switches);
    printf("Cumulative utility: %.6f\n", ts->cumulative_utility);
    for (int i = 0; i < ts->n_goals; i++) {
        GoalSpec* g = &ts->goals[i];
        printf("  Goal %d [%s]: priority=%.2f, achieved=%d, ratio=%.4f\n",
               i, g->description, g->priority, g->is_achieved, g->achievement_ratio);
    }
}

/* ============================================================================
 * Purposive Analysis
 * ============================================================================ */

PurposiveAnalysis* cp_purposive_analyze(const TeleologicalSystem* ts) {
    if (!ts) return NULL;
    PurposiveAnalysis* pa = (PurposiveAnalysis*)calloc(1, sizeof(PurposiveAnalysis));
    if (!pa) return NULL;
    pa->mode = ts->sys->teleology;
    pa->has_feedback = (ts->sys->paradigm >= CP_CLOSED_LOOP);
    pa->has_extrapolation = (ts->sys->paradigm >= CP_PREDICTIVE);
    pa->prediction_horizon = pa->has_extrapolation ? 10 : 0;

    /* Compute purpose efficiency */
    double max_util = 0.0;
    for (int i = 0; i < ts->n_goals; i++) {
        if (ts->goals[i].priority > max_util) max_util = ts->goals[i].priority;
    }
    pa->purpose_efficiency = (max_util > CP_EPS)
        ? ts->cumulative_utility / (max_util * (ts->sys->step_count + 1)) : 0.0;

    /* Trajectory deviation */
    if (ts->history_len > 1) {
        pa->trajectory_len = ts->history_len;
        pa->trajectory_deviation = (double*)malloc((size_t)ts->history_len * sizeof(double));
        if (pa->trajectory_deviation && ts->active_goal) {
            for (int t = 0; t < ts->history_len; t++) {
                double dev = 0.0;
                for (int i = 0; i < ts->sys->ny; i++) {
                    double o = ts->outcome_history[t * ts->sys->ny + i];
                    double r = ts->active_goal->target_values[i];
                    dev += (o - r) * (o - r);
                }
                pa->trajectory_deviation[t] = sqrt(dev);
            }
        }
    }
    return pa;
}

void cp_purposive_free(PurposiveAnalysis* pa) {
    if (!pa) return;
    free(pa->trajectory_deviation);
    free(pa);
}

const char* cp_teleological_mode_name(TeleologicalMode mode) {
    switch (mode) {
        case CP_PASSIVE_PURPOSE:    return "Passive";
        case CP_PREDICTIVE_PURPOSE: return "Predictive";
        case CP_ACTIVE_PURPOSE:     return "Active";
        case CP_PURPOSEFUL:         return "Purposeful";
        default:                    return "Unknown";
    }
}

void cp_purposive_print(const PurposiveAnalysis* pa) {
    if (!pa) { printf("PurposiveAnalysis: NULL\n"); return; }
    printf("=== Purposive Analysis ===\n");
    printf("Mode: %s\n", cp_teleological_mode_name(pa->mode));
    printf("Has feedback: %d, Has extrapolation: %d, Horizon: %d\n",
           pa->has_feedback, pa->has_extrapolation, pa->prediction_horizon);
    printf("Purpose efficiency: %.4f\n", pa->purpose_efficiency);
    if (pa->trajectory_deviation && pa->trajectory_len > 0) {
        printf("Final deviation: %.6f\n", pa->trajectory_deviation[pa->trajectory_len - 1]);
    }
}

/* ============================================================================
 * Value of Information
 * ============================================================================ */

double cp_value_of_information(const double* prior, const double** likelihood,
                                const double** utility_matrix,
                                int n_states, int n_actions, int n_observations) {
    /* VOI = E[max_a U(a,s)]_with_info - max_a E[U(a,s)]_without_info */
    if (!prior || !likelihood || !utility_matrix || n_states <= 0 || n_actions <= 0)
        return 0.0;

    /* Without information: max_a sum_s P(s) * U(a,s) */
    double best_no_info = -1e18;
    for (int a = 0; a < n_actions; a++) {
        double expected = 0.0;
        for (int s = 0; s < n_states; s++) {
            expected += prior[s] * utility_matrix[a][s];
        }
        if (expected > best_no_info) best_no_info = expected;
    }

    /* With information: sum_o P(o) * max_a sum_s P(s|o) * U(a,s) */
    double best_with_info = 0.0;
    for (int o = 0; o < n_observations; o++) {
        /* P(o) = sum_s P(s) * P(o|s) */
        double p_o = 0.0;
        for (int s = 0; s < n_states; s++) {
            p_o += prior[s] * likelihood[o][s];
        }
        if (p_o < CP_EPS) continue;

        /* best action for this observation */
        double best_a = -1e18;
        for (int a = 0; a < n_actions; a++) {
            double expected = 0.0;
            for (int s = 0; s < n_states; s++) {
                double posterior = prior[s] * likelihood[o][s] / p_o;
                expected += posterior * utility_matrix[a][s];
            }
            if (expected > best_a) best_a = expected;
        }
        best_with_info += p_o * best_a;
    }
    return best_with_info - best_no_info;
}

double cp_control_premium(double util_closed_loop, double util_open_loop) {
    return util_closed_loop - util_open_loop;
}
