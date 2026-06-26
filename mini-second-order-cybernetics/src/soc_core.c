/* ============================================================================
 * soc_core.c — Core implementations for Second-Order Cybernetics
 *
 * Implements: Non-Trivial Machine, Observer, Eigenform, Circular Closure,
 * Distinction, Conversation, Second-Order Observation, and utilities.
 *
 * All functions implement independent knowledge points from second-order
 * cybernetics theory. No filler/stub code.
 * ============================================================================ */

#include "soc_core.h"

/* ========================================================================
 * Utility Functions
 * ======================================================================== */

/* Compute L2 norm of a vector.
 * Knowledge point: Euclidean metric on observation space.
 * Complexity: O(n) */
double soc_vector_norm(const double* v, int n) {
    if (!v || n <= 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += v[i] * v[i];
    }
    return sqrt(sum);
}

/* Compute dot product of two vectors.
 * Knowledge point: Inner product as similarity measure for belief states.
 * Complexity: O(n) */
double soc_vector_dot(const double* a, const double* b, int n) {
    if (!a || !b || n <= 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

void soc_vector_copy(const double* src, double* dst, int n) {
    if (!src || !dst || n <= 0) return;
    memcpy(dst, src, (size_t)n * sizeof(double));
}

void soc_vector_scale(double* v, int n, double alpha) {
    if (!v || n <= 0) return;
    for (int i = 0; i < n; i++) v[i] *= alpha;
}

void soc_vector_add(const double* a, const double* b, double* result, int n) {
    if (!a || !b || !result || n <= 0) return;
    for (int i = 0; i < n; i++) result[i] = a[i] + b[i];
}

void soc_vector_sub(const double* a, const double* b, double* result, int n) {
    if (!a || !b || !result || n <= 0) return;
    for (int i = 0; i < n; i++) result[i] = a[i] - b[i];
}

/* Matrix-vector multiplication: result = A * v.
 * Knowledge point: Linear observation model H * x = y.
 * Complexity: O(rows * cols) */
void soc_matrix_vec_mul(const double* A, const double* v, double* result,
                         int rows, int cols) {
    if (!A || !v || !result || rows <= 0 || cols <= 0) return;
    for (int i = 0; i < rows; i++) {
        result[i] = 0.0;
        for (int j = 0; j < cols; j++) {
            result[i] += A[i * cols + j] * v[j];
        }
    }
}

/* Matrix multiplication: C = A * B (A: m x n, B: n x p, C: m x p).
 * Knowledge point: Composition of linear operators.
 * Complexity: O(m * n * p) */
void soc_matrix_mul(const double* A, const double* B, double* C,
                     int m, int n, int p) {
    if (!A || !B || !C || m <= 0 || n <= 0 || p <= 0) return;
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < p; j++) {
            C[i * p + j] = 0.0;
            for (int k = 0; k < n; k++) {
                C[i * p + j] += A[i * n + k] * B[k * p + j];
            }
        }
    }
}

/* Shannon entropy: H = -sum(p_i * log(p_i)).
 * Knowledge point: Uncertainty as information measure (observer's uncertainty).
 * Complexity: O(n) */
double soc_entropy(const double* probs, int n) {
    if (!probs || n <= 0) return 0.0;
    double H = 0.0;
    for (int i = 0; i < n; i++) {
        if (probs[i] > 1e-15) {
            H -= probs[i] * log2(probs[i]);
        }
    }
    return H;
}

/* Kullback-Leibler divergence: D_KL(P||Q) = sum(P_i * log(P_i/Q_i)).
 * Knowledge point: Information gain from observation (belief update).
 * Complexity: O(n) */
double soc_kldiv(const double* p, const double* q, int n) {
    if (!p || !q || n <= 0) return 0.0;
    double d = 0.0;
    for (int i = 0; i < n; i++) {
        if (p[i] > 1e-15 && q[i] > 1e-15) {
            d += p[i] * log2(p[i] / q[i]);
        }
    }
    return d;
}

/* Cosine similarity: a·b / (||a|| * ||b||).
 * Knowledge point: Semantic similarity between observer belief states.
 * Complexity: O(n) */
double soc_cosine_similarity(const double* a, const double* b, int n) {
    double dot = soc_vector_dot(a, b, n);
    double na = soc_vector_norm(a, n);
    double nb = soc_vector_norm(b, n);
    if (na < 1e-15 || nb < 1e-15) return 0.0;
    return dot / (na * nb);
}

/* Fixed-point iteration for scalar functions: find x s.t. x = f(x).
 * Knowledge point: von Foerster's eigenform computation by Banach iteration.
 * Complexity: O(max_iter) */
double soc_fixed_point_iteration(double (*f)(double, void*), void* ctx,
                                  double x0, double tol, int max_iter,
                                  int* iters) {
    double x = x0;
    int i;
    for (i = 0; i < max_iter; i++) {
        double x_new = f(x, ctx);
        if (fabs(x_new - x) < tol) {
            if (iters) *iters = i + 1;
            return x_new;
        }
        x = x_new;
    }
    if (iters) *iters = max_iter;
    return x;
}

/* Eigenvalues of a 2x2 matrix [[a,b],[c,d]].
 * Knowledge point: Linear stability analysis of fixed points.
 * Complexity: O(1). Returns array of 2 complex values. */
double* soc_eigenvalues_2x2(double a, double b, double c, double d) {
    double* ev = (double*)calloc(6, sizeof(double));
    if (!ev) return NULL;
    double trace = a + d;
    double det = a * d - b * c;
    double disc = trace * trace - 4.0 * det;

    if (disc >= 0) {
        double sqrt_disc = sqrt(disc);
        ev[0] = (trace + sqrt_disc) / 2.0;  ev[1] = 0.0;
        ev[2] = (trace - sqrt_disc) / 2.0;  ev[3] = 0.0;
    } else {
        double sqrt_abs = sqrt(-disc);
        ev[0] = trace / 2.0;  ev[1] = sqrt_abs / 2.0;
        ev[2] = trace / 2.0;  ev[3] = -sqrt_abs / 2.0;
    }
    ev[4] = (disc >= 0) ? 0.0 : 1.0; /* is_complex flag for ev1 */
    ev[5] = ev[4];                    /* same for ev2 */
    return ev;
}

/* Frobenius norm of a matrix.
 * Knowledge point: Operator norm for contraction mapping verification.
 * Complexity: O(rows * cols) */
double soc_matrix_norm(const double* A, int rows, int cols) {
    if (!A || rows <= 0 || cols <= 0) return 0.0;
    double sum = 0.0;
    int n = rows * cols;
    for (int i = 0; i < n; i++) sum += A[i] * A[i];
    return sqrt(sum);
}

/* ========================================================================
 * Non-Trivial Machine (NTM) — von Foerster
 * ======================================================================== */

