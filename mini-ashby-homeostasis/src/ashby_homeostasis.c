#include "ashby_homeostasis.h"
#include "essential_variables.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <float.h>

/* Forward declarations for functions used before definition */
void ashby_ev_compute_severity(EssentialVariable* ev);

#define PI 3.14159265358979323846
#define DEFAULT_CAPACITY 16
#define HISTORY_DEFAULT_CAP 10000

static double urand(void) { return (double)rand() / (double)RAND_MAX; }
static double gauss_rand(void) {
    double u1 = urand(), u2 = urand();
    return sqrt(-2.0 * log(u1 + 1e-15)) * cos(2.0 * PI * u2);
}

EssentialVariableSet* ashby_ev_set_create(void) {
    EssentialVariableSet* evs = (EssentialVariableSet*)calloc(1, sizeof(EssentialVariableSet));
    evs->capacity = DEFAULT_CAPACITY;
    evs->variables = (EssentialVariable*)calloc(DEFAULT_CAPACITY, sizeof(EssentialVariable));
    evs->n_variables = 0;
    evs->status = HOMEOSTASIS_SAFE;
    evs->survival_volume = 0.0;
    evs->current_deviation = 0.0;
    evs->max_observed_deviation = 0.0;
    evs->time_since_last_violation = 0.0;
    evs->total_violation_duration = 0.0;
    return evs;
}

void ashby_ev_set_free(EssentialVariableSet* evs) {
    if (!evs) return;
    for (int i = 0; i < evs->n_variables; i++) free(evs->variables[i].name);
    free(evs->variables); free(evs);
}

int ashby_ev_add(EssentialVariableSet* evs, const char* name, double lower, double upper, bool critical) {
    if (evs->n_variables >= evs->capacity) {
        evs->capacity *= 2;
        evs->variables = (EssentialVariable*)realloc(evs->variables, evs->capacity * sizeof(EssentialVariable));
        memset(evs->variables + evs->n_variables, 0, (evs->capacity - evs->n_variables) * sizeof(EssentialVariable));
    }
    int idx = evs->n_variables;
    EssentialVariable* ev = &evs->variables[idx];
    ev->name = strdup(name);
    ev->lower_bound = lower; ev->upper_bound = upper; ev->is_critical = critical;
    double range = upper - lower;
    ev->warning_lower = lower + 0.2 * range; ev->warning_upper = upper - 0.2 * range;
    ev->current_value = (lower + upper) / 2.0;
    ev->rate_of_change = 0.0; ev->acceptable_rate = range * 0.1;
    ev->violation_severity = 0.0;
    evs->n_variables++;
    return idx;
}

void ashby_ev_remove(EssentialVariableSet* evs, int idx) {
    if (idx < 0 || idx >= evs->n_variables) return;
    free(evs->variables[idx].name);
    for (int i = idx; i < evs->n_variables - 1; i++) evs->variables[i] = evs->variables[i + 1];
    evs->n_variables--;
}

void ashby_ev_update(EssentialVariableSet* evs, int idx, double value, double dt) {
    if (idx < 0 || idx >= evs->n_variables) return;
    EssentialVariable* ev = &evs->variables[idx];
    double prev = ev->current_value;
    ev->current_value = value;
    if (dt > 1e-10) ev->rate_of_change = (value - prev) / dt;
    ashby_ev_compute_severity(ev);
}

HomeostasisStatus ashby_ev_check_status(const EssentialVariableSet* evs) {
    if (evs->n_variables == 0) return HOMEOSTASIS_SAFE;
    int violated = 0, critical = 0, warning = 0;
    for (int i = 0; i < evs->n_variables; i++) {
        const EssentialVariable* ev = &evs->variables[i];
        if (ev->current_value < ev->lower_bound || ev->current_value > ev->upper_bound) {
            violated++;
            if (ev->is_critical) return HOMEOSTASIS_DEAD;
        } else if (ev->current_value < ev->warning_lower || ev->current_value > ev->warning_upper) {
            critical++;
        } else {
            double margin_to_low = ev->current_value - ev->lower_bound;
            double margin_to_high = ev->upper_bound - ev->current_value;
            double min_margin = (margin_to_low < margin_to_high) ? margin_to_low : margin_to_high;
            if (ev->rate_of_change != 0.0) {
                double time_to_boundary = min_margin / fabs(ev->rate_of_change);
                if (time_to_boundary < 5.0) warning++;
            }
        }
    }
    if (violated > 0) return HOMEOSTASIS_VIOLATED;
    if (critical > 0) return HOMEOSTASIS_CRITICAL;
    if (warning > 0) return HOMEOSTASIS_WARNING;
    return HOMEOSTASIS_SAFE;
}

double ashby_ev_deviation(const EssentialVariableSet* evs) {
    if (evs->n_variables == 0) return 0.0;
    double sum_sq = 0.0;
    for (int i = 0; i < evs->n_variables; i++) {
        const EssentialVariable* ev = &evs->variables[i];
        double mid = (ev->lower_bound + ev->upper_bound) / 2.0;
        double range = ev->upper_bound - ev->lower_bound;
        if (range < 1e-10) continue;
        double norm_dev = (ev->current_value - mid) / (range / 2.0);
        sum_sq += norm_dev * norm_dev;
    }
    return sqrt(sum_sq / evs->n_variables);
}

