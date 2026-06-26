#ifndef CP_CORE_H
#define CP_CORE_H

#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * Control Philosophy (CP) ? Core Types and Structures
 *
 * Based on the foundational works of:
 *   Norbert Wiener         (Cybernetics, 1948)
 *   W. Ross Ashby          (Design for a Brain, 1952; Introduction to Cybernetics, 1956)
 *   Arturo Rosenblueth, Norbert Wiener, Julian Bigelow
 *                          (Behavior, Purpose and Teleology, 1943)
 *   John von Neumann       (Theory of Self-Reproducing Automata, 1966)
 *   Heinz von Foerster     (Observing Systems, 1981)
 *   Stafford Beer          (Brain of the Firm, 1972)
 *   Humberto Maturana & Francisco Varela (Autopoiesis and Cognition, 1980)
 *   Roger C. Conant & W. Ross Ashby
 *                          (Every Good Regulator of a System Must Be a Model
 *                           of That System, 1970)
 * ============================================================================ */

typedef enum {
    CP_OPEN_LOOP        = 0,
    CP_CLOSED_LOOP      = 1,
    CP_FEEDFORWARD      = 2,
    CP_ADAPTIVE         = 3,
    CP_PREDICTIVE       = 4,
    CP_INTELLIGENT      = 5
} ControlParadigm;

typedef enum {
    CP_NEGATIVE_FEEDBACK  = 0,
    CP_POSITIVE_FEEDBACK  = 1,
    CP_BALANCING_FEEDBACK = 2,
    CP_REINFORCING_FEEDBACK = 3
} FeedbackType;

typedef enum {
    CP_ASYMPTOTICALLY_STABLE = 0,
    CP_MARGINALLY_STABLE     = 1,
    CP_NEUTRAL               = 2,
    CP_UNSTABLE              = 3,
    CP_BIBO_STABLE           = 4,
    CP_STRUCTURALLY_UNSTABLE = 5
} StabilityClass;

typedef enum {
    CP_NO_OBSERVER         = 0,
    CP_LUENBERGER_OBSERVER = 1,
    CP_KALMAN_FILTER       = 2,
    CP_PARTICLE_FILTER     = 3,
    CP_SET_MEMBERSHIP      = 4
} ObserverType;

typedef enum {
    CP_EXECUTION_LAYER     = 0,
    CP_COORDINATION_LAYER  = 1,
    CP_ADAPTATION_LAYER    = 2,
    CP_STRATEGIC_LAYER     = 3,
    CP_META_LAYER          = 4
} HierarchyLevel;

typedef enum {
    CP_PASSIVE_PURPOSE     = 0,
    CP_PREDICTIVE_PURPOSE  = 1,
    CP_ACTIVE_PURPOSE      = 2,
    CP_PURPOSEFUL          = 3
} TeleologicalMode;

typedef enum {
    CP_REGULATOR_IDENTITY  = 0,
    CP_REGULATOR_LINEAR    = 1,
    CP_REGULATOR_NONLINEAR = 2,
    CP_REGULATOR_ADAPTIVE  = 3,
    CP_REGULATOR_PREDICTIVE = 4
} RegulatorType;

typedef enum {
    CP_AUTONOMY_NONE        = 0,
    CP_AUTONOMY_REACTIVE    = 1,
    CP_AUTONOMY_DELIBERATIVE = 2,
    CP_AUTONOMY_SELF_MODIFYING = 3,
    CP_AUTONOMY_FULL         = 4
} AutonomyLevel;

typedef struct {
    double* data;
    int rows;
    int cols;
    bool owns_data;
} CPMatrix;

typedef struct {
    double* components;
    int dimension;
} CPVector;

typedef struct {
    double real;
    double imag;
} CPComplex;

typedef struct {
    double entropy;
    double conditional_entropy;
    double mutual_information;
    double transfer_entropy;
    int    sample_size;
    int    alphabet_size;
} CPInfoMetrics;

typedef struct {
    char* name;
    ControlParadigm paradigm;
    TeleologicalMode teleology;
    double*  x;
    double*  x_dot;
    int      nx;
    double*  u;
    int      nu;
    double*  y;
    int      ny;
    double*  r;
    double*  goal_tolerance;
    double*  model_params;
    int      n_model_params;
    double*  x_hat;
    double*  estimation_error;
    ObserverType observer_type;
    FeedbackType feedback_mode;
    double   feedback_gain;
    double*  gain_matrix;
    double   cost_to_go;
    double   squared_error_sum;
    int      step_count;
    StabilityClass stability;
    double   lyapunov_value;
    double   lyapunov_derivative;
    CPInfoMetrics info;
} ControlSystem;

ControlSystem* cp_system_create(const char* name, int nx, int nu, int ny);
void cp_system_free(ControlSystem* sys);
void cp_system_set_reference(ControlSystem* sys, const double* r, const double* tolerance);
void cp_system_set_paradigm(ControlSystem* sys, ControlParadigm paradigm);
void cp_system_set_feedback(ControlSystem* sys, FeedbackType fb);
void cp_system_step(ControlSystem* sys, double dt);
void cp_system_simulate(ControlSystem* sys, double duration, double dt);
void cp_system_compute_error(ControlSystem* sys, double* error_out);
void cp_system_disturb(ControlSystem* sys, const double* disturbance);
double cp_system_lyapunov_value(const ControlSystem* sys);
double cp_system_lyapunov_derivative(ControlSystem* sys, double dt);
StabilityClass cp_system_classify_stability(const ControlSystem* sys, int window);
void cp_system_observability_gramian(const ControlSystem* sys, double* Wo_out);
void cp_system_controllability_gramian(const ControlSystem* sys, double* Wc_out);
bool cp_system_check_internal_model(const ControlSystem* sys, const double* disturbance_model);
void cp_system_compute_info_metrics(ControlSystem* sys, int history_length, int bins);
double cp_system_channel_capacity(const ControlSystem* sys);
void cp_system_print(const ControlSystem* sys);
double cp_matrix_determinant_2x2(double a, double b, double c, double d);
double cp_matrix_trace_2x2(double a, double b, double c, double d);
void   cp_matrix_mul(double* C, const double* A, const double* B, int m, int n, int p);
double cp_vector_norm(const double* v, int n);
double cp_vector_dot(const double* a, const double* b, int n);
void   cp_vector_scale(double* out, const double* v, double scalar, int n);
double cp_sigmoid(double x, double k, double x0);
double cp_clamp(double x, double lo, double hi);

#endif /* CP_CORE_H */