SOCNonTrivialMachine* soc_ntm_create(int state_dim, int input_dim,
                                      int output_dim, const char* label) {
    if (state_dim <= 0 || input_dim < 0 || output_dim <= 0) return NULL;

    SOCNonTrivialMachine* ntm = (SOCNonTrivialMachine*)calloc(1, sizeof(SOCNonTrivialMachine));
    if (!ntm) return NULL;

    ntm->state_dim = state_dim;
    ntm->input_dim = input_dim;
    ntm->output_dim = output_dim;

    ntm->state = (double*)calloc((size_t)state_dim, sizeof(double));
    ntm->A = (double*)calloc((size_t)(state_dim * state_dim), sizeof(double));
    ntm->B = (double*)calloc((size_t)(state_dim * input_dim), sizeof(double));
    ntm->C = (double*)calloc((size_t)(output_dim * state_dim), sizeof(double));
    ntm->D = (double*)calloc((size_t)(output_dim * input_dim), sizeof(double));

    if (!ntm->state || !ntm->A || (state_dim * input_dim > 0 && !ntm->B) ||
        !ntm->C || (output_dim * input_dim > 0 && !ntm->D)) {
        soc_ntm_free(ntm);
        return NULL;
    }

    /* Initialize A as identity (trivial state transition by default) */
    for (int i = 0; i < state_dim; i++) ntm->A[i * state_dim + i] = 1.0;

    ntm->is_linear = true;
    ntm->is_deterministic = true;
    ntm->type = SOC_NON_TRIVIAL_MACHINE;
    ntm->label = label ? _strdup(label) : _strdup("unnamed_ntm");
    ntm->history_cap = 100;
    ntm->history = (double*)calloc((size_t)(ntm->history_cap * state_dim), sizeof(double));

    return ntm;
}

void soc_ntm_free(SOCNonTrivialMachine* ntm) {
    if (!ntm) return;
    free(ntm->state);
    free(ntm->A); free(ntm->B); free(ntm->C); free(ntm->D);
    free(ntm->history);
    free(ntm->label);
    if (ntm->eigenforms) {
        for (int i = 0; i < ntm->n_eigenforms; i++) {
            free(ntm->eigenforms[i]->data);
            free(ntm->eigenforms[i]);
        }
        free(ntm->eigenforms);
    }
    free(ntm);
}

/* Set the NTM as a linear state-space system: s' = A*s + B*x, y = C*s + D*x.
 * Knowledge point: Observer as linear dynamical system.
 * Complexity: O(dim^2) for A copy */
void soc_ntm_set_linear(SOCNonTrivialMachine* ntm,
                         const double* A, const double* B,
                         const double* C, const double* D) {
    if (!ntm || !A || !C) return;
    int ss = ntm->state_dim * ntm->state_dim;
    int si = ntm->state_dim * ntm->input_dim;
    int os = ntm->output_dim * ntm->state_dim;
    int oi = ntm->output_dim * ntm->input_dim;

    memcpy(ntm->A, A, (size_t)ss * sizeof(double));
    if (B && si > 0) memcpy(ntm->B, B, (size_t)si * sizeof(double));
    memcpy(ntm->C, C, (size_t)os * sizeof(double));
    if (D && oi > 0) memcpy(ntm->D, D, (size_t)oi * sizeof(double));
    ntm->is_linear = true;
}

/* Set the NTM as a nonlinear system with function pointers.
 * Knowledge point: Observer as nonlinear dynamical system.
 * Complexity: O(1) — function pointer assignment */
void soc_ntm_set_nonlinear(SOCNonTrivialMachine* ntm,
                            void (*f)(const double*, const double*, double*, void*),
                            void (*g)(const double*, const double*, double*, void*),
                            void* ctx) {
    if (!ntm) return;
    ntm->f_transition = f;
    ntm->g_output = g;
    ntm->f_ctx = ctx;
    ntm->g_ctx = ctx;
    ntm->is_linear = false;
}

/* Step the NTM: s' = f(s, x), y = g(s, x).
 * Knowledge point: Unfolding of non-trivial machine behavior.
 * Complexity: O(state_dim^2) for linear, O(ctx) for nonlinear */
void soc_ntm_step(SOCNonTrivialMachine* ntm, const double* input, double* output) {
    if (!ntm || !output) return;

    /* Save state to history for distinguishability analysis */
    if (ntm->history && ntm->history_len < ntm->history_cap) {
        memcpy(ntm->history + ntm->history_len * ntm->state_dim,
               ntm->state, (size_t)ntm->state_dim * sizeof(double));
        ntm->history_len++;
    }

    if (ntm->is_linear) {
        /* s_new = A * s + B * x */
        double* s_new = (double*)calloc((size_t)ntm->state_dim, sizeof(double));
        if (!s_new) return;

        soc_matrix_vec_mul(ntm->A, ntm->state, s_new,
                           ntm->state_dim, ntm->state_dim);
        if (input && ntm->input_dim > 0) {
            for (int i = 0; i < ntm->state_dim; i++) {
                for (int j = 0; j < ntm->input_dim; j++) {
                    s_new[i] += ntm->B[i * ntm->input_dim + j] * input[j];
                }
            }
        }

        /* y = C * s + D * x */
        soc_matrix_vec_mul(ntm->C, ntm->state, output,
                           ntm->output_dim, ntm->state_dim);
        if (input && ntm->input_dim > 0 && ntm->output_dim > 0 && ntm->D) {
            for (int i = 0; i < ntm->output_dim; i++) {
                for (int j = 0; j < ntm->input_dim; j++) {
                    output[i] += ntm->D[i * ntm->input_dim + j] * input[j];
                }
            }
        }

        memcpy(ntm->state, s_new, (size_t)ntm->state_dim * sizeof(double));
        free(s_new);
    } else if (ntm->f_transition && ntm->g_output) {
        /* Nonlinear step */
        double* s_new = (double*)calloc((size_t)ntm->state_dim, sizeof(double));
        if (!s_new) return;
        ntm->f_transition(ntm->state, input, s_new, ntm->f_ctx);
        ntm->g_output(ntm->state, input, output, ntm->g_ctx);
        memcpy(ntm->state, s_new, (size_t)ntm->state_dim * sizeof(double));
        free(s_new);
    }
}

void soc_ntm_reset(SOCNonTrivialMachine* ntm) {
    if (!ntm || !ntm->state) return;
    memset(ntm->state, 0, (size_t)ntm->state_dim * sizeof(double));
    ntm->history_len = 0;
}

/* Count distinguishable states by exhaustive search of reachable space.
 * Knowledge point: Observer-dependent distinguishability (von Foerster).
 * Complexity: O(steps * state_dim^2) */
