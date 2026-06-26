#ifndef SOC_CORE_H
#define SOC_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ============================================================================
 * Second-Order Cybernetics — Core Types and Definitions
 *
 * "Cybernetics of cybernetics" — the study of observing systems rather than
 * observed systems, where the observer is understood to be part of the system.
 *
 * Foundational works:
 *   Heinz von Foerster     (Observing Systems, 1981; Cybernetics of Cybernetics, 1974)
 *   Humberto R. Maturana   (Autopoiesis and Cognition, 1980; with F. Varela)
 *   Francisco J. Varela    (Principles of Biological Autonomy, 1979)
 *   Gordon Pask            (Conversation Theory, 1975)
 *   Ernst von Glasersfeld  (Radical Constructivism, 1995)
 *   Niklas Luhmann         (Social Systems, 1984)
 *   George Spencer Brown   (Laws of Form, 1969)
 *   Ranulph Glanville      (The Object of Objects, 1975)
 *   Margaret Mead          (Cybernetics of Cybernetics, 1968)
 * ============================================================================ */

/* ---- Fundamental Enumerations ---- */

typedef enum {
    SOC_TRIVIAL_MACHINE = 0,
    SOC_NON_TRIVIAL_MACHINE = 1,
    SOC_OBSERVING_SYSTEM = 2,
    SOC_SELF_REFERENTIAL = 3,
    SOC_AUTOPOIETIC = 4,
    SOC_CONVERSATIONAL = 5,
    SOC_EIGENFORM = 6
} SOCSystemType;

typedef enum {
    SOC_OBSERVER_EXTERNAL = 0,
    SOC_OBSERVER_PARTICIPANT = 1,
    SOC_OBSERVER_SELF = 2,
    SOC_OBSERVER_RECURSIVE = 3,
    SOC_OBSERVER_META = 4
} SOCObserverStance;

typedef enum {
    SOC_DISTINCTION_PRIMARY = 0,
    SOC_DISTINCTION_INNER_OUTER = 1,
    SOC_DISTINCTION_SELF_OTHER = 2,
    SOC_DISTINCTION_TRUE_FALSE = 3,
    SOC_DISTINCTION_FIGURE_GROUND = 4,
    SOC_DISTINCTION_FORM_MEDIUM = 5
} SOCDistinctionType;

typedef enum {
    SOC_SR_DIRECT = 0,
    SOC_SR_INDIRECT = 1,
    SOC_SR_STRANGE_LOOP = 2,
    SOC_SR_PARADOXICAL = 3,
    SOC_SR_PRODUCTIVE = 4,
    SOC_SR_EIGENFORM = 5
} SOCSelfReferenceType;

typedef enum {
    SOC_EIGEN_STABLE = 0,
    SOC_EIGEN_UNSTABLE = 1,
    SOC_EIGEN_SADDLE = 2,
    SOC_EIGEN_CYCLE = 3,
    SOC_EIGEN_CHAOTIC = 4,
    SOC_EIGEN_UNDEFINED = 5
} SOCEigenStability;

/* ---- Generic Numeric and Vector Types ---- */

typedef struct {
    double* data;
    int dim;
} SOCVector;

typedef struct {
    double* data;
    int rows;
    int cols;
} SOCMatrix;

typedef struct {
    double real;
    double imag;
    bool is_complex;
} SOCComplex;

/* ============================================================================
 * Non-Trivial Machine (NTM) — von Foerster's core concept
 *
 * An NTM is defined by: (S, X, Y, f: SxX->S, g: SxX->Y)
 * Unlike a trivial machine (y = F(x) is constant for fixed F),
 * the NTM's output is history-dependent through its internal state.
 * To "understand" an NTM, an observer must itself be an NTM.
 * This is the foundational insight of second-order cybernetics.
 *
 * Theorem (von Foerster): For any NTM M, there exists another NTM M'
 * that can predict M if and only if M' is at least as complex as M.
 * ============================================================================ */