double ashby_ev_proximity_to_boundary(const EssentialVariableSet* evs, int idx) {
    if (idx < 0 || idx >= evs->n_variables) return 1.0;
    const EssentialVariable* ev = &evs->variables[idx];
    double dist_low = ev->current_value - ev->lower_bound;
    double dist_high = ev->upper_bound - ev->current_value;
    double min_dist = (dist_low < dist_high) ? dist_low : dist_high;
    double half_range = (ev->upper_bound - ev->lower_bound) / 2.0;
    if (half_range < 1e-10) return 0.0;
    double proximity = min_dist / half_range;
    if (proximity < 0.0) proximity = 0.0;
    if (proximity > 1.0) proximity = 1.0;
    return proximity;
}

bool ashby_ev_is_survivable(const EssentialVariableSet* evs) {
    if (evs->n_variables == 0) return true;
    for (int i = 0; i < evs->n_variables; i++) {
        const EssentialVariable* ev = &evs->variables[i];
        if (ev->is_critical && (ev->current_value < ev->lower_bound || ev->current_value > ev->upper_bound))
            return false;
    }
    return ashby_ev_check_status(evs) != HOMEOSTASIS_DEAD;
}

void ashby_ev_set_acceptablerate(EssentialVariableSet* evs, int idx, double rate) {
    if (idx < 0 || idx >= evs->n_variables) return;
    evs->variables[idx].acceptable_rate = rate;
}
/* Extended Essential Variable API */
void ashby_ev_set_bounds(EssentialVariable* ev, double lower, double upper) {
    if (!ev) return;
    ev->lower_bound = lower; ev->upper_bound = upper;
    double range = upper - lower;
    ev->warning_lower = lower + 0.2 * range;
    ev->warning_upper = upper - 0.2 * range;
}

void ashby_ev_set_warning_thresholds(EssentialVariable* ev, double warn_low, double warn_high) {
    if (!ev) return;
    ev->warning_lower = warn_low; ev->warning_upper = warn_high;
}

double ashby_ev_midpoint(const EssentialVariable* ev) {
    if (!ev) return 0.0;
    return (ev->lower_bound + ev->upper_bound) / 2.0;
}

double ashby_ev_range(const EssentialVariable* ev) {
    if (!ev) return 0.0;
    return ev->upper_bound - ev->lower_bound;
}

double ashby_ev_normalized_position(const EssentialVariable* ev) {
    if (!ev) return 0.0;
    double range = ev->upper_bound - ev->lower_bound;
    if (range < 1e-10) return 0.0;
    return (ev->current_value - ev->lower_bound) / range;
}

double ashby_ev_margin(const EssentialVariable* ev) {
    if (!ev) return 0.0;
    double dist_low = ev->current_value - ev->lower_bound;
    double dist_high = ev->upper_bound - ev->current_value;
    return (dist_low < dist_high) ? dist_low : dist_high;
}

double ashby_ev_urgency(const EssentialVariable* ev) {
    if (!ev) return 0.0;
    double margin = ashby_ev_margin(ev);
    if (margin <= 0.0) return 1.0;
    double range = ev->upper_bound - ev->lower_bound;
    if (range < 1e-10) return 0.0;
    double norm_margin = margin / (range * 0.5);
    double time_to_boundary = 1e100;
    if (fabs(ev->rate_of_change) > 1e-10) time_to_boundary = margin / fabs(ev->rate_of_change);
    double rate_factor = 1.0;
    if (time_to_boundary < 10.0 && time_to_boundary > 0.0) rate_factor = 10.0 / time_to_boundary;
    double urgency = (1.0 - norm_margin) * rate_factor;
    if (urgency > 1.0) urgency = 1.0;
    return urgency;
}

bool ashby_ev_within_bounds(const EssentialVariable* ev) {
    if (!ev) return false;
    return ev->current_value >= ev->lower_bound && ev->current_value <= ev->upper_bound;
}

bool ashby_ev_within_warning(const EssentialVariable* ev) {
    if (!ev) return false;
    return ev->current_value >= ev->warning_lower && ev->current_value <= ev->warning_upper;
}

double ashby_ev_violation_magnitude(const EssentialVariable* ev) {
    if (!ev) return 0.0;
    if (ev->current_value >= ev->lower_bound && ev->current_value <= ev->upper_bound) return 0.0;
    if (ev->current_value < ev->lower_bound) return ev->lower_bound - ev->current_value;
    return ev->current_value - ev->upper_bound;
}

void ashby_ev_predict_crossing(const EssentialVariable* ev, double dt, double* time_to_cross, int* which_bound) {
    *time_to_cross = 1e100; *which_bound = 0;
    if (!ev || fabs(ev->rate_of_change) < 1e-10) return;
    double margin_low = ev->current_value - ev->lower_bound;
    double margin_high = ev->upper_bound - ev->current_value;
    if (ev->rate_of_change < 0 && margin_low > 0) {
        *time_to_cross = -margin_low / ev->rate_of_change; *which_bound = -1;
    } else if (ev->rate_of_change > 0 && margin_high > 0) {
        *time_to_cross = margin_high / ev->rate_of_change; *which_bound = 1;
    }
    (void)dt;
}

