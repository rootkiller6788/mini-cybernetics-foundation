#ifndef WIENER_FEEDBACK_H
#define WIENER_FEEDBACK_H
#include "wiener_core.h"

/* Wiener Feedback Control Theory
 * Based on: Wiener (1948) Cybernetics Ch. IV
 *           Rosenblueth, Wiener, Bigelow (1943)
 *
 * L4: Feedback stability criterion
 * L4: Negative feedback as purposeful behavior
 */

typedef struct {
    double* num;
    double* den;
    int     num_order;
    int     den_order;
} WCTransferFunction;

typedef struct {
    double* A;
    double* B;
    double* C;
    double* D;
    int     n, m, p;
} WCStateSpace;

typedef struct {
    WCTransferFunction* plant;
    WCTransferFunction* controller;
    WCTransferFunction* sensitivity;
    WCTransferFunction* comp_sens;
    double  gain_margin;
    double  phase_margin;
    double  bandwidth;
    bool    is_stable;
} WCClosedLoop;

typedef struct {
    WCStateSpace* motor;
    WCStateSpace* load;
    WCFeedbackLoop* fb;
    double  inertia;
    double  damping;
    double  stiffness;
    double  position;
    double  velocity;
    double  torque;
} WCServomechanism;

typedef struct {
    double* past_positions;
    double* past_times;
    int     history_len;
    double  predicted_position;
    double  predicted_velocity;
    double  prediction_time;
    double  error_estimate;
} WCAntiAircraftPredictor;

typedef struct {
    WCStateSpace*   system;
    WCFeedbackLoop* feedback;
    double*         reference_trajectory;
    double*         output_trajectory;
    int             trajectory_len;
    double          tracking_error;
    double          control_effort;
} WCCyberneticRegulator;

/* Transfer Function API */
WCTransferFunction* wc_tf_create(int num_order, int den_order);
void      wc_tf_free(WCTransferFunction* tf);
WCTransferFunction* wc_tf_series(const WCTransferFunction* g1, const WCTransferFunction* g2);
WCTransferFunction* wc_tf_parallel(const WCTransferFunction* g1, const WCTransferFunction* g2);
WCTransferFunction* wc_tf_feedback(const WCTransferFunction* g, const WCTransferFunction* h);
double    wc_tf_eval(const WCTransferFunction* tf, double s_re, double s_im);
double    wc_tf_dcgain(const WCTransferFunction* tf);
void      wc_tf_bode(const WCTransferFunction* tf, double* omega, double* mag, double* phase, int n_pts);
bool      wc_tf_is_stable(const WCTransferFunction* tf);
double    wc_tf_bandwidth(const WCTransferFunction* tf);

/* State-Space API */
WCStateSpace* wc_ss_create(int n, int m, int p);
void      wc_ss_free(WCStateSpace* ss);
void      wc_ss_step(WCStateSpace* ss, const double* x, const double* u, double* x_next, double* y, double dt);
void      wc_ss_simulate(WCStateSpace* ss, const double* x0, const double* u_seq, double* x_hist, double* y_hist, int steps, double dt);
bool      wc_ss_is_stable(const WCStateSpace* ss);
bool      wc_ss_is_controllable(const WCStateSpace* ss);
bool      wc_ss_is_observable(const WCStateSpace* ss);
void      wc_ss_eigenvalues(const WCStateSpace* ss, double* re, double* im);
double    wc_ss_dcgain(const WCStateSpace* ss, int out_idx, int in_idx);

/* Closed-Loop API */
WCClosedLoop* wc_closedloop_create(WCTransferFunction* plant, WCTransferFunction* controller);
void      wc_closedloop_free(WCClosedLoop* cl);
void      wc_closedloop_analyze(WCClosedLoop* cl);
double    wc_closedloop_tracking_error(const WCClosedLoop* cl, double ref);
bool      wc_closedloop_nyquist_check(const WCClosedLoop* cl);

/* Servomechanism API */
WCServomechanism* wc_servo_create(double inertia, double damping, double stiffness);
void      wc_servo_free(WCServomechanism* sv);
void      wc_servo_step(WCServomechanism* sv, double torque_cmd, double dt);
void      wc_servo_position_control(WCServomechanism* sv, double target, double dt);
void      wc_servo_velocity_control(WCServomechanism* sv, double target_vel, double dt);

/* Anti-Aircraft Predictor API */
WCAntiAircraftPredictor* wc_aap_create(int history_len, double prediction_time);
void      wc_aap_free(WCAntiAircraftPredictor* aap);
void      wc_aap_add_observation(WCAntiAircraftPredictor* aap, double position, double time);
double    wc_aap_predict(WCAntiAircraftPredictor* aap);
double    wc_aap_predict_velocity(WCAntiAircraftPredictor* aap);

/* Cybernetic Regulator API */
WCCyberneticRegulator* wc_regulator_create(WCStateSpace* system, double kp, double ki, double kd);
void      wc_regulator_free(WCCyberneticRegulator* reg);
void      wc_regulator_set_reference(WCCyberneticRegulator* reg, const double* ref, int len);
void      wc_regulator_run(WCCyberneticRegulator* reg, int steps, double dt);
double    wc_regulator_ise(const WCCyberneticRegulator* reg);
double    wc_regulator_iae(const WCCyberneticRegulator* reg);
double    wc_regulator_itae(const WCCyberneticRegulator* reg);

#endif