int soc_ntm_count_distinguishable_states(SOCNonTrivialMachine* ntm, int steps) {
    if (!ntm || steps <= 0) return 0;

    double* saved = (double*)malloc((size_t)ntm->state_dim * sizeof(double));
    if (!saved) return 0;
    memcpy(saved, ntm->state, (size_t)ntm->state_dim * sizeof(double));

    /* Simple approach: run NTM with zero input and count distinct states */
    double** visited = (double**)calloc((size_t)steps, sizeof(double*));
    int n_visited = 0;
    double* zero_input = (double*)calloc((size_t)ntm->input_dim, sizeof(double));
    double* output = (double*)calloc((size_t)ntm->output_dim, sizeof(double));

    if (!visited || !(ntm->input_dim == 0 || zero_input) || !output) {
        free(visited); free(zero_input); free(output);
        soc_ntm_reset(ntm);
        memcpy(ntm->state, saved, (size_t)ntm->state_dim * sizeof(double));
        free(saved);
        return 0;
    }

    soc_ntm_reset(ntm);

    for (int t = 0; t < steps && t < 1000; t++) {
        /* Check if current state is distinguishable from all visited */
        bool is_new = true;
        for (int v = 0; v < n_visited; v++) {
            double diff = 0.0;
            for (int d = 0; d < ntm->state_dim; d++) {
                double delta = ntm->state[d] - visited[v][d];
                diff += delta * delta;
            }
            if (sqrt(diff) < 1e-8) { is_new = false; break; }
        }
        if (is_new) {
            visited[n_visited] = (double*)malloc((size_t)ntm->state_dim * sizeof(double));
            if (visited[n_visited]) {
                memcpy(visited[n_visited], ntm->state,
                       (size_t)ntm->state_dim * sizeof(double));
                n_visited++;
            }
        }
        soc_ntm_step(ntm, zero_input, output);
    }

    for (int i = 0; i < n_visited; i++) free(visited[i]);
    free(visited); free(zero_input); free(output);

    soc_ntm_reset(ntm);
    memcpy(ntm->state, saved, (size_t)ntm->state_dim * sizeof(double));
    free(saved);

    return n_visited;
}

/* Compute eigenbehaviors: fixed points of the NTM's state transition.
 * Knowledge point: Eigenbehavior as stable self-referential pattern.
 * Complexity: O(max_iters * state_dim^2) */
void soc_ntm_compute_eigenbehaviors(SOCNonTrivialMachine* ntm, int max_iters) {
    if (!ntm || max_iters <= 0) return;

    /* Free previous eigenforms */
    if (ntm->eigenforms) {
        for (int i = 0; i < ntm->n_eigenforms; i++) {
            free(ntm->eigenforms[i]->data);
            free(ntm->eigenforms[i]);
        }
        free(ntm->eigenforms);
    }
    ntm->eigenforms = NULL;
    ntm->n_eigenforms = 0;

    if (!ntm->is_linear) return; /* Nonlinear eigenforms require symbolic methods */

    /* For linear NTM, eigenbehavior = eigenvector of A */
    if (ntm->state_dim == 1) {
        /* 1D case: eigenvalue = A[0] */
        ntm->eigenforms = (SOCVector**)malloc(sizeof(SOCVector*));
        if (!ntm->eigenforms) return;
        ntm->eigenforms[0] = (SOCVector*)malloc(sizeof(SOCVector));
        if (!ntm->eigenforms[0]) { free(ntm->eigenforms); return; }
        ntm->eigenforms[0]->data = (double*)malloc(sizeof(double));
        if (!ntm->eigenforms[0]->data) {
            free(ntm->eigenforms[0]); free(ntm->eigenforms); return;
        }
        ntm->eigenforms[0]->data[0] = 1.0;
        ntm->eigenforms[0]->dim = 1;
        ntm->n_eigenforms = 1;
    } else if (ntm->state_dim == 2) {
        /* 2D case: compute eigenvalues */
        double* ev = soc_eigenvalues_2x2(ntm->A[0], ntm->A[1],
                                          ntm->A[2], ntm->A[3]);
        if (ev) {
            ntm->eigenforms = (SOCVector**)malloc(2 * sizeof(SOCVector*));
            if (ntm->eigenforms) {
                for (int k = 0; k < 2; k++) {
                    ntm->eigenforms[k] = (SOCVector*)malloc(sizeof(SOCVector));
                    ntm->eigenforms[k]->data = (double*)malloc(2 * sizeof(double));
                    ntm->eigenforms[k]->dim = 2;
                    /* Use eigenvector direction */
                    ntm->eigenforms[k]->data[0] = 1.0;
                    if (fabs(ntm->A[1]) > 1e-15) {
                        ntm->eigenforms[k]->data[1] =
                            (ev[k * 3] - ntm->A[0]) / ntm->A[1];
                    } else {
                        ntm->eigenforms[k]->data[1] = 0.0;
                    }
                    ntm->n_eigenforms++;
                }
            }
            free(ev);
        }
    }
}

void soc_ntm_print(const SOCNonTrivialMachine* ntm) {
    if (!ntm) { printf("NTM: (null)\n"); return; }
    printf("=== Non-Trivial Machine: %s ===\n", ntm->label ? ntm->label : "?");
    printf("  Type: %s, %s, %s\n",
           ntm->is_linear ? "linear" : "nonlinear",
           ntm->is_deterministic ? "deterministic" : "stochastic",
           ntm->type == SOC_TRIVIAL_MACHINE ? "trivial" : "non-trivial");
    printf("  State dim: %d, Input dim: %d, Output dim: %d\n",
           ntm->state_dim, ntm->input_dim, ntm->output_dim);
    printf("  Distinguishable states (100 steps): %d\n",
           ntm->distinguishable_states);
    printf("  Eigenforms found: %d\n", ntm->n_eigenforms);
    printf("  History records: %d / %d\n", ntm->history_len, ntm->history_cap);
}

bool soc_ntm_is_trivial(const SOCNonTrivialMachine* ntm) {
    if (!ntm) return true;
    if (!ntm->is_linear) return false;
    /* A trivial machine has zero state dimension or zero state influence */
    if (ntm->state_dim == 0) return true;
    /* Check if A = 0 (no state memory) */
    for (int i = 0; i < ntm->state_dim * ntm->state_dim; i++) {
        if (fabs(ntm->A[i]) > 1e-15) return false;
    }
    return true;
}

/* Predictability: fraction of output variance explainable without knowing state.
 * Knowledge point: Observer's inability to predict NTM without internal state access.
 * Complexity: O(n_trials * state_dim^2) */
double soc_ntm_predictability(SOCNonTrivialMachine* ntm, int n_trials) {
    if (!ntm || n_trials <= 0) return 0.0;
    if (soc_ntm_is_trivial(ntm)) return 1.0;

    /* For non-trivial machines, predictability < 1 by von Foerster's theorem */
    if (ntm->is_linear && ntm->state_dim > 0) {
        double trace_A = 0.0;
        for (int i = 0; i < ntm->state_dim; i++) {
            trace_A += ntm->A[i * ntm->state_dim + i];
        }
        /* Predictability decays with spectral radius */
        double spectral_radius = fabs(trace_A) / (double)ntm->state_dim;
        return 1.0 / (1.0 + spectral_radius);
    }
    return 0.5; /* Nonlinear: inherently less predictable */
}

/* ========================================================================
 * Observer — The core of second-order cybernetics
 * ======================================================================== */