void ashby_ev_compute_severity(EssentialVariable* ev) {
    if (!ev) return;
    double range = ev->upper_bound - ev->lower_bound;
    if (range < 1e-10) { ev->violation_severity = 0.0; return; }
    double mid = (ev->lower_bound + ev->upper_bound) / 2.0;
    double dist_from_mid = fabs(ev->current_value - mid);
    double safe_half = (ev->warning_upper - ev->warning_lower) / 2.0;
    if (safe_half < 1e-10) safe_half = range * 0.3;
    if (dist_from_mid <= safe_half) {
        ev->violation_severity = dist_from_mid / safe_half * 0.5;
    } else if (ev->current_value >= ev->lower_bound && ev->current_value <= ev->upper_bound) {
        double excess = dist_from_mid - safe_half;
        double danger_zone = range / 2.0 - safe_half;
        ev->violation_severity = 0.5 + 0.5 * (excess / danger_zone);
    } else {
        double over = ashby_ev_violation_magnitude(ev);
        ev->violation_severity = 1.0 + over / (range * 0.5);
    }
}

double ashby_ev_set_survival_volume(const EssentialVariableSet* evs) {
    double volume = 1.0;
    for (int i = 0; i < evs->n_variables; i++) {
        double effective_range = evs->variables[i].upper_bound - evs->variables[i].lower_bound;
        if (effective_range < 1e-10) continue;
        volume *= effective_range;
    }
    return volume;
}

double ashby_ev_set_centroid_distance(const EssentialVariableSet* evs) {
    if (evs->n_variables == 0) return 0.0;
    double dist_sq = 0.0;
    for (int i = 0; i < evs->n_variables; i++) {
        const EssentialVariable* ev = &evs->variables[i];
        double mid = (ev->lower_bound + ev->upper_bound) / 2.0;
        double range = ev->upper_bound - ev->lower_bound;
        if (range < 1e-10) continue;
        double norm = (ev->current_value - mid) / (range / 2.0);
        dist_sq += norm * norm;
    }
    return sqrt(dist_sq / evs->n_variables);
}

int ashby_ev_set_count_violated(const EssentialVariableSet* evs) {
    int count = 0;
    for (int i = 0; i < evs->n_variables; i++)
        if (!ashby_ev_within_bounds(&evs->variables[i])) count++;
    return count;
}

int ashby_ev_set_worst_variable(const EssentialVariableSet* evs) {
    if (evs->n_variables == 0) return -1;
    int worst = 0;
    double worst_sev = evs->variables[0].violation_severity;
    for (int i = 1; i < evs->n_variables; i++)
        if (evs->variables[i].violation_severity > worst_sev) {
            worst_sev = evs->variables[i].violation_severity; worst = i;
        }
    return worst;
}

double ashby_ev_set_margin(const EssentialVariableSet* evs) {
    if (evs->n_variables == 0) return 1e100;
    double min_margin = 1e100;
    for (int i = 0; i < evs->n_variables; i++) {
        double m = ashby_ev_margin(&evs->variables[i]);
        if (m < min_margin) min_margin = m;
    }
    return min_margin;
}

double ashby_ev_set_criticality_index(const EssentialVariableSet* evs) {
    if (evs->n_variables == 0) return 0.0;
    double ci = 0.0;
    for (int i = 0; i < evs->n_variables; i++) {
        double s = evs->variables[i].violation_severity;
        double w = evs->variables[i].is_critical ? 3.0 : 1.0;
        ci += w * s;
    }
    return ci / evs->n_variables;
}

void ashby_ev_set_classify(const EssentialVariableSet* evs, int* safe_count, int* warning_count, int* critical_count, int* violated_count) {
    *safe_count = *warning_count = *critical_count = *violated_count = 0;
    for (int i = 0; i < evs->n_variables; i++) {
        const EssentialVariable* ev = &evs->variables[i];
        if (!ashby_ev_within_bounds(ev)) (*violated_count)++;
        else if (!ashby_ev_within_warning(ev)) (*critical_count)++;
        else if (ev->violation_severity > 0.3) (*warning_count)++;
        else (*safe_count)++;
    }
}

void ashby_ev_set_synchronize(EssentialVariableSet* evs, const double* values, int n) {
    int m = (n < evs->n_variables) ? n : evs->n_variables;
    for (int i = 0; i < m; i++) {
        evs->variables[i].current_value = values[i];
        ashby_ev_compute_severity(&evs->variables[i]);
    }
}

bool ashby_ev_set_is_survivable_fast(const EssentialVariableSet* evs) {
    for (int i = 0; i < evs->n_variables; i++) {
        const EssentialVariable* ev = &evs->variables[i];
        if (!ev->is_critical) continue;
        if (ev->current_value < ev->lower_bound || ev->current_value > ev->upper_bound) return false;
    }
    return true;
}

/* EV Statistics */
EVStatistics* ashby_ev_statistics_create(void) {
    EVStatistics* s = (EVStatistics*)calloc(1, sizeof(EVStatistics));
    s->min_observed = 1e100; s->max_observed = -1e100;
    return s;
}

void ashby_ev_statistics_free(EVStatistics* s) { free(s); }

