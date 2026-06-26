#ifndef ADAPTIVE_REGULATOR_H
#define ADAPTIVE_REGULATOR_H

#include "ashby_homeostasis.h"

/* ============================================================================
 * Adaptive Regulator Theory — Conant-Ashby Theorem & Model-Based Regulation
 *
 * Conant & Ashby, "Every Good Regulator of a System Must Be a Model of
 * That System" (International Journal of Systems Science, 1970).
 *
 * THE CONANT-ASHBY THEOREM:
 *   Every good regulator of a system must be (contain) a model of that
 *   system. That is, the structure of the regulator must be isomorphic
 *   to the structure of the regulated system.
 *
 * Corollary: The best regulator for a given system has internal states
 * that map onto the system's states. Regulation quality is bounded by
 * the fidelity of the internal model.
 *
 * This module implements:
 * 1. Internal model representation (forward and inverse models)
 * 2. Model-based regulation (feedforward + feedback)
 * 3. Online model adaptation (learning from regulation errors)
 * 4. Bayesian adaptive regulation (probabilistic model updating)
 * 5. Two-level regulation architecture (homeostatic + adaptive)
 * ============================================================================ */

/** Internal model types for regulation */
typedef enum {
    MODEL_FORWARD       = 0,  /* Forward model: (state, action) -> next_state */
    MODEL_INVERSE       = 1,  /* Inverse model: (state, desired) -> action */
    MODEL_FORWARD_INVERSE = 2,/* Combined forward + inverse */
    MODEL_LINEAR        = 3,  /* Linear dynamical model: x' = Ax + Bu */
    MODEL_NONLINEAR     = 4,  /* Nonlinear model (e.g. neural, polynomial) */
    MODEL_BAYESIAN      = 5,  /* Probabilistic model with uncertainty */
    MODEL_ENSEMBLE      = 6   /* Ensemble of multiple models */
} InternalModelType;

/** A linear dynamical model: x(t+1) = A x(t) + B u(t) + w(t) */
typedef struct {
    AHMatrix* A;            /* State transition matrix (n x n) */
    AHMatrix* B;            /* Control input matrix (n x m) */
    AHMatrix* C;            /* Observation matrix (p x n) */
    double* process_noise;  /* Process noise covariance diagonal */
    double* observation_noise;/* Observation noise covariance diagonal */
    int state_dim;
    int control_dim;
    int observation_dim;
    bool is_learned;
} LinearDynamicalModel;

/** Bayesian belief about system parameters */
typedef struct {
    double* param_means;     /* Mean of parameter belief */
    double* param_vars;      /* Variance (uncertainty) of each parameter */
    double** param_cov;      /* Full covariance matrix */
    int n_params;
    double prior_strength;   /* Weight of prior vs new data */
    double learning_rate;    /* Decay factor for old observations */
} BayesianParameterBelief;

/** An adaptive regulator with internal model */
typedef struct {
    char* name;
    InternalModelType model_type;

    /* Internal model */
    LinearDynamicalModel* linear_model;
    double* model_params;       /* Generic parameter vector for nonlinear models */
    int n_model_params;
    double model_fidelity;      /* How well model matches reality (0-1) */
    double model_uncertainty;   /* Estimated uncertainty of model predictions */

    /* Bayesian belief (for Bayesian regulator) */
    BayesianParameterBelief* bayesian_belief;

    /* Regulation strategy */
    double* setpoint;          /* Desired values for essential variables */
    double* current_action;    /* Current regulatory action vector */
    int n_control_outputs;

    /* Gain parameters */
    double proportional_gain;  /* Kp: proportional term */
    double integral_gain;      /* Ki: integral term */
    double derivative_gain;    /* Kd: derivative term */
    double* integral_error;    /* Accumulated error for integral action */
    double* prev_error;        /* Previous error for derivative action */

    /* Feedforward */
    bool use_feedforward;      /* Enable model-based feedforward */
    double feedforward_gain;   /* Weight of feedforward vs feedback */

    /* Adaptation */
    bool adapt_model;          /* Whether to update model online */
    double model_update_rate;  /* Rate of model parameter updates */
    double prediction_error;   /* Recent model prediction error */
    double* error_history;     /* History of regulation errors */
    int error_history_len;
    int error_history_capacity;

    /* Performance metrics */
    double rise_time;          /* Time to reach 90% of setpoint */
    double settling_time;      /* Time to reach and stay within 5% of setpoint */
    double overshoot;          /* Maximum overshoot percentage */
    double steady_state_error; /* Final error magnitude */
    double control_effort;     /* Integral of squared control action */
} AdaptiveRegulator;