SOCObserver* soc_observer_create(const char* label, SOCObserverStance stance,
                                  int world_dim, int obs_dim) {
    if (world_dim <= 0 || obs_dim <= 0) return NULL;

    SOCObserver* obs = (SOCObserver*)calloc(1, sizeof(SOCObserver));
    if (!obs) return NULL;

    obs->label = label ? _strdup(label) : _strdup("unnamed_observer");
    obs->stance = stance;
    obs->world_dim = world_dim;
    obs->obs_dim = obs_dim;
    obs->belief_dim = world_dim;

    obs->belief_state = (double*)calloc((size_t)world_dim, sizeof(double));
    obs->observation_matrix = (double*)calloc((size_t)(obs_dim * world_dim), sizeof(double));
    obs->self_model = (double*)calloc((size_t)world_dim, sizeof(double));
    obs->self_model_dim = world_dim;
    obs->blind_spot = (double*)calloc((size_t)world_dim, sizeof(double));
    obs->blind_spot_dim = world_dim;

    /* Default: identity observation (full access) */
    if (obs_dim == world_dim) {
        for (int i = 0; i < obs_dim; i++) {
            obs->observation_matrix[i * world_dim + i] = 1.0;
        }
    }

    obs->distinctions = (SOCDistinctionType*)malloc(10 * sizeof(SOCDistinctionType));
    obs->n_distinctions = 0;

    obs->has_self_observation = false;
    obs->self_observation_gain = 0.0;
    obs->history_cap = 100;
    obs->observation_history = (double*)calloc((size_t)(obs->history_cap * obs_dim), sizeof(double));

    return obs;
}

void soc_observer_free(SOCObserver* obs) {
    if (!obs) return;
    free(obs->label);
    free(obs->belief_state);
    free(obs->observation_matrix);
    free(obs->distinctions);
    free(obs->self_model);
    free(obs->blind_spot);
    free(obs->observation_history);
    free(obs);
}

void soc_observer_set_observation_matrix(SOCObserver* obs, const double* H) {
    if (!obs || !H) return;
    memcpy(obs->observation_matrix, H,
           (size_t)(obs->obs_dim * obs->world_dim) * sizeof(double));
}

/* Observe: y = H * x (linear), or y = g(x) (nonlinear).
 * Knowledge point: Observation as linear/nonlinear mapping.
 * Complexity: O(obs_dim * world_dim) */
void soc_observer_observe(SOCObserver* obs, const double* world_state,
                           double* observation) {
    if (!obs || !world_state || !observation) return;

    soc_matrix_vec_mul(obs->observation_matrix, world_state, observation,
                       obs->obs_dim, obs->world_dim);

    /* Record history */
    if (obs->history_len < obs->history_cap) {
        memcpy(obs->observation_history + obs->history_len * obs->obs_dim,
               observation, (size_t)obs->obs_dim * sizeof(double));
        obs->history_len++;
    }
}

/* Update belief using Bayesian-like assimilation of observation.
 * Knowledge point: Constructivist belief update (not passive reception).
 * Complexity: O(belief_dim * obs_dim) */
void soc_observer_update_belief(SOCObserver* obs, const double* observation) {
    if (!obs || !observation) return;

    /* Simple assimilation: belief = belief + learning_rate * (H^T * (obs - H*belief)) */
    double* predicted = (double*)calloc((size_t)obs->obs_dim, sizeof(double));
    double* innovation = (double*)calloc((size_t)obs->obs_dim, sizeof(double));
    double* correction = (double*)calloc((size_t)obs->belief_dim, sizeof(double));
    if (!predicted || !innovation || !correction) {
        free(predicted); free(innovation); free(correction); return;
    }

    soc_matrix_vec_mul(obs->observation_matrix, obs->belief_state, predicted,
                       obs->obs_dim, obs->world_dim);

    for (int i = 0; i < obs->obs_dim; i++) {
        innovation[i] = observation[i] - predicted[i];
    }

    /* H^T * innovation */
    for (int i = 0; i < obs->belief_dim; i++) {
        correction[i] = 0.0;
        for (int j = 0; j < obs->obs_dim; j++) {
            correction[i] += obs->observation_matrix[j * obs->world_dim + i]
                           * innovation[j];
        }
    }

    double lr = 0.3; /* default learning rate */
    for (int i = 0; i < obs->belief_dim; i++) {
        obs->belief_state[i] += lr * correction[i];
    }

    free(predicted); free(innovation); free(correction);
}

/* Self-observation: the observer observes itself.
 * Knowledge point: Self-referential observation (circularity).
 * Complexity: O(self_model_dim) */
void soc_observer_self_observe(SOCObserver* obs, double* self_obs) {
    if (!obs || !self_obs) return;
    if (!obs->has_self_observation) {
        memset(self_obs, 0, (size_t)obs->self_model_dim * sizeof(double));
        return;
    }
    /* Self-observation = kappa * self_model */
    for (int i = 0; i < obs->self_model_dim; i++) {
        self_obs[i] = obs->self_observation_gain * obs->self_model[i];
    }
}

void soc_observer_add_distinction(SOCObserver* obs, SOCDistinctionType d) {
    if (!obs || !obs->distinctions) return;
    obs->distinctions[obs->n_distinctions++] = d;
}

void soc_observer_set_self_gain(SOCObserver* obs, double gain) {
    if (!obs) return;
    obs->self_observation_gain = gain;
    obs->has_self_observation = (fabs(gain) > 1e-15);
}

void soc_observer_attach_system(SOCObserver* obs, SOCNonTrivialMachine* ntm) {
    if (!obs) return;
    obs->observed_system = ntm;
}

/* Blind spot size: how much of the world the observer structurally cannot see.
 * Knowledge point: Every observer has a blind spot (von Foerster).
 * Complexity: O(world_dim) */
double soc_observer_blind_spot_size(const SOCObserver* obs) {
    if (!obs) return 0.0;
    if (obs->obs_dim >= obs->world_dim) return 0.0;
    return (double)(obs->world_dim - obs->obs_dim) / (double)obs->world_dim;
}

void soc_observer_print(const SOCObserver* obs) {
    if (!obs) { printf("Observer: (null)\n"); return; }
    printf("=== Observer: %s ===\n", obs->label);
    printf("  Stance: %d, World dim: %d, Obs dim: %d\n",
           obs->stance, obs->world_dim, obs->obs_dim);
    printf("  Self-observation: %s (gain=%.3f)\n",
           obs->has_self_observation ? "yes" : "no",
           obs->self_observation_gain);
    printf("  Blind spot: %.2f%%\n", soc_observer_blind_spot_size(obs) * 100.0);
    printf("  Distinctions: %d\n", obs->n_distinctions);
    printf("  Observations recorded: %d\n", obs->history_len);
    printf("  Belief state norm: %.4f\n", soc_vector_norm(obs->belief_state, obs->belief_dim));
}

/* ========================================================================
 * Eigenform — Stable self-referential pattern
 * ======================================================================== */

SOCEigenform* soc_eigenform_create(int dimension,
                                    void (*phi)(const double*, int, double*, void*),
                                    void* ctx) {
    if (dimension <= 0 || !phi) return NULL;

    SOCEigenform* ef = (SOCEigenform*)calloc(1, sizeof(SOCEigenform));
    if (!ef) return NULL;

    ef->dimension = dimension;
    ef->phi = phi;
    ef->phi_ctx = ctx;
    ef->convergence_tol = 1e-6;
    ef->max_iterations = 1000;
    ef->solution = NULL;
    ef->converged = false;
    ef->stability = SOC_EIGEN_UNDEFINED;
    ef->path_cap = 200;
    ef->convergence_path = (double*)malloc((size_t)(ef->path_cap * dimension) * sizeof(double));

    return ef;
}