void ashby_ev_statistics_update(EVStatistics* s, const EssentialVariable* ev, double dt) {
    if (!s || !ev) return;
    double x = ev->current_value;
    s->n_samples++;
    double delta = x - s->mean;
    s->mean += delta / s->n_samples;
    double delta2 = x - s->mean;
    s->variance = ((s->n_samples - 1) * s->variance + delta * delta2) / s->n_samples;
    if (s->variance < 0.0) s->variance = 0.0;
    if (x < s->min_observed) s->min_observed = x;
    if (x > s->max_observed) s->max_observed = x;
    if (!ashby_ev_within_bounds(ev)) {
        s->n_violations++;
        s->fractional_time_violated += dt;
        double mag = ashby_ev_violation_magnitude(ev);
        s->integral_abs_deviation += mag * dt;
        if (mag > s->worst_case_distance) s->worst_case_distance = mag;
    }
    double dev = x - s->mean;
    if (s->n_samples > 2) {
        s->skewness = (dev*dev*dev) / (s->variance*sqrt(s->variance) * s->n_samples + 1e-10);
        s->kurtosis = (dev*dev*dev*dev) / (s->variance*s->variance * s->n_samples + 1e-10) - 3.0;
    }
}

void ashby_ev_statistics_finalize(EVStatistics* s) {
    if (!s || s->n_samples == 0) return;
    if (s->variance < 1e-10) s->variance = 0.0;
}

void ashby_ev_statistics_print(const EVStatistics* s) {
    if (!s) return;
    printf("EV Statistics: n=%d, mean=%.4f, var=%.4f\n", s->n_samples, s->mean, s->variance);
    printf("  Range: [%.4f, %.4f]\n", s->min_observed, s->max_observed);
    printf("  Violations: %d (%.2f%%), worst=%.4f\n", s->n_violations, s->fractional_time_violated*100.0, s->worst_case_distance);
    printf("  Shape: skew=%.4f, kurt=%.4f\n", s->skewness, s->kurtosis);
}

EVTrajectory* ashby_ev_trajectory_create(int n_vars, int capacity) {
    EVTrajectory* traj = (EVTrajectory*)calloc(1, sizeof(EVTrajectory));
    traj->n_variables = n_vars; traj->capacity = capacity; traj->n_timepoints = 0;
    traj->trajectories = (double**)malloc(n_vars * sizeof(double*));
    for (int i = 0; i < n_vars; i++)
        traj->trajectories[i] = (double*)malloc(capacity * sizeof(double));
    traj->time_points = (double*)malloc(capacity * sizeof(double));
    traj->is_alive = (bool*)malloc(capacity * sizeof(bool));
    return traj;
}

void ashby_ev_trajectory_free(EVTrajectory* traj) {
    if (!traj) return;
    for (int i = 0; i < traj->n_variables; i++) free(traj->trajectories[i]);
    free(traj->trajectories); free(traj->time_points); free(traj->is_alive); free(traj);
}

void ashby_ev_trajectory_record(EVTrajectory* traj, const EssentialVariableSet* evs, double time) {
    if (!traj || traj->n_timepoints >= traj->capacity) return;
    int t = traj->n_timepoints;
    int n = (evs->n_variables < traj->n_variables) ? evs->n_variables : traj->n_variables;
    for (int i = 0; i < n; i++) traj->trajectories[i][t] = evs->variables[i].current_value;
    traj->time_points[t] = time;
    traj->is_alive[t] = ashby_ev_is_survivable(evs);
    traj->n_timepoints++;
}

int ashby_ev_trajectory_first_violation(const EVTrajectory* traj) {
    if (!traj) return -1;
    for (int t = 0; t < traj->n_timepoints; t++)
        if (!traj->is_alive[t]) return t;
    return -1;
}

double ashby_ev_trajectory_survival_time(const EVTrajectory* traj) {
    int first = ashby_ev_trajectory_first_violation(traj);
    if (first < 0) return traj->n_timepoints > 0 ? traj->time_points[traj->n_timepoints-1] : 0.0;
    return traj->time_points[first];
}

double ashby_ev_trajectory_time_in_violation(const EVTrajectory* traj, int var_idx) {
    if (!traj || var_idx < 0 || var_idx >= traj->n_variables) return 0.0;
    double total = 0.0;
    for (int t = 1; t < traj->n_timepoints; t++)
        if (!traj->is_alive[t]) total += traj->time_points[t] - traj->time_points[t-1];
    return total;
}

void ashby_ev_trajectory_phase_portrait(const EVTrajectory* traj, int var_x, int var_y, double** out_x, double** out_y, int* n_points) {
    if (!traj || var_x < 0 || var_x >= traj->n_variables || var_y < 0 || var_y >= traj->n_variables) {
        *out_x = NULL; *out_y = NULL; *n_points = 0; return;
    }
    *n_points = traj->n_timepoints;
    *out_x = (double*)malloc(*n_points * sizeof(double));
    *out_y = (double*)malloc(*n_points * sizeof(double));
    for (int t = 0; t < *n_points; t++) {
        (*out_x)[t] = traj->trajectories[var_x][t];
        (*out_y)[t] = traj->trajectories[var_y][t];
    }
}

void ashby_ev_set_adaptive_bounds(EssentialVariable* ev, double base_lower, double base_upper, double plasticity) {
    if (!ev) return;
    ev->lower_bound = base_lower; ev->upper_bound = base_upper;
    (void)plasticity;
}

void ashby_ev_shift_bounds(EssentialVariable* ev, double shift) {
    if (!ev) return;
    ev->lower_bound += shift; ev->upper_bound += shift;
    ev->warning_lower += shift; ev->warning_upper += shift;
}

