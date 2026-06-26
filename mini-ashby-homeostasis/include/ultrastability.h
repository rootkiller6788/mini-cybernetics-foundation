#ifndef ULTRASTABILITY_H
#define ULTRASTABILITY_H

#include "ashby_homeostasis.h"

/* ============================================================================
 * Ultrastability Theory — Ashby's Self-Organizing Principle
 *
 * Ashby, "Design for a Brain" (1952/1960), Chapters 7-8, 16.
 * Ashby, "Principles of the Self-Organizing System" (1962).
 *
 * Ultrastability is the property whereby a system, when disturbed from its
 * equilibrium, changes its internal organization (parameters) until it
 * finds a configuration that restores stability of its essential variables.
 *
 * Key insight: the system has TWO feedback loops operating at different
 * time scales:
 *   Level 1 (reacting part): Fast, continuous feedback using fixed parameters
 *   Level 2 (step-function part): Slow, discrete parameter changes when
 *        Level 1 cannot maintain essential variables within limits.
 *
 * The step-function mechanism does NOT optimize; it randomly samples
 * parameter configurations until one works. This is what Ashby meant by
 * "trial-and-error" — not intelligent search, but blind variation
 * followed by selective retention of stable configurations.
 * ============================================================================ */

/** Level of ultrastability */
typedef enum {
    ULTRASTABLE_NONE     = 0,  /* Simple error-driven regulation only */
    ULTRASTABLE_SINGLE   = 1,  /* One level of step-function (1 field) */
    ULTRASTABLE_DUAL     = 2,  /* Two fields of step-functions */
    ULTRASTABLE_MULTI    = 3,  /* Hierarchical multilevel ultrastability */
    ULTRASTABLE_CONTINUOUS = 4 /* Continuous parameter adaptation */
} UltrastabilityLevel;

/** Parameter field: a set of parameters that can be reconfigured */
typedef struct {
    char* name;
    double* values;          /* Current parameter values */
    double** candidates;     /* Pool of candidate parameter sets */
    int n_params;            /* Number of parameters in this field */
    int n_candidates;        /* Number of alternative configurations */
    int current_index;       /* Which candidate is currently active */
    int* tried_indices;      /* History of indices tried */
    bool* index_valid;       /* Whether each candidate is still viable */
    int n_tried;
    double step_cost;        /* Cost/reward of each step (energy, time) */
    double step_interval;    /* Minimum time between parameter changes */

    /* Selection mechanism */
    bool use_random_search;  /* True = random step, False = sequential scan */
    double success_threshold;/* Performance metric to declare success */
    int max_steps;           /* Maximum steps before declaring failure */
} ParameterField;

/** The two-level ultrastable system */
typedef struct {
    /* Level 2: the step-function part (slow) */
    ParameterField* field_primary;    /* Primary parameter field */
    ParameterField* field_secondary;  /* Secondary field (for dual ultrastability) */
    UltrastabilityLevel level;

    /* Level 1: the reacting part (fast) — represented via parent AshbySystem */

    /* Trigger mechanism */
    double trigger_threshold;/* EV deviation above which step-function activates */
    double recovery_threshold;/* EV deviation below which search stops */
    double max_search_time;  /* Maximum time allowed for parameter search */
    double min_settle_time;  /* Minimum wait after parameter change before judging */

    /* State */
    bool is_searching;       /* True while actively stepping parameters */
    bool has_converged;      /* True once a stable configuration is found */
    double time_searching;   /* Accumulated searching duration */
    double time_stable;      /* Accumulated time in stable state */
    int steps_performed;     /* Total parameter steps executed */
    int successful_configs;  /* Number of configurations that maintained stability */

    /* History of parameter configurations */
    double** config_history; /* History of parameter values tried */
    double* config_score;    /* Stability score for each configuration */
    int n_configs_tried;
    int config_history_capacity;

    /* Adaptation meta-parameters */
    double search_aggressiveness; /* 0=conservative, 1=aggressive step size */
    double hysteresis;            /* Hysteresis in trigger threshold */
} UltrastableSystem;

/* --- Ultrastability API --- */

/* Parameter field */
ParameterField* param_field_create(const char* name, int n_params,
                                    int n_candidates);
void param_field_free(ParameterField* pf);
void param_field_set_candidate(ParameterField* pf, int idx,
                                const double* values);
void param_field_initialize_random(ParameterField* pf,
                                    const double* mins, const double* maxs);
void param_field_step(ParameterField* pf);
void param_field_step_to(ParameterField* pf, int index);
int param_field_best_untried(ParameterField* pf,
                              double (*evaluate)(const double*, int));
double* param_field_current(const ParameterField* pf);
void param_field_mark_success(ParameterField* pf);
void param_field_mark_failure(ParameterField* pf);
bool param_field_exhausted(const ParameterField* pf);
void param_field_reset(ParameterField* pf);
void param_field_print(const ParameterField* pf);

/* Ultrastable system */
UltrastableSystem* ultrastable_system_create(UltrastabilityLevel level);
void ultrastable_system_free(UltrastableSystem* us);
void ultrastable_system_set_trigger(UltrastableSystem* us,
                                     double trigger, double recovery);
void ultrastable_system_set_search_params(UltrastableSystem* us,
    double max_time, double settle_time, double aggressiveness);
bool ultrastable_system_should_step(const UltrastableSystem* us,
                                     double current_deviation);
void ultrastable_system_execute_step(UltrastableSystem* us);
bool ultrastable_system_should_stop(const UltrastableSystem* us,
                                     double current_deviation);
void ultrastable_system_record_config(UltrastableSystem* us,
                                       const double* params, double score);
bool ultrastable_system_is_stable(const UltrastableSystem* us);
double ultrastable_system_adaptation_efficiency(const UltrastableSystem* us);
void ultrastable_system_print(const UltrastableSystem* us);

/* Analysis */
double ultrastable_search_complexity(int n_params, int values_per_param,
                                      double target_probability);
double ultrastable_expected_search_time(const ParameterField* pf,
                                         double eval_time_per_step);
int ultrastable_count_viable_configs(const ParameterField* pf,
                                      double (*check)(const double*, int),
                                      double threshold);
void ultrastable_parameter_sensitivity(const ParameterField* pf,
    double (*evaluate)(const double*, int), double* sensitivity);

#endif /* ULTRASTABILITY_H */