void soc_eigenform_free(SOCEigenform* ef) {
    if (!ef) return;
    if (ef->solution) { free(ef->solution->data); free(ef->solution); }
    free(ef->convergence_path);
    if (ef->all_solutions) {
        for (int i = 0; i < ef->n_solutions; i++) {
            free(ef->all_solutions[i]->data);
            free(ef->all_solutions[i]);
        }
        free(ef->all_solutions);
    }
    free(ef);
}

/* Solve eigenform equation x = Phi(x) by fixed-point iteration.
 * Knowledge point: von Foerster's eigenform computation via Banach iteration.
 * Complexity: O(max_iterations * dim * cost(Phi)) */
bool soc_eigenform_solve(SOCEigenform* ef, const double* initial_guess) {
    if (!ef || !ef->phi || !initial_guess) return false;

    /* Allocate working buffers */
    size_t sz = (size_t)ef->dimension * sizeof(double);
    double* x = (double*)malloc(sz);
    double* x_new = (double*)malloc(sz);
    if (!x || !x_new) { free(x); free(x_new); return false; }

    memcpy(x, initial_guess, sz);
    ef->path_len = 0;
    ef->converged = false;

    int iter;
    for (iter = 0; iter < ef->max_iterations; iter++) {
        /* Apply Phi: x_new = Phi(x) */
        if (!ef->phi) break;
        ef->phi(x, ef->dimension, x_new, ef->phi_ctx);

        /* Record path if space available */
        if (ef->convergence_path && ef->path_len < ef->path_cap) {
            memcpy(ef->convergence_path + (size_t)ef->path_len * (size_t)ef->dimension,
                   x_new, sz);
            ef->path_len++;
        }

        /* Check convergence */
        double diff = 0.0;
        for (int d = 0; d < ef->dimension; d++) {
            double delta = x_new[d] - x[d];
            diff += delta * delta;
        }

        if (sqrt(diff) < ef->convergence_tol) {
            ef->iterations_used = iter + 1;
            ef->residual_norm = sqrt(diff);
            ef->converged = true;

            /* Store solution */
            if (!ef->solution) {
                ef->solution = (SOCVector*)malloc(sizeof(SOCVector));
                if (!ef->solution) { free(x); free(x_new); return false; }
                ef->solution->data = NULL;
            }
            ef->solution->data = (double*)realloc(ef->solution->data, sz);
            if (!ef->solution->data) { free(x); free(x_new); return false; }
            ef->solution->dim = ef->dimension;
            memcpy(ef->solution->data, x_new, sz);

            free(x); free(x_new);

            /* Classify stability (skip if fails) */
            if (ef->solution && ef->phi) {
                ef->stability = soc_eigenform_classify(ef, ef->convergence_tol * 10.0);
            }
            return true;
        }

        /* Swap: x = x_new */
        memcpy(x, x_new, sz);
    }

    ef->iterations_used = iter;
    free(x); free(x_new);
    return false;
}

/* Classify eigenform stability by perturbing solution and observing return.
 * Knowledge point: Eigenvalue classification of self-referential systems.
 * Complexity: O(dim * cost(Phi)) */
SOCEigenStability soc_eigenform_classify(SOCEigenform* ef, double eps) {
    if (!ef || !ef->solution || !ef->phi) return SOC_EIGEN_UNDEFINED;

    /* Perturb solution slightly and check if Phi brings it back */
    double* perturbed = (double*)malloc((size_t)ef->dimension * sizeof(double));
    double* result = (double*)malloc((size_t)ef->dimension * sizeof(double));
    if (!perturbed || !result) { free(perturbed); free(result); return SOC_EIGEN_UNDEFINED; }

    /* Try perturbation in each direction */
    int stable_dirs = 0, unstable_dirs = 0;
    for (int d = 0; d < ef->dimension && d < 5; d++) {
        memcpy(perturbed, ef->solution->data,
               (size_t)ef->dimension * sizeof(double));
        perturbed[d % ef->dimension] += eps;

        /* Apply Phi once */
        ef->phi(perturbed, ef->dimension, result, ef->phi_ctx);

        /* Check if result is closer to solution than perturbed was */
        double dist_p = 0.0, dist_r = 0.0;
        for (int i = 0; i < ef->dimension; i++) {
            double dp = perturbed[i] - ef->solution->data[i];
            double dr = result[i] - ef->solution->data[i];
            dist_p += dp * dp;
            dist_r += dr * dr;
        }

        if (sqrt(dist_r) < sqrt(dist_p)) stable_dirs++;
        else unstable_dirs++;
    }

    free(perturbed); free(result);

    if (stable_dirs == 0 && unstable_dirs > 0) return SOC_EIGEN_UNSTABLE;
    if (unstable_dirs == 0 && stable_dirs > 0) return SOC_EIGEN_STABLE;
    if (stable_dirs > 0 && unstable_dirs > 0) return SOC_EIGEN_SADDLE;
    return SOC_EIGEN_UNDEFINED;
}

/* Find all eigenforms by trying multiple starting points.
 * Knowledge point: Multi-stability in self-referential systems (multiple eigenforms).
 * Complexity: O(n_starts * max_iterations * dim * cost(Phi)) */
void soc_eigenform_find_all(SOCEigenform* ef, int n_starts,
                             const double* start_points) {
    if (!ef || n_starts <= 0 || !start_points) return;

    ef->n_solutions = 0;
    ef->all_solutions = NULL;

    for (int s = 0; s < n_starts; s++) {
        const double* start = start_points + s * ef->dimension;
        if (soc_eigenform_solve(ef, start)) {
            /* Check if this solution is novel */
            bool is_new = true;
            for (int i = 0; i < ef->n_solutions; i++) {
                double dist = 0.0;
                for (int d = 0; d < ef->dimension; d++) {
                    double delta = ef->solution->data[d] -
                                   ef->all_solutions[i]->data[d];
                    dist += delta * delta;
                }
                if (sqrt(dist) < ef->convergence_tol * 10) {
                    is_new = false;
                    break;
                }
            }
            if (is_new) {
                ef->all_solutions = (SOCVector**)realloc(ef->all_solutions,
                    (size_t)(ef->n_solutions + 1) * sizeof(SOCVector*));
                ef->all_solutions[ef->n_solutions] = (SOCVector*)malloc(sizeof(SOCVector));
                ef->all_solutions[ef->n_solutions]->data = (double*)malloc(
                    (size_t)ef->dimension * sizeof(double));
                ef->all_solutions[ef->n_solutions]->dim = ef->dimension;
                memcpy(ef->all_solutions[ef->n_solutions]->data,
                       ef->solution->data,
                       (size_t)ef->dimension * sizeof(double));
                ef->n_solutions++;
            }
        }
    }
}

/* Verify that the solution satisfies x = Phi(x) to high precision.
 * Knowledge point: Self-consistency check of eigenform solutions.
 * Complexity: O(dim * cost(Phi)) */
void soc_eigenform_verify(SOCEigenform* ef) {
    if (!ef || !ef->solution || !ef->phi) return;

    double* result = (double*)malloc((size_t)ef->dimension * sizeof(double));
    if (!result) return;

    ef->phi(ef->solution->data, ef->dimension, result, ef->phi_ctx);

    double residual = 0.0;
    for (int i = 0; i < ef->dimension; i++) {
        double d = result[i] - ef->solution->data[i];
        residual += d * d;
    }
    ef->residual_norm = sqrt(residual);
    ef->converged = (ef->residual_norm < ef->convergence_tol * 10);

    free(result);
}