void ashby_ev_expand_bounds(EssentialVariable* ev, double factor) {
    if (!ev || factor <= 0.0) return;
    double mid = (ev->lower_bound + ev->upper_bound) / 2.0;
    double half = (ev->upper_bound - ev->lower_bound) / 2.0 * factor;
    ev->lower_bound = mid - half; ev->upper_bound = mid + half;
}

void ashby_ev_reset_to_baseline(EssentialVariable* ev) {
    if (!ev) return;
    double range = ev->upper_bound - ev->lower_bound;
    ev->warning_lower = ev->lower_bound + 0.2 * range;
    ev->warning_upper = ev->upper_bound - 0.2 * range;
}
/* ===== Environment ===== */
AshbyEnvironment* ashby_env_create(const char* name, EnvironmentClass cls) {
    AshbyEnvironment* env = (AshbyEnvironment*)calloc(1, sizeof(AshbyEnvironment));
    env->name = strdup(name); env->classification = cls;
    env->complexity = 0.0; env->predictability = 0.5; env->hostility = 0.0;
    env->disturbance_vector = NULL; env->n_disturbances = 0;
    env->d_types = NULL; env->d_params = NULL; env->n_d_params = 0;
    env->noise_amplitude = NULL;
    return env;
}

void ashby_env_free(AshbyEnvironment* env) {
    if (!env) return;
    free(env->name); free(env->disturbance_vector); free(env->d_types);
    free(env->d_params); free(env->noise_amplitude); free(env);
}

void ashby_env_add_disturbance(AshbyEnvironment* env, DisturbanceType type, double* params, int n_params, double noise) {
    if (!env) return;
    int n = env->n_disturbances;
    env->disturbance_vector = (double*)realloc(env->disturbance_vector, (n+1)*sizeof(double));
    env->d_types = (DisturbanceType*)realloc(env->d_types, (n+1)*sizeof(DisturbanceType));
    env->noise_amplitude = (double*)realloc(env->noise_amplitude, (n+1)*sizeof(double));
    int old = env->n_d_params;
    env->n_d_params += n_params;
    env->d_params = (double*)realloc(env->d_params, env->n_d_params*sizeof(double));
    env->disturbance_vector[n] = 0.0; env->d_types[n] = type; env->noise_amplitude[n] = noise;
    for (int i = 0; i < n_params; i++) env->d_params[old+i] = params[i];
    env->n_disturbances = n+1;
}

double ashby_env_disturbance_value(const AshbyEnvironment* env, int d_idx, double time) {
    if (!env || d_idx < 0 || d_idx >= env->n_disturbances) return 0.0;
    double noise = env->noise_amplitude[d_idx] * gauss_rand();
    int pp = env->n_d_params / env->n_disturbances;
    int poff = d_idx * pp;
    switch (env->d_types[d_idx]) {
        case DISTURBANCE_STEP: return env->d_params[poff] + noise;
        case DISTURBANCE_RAMP: return env->d_params[poff] * time + noise;
        case DISTURBANCE_SINUSOIDAL:
            return env->d_params[poff] * sin(2.0*PI*env->d_params[poff+1]*time + env->d_params[poff+2]) + noise;
        case DISTURBANCE_IMPULSE: {
            double w = env->d_params[poff+2]; if (w < 0.01) w = 0.01;
            double t0 = env->d_params[poff+1];
            return env->d_params[poff] * exp(-(time-t0)*(time-t0)/(2.0*w*w)) + noise;
        }
        case DISTURBANCE_RANDOM_WALK:
            return env->disturbance_vector[d_idx] + env->d_params[poff] * gauss_rand();
        case DISTURBANCE_GAUSSIAN: return env->d_params[poff] * gauss_rand() + noise;
        default: return noise;
    }
}

void ashby_env_update_disturbances(AshbyEnvironment* env, double time, double* output, int n_output) {
    if (!env) return;
    int n = (n_output < env->n_disturbances) ? n_output : env->n_disturbances;
    for (int i = 0; i < n; i++) {
        output[i] = ashby_env_disturbance_value(env, i, time);
        env->disturbance_vector[i] = output[i];
    }
}

/* ===== AshbySystem ===== */
AshbySystem* ashby_system_create(const char* name, int n_states) {
    AshbySystem* sys = (AshbySystem*)calloc(1, sizeof(AshbySystem));
    sys->name = strdup(name); sys->n_states = n_states;
    sys->state_vector = (double*)calloc(n_states, sizeof(double));
    sys->time = 0.0; sys->dt = 0.01;
    sys->essential_vars = ashby_ev_set_create();
    sys->environment = NULL; sys->regulator_type = REGULATOR_ERROR_DRIVEN;
    sys->regulatory_capacity = 1.0; sys->n_actions = 0; sys->regulatory_action = NULL;
    sys->env_coupling = ashby_matrix_create(n_states,1);
    sys->internal_coupling = ashby_matrix_create(n_states,n_states);
    for (int i = 0; i < n_states; i++)
        ashby_matrix_set(sys->internal_coupling, i, i, -0.1);
    sys->state_history = ashby_timeseries_create(n_states);
    sys->ev_history = ashby_timeseries_create(1);
    sys->n_adaptations = 0; sys->adaptation_rate = 0.0; sys->is_adapting = false;
    return sys;
}

