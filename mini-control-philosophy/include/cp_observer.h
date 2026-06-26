#ifndef CP_OBSERVER_H
#define CP_OBSERVER_H

#include "cp_core.h"
#include <stdbool.h>

/* ============================================================================
 * Control Philosophy ? Observer Theory and the Separation Principle
 *
 * "The observer reconstructs what the controller needs to know."
 *
 * Key philosophical insight:
 *   Control requires knowledge. Knowledge requires observation.
 *   Observation is inference under uncertainty. The observer is the
 *   epistemological backbone of closed-loop control.
 *
 * The Separation Principle (Luenberger, 1964):
 *   For linear systems with Gaussian noise, optimal control and optimal
 *   estimation can be designed independently and composed.
 *   This is the LQG = LQR + Kalman decomposition.
 *
 * References:
 *   Kalman, R.E. (1960). "A New Approach to Linear Filtering..."
 *   Luenberger, D.G. (1964). "Observing the State of a Linear System"
 *   Wonham, W.M. (1968). "On the Separation Theorem of Stochastic Control"
 *   Mitter, S.K. (1996). "Filtering and Stochastic Control"
 * ============================================================================ */

/** Observer state: internal representation of the external system. */
typedef struct {
    ObserverType    type;
    double*         x_hat;           /* estimated state ? R^n */
    double*         P;               /* error covariance (Kalman) or observer gain (Luenberger) */
    double*         K;               /* observer gain matrix */
    double*         Q;               /* process noise covariance */
    double*         R;               /* measurement noise covariance */
    double*         innovation;      /* y - C*x_hat: prediction error */
    double          innovation_norm; /* scalar measure of surprise */
    int             n;               /* state dimension */
    int             m;               /* measurement dimension */
    double          estimation_error;/* ||x - x_hat||? */
    bool            is_converged;    /* estimation error < threshold */
    int             convergence_steps;
    double          convergence_rate;/* geometric decay rate of error */
} Observer;

/** Dual Observer-Controller structure embodying the Separation Principle. */
typedef struct {
    Observer*       observer;
    ControlSystem*  controller;
    double*         composite_state;  /* [x_hat; controller_state] */
    int             composite_dim;
    double          separation_error; /* suboptimality from separation vs joint design */
    bool            separation_holds; /* true for LTI+Gaussian systems */
} ObserverController;

/** The observation process as an informational channel.
 *  Quantifies: how much does observing reduce uncertainty? */
typedef struct {
    double prior_entropy;        /* H(x) before observation */
    double posterior_entropy;    /* H(x|y) after observation */
    double information_gain;     /* I(x;y) = H(x) - H(x|y) */
    double observation_efficiency; /* information_gain / channel_capacity */
    int    effective_rank;       /* rank of observability matrix */
} ObservationInfo;

/* --- API: Observer --- */

Observer* cp_observer_create(ObserverType type, int n, int m);
void cp_observer_free(Observer* obs);
void cp_observer_set_noise(Observer* obs, const double* Q, const double* R);
void cp_observer_set_gain(Observer* obs, const double* K);
void cp_observer_predict(Observer* obs, const double* A, const double* B,
                          const double* u, int n, double dt);
void cp_observer_update(Observer* obs, const double* C, const double* y_measured, int n, int m);
void cp_observer_innovation(Observer* obs, const double* C, const double* y, int n, int m);
double cp_observer_estimation_error(const Observer* obs, const double* x_true, int n);
bool cp_observer_check_convergence(const Observer* obs, double threshold);
double cp_observer_convergence_rate(const Observer* obs, const double* A, int n);
void cp_observer_print(const Observer* obs);

/* --- API: Separation Principle --- */

ObserverController* cp_observer_controller_create(Observer* obs, ControlSystem* ctrl);
void cp_observer_controller_free(ObserverController* oc);
void cp_observer_controller_step(ObserverController* oc, double dt,
                                  const double* y_measured);
bool cp_observer_controller_check_separation(const ObserverController* oc,
                                              const double* A, const double* C, int n);

/* --- API: Observability Analysis --- */

/**
 * Compute the observability matrix O = [C; CA; CA?; ...; CA^(n-1)].
 * The pair (A,C) is observable iff rank(O) = n.
 * [Source: Kalman (1960)]
 * Complexity: O(n?)
 */
double* cp_observability_matrix(const double* A, const double* C, int n, int* total_rows);

/**
 * Compute the rank of the observability matrix via QR decomposition
 * (simplified: count non-negligible singular values via power iteration).
 * Complexity: O(n?)
 */
int cp_observability_rank(const double* A, const double* C, int n, double tol);

/**
 * Compute the observability Gramian Wo via the Lyapunov equation:
 * A? Wo + Wo A = -C? C.
 * Solved via Bartels-Stewart algorithm (simplified iterative method).
 * Complexity: O(n? ? max_iter)
 */
void cp_observability_gramian_solve(const double* A, const double* C,
                                     int n, double* Wo, int max_iter, double tol);

/* --- API: Observation Information --- */

ObservationInfo* cp_observation_info_compute(const Observer* obs,
                                              const double* prior_cov,
                                              const double* posterior_cov, int n);
void cp_observation_info_free(ObservationInfo* oi);
void cp_observation_info_print(const ObservationInfo* oi);

/**
 * Compute the information-theoretic value of an observation:
 * the reduction in uncertainty about the state from the measurement.
 * [Source: Stratonovich, "On Value of Information" (1965)]
 */
double cp_observation_value(const double* prior_entropy, const double* posterior_entropy);

#endif /* CP_OBSERVER_H */
