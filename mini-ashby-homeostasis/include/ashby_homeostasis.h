#ifndef ASHBY_HOMEOSTASIS_H
#define ASHBY_HOMEOSTASIS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * Ashby Homeostasis Framework - Core Types and API
 *
 * Based on the foundational works of W. Ross Ashby:
 *   "Design for a Brain" (1952, 2nd ed. 1960)
 *   "An Introduction to Cybernetics" (1956)
 *   "Principles of the Self-Organizing System" (1962)
 *
 * Homeostasis (Ashby): "The maintenance of a number of variables within
 * physiological limits." The organism's adaptation consists in keeping its
 * essential variables within limits compatible with survival.
 * ============================================================================ */

/* --- Fundamental Enumerations --- */

/** State of a homeostatic system relative to its survival zone */
typedef enum {
    HOMEOSTASIS_SAFE      = 0,  /* All essential variables within bounds */
    HOMEOSTASIS_WARNING   = 1,  /* EV approaching boundaries */
    HOMEOSTASIS_CRITICAL  = 2,  /* At least one EV near limit */
    HOMEOSTASIS_VIOLATED  = 3,  /* At least one EV outside survival zone */
    HOMEOSTASIS_DEAD      = 4   /* Irrecoverable violation of essential bounds */
} HomeostasisStatus;

/** Type of regulatory mechanism employed */
typedef enum {
    REGULATOR_FIXED       = 0,  /* Fixed-response regulator */
    REGULATOR_ERROR_DRIVEN = 1, /* Error-driven (negative feedback) */
    REGULATOR_ADAPTIVE    = 2,  /* Adapts parameters online */
    REGULATOR_ULTRASTABLE = 3,  /* Ultrastable (step-function reconfiguration) */
    REGULATOR_PREDICTIVE  = 4,  /* Model-predictive regulator */
    REGULATOR_LEARNING    = 5   /* Learns regulation from experience */
} RegulatorType;

/** Type of disturbance a system may face */
typedef enum {
    DISTURBANCE_STEP      = 0,  /* Instantaneous step change */
    DISTURBANCE_RAMP      = 1,  /* Linear increase over time */
    DISTURBANCE_SINUSOIDAL = 2, /* Periodic forcing */
    DISTURBANCE_IMPULSE   = 3,  /* Short-duration impulse */
    DISTURBANCE_RANDOM_WALK = 4,/* Brownian-motion-like drift */
    DISTURBANCE_GAUSSIAN  = 5,  /* i.i.d. Gaussian noise */
    DISTURBANCE_CUSTOM    = 6   /* User-defined disturbance function */
} DisturbanceType;

/** Environment classification for regulation difficulty */
typedef enum {
    ENV_BENIGN           = 0,  /* Favorable to system survival */
    ENV_NEUTRAL          = 1,  /* Neither helps nor hinders significantly */
    ENV_CHALLENGING      = 2,  /* Requires substantial regulation */
    ENV_HOSTILE          = 3,  /* Actively threatens system survival */
    ENV_UNKNOWN          = 4   /* Environment properties unknown a priori */
} EnvironmentClass;

/* --- Core Numeric Types --- */

typedef struct {
    double x;
    double y;
} AHVec2;

typedef struct {
    double* data;
    int rows;
    int cols;
} AHMatrix;

typedef struct {
    double* buffer;
    int length;
    int capacity;
    int stride;
} AHTimeSeries;

/* --- Essential Variables --- */

typedef struct {
    char* name;
    double current_value;
    double lower_bound;
    double upper_bound;
    double warning_lower;
    double warning_upper;
    double rate_of_change;
    double acceptable_rate;
    bool is_critical;
    double violation_severity;
} EssentialVariable;

typedef struct {
    EssentialVariable* variables;
    int n_variables;
    int capacity;
    HomeostasisStatus status;
    double survival_volume;
    double current_deviation;
    double max_observed_deviation;
    double time_since_last_violation;
    double total_violation_duration;
} EssentialVariableSet;

/* --- Environment and Disturbance --- */

typedef struct {
    char* name;
    EnvironmentClass classification;
    double complexity;
    double predictability;
    double hostility;
    double* disturbance_vector;
    int n_disturbances;
    DisturbanceType* d_types;
    double* d_params;
    int n_d_params;
    double* noise_amplitude;
} AshbyEnvironment;