void ashby_system_free(AshbySystem* sys) {
    if (!sys) return;
    free(sys->name);
    if (sys->essential_vars) ashby_ev_set_free(sys->essential_vars);
    if (sys->environment) ashby_env_free(sys->environment);
    free(sys->state_vector); free(sys->regulatory_action);
    if (sys->env_coupling) ashby_matrix_free(sys->env_coupling);
    if (sys->internal_coupling) ashby_matrix_free(sys->internal_coupling);
    if (sys->state_history) ashby_timeseries_free(sys->state_history);
    if (sys->ev_history) ashby_timeseries_free(sys->ev_history);
    free(sys);
}

void ashby_system_set_environment(AshbySystem* sys, AshbyEnvironment* env) {
    if (!sys) return;
    if (sys->environment) ashby_env_free(sys->environment);
    sys->environment = env;
}

void ashby_system_set_coupling(AshbySystem* sys, const double* env_coupling, const double* internal_coupling, int rows, int cols) {
    if (!sys) return;
    if (sys->env_coupling) ashby_matrix_free(sys->env_coupling);
    sys->env_coupling = ashby_matrix_create(rows, cols);
    for (int i = 0; i < rows*cols; i++) sys->env_coupling->data[i] = env_coupling[i];
    if (sys->internal_coupling) ashby_matrix_free(sys->internal_coupling);
    sys->internal_coupling = ashby_matrix_create(rows, cols);
    for (int i = 0; i < rows*cols; i++) sys->internal_coupling->data[i] = internal_coupling[i];
}

void ashby_system_set_regulator(AshbySystem* sys, RegulatorType type) {
    if (!sys) return;
    sys->regulator_type = type;
    int na = sys->essential_vars->n_variables;
    if (na > sys->n_actions) {
        sys->regulatory_action = (double*)realloc(sys->regulatory_action, na*sizeof(double));
        for (int i = sys->n_actions; i < na; i++) sys->regulatory_action[i] = 0.0;
        sys->n_actions = na;
    }
}

void ashby_system_apply_regulation(AshbySystem* sys) {
    if (!sys || sys->n_actions == 0) return;
    int n = (sys->essential_vars->n_variables < sys->n_actions) ? sys->essential_vars->n_variables : sys->n_actions;
    switch (sys->regulator_type) {
        case REGULATOR_FIXED: break;
        case REGULATOR_ERROR_DRIVEN:
            for (int i = 0; i < n; i++) {
                EssentialVariable* ev = &sys->essential_vars->variables[i];
                double mid = (ev->lower_bound+ev->upper_bound)/2.0;
                double err = mid - ev->current_value;
                double range = (ev->upper_bound-ev->lower_bound)/2.0;
                if (range < 1e-10) range = 1.0;
                sys->regulatory_action[i] = sys->regulatory_capacity * err / range;
                if (sys->regulatory_action[i] > sys->regulatory_capacity) sys->regulatory_action[i] = sys->regulatory_capacity;
                if (sys->regulatory_action[i] < -sys->regulatory_capacity) sys->regulatory_action[i] = -sys->regulatory_capacity;
            }
            break;
        case REGULATOR_ADAPTIVE: {
            double dev = sys->essential_vars->current_deviation;
            double gain = sys->regulatory_capacity * (1.0+dev);
            for (int i = 0; i < n; i++) {
                EssentialVariable* ev = &sys->essential_vars->variables[i];
                double mid = (ev->lower_bound+ev->upper_bound)/2.0;
                double err = mid - ev->current_value;
                double range = (ev->upper_bound-ev->lower_bound)/2.0;
                if (range < 1e-10) range = 1.0;
                sys->regulatory_action[i] = gain * err / range;
                if (sys->regulatory_action[i] > sys->regulatory_capacity*2) sys->regulatory_action[i] = sys->regulatory_capacity*2;
                if (sys->regulatory_action[i] < -sys->regulatory_capacity*2) sys->regulatory_action[i] = -sys->regulatory_capacity*2;
            }
            break;
        }
        case REGULATOR_ULTRASTABLE: break;
        case REGULATOR_PREDICTIVE:
            for (int i = 0; i < n; i++) {
                EssentialVariable* ev = &sys->essential_vars->variables[i];
                double mid = (ev->lower_bound+ev->upper_bound)/2.0;
                double pred = ev->current_value + ev->rate_of_change * sys->dt * 5.0;
                double err = mid - pred;
                double range = (ev->upper_bound-ev->lower_bound)/2.0;
                if (range < 1e-10) range = 1.0;
                sys->regulatory_action[i] = sys->regulatory_capacity * err / range;
            }
            break;
        case REGULATOR_LEARNING: {
            static double* integral = NULL; static int isize = 0;
            if (!integral || isize < n) {
                integral = (double*)realloc(integral, n*sizeof(double));
                for (int i = isize; i < n; i++) integral[i] = 0.0;
                isize = n;
            }
            for (int i = 0; i < n; i++) {
                EssentialVariable* ev = &sys->essential_vars->variables[i];
                double mid = (ev->lower_bound+ev->upper_bound)/2.0;
                double err = mid - ev->current_value;
                integral[i] += err * sys->dt;
                integral[i] *= 0.99;
                double range = (ev->upper_bound-ev->lower_bound)/2.0;
                if (range < 1e-10) range = 1.0;
                sys->regulatory_action[i] = sys->regulatory_capacity * (0.7*err+0.3*integral[i])/range;
            }
            break;
        }
        default: break;
    }
}
void ashby_system_step(AshbySystem* sys) {
    if (!sys || sys->n_states == 0) return;
    double dt = sys->dt; int n = sys->n_states;
    double* disturbances = (double*)calloc(n, sizeof(double));
    if (sys->environment) ashby_env_update_disturbances(sys->environment, sys->time, disturbances, n);
    double* env_effect = (double*)calloc(n, sizeof(double));
    ashby_matrix_vec_mul(sys->env_coupling, disturbances, env_effect);
    double* internal_effect = (double*)calloc(n, sizeof(double));
    ashby_matrix_vec_mul(sys->internal_coupling, sys->state_vector, internal_effect);
    double* reg_effect = (double*)calloc(n, sizeof(double));
    if (sys->regulatory_action && sys->n_actions > 0)
        for (int i = 0; i < n && i < sys->n_actions; i++) reg_effect[i] = sys->regulatory_action[i];
    for (int i = 0; i < n; i++) {
        double dx = env_effect[i] + internal_effect[i] + reg_effect[i];
        if (fabs(sys->state_vector[i] + dx*dt) > 1000.0) dx *= 0.1;
        sys->state_vector[i] += dx * dt;
    }
    for (int i = 0; i < sys->essential_vars->n_variables && i < n; i++)
        ashby_ev_update(sys->essential_vars, i, sys->state_vector[i], dt);
    ashby_system_apply_regulation(sys);
    HomeostasisStatus st = ashby_ev_check_status(sys->essential_vars);
    sys->essential_vars->status = st;
    double dev = ashby_ev_deviation(sys->essential_vars);
    sys->essential_vars->current_deviation = dev;
    if (dev > sys->essential_vars->max_observed_deviation) sys->essential_vars->max_observed_deviation = dev;
    if (st == HOMEOSTASIS_VIOLATED || st == HOMEOSTASIS_DEAD) {
        sys->essential_vars->total_violation_duration += dt;
        sys->essential_vars->time_since_last_violation = 0.0;
    } else sys->essential_vars->time_since_last_violation += dt;
    ashby_timeseries_record_vector(sys->state_history, sys->state_vector, n);
    ashby_timeseries_record(sys->ev_history, sys->essential_vars->current_deviation);
    sys->time += dt;
    free(disturbances); free(env_effect); free(internal_effect); free(reg_effect);
}

