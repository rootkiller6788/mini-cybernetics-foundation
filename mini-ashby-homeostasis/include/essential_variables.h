#ifndef ESSENTIAL_VARIABLES_H
#define ESSENTIAL_VARIABLES_H

#include "ashby_homeostasis.h"

/* ============================================================================
 * Essential Variables Theory — The Core of Ashbyan Homeostasis
 *
 * Ashby, "Design for a Brain" (1952/1960), Sections 5/14–5/16:
 *
 * "The essential variables are those variables which must be kept within
 * certain limits if the organism is to survive."
 *
 * The concept of essential variables (EV) is the foundation of Ashby's
 * cybernetics. An organism survives if and only if its essential variables
 * remain within their physiological limits. All adaptive behavior is
 * defined in relation to keeping EVs within bounds.
 *
 * The survival zone is the Cartesian product of the acceptable ranges of
 * all essential variables — an N-dimensional box in state space. When the
 * system's trajectory nears or crosses the boundary of this box, adaptation
 * must occur or the system dies.
 * ============================================================================ */

/** Type of bounds for an essential variable */
typedef enum {
    EV_BOUND_HARD = 0,    /* Exceeding this bound causes immediate death */
    EV_BOUND_SOFT = 1,    /* Exceeding this bound causes damage accumulation */
    EV_BOUND_ADAPTIVE = 2 /* Bound that shifts with environmental context */
} EVBoundType;

/** Statistical summary of an essential variable over time */
typedef struct {
    double mean;
    double variance;
    double min_observed;
    double max_observed;
    double skewness;
    double kurtosis;
    int n_samples;
    int n_violations;
    double fractional_time_violated;
    double integral_abs_deviation;  /* \int |x - midpoint| dt */
    double worst_case_distance;    /* Maximum distance from safe zone */
} EVStatistics;

/** Time-course trajectory of multiple essential variables */
typedef struct {
    double** trajectories;   /* traj[var_idx][time_idx] */
    int n_variables;
    int n_timepoints;
    int capacity;
    double* time_points;
    bool* is_alive;          /* per-timepoint survival status */
} EVTrajectory;

/* --- Essential Variables API --- */

/* Core EV operations */
void ashby_ev_set_bounds(EssentialVariable* ev, double lower, double upper);
void ashby_ev_set_warning_thresholds(EssentialVariable* ev,
                                      double warn_low, double warn_high);
double ashby_ev_midpoint(const EssentialVariable* ev);
double ashby_ev_range(const EssentialVariable* ev);
double ashby_ev_normalized_position(const EssentialVariable* ev);
double ashby_ev_margin(const EssentialVariable* ev);
double ashby_ev_urgency(const EssentialVariable* ev);

/* Survival zone analysis */
bool ashby_ev_within_bounds(const EssentialVariable* ev);
bool ashby_ev_within_warning(const EssentialVariable* ev);
double ashby_ev_violation_magnitude(const EssentialVariable* ev);
void ashby_ev_predict_crossing(const EssentialVariable* ev, double dt,
                                double* time_to_cross, int* which_bound);
void ashby_ev_compute_severity(EssentialVariable* ev);

/* Multi-EV operations */
double ashby_ev_set_survival_volume(const EssentialVariableSet* evs);
double ashby_ev_set_centroid_distance(const EssentialVariableSet* evs);
int ashby_ev_set_count_violated(const EssentialVariableSet* evs);
int ashby_ev_set_worst_variable(const EssentialVariableSet* evs);
double ashby_ev_set_margin(const EssentialVariableSet* evs);
double ashby_ev_set_criticality_index(const EssentialVariableSet* evs);
void ashby_ev_set_classify(const EssentialVariableSet* evs,
                            int* safe_count, int* warning_count,
                            int* critical_count, int* violated_count);
void ashby_ev_set_synchronize(EssentialVariableSet* evs,
                               const double* values, int n);
bool ashby_ev_set_is_survivable_fast(const EssentialVariableSet* evs);

/* EV Statistics */
EVStatistics* ashby_ev_statistics_create(void);
void ashby_ev_statistics_free(EVStatistics* s);
void ashby_ev_statistics_update(EVStatistics* s,
                                 const EssentialVariable* ev, double dt);
void ashby_ev_statistics_finalize(EVStatistics* s);
void ashby_ev_statistics_print(const EVStatistics* s);

/* EV Trajectory for time-series analysis */
EVTrajectory* ashby_ev_trajectory_create(int n_vars, int capacity);
void ashby_ev_trajectory_free(EVTrajectory* traj);
void ashby_ev_trajectory_record(EVTrajectory* traj,
                                 const EssentialVariableSet* evs,
                                 double time);
int ashby_ev_trajectory_first_violation(const EVTrajectory* traj);
double ashby_ev_trajectory_survival_time(const EVTrajectory* traj);
double ashby_ev_trajectory_time_in_violation(const EVTrajectory* traj,
                                              int var_idx);
void ashby_ev_trajectory_phase_portrait(const EVTrajectory* traj,
                                         int var_x, int var_y,
                                         double** out_x, double** out_y,
                                         int* n_points);

/* Adaptive bounds */
void ashby_ev_set_adaptive_bounds(EssentialVariable* ev,
                                   double base_lower, double base_upper,
                                   double plasticity);
void ashby_ev_shift_bounds(EssentialVariable* ev, double shift);
void ashby_ev_expand_bounds(EssentialVariable* ev, double factor);
void ashby_ev_reset_to_baseline(EssentialVariable* ev);

#endif /* ESSENTIAL_VARIABLES_H */