/* --- Core Homeostatic System --- */

typedef struct {
    char* name;
    EssentialVariableSet* essential_vars;
    AshbyEnvironment* environment;
    RegulatorType regulator_type;
    double regulatory_capacity;
    double* regulatory_action;
    int n_actions;
    double* state_vector;
    int n_states;
    double time;
    double dt;
    AHMatrix* env_coupling;
    AHMatrix* internal_coupling;
    AHTimeSeries* state_history;
    AHTimeSeries* ev_history;
    int n_adaptations;
    double adaptation_rate;
    bool is_adapting;
} AshbySystem;

/* --- Core API --- */

AshbySystem* ashby_system_create(const char* name, int n_states);
void ashby_system_free(AshbySystem* sys);

EssentialVariableSet* ashby_ev_set_create(void);
void ashby_ev_set_free(EssentialVariableSet* evs);
int ashby_ev_add(EssentialVariableSet* evs, const char* name,
                  double lower, double upper, bool critical);
void ashby_ev_remove(EssentialVariableSet* evs, int idx);
void ashby_ev_update(EssentialVariableSet* evs, int idx, double value,
                      double dt);
HomeostasisStatus ashby_ev_check_status(const EssentialVariableSet* evs);
double ashby_ev_deviation(const EssentialVariableSet* evs);
double ashby_ev_proximity_to_boundary(const EssentialVariableSet* evs, int idx);
bool ashby_ev_is_survivable(const EssentialVariableSet* evs);
void ashby_ev_set_acceptablerate(EssentialVariableSet* evs, int idx, double rate);

AshbyEnvironment* ashby_env_create(const char* name, EnvironmentClass cls);
void ashby_env_free(AshbyEnvironment* env);
void ashby_env_add_disturbance(AshbyEnvironment* env, DisturbanceType type,
                                double* params, int n_params, double noise);
double ashby_env_disturbance_value(const AshbyEnvironment* env,
                                    int d_idx, double time);
void ashby_env_update_disturbances(AshbyEnvironment* env, double time,
                                    double* output, int n_output);

void ashby_system_set_environment(AshbySystem* sys, AshbyEnvironment* env);
void ashby_system_set_coupling(AshbySystem* sys,
                                const double* env_coupling,
                                const double* internal_coupling,
                                int rows, int cols);
void ashby_system_set_regulator(AshbySystem* sys, RegulatorType type);
void ashby_system_step(AshbySystem* sys);
void ashby_system_simulate(AshbySystem* sys, double duration);
void ashby_system_apply_regulation(AshbySystem* sys);
bool ashby_system_is_homeostatic(const AshbySystem* sys);
double ashby_system_homeostatic_efficiency(const AshbySystem* sys);
void ashby_system_print(const AshbySystem* sys);
void ashby_system_print_status(const AshbySystem* sys);

AHTimeSeries* ashby_timeseries_create(int stride);
void ashby_timeseries_free(AHTimeSeries* ts);
void ashby_timeseries_record(AHTimeSeries* ts, double value);
void ashby_timeseries_record_vector(AHTimeSeries* ts,
                                     const double* values, int n);
double ashby_timeseries_mean(const AHTimeSeries* ts);
double ashby_timeseries_variance(const AHTimeSeries* ts);
double ashby_timeseries_autocorrelation(const AHTimeSeries* ts, int lag);
int ashby_timeseries_crossing_count(const AHTimeSeries* ts, double threshold);

AHMatrix* ashby_matrix_create(int rows, int cols);
void ashby_matrix_free(AHMatrix* m);
double ashby_matrix_get(const AHMatrix* m, int r, int c);
void ashby_matrix_set(AHMatrix* m, int r, int c, double v);
AHMatrix* ashby_matrix_mul(const AHMatrix* a, const AHMatrix* b);
void ashby_matrix_vec_mul(const AHMatrix* a, const double* x, double* y);
void ashby_matrix_print(const AHMatrix* m);

double ashby_clamp(double x, double lo, double hi);
double ashby_linear_map(double x, double in_lo, double in_hi,
                         double out_lo, double out_hi);
double ashby_sigmoid_01(double x, double k);
double* ashby_linspace(double start, double end, int n);
void ashby_normalize_vector(double* v, int n);

#endif /* ASHBY_HOMEOSTASIS_H */