void ashby_system_simulate(AshbySystem* sys, double duration) {
    if (!sys) return;
    int steps = (int)(duration / sys->dt);
    for (int i = 0; i < steps; i++) ashby_system_step(sys);
}

bool ashby_system_is_homeostatic(const AshbySystem* sys) {
    if (!sys || sys->essential_vars->n_variables == 0) return false;
    return ashby_ev_check_status(sys->essential_vars) == HOMEOSTASIS_SAFE;
}

double ashby_system_homeostatic_efficiency(const AshbySystem* sys) {
    if (!sys || sys->essential_vars->n_variables == 0) return 0.0;
    double tt = sys->time; if (tt < 1e-10) return 1.0;
    double eff = 1.0 - sys->essential_vars->total_violation_duration / tt;
    return eff < 0.0 ? 0.0 : (eff > 1.0 ? 1.0 : eff);
}

void ashby_system_print(const AshbySystem* sys) {
    if (!sys) return;
    printf("=== Ashby Homeostatic System: %s ===\n", sys->name);
    printf("Time: %.3f, States: %d, Regulator: %d\n", sys->time, sys->n_states, sys->regulator_type);
    printf("Essential Variables (%d):\n", sys->essential_vars->n_variables);
    for (int i = 0; i < sys->essential_vars->n_variables; i++) {
        EssentialVariable* ev = &sys->essential_vars->variables[i];
        printf("  %s: %.4f [%.2f,%.2f] sev=%.2f rate=%.2f %s\n",
               ev->name, ev->current_value, ev->lower_bound, ev->upper_bound,
               ev->violation_severity, ev->rate_of_change, ev->is_critical?"CRITICAL":"");
    }
    printf("Status: %d, Deviation: %.4f, Efficiency: %.2f%%\n",
           sys->essential_vars->status, sys->essential_vars->current_deviation,
           ashby_system_homeostatic_efficiency(sys)*100.0);
    printf("Adaptations: %d\n", sys->n_adaptations);
}

void ashby_system_print_status(const AshbySystem* sys) {
    if (!sys) return;
    const char* ss[] = {"SAFE","WARNING","CRITICAL","VIOLATED","DEAD"};
    printf("[t=%.3f] %s | dev=%.4f | eff=%.1f%%\n",
           sys->time, ss[sys->essential_vars->status],
           sys->essential_vars->current_deviation,
           ashby_system_homeostatic_efficiency(sys)*100.0);
}

/* ===== TimeSeries ===== */
AHTimeSeries* ashby_timeseries_create(int stride) {
    AHTimeSeries* ts = (AHTimeSeries*)calloc(1, sizeof(AHTimeSeries));
    ts->capacity = HISTORY_DEFAULT_CAP;
    ts->buffer = (double*)calloc(HISTORY_DEFAULT_CAP, sizeof(double));
    ts->length = 0; ts->stride = (stride > 0) ? stride : 1;
    return ts;
}

void ashby_timeseries_free(AHTimeSeries* ts) { if(!ts)return; free(ts->buffer); free(ts); }