typedef struct SOCNonTrivialMachine {
    int state_dim;
    int input_dim;
    int output_dim;

    double* state;          /* current internal state */

    /* Linear model: s' = A*s + B*x,  y = C*s + D*x */
    double* A;              /* state_dim x state_dim */
    double* B;              /* state_dim x input_dim  */
    double* C;              /* output_dim x state_dim */
    double* D;              /* output_dim x input_dim  */

    /* Nonlinear model: function pointers */
    void (*f_transition)(const double* s, const double* x, double* s_next, void* ctx);
    void (*g_output)(const double* s, const double* x, double* y, void* ctx);
    void* f_ctx;
    void* g_ctx;

    /* State history for distinguishability analysis */
    double* history;
    int history_len;
    int history_cap;

    int distinguishable_states;

    /* Eigenbehaviors: stable patterns of the NTM */
    SOCVector** eigenforms;
    int n_eigenforms;

    bool is_linear;
    bool is_deterministic;
    SOCSystemType type;
    char* label;
} SOCNonTrivialMachine;

/* ============================================================================
 * Observer — the core of second-order cybernetics
 *
 * Key principle (Maturana & Varela):
 * "Anything said is said by an observer."
 * The observer's model constitutes a non-trivial machine understood
 * in terms of its own structure — not external reality.
 * ============================================================================ */

typedef struct SOCObserver {
    char* label;
    SOCObserverStance stance;

    double* belief_state;
    int belief_dim;

    double* observation_matrix;  /* H: maps true state to sensory observation */
    int obs_dim;
    int world_dim;

    SOCDistinctionType* distinctions;
    int n_distinctions;

    double* self_model;
    int self_model_dim;

    bool has_self_observation;
    double self_observation_gain;  /* kappa in the eigenform equation */

    struct SOCNonTrivialMachine* observed_system;

    double* blind_spot;         /* what the observer structurally cannot see */
    int blind_spot_dim;

    double* observation_history;
    int history_len;
    int history_cap;

    void (*send_message)(const double* msg, int len, void* recipient);
    void* communication_context;
} SOCObserver;

/* ============================================================================
 * Eigenform — stable self-referential pattern
 *
 * von Foerster's eigenform equation:  x = Phi(x)
 * The eigenform is the fixed point of recursive application.
 * Cognition = computation of eigenforms.
 *
 * Theorem (von Foerster): For any recursive operator Phi on a complete
 * metric space, if ||Phi'|| < 1, there exists a unique eigenform x* = Phi(x*).
 * ============================================================================ */

typedef struct SOCEigenform {
    void (*phi)(const double* x, int dim, double* result, void* ctx);
    void* phi_ctx;

    int dimension;
    double convergence_tol;
    int max_iterations;

    SOCVector* solution;        /* the computed eigenform x = Phi(x) */
    SOCEigenStability stability;
    double eigenvalue;
    int iterations_used;
    double residual_norm;       /* ||x - Phi(x)|| at termination */

    double* convergence_path;
    int path_len;
    int path_cap;

    SOCVector** all_solutions;
    int n_solutions;

    SOCSelfReferenceType sr_type;
    bool converged;
} SOCEigenform;

/* ============================================================================
 * Circular Closure — the logical structure of self-reference
 *
 * Circularity is not a logical flaw but a fundamental organizational
 * principle. Four forms: operational, organizational, semantic,
 * and epistemological closure.
 *
 * Theorem (von Foerster): A system with full circular closure has the
 * property that its description requires reference to itself.
 * This is the defining property of living systems.
 * ============================================================================ */

typedef struct SOCCircularClosure {
    SOCSelfReferenceType closure_type;
    void (*closure_op)(const double* input, int dim, double* output, void* ctx);
    void* closure_ctx;

    int dimension;
    SOCVector* fixed_point;
    SOCEigenStability stability;
    double* jacobian;

    struct SOCObserver* enclosed_observer;

    double closure_degree;
    double perturbation_response_rate;

    double* trajectory;
    int traj_len;
    int traj_cap;
} SOCCircularClosure;

/* ============================================================================
 * Distinction — Spencer Brown's fundamental operation
 *
 * "Draw a distinction." — Laws of Form (1969)
 * A distinction is the most primitive cognitive operation.
 * The calculus of indications provides a formal system for
 * reasoning about distinctions.
 * ============================================================================ */