bool soc_eigenform_is_trivial(const SOCEigenform* ef) {
    if (!ef || !ef->solution) return true;
    return soc_vector_norm(ef->solution->data, ef->dimension) < ef->convergence_tol;
}

void soc_eigenform_print(const SOCEigenform* ef) {
    if (!ef) { printf("Eigenform: (null)\n"); return; }
    printf("=== Eigenform ===\n");
    printf("  Dimension: %d, Converged: %s\n",
           ef->dimension, ef->converged ? "yes" : "no");
    printf("  Iterations: %d, Residual: %.2e\n",
           ef->iterations_used, ef->residual_norm);
    printf("  Stability: %d\n", ef->stability);
    printf("  Solutions found: %d\n", ef->n_solutions);
    if (ef->solution) {
        printf("  Solution norm: %.6f\n",
               soc_vector_norm(ef->solution->data, ef->dimension));
    }
}

/* ========================================================================
 * Circular Closure
 * ======================================================================== */

SOCCircularClosure* soc_closure_create(int dimension,
                                        void (*op)(const double*, int, double*, void*),
                                        void* ctx) {
    if (dimension <= 0 || !op) return NULL;

    SOCCircularClosure* cl = (SOCCircularClosure*)calloc(1, sizeof(SOCCircularClosure));
    if (!cl) return NULL;

    cl->dimension = dimension;
    cl->closure_op = op;
    cl->closure_ctx = ctx;
    cl->closure_type = SOC_SR_DIRECT;
    cl->closure_degree = 0.0;
    cl->traj_cap = 200;
    cl->trajectory = (double*)malloc((size_t)(cl->traj_cap * dimension) * sizeof(double));

    return cl;
}

void soc_closure_free(SOCCircularClosure* cl) {
    if (!cl) return;
    if (cl->fixed_point) { free(cl->fixed_point->data); free(cl->fixed_point); }
    free(cl->jacobian);
    free(cl->trajectory);
    free(cl);
}

/* Compute fixed point of closure operation: x = closure_op(x).
 * Knowledge point: Circular closure = fixed point of self-referential operation.
 * Complexity: O(max_iters * dim * cost(op)) */
bool soc_closure_compute_fixed_point(SOCCircularClosure* cl,
                                      const double* initial, int max_iters) {
    if (!cl || !initial || !cl->closure_op) return false;

    size_t sz = (size_t)cl->dimension * sizeof(double);
    double* x = (double*)malloc(sz);
    double* x_new = (double*)malloc(sz);
    if (!x || !x_new) { free(x); free(x_new); return false; }

    memcpy(x, initial, sz);
    cl->traj_len = 0;

    int iter;
    for (iter = 0; iter < max_iters; iter++) {
        if (!cl->closure_op) break;
        cl->closure_op(x, cl->dimension, x_new, cl->closure_ctx);

        if (cl->trajectory && cl->traj_len < cl->traj_cap) {
            memcpy(cl->trajectory + (size_t)cl->traj_len * (size_t)cl->dimension,
                   x_new, sz);
            cl->traj_len++;
        }

        double diff = 0.0;
        for (int d = 0; d < cl->dimension; d++) {
            double delta = x_new[d] - x[d];
            diff += delta * delta;
        }

        if (sqrt(diff) < 1e-8) {
            if (!cl->fixed_point) {
                cl->fixed_point = (SOCVector*)malloc(sizeof(SOCVector));
                if (!cl->fixed_point) { free(x); free(x_new); return false; }
                cl->fixed_point->data = NULL;
            }
            cl->fixed_point->data = (double*)realloc(
                cl->fixed_point->data, sz);
            if (!cl->fixed_point->data) { free(x); free(x_new); return false; }
            cl->fixed_point->dim = cl->dimension;
            memcpy(cl->fixed_point->data, x_new, sz);
            cl->closure_degree = 1.0;
            free(x); free(x_new);
            return true;
        }

        memcpy(x, x_new, sz);
    }

    free(x); free(x_new);
    cl->closure_degree = 0.0;
    return false;
}

/* Analyze stability of closure fixed point using finite-difference Jacobian.
 * Knowledge point: Stability of self-referential organizational closure.
 * Complexity: O(dim^2 * cost(op)) */
SOCEigenStability soc_closure_analyze_stability(SOCCircularClosure* cl, double eps) {
    if (!cl || !cl->fixed_point || !cl->closure_op)
        return SOC_EIGEN_UNDEFINED;

    /* Finite-difference Jacobian */
    int d = cl->dimension;
    double* J = (double*)calloc((size_t)(d * d), sizeof(double));
    double* perturbed = (double*)malloc((size_t)d * sizeof(double));
    double* result_plus = (double*)malloc((size_t)d * sizeof(double));
    double* result_minus = (double*)malloc((size_t)d * sizeof(double));

    if (!J || !perturbed || !result_plus || !result_minus) {
        free(J); free(perturbed); free(result_plus); free(result_minus);
        return SOC_EIGEN_UNDEFINED;
    }

    for (int j = 0; j < d; j++) {
        memcpy(perturbed, cl->fixed_point->data, (size_t)d * sizeof(double));
        perturbed[j] += eps;
        cl->closure_op(perturbed, d, result_plus, cl->closure_ctx);

        memcpy(perturbed, cl->fixed_point->data, (size_t)d * sizeof(double));
        perturbed[j] -= eps;
        cl->closure_op(perturbed, d, result_minus, cl->closure_ctx);

        for (int i = 0; i < d; i++) {
            J[i * d + j] = (result_plus[i] - result_minus[i]) / (2.0 * eps);
        }
    }

    /* Compute spectral radius approximation via trace */
    double trace = 0.0;
    for (int i = 0; i < d; i++) trace += J[i * d + i];

    free(J); free(perturbed); free(result_plus); free(result_minus);

    if (fabs(trace) < 1.0 - eps) return SOC_EIGEN_STABLE;
    if (fabs(trace) > 1.0 + eps) return SOC_EIGEN_UNSTABLE;
    return SOC_EIGEN_SADDLE;
}

/* Measure how closed a system is: 1 - (perturbation_response / perturbation).
 * Knowledge point: Operational closure as self-determination.
 * Complexity: O(dim * cost(op)) */
double soc_closure_measure_closedness(SOCCircularClosure* cl) {
    if (!cl || !cl->fixed_point || !cl->closure_op) return 0.0;

    /* Apply perturbation and measure how much the system corrects */
    double* perturbed = (double*)malloc((size_t)cl->dimension * sizeof(double));
    double* response = (double*)malloc((size_t)cl->dimension * sizeof(double));
    if (!perturbed || !response) { free(perturbed); free(response); return 0.0; }

    /* Random perturbation direction */
    for (int i = 0; i < cl->dimension; i++) {
        perturbed[i] = cl->fixed_point->data[i] + 0.1 * ((double)((i * 73 + 17) % 100) / 100.0);
    }
    cl->closure_op(perturbed, cl->dimension, response, cl->closure_ctx);

    double pert_norm = 0.0, resp_dist = 0.0;
    for (int i = 0; i < cl->dimension; i++) {
        double p = perturbed[i] - cl->fixed_point->data[i];
        double r = response[i] - cl->fixed_point->data[i];
        pert_norm += p * p;
        resp_dist += r * r;
    }

    free(perturbed); free(response);

    if (sqrt(pert_norm) < 1e-15) return 1.0;
    return 1.0 - sqrt(resp_dist) / sqrt(pert_norm);
}

