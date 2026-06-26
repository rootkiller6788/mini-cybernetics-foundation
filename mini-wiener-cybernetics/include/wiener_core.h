#ifndef WIENER_CORE_H
#define WIENER_CORE_H
#include <stdbool.h>
#include <stddef.h>

/* ==============================================================
 * Wiener Cybernetics -- Core Definitions
 * Based on: Wiener (1948) "Cybernetics: Control and Communication
 *            in the Animal and the Machine"
 * Companion source: wiener_core.c
 *
 * Central abstractions of Norbert Wiener's cybernetics:
 * feedback, information, control, communication, homeostasis.
 * ============================================================== */

/* Constants */
#define WC_EPSILON      1e-12
#define WC_MAX_DIM      128
#define WC_PI           3.14159265358979323846
#define WC_BOLTZMANN    1.380649e-23
#define WC_E             2.71828182845904523536

/* ---------- L1: Core Type Definitions -------------------- */

typedef struct {
    double* samples;
    int     length;
    int     capacity;
    double  dt;
    double  t0;
} WCSignal;

typedef struct {
    double* state;
    double* input;
    double* output;
    double* feedback;
    int     n_state;
    int     n_input;
    int     n_output;
    double  dt;
    double  t;
    void*   parameters;
} WCCyberneticSystem;

typedef struct {
    double  entropy;
    double  negentropy;
    double  mutual_info;
    double  channel_cap;
    int     n_symbols;
    double* prob_dist;
} WCInformation;

typedef struct {
    double  setpoint;
    double  error;
    double  control;
    double  gain;
    double  integral;
    double  derivative;
    double  kp, ki, kd;
    double  dt;
    double  integral_limit;
    bool    is_active;
} WCFeedbackLoop;

typedef struct {
    double  entropy_rate;
    double  information_flow;
    double  noise_power;
    double  signal_power;
    double  bandwidth;
    double  temperature;
} WCEntropyBudget;

/* ---------- L2: Core Concepts --------------------------- */

typedef struct {
    WCSignal*  control_signal;
    WCSignal*  communication_signal;
    double     coupling_strength;
    bool       is_unified;
} WCControlCommDuality;

typedef struct {
    double*   regulated_vars;
    double*   setpoints;
    double*   tolerances;
    double*   current_values;
    int       n_vars;
    bool      is_homeostatic;
    double    allostatic_load;
} WCHomeostasis;

typedef struct {
    WCCyberneticSystem* base_system;
    double*  adaptation_params;
    double   learning_rate;
    double   performance;
    double   best_performance;
    int      n_adaptations;
    bool     is_converging;
} WCSelfOrganizing;

typedef struct {
    double*  goal_state;
    double*  current_state;
    double   goal_error;
    double   approach_rate;
    bool     goal_reached;
    int      n_dimensions;
} WCGoalDirected;

/* ---------- Signal API ---------------------------------- */

WCSignal* wc_signal_create(int length, double dt, double t0);
WCSignal* wc_signal_create_from(const double* data, int length, double dt, double t0);
WCSignal* wc_signal_clone(const WCSignal* s);
void      wc_signal_free(WCSignal* s);
double    wc_signal_get(const WCSignal* s, int i);
void      wc_signal_set(WCSignal* s, int i, double val);
double    wc_signal_mean(const WCSignal* s);
double    wc_signal_variance(const WCSignal* s);
double    wc_signal_rms(const WCSignal* s);
double    wc_signal_energy(const WCSignal* s);
double    wc_signal_power(const WCSignal* s);
WCSignal* wc_signal_add(const WCSignal* a, const WCSignal* b);
WCSignal* wc_signal_sub(const WCSignal* a, const WCSignal* b);
WCSignal* wc_signal_mul(const WCSignal* a, double scalar);
WCSignal* wc_signal_convolve(const WCSignal* a, const WCSignal* b);
WCSignal* wc_signal_derivative(const WCSignal* s);
WCSignal* wc_signal_integral(const WCSignal* s);
void      wc_signal_print(const WCSignal* s, const char* label);