/* --- Adaptive Regulator API --- */

/* Model creation and management */
AdaptiveRegulator* adaptive_regulator_create(const char* name,
                                              int state_dim, int control_dim);
void adaptive_regulator_free(AdaptiveRegulator* reg);
void adaptive_regulator_set_model_type(AdaptiveRegulator* reg,
                                        InternalModelType type);
void adaptive_regulator_set_linear_model(AdaptiveRegulator* reg,
    const double* A, const double* B, int n, int m);
void adaptive_regulator_set_setpoint(AdaptiveRegulator* reg,
                                      const double* setpoint, int n);
void adaptive_regulator_set_gains(AdaptiveRegulator* reg,
                                   double Kp, double Ki, double Kd);

/* Regulation action computation */
void adaptive_regulator_compute_error(AdaptiveRegulator* reg,
                                       const double* current_state,
                                       double* error, int n);
void adaptive_regulator_pid_action(AdaptiveRegulator* reg,
                                    const double* error, double dt,
                                    double* action, int n);
void adaptive_regulator_feedforward_action(AdaptiveRegulator* reg,
    const double* current_state, const double* disturbance_estimate,
    double* action, int n);
void adaptive_regulator_total_action(AdaptiveRegulator* reg,
                                      const double* current_state,
                                      const double* disturbance_estimate,
                                      double dt, double* action, int n);

/* Model prediction */
void adaptive_regulator_predict_forward(AdaptiveRegulator* reg,
    const double* current_state, const double* action,
    double* predicted_next, int n);
void adaptive_regulator_predict_inverse(AdaptiveRegulator* reg,
    const double* current_state, const double* desired_next,
    double* required_action, int n);

/* Online model adaptation */
void adaptive_regulator_update_model(AdaptiveRegulator* reg,
    const double* state_before, const double* action,
    const double* state_after, double dt);
double adaptive_regulator_model_prediction_error(AdaptiveRegulator* reg,
    const double* predicted, const double* actual, int n);

/* Bayesian adaptation */
BayesianParameterBelief* bayesian_belief_create(int n_params);
void bayesian_belief_free(BayesianParameterBelief* b);
void bayesian_belief_initialize(BayesianParameterBelief* b,
                                 const double* prior_means,
                                 const double* prior_vars,
                                 double prior_strength);
void bayesian_belief_update(BayesianParameterBelief* b,
                             const double* observation,
                             const double* predicted,
                             const double* jacobian, int n_obs);
double bayesian_belief_uncertainty(const BayesianParameterBelief* b);
void bayesian_belief_sample(BayesianParameterBelief* b, double* sample);
void bayesian_belief_print(const BayesianParameterBelief* b);

/* Performance analysis */
void adaptive_regulator_evaluate_performance(AdaptiveRegulator* reg,
    double dt, double total_time);
double adaptive_regulator_cost(const AdaptiveRegulator* reg);
double adaptive_regulator_model_fidelity_score(AdaptiveRegulator* reg,
    const double* actual_params, int n);
void adaptive_regulator_print(const AdaptiveRegulator* reg);

/* Conant-Ashby theorem verification */
double conant_ashby_isomorphism_measure(const AdaptiveRegulator* reg,
    const double* system_params, int n_sys_params);
bool conant_ashby_regulator_quality_check(const AdaptiveRegulator* reg,
    double regulation_error, double model_error, double threshold);

#endif /* ADAPTIVE_REGULATOR_H */