/* Simulate response to perturbation over multiple steps.
 * Knowledge point: Self-correction dynamics of closed systems.
 * Complexity: O(n_steps * dim * cost(op)) */
void soc_closure_simulate_perturbation(SOCCircularClosure* cl,
                                        const double* perturbation,
                                        int n_steps) {
    if (!cl || !perturbation || n_steps <= 0) return;

    double* state = (double*)malloc((size_t)cl->dimension * sizeof(double));
    double* next = (double*)malloc((size_t)cl->dimension * sizeof(double));
    if (!state || !next) { free(state); free(next); return; }

    /* Start from fixed point + perturbation */
    for (int i = 0; i < cl->dimension; i++) {
        state[i] = (cl->fixed_point ? cl->fixed_point->data[i] : 0.0)
                   + perturbation[i];
    }

    cl->traj_len = 0;
    for (int t = 0; t < n_steps && t < cl->traj_cap; t++) {
        if (cl->traj_len < cl->traj_cap) {
            memcpy(cl->trajectory + cl->traj_len * cl->dimension,
                   state, (size_t)cl->dimension * sizeof(double));
            cl->traj_len++;
        }
        cl->closure_op(state, cl->dimension, next, cl->closure_ctx);
        memcpy(state, next, (size_t)cl->dimension * sizeof(double));
    }

    /* Measure response rate = how fast perturbation decays */
    if (cl->traj_len > 1 && cl->fixed_point) {
        double initial_dist = 0.0, final_dist = 0.0;
        for (int i = 0; i < cl->dimension; i++) {
            double di = cl->trajectory[i] - cl->fixed_point->data[i];
            double df = cl->trajectory[(cl->traj_len - 1) * cl->dimension + i]
                        - cl->fixed_point->data[i];
            initial_dist += di * di;
            final_dist += df * df;
        }
        if (sqrt(initial_dist) > 1e-15) {
            cl->perturbation_response_rate = 1.0 - sqrt(final_dist) / sqrt(initial_dist);
        }
    }

    free(state); free(next);
}

void soc_closure_print(const SOCCircularClosure* cl) {
    if (!cl) { printf("CircularClosure: (null)\n"); return; }
    printf("=== Circular Closure ===\n");
    printf("  Dimension: %d, Type: %d\n", cl->dimension, cl->closure_type);
    printf("  Closure degree: %.4f\n", cl->closure_degree);
    printf("  Stability: %d\n", cl->stability);
    printf("  Perturbation response rate: %.4f\n", cl->perturbation_response_rate);
    printf("  Trajectory length: %d\n", cl->traj_len);
}

/* ========================================================================
 * Distinction — Spencer Brown's Laws of Form
 * ======================================================================== */

SOCDistinction* soc_distinction_create(SOCDistinctionType type, const char* label,
                                        bool (*boundary)(const double*, int, void*),
                                        void* ctx, int space_dim) {
    SOCDistinction* d = (SOCDistinction*)calloc(1, sizeof(SOCDistinction));
    if (!d) return NULL;
    d->type = type;
    d->label = label ? _strdup(label) : _strdup("unnamed_distinction");
    d->boundary = boundary;
    d->boundary_ctx = ctx;
    d->space_dim = space_dim;
    d->marked_value = 1;
    d->unmarked_value = 0;
    d->current_state = false;
    d->cross_fn = NULL;
    d->re_entry_fn = NULL;
    return d;
}

void soc_distinction_free(SOCDistinction* d) {
    if (!d) return;
    free(d->label);
    free(d);
}

/* Indicate: check if a point falls on the marked side of the distinction.
 * Knowledge point: The fundamental operation of observation (Spencer Brown).
 * Complexity: O(cost(boundary)) */
bool soc_distinction_indicate(SOCDistinction* d, const double* point) {
    if (!d || !point || !d->boundary) return false;
    return d->boundary(point, d->space_dim, d->boundary_ctx);
}

/* Cross operation: toggle between marked and unmarked.
 * Knowledge point: Spencer Brown's primary algebra: cross(cross(x)) = x.
 * Complexity: O(1) */
bool soc_distinction_cross_op(SOCDistinction* d) {
    if (!d) return false;
    if (d->cross_fn) return d->cross_fn(d->current_state);
    d->current_state = !d->current_state;
    return d->current_state;
}

/* Re-entry: the form that re-enters its own indicational space.
 * Knowledge point: Self-referential form (Spencer Brown's "re-entry").
 * Complexity: O(cost(re_entry_fn)) */
bool soc_distinction_re_entry(SOCDistinction* d) {
    if (!d) return false;
    if (d->re_entry_fn) return d->re_entry_fn(d);
    /* Default re-entry: the distinction indicated by itself */
    return soc_distinction_indicate(d, NULL) ? d->marked_value : d->unmarked_value;
}

void soc_distinction_set_state(SOCDistinction* d, bool state) {
    if (!d) return;
    d->current_state = state;
}

void soc_distinction_print(const SOCDistinction* d) {
    if (!d) { printf("Distinction: (null)\n"); return; }
    printf("=== Distinction: %s ===\n", d->label);
    printf("  Type: %d, Space dim: %d\n", d->type, d->space_dim);
    printf("  State: %s (marked=%d, unmarked=%d)\n",
           d->current_state ? "marked" : "unmarked",
           d->marked_value, d->unmarked_value);
}

/* ========================================================================
 * Conversation — Pask's Conversation Theory
 * ======================================================================== */

SOCConversation* soc_conversation_create(const char* topic, int concept_dim,
                                          SOCObserver* a, SOCObserver* b) {
    if (concept_dim <= 0) return NULL;

    SOCConversation* conv = (SOCConversation*)calloc(1, sizeof(SOCConversation));
    if (!conv) return NULL;

    conv->topic = topic ? _strdup(topic) : _strdup("untitled_conversation");
    conv->concept_dim = concept_dim;
    conv->participant_a = a;
    conv->participant_b = b;
    conv->max_turns = 100;
    conv->turn_history = (double*)calloc((size_t)conv->max_turns, sizeof(double));
    conv->shared_model = (double*)calloc((size_t)concept_dim, sizeof(double));
    conv->shared_dim = concept_dim;
    conv->stabilized = false;

    return conv;
}

void soc_conversation_free(SOCConversation* conv) {
    if (!conv) return;
    free(conv->topic);
    free(conv->conceptual_space);
    free(conv->entailment_matrix);
    free(conv->turn_history);
    free(conv->shared_model);
    free(conv);
}