typedef struct SOCDistinction {
    SOCDistinctionType type;
    char* label;
    int marked_value;
    int unmarked_value;

    bool (*boundary)(const double* point, int dim, void* ctx);
    void* boundary_ctx;
    int space_dim;

    bool (*cross_fn)(bool state);
    bool current_state;

    bool (*re_entry_fn)(void* self);
} SOCDistinction;

/* ============================================================================
 * Conversation — Pask's theory of cognitive interaction
 *
 * Conversation Theory formalizes how understanding emerges through
 * recursive interaction. L-space (conceptual), P-individual (processor),
 * M-individual (mechanical pair), entailment mesh, teachback.
 *
 * Theorem (Pask): Agreement converges iff participants share an
 * entailment structure.
 * ============================================================================ */

typedef struct SOCConversation {
    char* topic;
    double* conceptual_space;
    int concept_dim;
    int n_concepts;

    double agreement_level;

    struct SOCObserver* participant_a;
    struct SOCObserver* participant_b;

    double* entailment_matrix;
    int entailment_dim;

    double* turn_history;
    int n_turns;
    int max_turns;

    double* shared_model;
    int shared_dim;

    double teachback_fidelity;
    bool stabilized;
    double stabilization_rate;
} SOCConversation;

/* ============================================================================
 * Second-Order Observation — Luhmann's key distinction
 *
 * First-order: observe WHAT is observed
 * Second-order: observe HOW someone observes
 * ============================================================================ */

typedef struct {
    SOCObserver* observer;
    SOCObserver* observer_of_observer;
    double* observed_distinctions;
    int n_observed_distinctions;
    double* observed_blind_spot;
    int meta_blind_dim;
} SOCSecondOrderObservation;

/* ---- Non-Trivial Machine API ---- */

SOCNonTrivialMachine* soc_ntm_create(int state_dim, int input_dim,
                                      int output_dim, const char* label);
void soc_ntm_free(SOCNonTrivialMachine* ntm);
void soc_ntm_set_linear(SOCNonTrivialMachine* ntm,
                         const double* A, const double* B,
                         const double* C, const double* D);
void soc_ntm_set_nonlinear(SOCNonTrivialMachine* ntm,
                            void (*f)(const double*, const double*, double*, void*),
                            void (*g)(const double*, const double*, double*, void*),
                            void* ctx);
void soc_ntm_step(SOCNonTrivialMachine* ntm, const double* input, double* output);
void soc_ntm_reset(SOCNonTrivialMachine* ntm);
int soc_ntm_count_distinguishable_states(SOCNonTrivialMachine* ntm, int steps);
void soc_ntm_compute_eigenbehaviors(SOCNonTrivialMachine* ntm, int max_iters);
void soc_ntm_print(const SOCNonTrivialMachine* ntm);
bool soc_ntm_is_trivial(const SOCNonTrivialMachine* ntm);
double soc_ntm_predictability(SOCNonTrivialMachine* ntm, int n_trials);

/* ---- Observer API ---- */

SOCObserver* soc_observer_create(const char* label, SOCObserverStance stance,
                                  int world_dim, int obs_dim);
void soc_observer_free(SOCObserver* obs);
void soc_observer_set_observation_matrix(SOCObserver* obs, const double* H);
void soc_observer_observe(SOCObserver* obs, const double* world_state,
                           double* observation);
void soc_observer_update_belief(SOCObserver* obs, const double* observation);
void soc_observer_self_observe(SOCObserver* obs, double* self_obs);
void soc_observer_add_distinction(SOCObserver* obs, SOCDistinctionType d);
void soc_observer_set_self_gain(SOCObserver* obs, double gain);
void soc_observer_print(const SOCObserver* obs);
void soc_observer_attach_system(SOCObserver* obs, SOCNonTrivialMachine* ntm);
double soc_observer_blind_spot_size(const SOCObserver* obs);

/* ---- Eigenform API ---- */

