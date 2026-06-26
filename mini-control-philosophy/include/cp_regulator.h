#ifndef CP_REGULATOR_H
#define CP_REGULATOR_H

#include "cp_core.h"
#include <stdbool.h>

/* ============================================================================
 * Control Philosophy ? Regulator Theory
 *
 * The Good Regulator Theorem (Conant & Ashby, 1970):
 *   "Every good regulator of a system must be a model of that system."
 *
 * This means: to control well, the controller must internally represent
 * the causal structure of the system it controls. The regulator's internal
 * states must be homomorphic to the states of the regulated system.
 *
 * References:
 *   Conant & Ashby (1970). Int. J. Systems Science, 1(2), 89-97.
 *   Francis & Wonham (1976). Automatica, 12(5), 457-465.
 *   Wonham, W.M. (1979). Linear Multivariable Control: A Geometric Approach.
 * ============================================================================ */

/** Internal model: the regulator's representation of the plant.
 *  For linear systems, this is (A_m, B_m, C_m) ? (A, B, C) of the plant. */
typedef struct {
    double* A;           /* state transition matrix, flattened n?n */
    double* B;           /* input matrix, flattened n?m */
    double* C;           /* output matrix, flattened p?n */
    int     n;           /* model state dimension */
    int     m;           /* model input dimension */
    int     p;           /* model output dimension */
    double  identification_error; /* Frobenius norm ||(A,B,C) - (A_m,B_m,C_m)|| */
    bool    is_accurate; /* error < tolerance */
} InternalModel;

/** Good Regulator analysis result. */
typedef struct {
    InternalModel model;
    double        homomorphism_degree; /* 0..1: how well regulator mirrors plant */
    double        regulation_error;    /* RMS output deviation from setpoint */
    double        model_accuracy;      /* 1 - normalized identification error */
    bool          satisfies_grt;       /* homomorphism_degree > threshold */
    double        grt_threshold;
    double*       residual_entropy;    /* H(plant | regulator model) per dimension */
    int           n_residuals;
} GoodRegulatorResult;

/** A regulator as described by Conant & Ashby. */
typedef struct {
    char*            name;
    RegulatorType    type;
    InternalModel*   plant_model;     /* the regulator's internal model of the plant */
    ControlSystem*   controlled_sys;  /* the system being regulated */
    double*          observed_output; /* latest measurement from plant */
    double*          predicted_output;/* model-predicted output */
    double*          control_action;  /* computed action to apply */
    double           prediction_error;/* ||y_observed - y_predicted|| */
    int              state_dim;
    int              input_dim;
    int              output_dim;
} Regulator;

/** Servomechanism: a regulator that tracks a time-varying reference.
 *  This is the generalization from regulation (constant setpoint)
 *  to servoing (tracking arbitrary reference trajectories). */
typedef struct {
    Regulator* reg;
    double*    reference_trajectory;  /* time-indexed reference values */
    double*    tracking_error;        /* ||y(t) - r(t)|| at each sample */
    int        trajectory_length;
    double     rms_tracking_error;
    double     peak_error;
    double     settling_time;         /* time to reach ?5% of steady state */
} Servomechanism;

/* --- API: Internal Model --- */

InternalModel* cp_internal_model_create(int n, int m, int p);
void cp_internal_model_free(InternalModel* im);
void cp_internal_model_set_A(InternalModel* im, const double* A);
void cp_internal_model_set_B(InternalModel* im, const double* B);
void cp_internal_model_set_C(InternalModel* im, const double* C);
void cp_internal_model_predict(const InternalModel* im,
                                const double* x, const double* u,
                                double* x_next, double* y_pred);
double cp_internal_model_identification_error(const InternalModel* im,
                                               const double* A_true,
                                               const double* B_true,
                                               const double* C_true);

/* --- API: Regulator --- */

Regulator* cp_regulator_create(const char* name, RegulatorType type,
                                int nx, int nu, int ny);
void cp_regulator_free(Regulator* reg);
void cp_regulator_attach_plant(Regulator* reg, ControlSystem* sys);
void cp_regulator_set_model(Regulator* reg, InternalModel* model);
void cp_regulator_update(Regulator* reg, const double* y_measured);
void cp_regulator_compute_action(Regulator* reg);
void cp_regulator_apply(Regulator* reg);

/* --- API: Good Regulator Theorem --- */

/**
 * Evaluate whether a regulator satisfies the Good Regulator Theorem.
 * Computes homomorphism degree between regulator's internal model and plant.
 * [Source: Conant & Ashby (1970)]
 * Complexity: O(n?) for model comparison
 */
GoodRegulatorResult* cp_good_regulator_evaluate(const Regulator* reg);
void cp_good_regulator_free(GoodRegulatorResult* grr);
void cp_good_regulator_print(const GoodRegulatorResult* grr);

/**
 * Compute the homomorphism degree between two state-transition structures.
 * Returns 0..1 where 1 = perfect isomorphism.
 * Uses normalized Frobenius norm: 1 - ||A_p - A_m|| / ||A_p||.
 * Complexity: O(n?)
 */
double cp_homomorphism_degree(const double* A_plant, const double* A_model, int n);

/**
 * Verify the Internal Model Principle: does the regulator embed
 * a model of the exogenous signal (disturbance/reference) generator?
 * [Source: Francis & Wonham (1976)]
 * Complexity: O(n?)
 */
bool cp_internal_model_principle_check(const Regulator* reg,
                                        const double* exo_generator, int exo_dim);

/* --- API: Servomechanism --- */

Servomechanism* cp_servo_create(Regulator* reg, int traj_len);
void cp_servo_free(Servomechanism* sm);
void cp_servo_set_reference(Servomechanism* sm, const double* trajectory, int len);
void cp_servo_simulate(Servomechanism* sm, double dt);
void cp_servo_print(const Servomechanism* sm);

/* --- API: Regulation Philosophies --- */

/**
 * Compute the Bode sensitivity integral (Waterbed Effect):
 * ??^? ln|S(j?)| d? = ? ? ? Re(p_i) for stable poles.
 * Reducing sensitivity at some frequencies inevitably increases it at others.
 * [Source: Bode (1945); Freudenberg & Looze (1985)]
 * Complexity: O(n_poles)
 */
double cp_bode_sensitivity_integral(const double* poles, int n_poles);

/**
 * Check if a regulator achieves the waterbed trade-off:
 * improvement in one band must be paid for in another.
 * Returns the "conservation of difficulty" index.
 */
double cp_waterbed_index(const double* sensitivity_old,
                          const double* sensitivity_new, int n_freqs);

#endif /* CP_REGULATOR_H */