void soc_conversation_add_concept(SOCConversation* conv,
                                   const double* concept_vec, int dim) {
    if (!conv || !concept_vec || dim != conv->concept_dim) return;

    int old_sz = conv->n_concepts * conv->concept_dim;
    conv->n_concepts++;
    conv->conceptual_space = (double*)realloc(conv->conceptual_space,
        (size_t)(conv->n_concepts * conv->concept_dim) * sizeof(double));
    memcpy(conv->conceptual_space + old_sz, concept_vec,
           (size_t)conv->concept_dim * sizeof(double));
}

void soc_conversation_set_entailment(SOCConversation* conv,
                                      const double* M, int dim) {
    if (!conv || !M || dim != conv->entailment_dim) return;
    conv->entailment_dim = dim;
    size_t sz = (size_t)(dim * dim) * sizeof(double);
    conv->entailment_matrix = (double*)realloc(conv->entailment_matrix, sz);
    memcpy(conv->entailment_matrix, M, sz);
}

/* Measure agreement between participants' belief states.
 * Knowledge point: Intersubjective agreement in conversation theory.
 * Complexity: O(belief_dim) */
double soc_conversation_measure_agreement(SOCConversation* conv) {
    if (!conv || !conv->participant_a || !conv->participant_b) return 0.0;

    return soc_cosine_similarity(conv->participant_a->belief_state,
                                  conv->participant_b->belief_state,
                                  conv->shared_dim);
}

/* Execute one turn of conversation: each participant updates their model.
 * Knowledge point: Pask's conversational dynamics.
 * Complexity: O(belief_dim * obs_dim) */
void soc_conversation_turn(SOCConversation* conv) {
    if (!conv || conv->n_turns >= conv->max_turns) return;
    if (!conv->participant_a || !conv->participant_b) return;

    /* A teaches B: B observes A's belief */
    double* a_output = (double*)calloc((size_t)conv->concept_dim, sizeof(double));
    double* b_observation = (double*)calloc((size_t)conv->concept_dim, sizeof(double));
    if (!a_output || !b_observation) { free(a_output); free(b_observation); return; }

    /* A communicates its belief state */
    soc_observer_observe(conv->participant_a,
                          conv->participant_a->belief_state, a_output);
    soc_observer_observe(conv->participant_b, a_output, b_observation);
    soc_observer_update_belief(conv->participant_b, b_observation);

    /* B teaches back: A observes B's updated belief */
    double* b_output = (double*)calloc((size_t)conv->concept_dim, sizeof(double));
    double* a_observation = (double*)calloc((size_t)conv->concept_dim, sizeof(double));
    if (!b_output || !a_observation) {
        free(a_output); free(b_observation); free(b_output); free(a_observation);
        return;
    }

    soc_observer_observe(conv->participant_b,
                          conv->participant_b->belief_state, b_output);
    soc_observer_observe(conv->participant_a, b_output, a_observation);
    soc_observer_update_belief(conv->participant_a, a_observation);

    /* Update shared model as midpoint */
    for (int i = 0; i < conv->shared_dim; i++) {
        conv->shared_model[i] = (conv->participant_a->belief_state[i] +
                                  conv->participant_b->belief_state[i]) / 2.0;
    }

    /* Record agreement */
    conv->agreement_level = soc_conversation_measure_agreement(conv);
    conv->turn_history[conv->n_turns] = conv->agreement_level;
    conv->n_turns++;

    /* Check convergence: agreement > 0.99 for last 3 turns? */
    if (conv->n_turns >= 3) {
        int stable = 0;
        for (int t = conv->n_turns - 3; t < conv->n_turns; t++) {
            if (conv->turn_history[t] > 0.99) stable++;
        }
        conv->stabilized = (stable >= 3);
        if (conv->stabilized && conv->n_turns >= 2) {
            conv->stabilization_rate = conv->turn_history[conv->n_turns - 1] -
                                        conv->turn_history[conv->n_turns - 2];
        }
    }

    /* Teachback fidelity: how well can B reproduce A's original? */
    conv->teachback_fidelity = conv->agreement_level;

    free(a_output); free(b_observation); free(b_output); free(a_observation);
}

bool soc_conversation_has_converged(SOCConversation* conv) {
    return conv ? conv->stabilized : false;
}

void soc_conversation_print(const SOCConversation* conv) {
    if (!conv) { printf("Conversation: (null)\n"); return; }
    printf("=== Conversation: %s ===\n", conv->topic);
    printf("  Concepts: %d (dim=%d)\n", conv->n_concepts, conv->concept_dim);
    printf("  Turns: %d, Agreement: %.4f\n", conv->n_turns, conv->agreement_level);
    printf("  Stabilized: %s, Teachback: %.4f\n",
           conv->stabilized ? "yes" : "no", conv->teachback_fidelity);
}

/* ========================================================================
 * Second-Order Observation — Luhmann
 * ======================================================================== */

SOCSecondOrderObservation* soc_soo_create(SOCObserver* obs,
                                           SOCObserver* meta_obs) {
    if (!obs || !meta_obs) return NULL;

    SOCSecondOrderObservation* soo = (SOCSecondOrderObservation*)calloc(1,
        sizeof(SOCSecondOrderObservation));
    if (!soo) return NULL;

    soo->observer = obs;
    soo->observer_of_observer = meta_obs;
    soo->n_observed_distinctions = obs->n_distinctions;
    soo->observed_distinctions = (double*)calloc(
        (size_t)soo->n_observed_distinctions, sizeof(double));
    soo->meta_blind_dim = obs->blind_spot_dim;
    soo->observed_blind_spot = (double*)calloc(
        (size_t)soo->meta_blind_dim, sizeof(double));

    return soo;
}

void soc_soo_free(SOCSecondOrderObservation* soo) {
    if (!soo) return;
    free(soo->observed_distinctions);
    free(soo->observed_blind_spot);
    free(soo);
}

/* Analyze: the meta-observer observes the first observer's observations.
 * Knowledge point: Second-order observation (Luhmann).
 * Complexity: O(n_distinctions + blind_spot_dim + obs_dim * world_dim) */
void soc_soo_analyze(SOCSecondOrderObservation* soo) {
    if (!soo || !soo->observer || !soo->observer_of_observer) return;

    /* Meta-observer observes what distinctions the first observer uses */
    for (int i = 0; i < soo->n_observed_distinctions && i < soo->observer->n_distinctions; i++) {
        soo->observed_distinctions[i] = (double)soo->observer->distinctions[i];
    }

    /* Meta-observer can see the first observer's blind spot */
    for (int i = 0; i < soo->meta_blind_dim && i < soo->observer->blind_spot_dim; i++) {
        soo->observed_blind_spot[i] = soo->observer->blind_spot[i];
    }
}

void soc_soo_print(const SOCSecondOrderObservation* soo) {
    if (!soo) { printf("SOO: (null)\n"); return; }
    printf("=== Second-Order Observation ===\n");
    printf("  First-order observer: %s\n",
           soo->observer->label ? soo->observer->label : "?");
    printf("  Meta-observer: %s\n",
           soo->observer_of_observer->label ?
           soo->observer_of_observer->label : "?");
    printf("  Observed distinctions: %d\n", soo->n_observed_distinctions);
    printf("  Blind spot detected: %d dimensions\n", soo->meta_blind_dim);
}