SOCEigenform* soc_eigenform_create(int dimension,
                                    void (*phi)(const double*, int, double*, void*),
                                    void* ctx);
void soc_eigenform_free(SOCEigenform* ef);
bool soc_eigenform_solve(SOCEigenform* ef, const double* initial_guess);
SOCEigenStability soc_eigenform_classify(SOCEigenform* ef, double eps);
void soc_eigenform_find_all(SOCEigenform* ef, int n_starts,
                             const double* start_points);
void soc_eigenform_verify(SOCEigenform* ef);
void soc_eigenform_print(const SOCEigenform* ef);
bool soc_eigenform_is_trivial(const SOCEigenform* ef);

/* ---- Circular Closure API ---- */

SOCCircularClosure* soc_closure_create(int dimension,
                                        void (*op)(const double*, int, double*, void*),
                                        void* ctx);
void soc_closure_free(SOCCircularClosure* cl);
bool soc_closure_compute_fixed_point(SOCCircularClosure* cl,
                                      const double* initial, int max_iters);
SOCEigenStability soc_closure_analyze_stability(SOCCircularClosure* cl, double eps);
double soc_closure_measure_closedness(SOCCircularClosure* cl);
void soc_closure_simulate_perturbation(SOCCircularClosure* cl,
                                        const double* perturbation, int n_steps);
void soc_closure_print(const SOCCircularClosure* cl);

/* ---- Distinction API ---- */

SOCDistinction* soc_distinction_create(SOCDistinctionType type, const char* label,
                                        bool (*boundary)(const double*, int, void*),
                                        void* ctx, int space_dim);
void soc_distinction_free(SOCDistinction* d);
bool soc_distinction_indicate(SOCDistinction* d, const double* point);
bool soc_distinction_cross_op(SOCDistinction* d);
bool soc_distinction_re_entry(SOCDistinction* d);
void soc_distinction_set_state(SOCDistinction* d, bool state);
void soc_distinction_print(const SOCDistinction* d);

/* ---- Conversation API ---- */

SOCConversation* soc_conversation_create(const char* topic, int concept_dim,
                                          SOCObserver* a, SOCObserver* b);
void soc_conversation_free(SOCConversation* conv);
void soc_conversation_add_concept(SOCConversation* conv,
                                   const double* concept_vec, int dim);
void soc_conversation_set_entailment(SOCConversation* conv,
                                      const double* M, int dim);
double soc_conversation_measure_agreement(SOCConversation* conv);
void soc_conversation_turn(SOCConversation* conv);
bool soc_conversation_has_converged(SOCConversation* conv);
void soc_conversation_print(const SOCConversation* conv);

/* ---- Second-Order Observation API ---- */

SOCSecondOrderObservation* soc_soo_create(SOCObserver* obs,
                                           SOCObserver* meta_obs);
void soc_soo_free(SOCSecondOrderObservation* soo);
void soc_soo_analyze(SOCSecondOrderObservation* soo);
void soc_soo_print(const SOCSecondOrderObservation* soo);

/* ---- Utility Functions ---- */

double soc_vector_norm(const double* v, int n);
double soc_vector_dot(const double* a, const double* b, int n);
void soc_vector_copy(const double* src, double* dst, int n);
void soc_vector_scale(double* v, int n, double alpha);
void soc_vector_add(const double* a, const double* b, double* result, int n);
void soc_vector_sub(const double* a, const double* b, double* result, int n);
void soc_matrix_vec_mul(const double* A, const double* v, double* result,
                         int rows, int cols);
void soc_matrix_mul(const double* A, const double* B, double* C,
                     int m, int n, int p);
double soc_entropy(const double* probs, int n);
double soc_kldiv(const double* p, const double* q, int n);
double soc_cosine_similarity(const double* a, const double* b, int n);
double soc_fixed_point_iteration(double (*f)(double, void*), void* ctx,
                                  double x0, double tol, int max_iter,
                                  int* iters);
double* soc_eigenvalues_2x2(double a, double b, double c, double d);
double soc_matrix_norm(const double* A, int rows, int cols);

#endif /* SOC_CORE_H */