/* ---------- Cybernetic System API ----------------------- */

WCCyberneticSystem* wc_system_create(int n_state, int n_input, int n_output, double dt);
void      wc_system_free(WCCyberneticSystem* sys);
void      wc_system_set_state(WCCyberneticSystem* sys, const double* state);
void      wc_system_get_state(const WCCyberneticSystem* sys, double* state);
void      wc_system_set_input(WCCyberneticSystem* sys, const double* input);
void      wc_system_get_output(const WCCyberneticSystem* sys, double* output);
void      wc_system_step(WCCyberneticSystem* sys);
void      wc_system_simulate(WCCyberneticSystem* sys, int steps,
                             double* state_history, double* output_history);
void      wc_system_print(const WCCyberneticSystem* sys);

/* ---------- Feedback Loop API --------------------------- */

WCFeedbackLoop* wc_feedback_create(double kp, double ki, double kd, double dt);
void      wc_feedback_free(WCFeedbackLoop* fb);
void      wc_feedback_set_setpoint(WCFeedbackLoop* fb, double sp);
double    wc_feedback_update(WCFeedbackLoop* fb, double measurement);
void      wc_feedback_reset(WCFeedbackLoop* fb);
bool      wc_feedback_is_stable(const WCFeedbackLoop* fb);
double    wc_feedback_steady_error(const WCFeedbackLoop* fb);
double    wc_feedback_overshoot(const WCFeedbackLoop* fb);
double    wc_feedback_settling_time(const WCFeedbackLoop* fb, double tolerance);

/* ---------- Information API ----------------------------- */

WCInformation* wc_info_create(const double* prob_dist, int n_symbols);
void      wc_info_free(WCInformation* info);
double    wc_info_entropy(const WCInformation* info);
double    wc_info_max_entropy(int n_symbols);
double    wc_info_kldivergence(const WCInformation* p, const WCInformation* q);
double    wc_info_mutual_info(const WCInformation* joint, const WCInformation* marg_a,
                              const WCInformation* marg_b);
double    wc_info_channel_capacity(const WCInformation* info, double snr, double bw);

/* ---------- Entropy Budget API -------------------------- */

WCEntropyBudget* wc_entropy_create(double temp, double bw);
void      wc_entropy_free(WCEntropyBudget* eb);
void      wc_entropy_update(WCEntropyBudget* eb, double signal_power, double noise_power);
double    wc_entropy_rate(const WCEntropyBudget* eb);
double    wc_entropy_snr(const WCEntropyBudget* eb);
double    wc_entropy_capacity(const WCEntropyBudget* eb);

/* ---------- Homeostasis API ----------------------------- */

WCHomeostasis* wc_homeostasis_create(int n_vars);
void      wc_homeostasis_free(WCHomeostasis* hs);
void      wc_homeostasis_set_target(WCHomeostasis* hs, int idx, double setpoint, double tol);
void      wc_homeostasis_update(WCHomeostasis* hs, int idx, double value);
bool      wc_homeostasis_check(const WCHomeostasis* hs);
double    wc_homeostasis_deviation(const WCHomeostasis* hs);

/* ---------- Goal-Directed Behavior API ------------------ */

WCGoalDirected* wc_goal_create(int n_dims);
void      wc_goal_free(WCGoalDirected* gd);
void      wc_goal_set_target(WCGoalDirected* gd, const double* goal);
void      wc_goal_update(WCGoalDirected* gd, const double* state);
double    wc_goal_distance(const WCGoalDirected* gd);
bool      wc_goal_reached(const WCGoalDirected* gd, double tolerance);

/* ---------- Utility ------------------------------------- */

double    wc_max(double a, double b);
double    wc_min(double a, double b);
bool      wc_is_zero(double x);
bool      wc_is_equal(double a, double b);
double    wc_gaussian_random(void);
void      wc_generate_noise(double* buffer, int n, double sigma);
double    wc_sinc(double x);
double    wc_erf_approx(double x);

#endif /* WIENER_CORE_H */