void ashby_timeseries_record(AHTimeSeries* ts, double value) {
    if (!ts) return;
    if (ts->length >= ts->capacity) { ts->capacity *= 2; ts->buffer = (double*)realloc(ts->buffer, ts->capacity*sizeof(double)); }
    ts->buffer[ts->length++] = value;
}

void ashby_timeseries_record_vector(AHTimeSeries* ts, const double* values, int n) {
    if (!ts) return;
    int needed = ts->length + n;
    while (needed >= ts->capacity) { ts->capacity *= 2; ts->buffer = (double*)realloc(ts->buffer, ts->capacity*sizeof(double)); }
    for (int i = 0; i < n; i++) ts->buffer[ts->length+i] = values[i];
    ts->length += n;
}

double ashby_timeseries_mean(const AHTimeSeries* ts) {
    if (!ts || ts->length == 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < ts->length; i++) sum += ts->buffer[i];
    return sum / ts->length;
}

double ashby_timeseries_variance(const AHTimeSeries* ts) {
    if (!ts || ts->length < 2) return 0.0;
    double mean = ashby_timeseries_mean(ts), sum_sq = 0.0;
    for (int i = 0; i < ts->length; i++) { double d = ts->buffer[i]-mean; sum_sq += d*d; }
    return sum_sq / (ts->length-1);
}

double ashby_timeseries_autocorrelation(const AHTimeSeries* ts, int lag) {
    if (!ts || lag < 0 || lag >= ts->length-1) return 0.0;
    double mean = ashby_timeseries_mean(ts), var = ashby_timeseries_variance(ts);
    if (var < 1e-10) return 0.0;
    double cov = 0.0; int N = ts->length - lag;
    for (int i = 0; i < N; i++) cov += (ts->buffer[i]-mean)*(ts->buffer[i+lag]-mean);
    return cov / (N*var + 1e-10);
}

int ashby_timeseries_crossing_count(const AHTimeSeries* ts, double threshold) {
    if (!ts || ts->length < 2) return 0;
    int count = 0;
    for (int i = 1; i < ts->length; i++)
        if ((ts->buffer[i-1]-threshold)*(ts->buffer[i]-threshold) < 0.0) count++;
    return count;
}

/* ===== Matrix ===== */
AHMatrix* ashby_matrix_create(int rows, int cols) {
    AHMatrix* m = (AHMatrix*)calloc(1, sizeof(AHMatrix));
    m->rows = rows; m->cols = cols;
    m->data = (double*)calloc(rows*cols, sizeof(double));
    return m;
}

void ashby_matrix_free(AHMatrix* m) { if(!m)return; free(m->data); free(m); }

double ashby_matrix_get(const AHMatrix* m, int r, int c) {
    if (!m || r < 0 || r >= m->rows || c < 0 || c >= m->cols) return 0.0;
    return m->data[r*m->cols+c];
}

void ashby_matrix_set(AHMatrix* m, int r, int c, double v) {
    if (!m || r < 0 || r >= m->rows || c < 0 || c >= m->cols) return;
    m->data[r*m->cols+c] = v;
}

AHMatrix* ashby_matrix_mul(const AHMatrix* a, const AHMatrix* b) {
    if (!a || !b || a->cols != b->rows) return NULL;
    AHMatrix* c = ashby_matrix_create(a->rows, b->cols);
    for (int i = 0; i < a->rows; i++)
        for (int j = 0; j < b->cols; j++) {
            double sum = 0.0;
            for (int k = 0; k < a->cols; k++) sum += ashby_matrix_get(a,i,k) * ashby_matrix_get(b,k,j);
            ashby_matrix_set(c,i,j,sum);
        }
    return c;
}

void ashby_matrix_vec_mul(const AHMatrix* a, const double* x, double* y) {
    if (!a || !x || !y) return;
    for (int i = 0; i < a->rows; i++) {
        y[i] = 0.0;
        for (int j = 0; j < a->cols; j++) y[i] += ashby_matrix_get(a,i,j) * x[j];
    }
}

void ashby_matrix_print(const AHMatrix* m) {
    if (!m) return;
    printf("Matrix %dx%d:\n", m->rows, m->cols);
    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++) printf("%8.4f ", ashby_matrix_get(m,i,j));
        printf("\n");
    }
}

/* ===== Utilities ===== */
double ashby_clamp(double x, double lo, double hi) { return x<lo?lo:(x>hi?hi:x); }

double ashby_linear_map(double x, double in_lo, double in_hi, double out_lo, double out_hi) {
    double ir = in_hi-in_lo, or2 = out_hi-out_lo;
    if (fabs(ir) < 1e-10) return out_lo;
    return out_lo + (x-in_lo)*or2/ir;
}

double ashby_sigmoid_01(double x, double k) { return 1.0/(1.0+exp(-k*x)); }

double* ashby_linspace(double start, double end, int n) {
    if (n < 1) return NULL;
    double* arr = (double*)malloc(n*sizeof(double));
    if (n == 1) { arr[0]=start; return arr; }
    double step = (end-start)/(n-1);
    for (int i = 0; i < n; i++) arr[i] = start + step*i;
    return arr;
}

void ashby_normalize_vector(double* v, int n) {
    double nsq = 0.0;
    for (int i = 0; i < n; i++) nsq += v[i]*v[i];
    double nr = sqrt(nsq);
    if (nr < 1e-15) return;
    for (int i = 0; i < n; i++) v[i] /= nr;
}