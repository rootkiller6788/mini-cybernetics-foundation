#include "cp_core.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define CP_EPS 1e-12

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

static double cp_max(double a, double b) { return (a > b) ? a : b; }
static double cp_abs(double x) { return (x >= 0) ? x : -x; }

/* ============================================================================
 * ControlSystem ? Lifecycle
 * ============================================================================ */

ControlSystem* cp_system_create(const char* name, int nx, int nu, int ny) {
    ControlSystem* sys = (ControlSystem*)calloc(1, sizeof(ControlSystem));
    if (!sys) return NULL;
    sys->name = (name && name[0]) ? strdup(name) : strdup("unnamed");
    sys->paradigm = CP_CLOSED_LOOP;
    sys->teleology = CP_ACTIVE_PURPOSE;
    sys->nx = nx;
    sys->nu = nu;
    sys->ny = ny;
    if (nx > 0) {
        sys->x     = (double*)calloc((size_t)nx, sizeof(double));
        sys->x_dot = (double*)calloc((size_t)nx, sizeof(double));
    }
    if (nu > 0) {
        sys->u     = (double*)calloc((size_t)nu, sizeof(double));
        if (nx > 0)
            sys->gain_matrix = (double*)calloc((size_t)nu * (size_t)nx, sizeof(double));
    }
    if (ny > 0) {
        sys->y     = (double*)calloc((size_t)ny, sizeof(double));
        sys->r     = (double*)calloc((size_t)ny, sizeof(double));
        sys->goal_tolerance = (double*)calloc((size_t)ny, sizeof(double));
        for (int i = 0; i < ny; i++) sys->goal_tolerance[i] = 0.01;
    }
    sys->x_hat = (nx > 0) ? (double*)calloc((size_t)nx, sizeof(double)) : NULL;
    sys->estimation_error = (ny > 0) ? (double*)calloc((size_t)ny, sizeof(double)) : NULL;
    sys->observer_type = CP_NO_OBSERVER;
    sys->feedback_mode = CP_NEGATIVE_FEEDBACK;
    sys->feedback_gain = 1.0;
    sys->cost_to_go = 0.0;
    sys->squared_error_sum = 0.0;
    sys->step_count = 0;
    sys->stability = CP_NEUTRAL;
    sys->lyapunov_value = 0.0;
    sys->lyapunov_derivative = 0.0;
    return sys;
}

void cp_system_free(ControlSystem* sys) {
    if (!sys) return;
    free(sys->name);
    free(sys->x);
    free(sys->x_dot);
    free(sys->u);
    free(sys->y);
    free(sys->r);
    free(sys->goal_tolerance);
    free(sys->model_params);
    free(sys->x_hat);
    free(sys->estimation_error);
    free(sys->gain_matrix);
    free(sys);
}

void cp_system_set_reference(ControlSystem* sys, const double* r, const double* tolerance) {
    if (!sys || !r) return;
    for (int i = 0; i < sys->ny; i++) {
        sys->r[i] = r[i];
        if (tolerance) sys->goal_tolerance[i] = tolerance[i];
    }
}

void cp_system_set_paradigm(ControlSystem* sys, ControlParadigm paradigm) {
    if (!sys) return;
    sys->paradigm = paradigm;
    if (paradigm >= CP_ADAPTIVE && sys->n_model_params == 0) {
        sys->n_model_params = sys->nx * sys->nx + sys->nx * sys->nu + sys->ny * sys->nx;
        sys->model_params = (double*)calloc((size_t)sys->n_model_params, sizeof(double));
    }
}

void cp_system_set_feedback(ControlSystem* sys, FeedbackType fb) {
    if (!sys) return;
    sys->feedback_mode = fb;
}

/* ============================================================================
 * ControlSystem ? Dynamics (the core control loop)
 * ============================================================================ */

void cp_system_step(ControlSystem* sys, double dt) {
    if (!sys || sys->nx == 0) return;

    /* 1. Observe: output equation y = C x (identity observation for simplicity) */
    for (int i = 0; i < sys->ny && i < sys->nx; i++) {
        sys->y[i] = sys->x[i];
    }

    /* 2. Compare: error signal e = r - y */
    for (int i = 0; i < sys->ny; i++) {
        double e_i = sys->r[i] - sys->y[i];
        sys->estimation_error[i] = e_i;
        sys->squared_error_sum += e_i * e_i;
    }

    /* 3. Decide: control law u = c(e) */
    for (int i = 0; i < sys->nu; i++) {
        double u_i = 0.0;
        switch (sys->feedback_mode) {
            case CP_NEGATIVE_FEEDBACK:
            case CP_BALANCING_FEEDBACK:
                /* u = K * e, proportional control */
                for (int j = 0; j < sys->nx && j < sys->ny; j++) {
                    int idx = i * sys->nx + j;
                    double k = (sys->gain_matrix && sys->gain_matrix[idx] != 0.0)
                                ? sys->gain_matrix[idx] : sys->feedback_gain;
                    u_i += k * sys->estimation_error[j] / cp_max((double)sys->ny, 1.0);
                }
                break;
            case CP_POSITIVE_FEEDBACK:
            case CP_REINFORCING_FEEDBACK:
                /* amplifies deviation */
                for (int j = 0; j < sys->nx && j < sys->ny; j++) {
                    u_i += sys->feedback_gain * sys->y[j];
                }
                break;
            default:
                break;
        }
        sys->u[i] = cp_clamp(u_i, -100.0, 100.0);
    }

    /* 4. Actuate: state dynamics x_dot = f(x, u) = A x + B u */
    if (sys->model_params && sys->n_model_params > 0) {
        /* A matrix is first nx*nx entries of model_params */
        int a_size = sys->nx * sys->nx;
        for (int i = 0; i < sys->nx; i++) {
            sys->x_dot[i] = 0.0;
            /* A * x */
            for (int j = 0; j < sys->nx; j++) {
                sys->x_dot[i] += sys->model_params[i * sys->nx + j] * sys->x[j];
            }
            /* B * u */
            int b_offset = a_size;
            for (int j = 0; j < sys->nu; j++) {
                int b_idx = b_offset + i * sys->nu + j;
                if (b_idx < sys->n_model_params) {
                    sys->x_dot[i] += sys->model_params[b_idx] * sys->u[j];
                }
            }
        }
    } else {
        /* Simple first-order dynamics with damping */
        for (int i = 0; i < sys->nx; i++) {
            double input_effect = 0.0;
            for (int j = 0; j < sys->nu && j < sys->nx; j++) {
                input_effect += (i == j ? 1.0 : 0.3) * sys->u[j];
            }
            sys->x_dot[i] = -0.5 * sys->x[i] + input_effect;
        }
    }

    /* 5. Integrate: x = x + x_dot * dt (Euler) */
    for (int i = 0; i < sys->nx; i++) {
        sys->x[i] += sys->x_dot[i] * dt;
    }

    /* Update Lyapunov estimate */
    sys->lyapunov_value = cp_system_lyapunov_value(sys);
    sys->lyapunov_derivative = cp_system_lyapunov_derivative(sys, dt);

    /* Update cost-to-go */
    for (int i = 0; i < sys->ny; i++) {
        sys->cost_to_go += sys->estimation_error[i] * sys->estimation_error[i] * dt;
    }

    sys->step_count++;
}

void cp_system_simulate(ControlSystem* sys, double duration, double dt) {
    if (!sys) return;
    int steps = (int)(duration / dt);
    if (steps < 1) steps = 1;
    for (int i = 0; i < steps; i++) {
        cp_system_step(sys, dt);
    }
}

void cp_system_compute_error(ControlSystem* sys, double* error_out) {
    if (!sys || !error_out) return;
    for (int i = 0; i < sys->ny; i++) {
        error_out[i] = sys->r[i] - sys->y[i];
    }
}

void cp_system_disturb(ControlSystem* sys, const double* disturbance) {
    if (!sys || !disturbance) return;
    for (int i = 0; i < sys->nx; i++) {
        sys->x[i] += disturbance[i];
    }
}

/* ============================================================================
 * ControlSystem ? Analysis
 * ============================================================================ */

double cp_system_lyapunov_value(const ControlSystem* sys) {
    if (!sys || sys->nx == 0) return 0.0;
    /* V(x) = 0.5 * x^T * x  (simplest quadratic Lyapunov function) */
    double v = 0.0;
    for (int i = 0; i < sys->nx; i++) {
        v += 0.5 * sys->x[i] * sys->x[i];
    }
    return v;
}

double cp_system_lyapunov_derivative(ControlSystem* sys, double dt) {
    if (!sys || sys->nx == 0 || dt < CP_EPS) return 0.0;
    /* V_dot ? (V(t+dt) - V(t)) / dt */
    double v_before = cp_system_lyapunov_value(sys);
    /* approximate V_after using current x_dot */
    double v_after = 0.0;
    for (int i = 0; i < sys->nx; i++) {
        double x_next = sys->x[i] + sys->x_dot[i] * dt;
        v_after += 0.5 * x_next * x_next;
    }
    return (v_after - v_before) / dt;
}

StabilityClass cp_system_classify_stability(const ControlSystem* sys, int window) {
    if (!sys) return CP_NEUTRAL;
    (void)window;
    double v_dot = sys->lyapunov_derivative;
    double v     = sys->lyapunov_value;

    if (v < CP_EPS) return CP_ASYMPTOTICALLY_STABLE;

    if (v_dot < -CP_EPS) {
        /* V_dot strictly negative ? asymptotic stability */
        double rate = -v_dot / v;
        if (rate > 1.0) return CP_ASYMPTOTICALLY_STABLE;
        else return CP_MARGINALLY_STABLE;
    } else if (v_dot > CP_EPS) {
        return CP_UNSTABLE;
    } else {
        return CP_NEUTRAL;
    }
}

void cp_system_observability_gramian(const ControlSystem* sys, double* Wo_out) {
    if (!sys || !Wo_out || sys->nx == 0) return;
    int n = sys->nx;
    /* Wo = C^T * C  (simplified: identity C for nx <= ny) */
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            Wo_out[i * n + j] = (i == j && i < sys->ny) ? 1.0 : 0.0;
        }
    }
    /* If model_params has A, compute full Lyapunov: A^T W + W A = -C^T C */
    if (sys->model_params && sys->n_model_params >= n * n) {
        for (int iter = 0; iter < 100; iter++) {
            double residual = 0.0;
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    double lhs = 0.0;
                    /* A^T W */
                    for (int k = 0; k < n; k++) {
                        lhs += sys->model_params[k * n + i] * Wo_out[k * n + j];
                    }
                    /* + W A */
                    for (int k = 0; k < n; k++) {
                        lhs += Wo_out[i * n + k] * sys->model_params[k * n + j];
                    }
                    /* + C^T C */
                    double ctc = 0.0;
                    for (int k = 0; k < sys->ny; k++) {
                        ctc += ((i == k) ? 1.0 : 0.0) * ((j == k) ? 1.0 : 0.0);
                    }
                    double delta = -(lhs + ctc);
                    Wo_out[i * n + j] += 0.1 * delta;
                    residual += delta * delta;
                }
            }
            if (residual < CP_EPS) break;
        }
    }
}

void cp_system_controllability_gramian(const ControlSystem* sys, double* Wc_out) {
    if (!sys || !Wc_out || sys->nx == 0) return;
    int n = sys->nx;
    /* Wc = B * B^T (simplified) */
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            Wc_out[i * n + j] = (i == j && i < sys->nu) ? 1.0 : 0.0;
        }
    }
    /* Full Lyapunov if A and B are known */
    if (sys->model_params && sys->n_model_params >= n * n + n * sys->nu) {
        for (int iter = 0; iter < 100; iter++) {
            double residual = 0.0;
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    double lhs = 0.0;
                    /* A W */
                    for (int k = 0; k < n; k++) {
                        lhs += sys->model_params[i * n + k] * Wc_out[k * n + j];
                    }
                    /* + W A^T */
                    for (int k = 0; k < n; k++) {
                        lhs += Wc_out[i * n + k] * sys->model_params[j * n + k];
                    }
                    /* Negative sum needs B B^T */
                    double bbt = 0.0;
                    int b_offset = n * n;
                    for (int k = 0; k < sys->nu; k++) {
                        int bi = b_offset + i * sys->nu + k;
                        int bj = b_offset + j * sys->nu + k;
                        if (bi < sys->n_model_params && bj < sys->n_model_params) {
                            bbt += sys->model_params[bi] * sys->model_params[bj];
                        }
                    }
                    double delta = lhs - bbt;
                    Wc_out[i * n + j] -= 0.1 * delta;
                    residual += delta * delta;
                }
            }
            if (residual < CP_EPS) break;
        }
    }
}

bool cp_system_check_internal_model(const ControlSystem* sys,
                                     const double* disturbance_model) {
    if (!sys || !disturbance_model || sys->n_model_params == 0) return false;
    /* Check: does model_params contain the disturbance dynamics?
     * Simplification: compare Frobenius norm of model_A vs disturbance_model */
    int n = sys->nx;
    int n2 = n * n;
    double diff = 0.0, norm_a = 0.0;
    for (int i = 0; i < n2 && i < sys->n_model_params; i++) {
        double d = sys->model_params[i] - disturbance_model[i];
        diff += d * d;
        norm_a += sys->model_params[i] * sys->model_params[i];
    }
    if (norm_a < CP_EPS) return false;
    return (diff / norm_a) < 0.1;  /* within 10% accuracy */
}

/* ============================================================================
 * ControlSystem ? Information Metrics
 * ============================================================================ */

void cp_system_compute_info_metrics(ControlSystem* sys, int history_length, int bins) {
    if (!sys || history_length < 2) return;
    /* Simplified: compute marginal entropies from state/error statistics */
    double ss_sum = 0.0;
    for (int i = 0; i < sys->ny; i++) {
        ss_sum += cp_abs(sys->estimation_error[i]);
    }
    if (ss_sum < CP_EPS) ss_sum = 1.0;

    double* probs = (double*)malloc((size_t)bins * sizeof(double));
    if (!probs) return;
    for (int b = 0; b < bins; b++) probs[b] = 0.0;

    for (int i = 0; i < sys->ny; i++) {
        double p = cp_abs(sys->estimation_error[i]) / ss_sum;
        int bin = (int)(p * (bins - 1));
        if (bin < 0) bin = 0;
        if (bin >= bins) bin = bins - 1;
        probs[bin] += 1.0 / sys->ny;
    }

    double h = 0.0;
    for (int b = 0; b < bins; b++) {
        if (probs[b] > CP_EPS) h -= probs[b] * log2(probs[b]);
    }
    sys->info.entropy = h;
    sys->info.sample_size = history_length;
    sys->info.alphabet_size = bins;
    free(probs);
    (void)history_length;
}

double cp_system_channel_capacity(const ControlSystem* sys) {
    if (!sys) return 0.0;
    /* Estimated by the maximum mutual information between input and output
     * Based on the signal-to-noise ratio of the control channel */
    double signal_power = 0.0, noise_power = 0.0;
    for (int i = 0; i < sys->ny; i++) {
        signal_power += sys->r[i] * sys->r[i];
        noise_power  += sys->estimation_error[i] * sys->estimation_error[i];
    }
    if (noise_power < CP_EPS) return 10.0;  /* effectively infinite capacity */
    double snr = signal_power / noise_power;
    return 0.5 * log2(1.0 + snr);
}

void cp_system_print(const ControlSystem* sys) {
    if (!sys) { printf("ControlSystem: NULL\n"); return; }
    printf("=== Control System: %s ===\n", sys->name);
    printf("Paradigm: %d, Teleology: %d, Feedback: %d, Observer: %d\n",
           sys->paradigm, sys->teleology, sys->feedback_mode, sys->observer_type);
    printf("Dimensions: nx=%d, nu=%d, ny=%d\n", sys->nx, sys->nu, sys->ny);
    printf("State x: ");
    for (int i = 0; i < sys->nx; i++) printf("%.4f ", sys->x[i]);
    printf("\nInput u:  ");
    for (int i = 0; i < sys->nu; i++) printf("%.4f ", sys->u[i]);
    printf("\nOutput y: ");
    for (int i = 0; i < sys->ny; i++) printf("%.4f ", sys->y[i]);
    printf("\nRef r:    ");
    for (int i = 0; i < sys->ny; i++) printf("%.4f ", sys->r[i]);
    printf("\nError e:  ");
    for (int i = 0; i < sys->ny; i++) printf("%.4f ", sys->estimation_error[i]);
    printf("\nLyapunov: V=%.6f, V_dot=%.6f\n", sys->lyapunov_value, sys->lyapunov_derivative);
    printf("Stability: %d, Cost-to-go: %.6f, Steps: %d\n",
           sys->stability, sys->cost_to_go, sys->step_count);
    printf("Info: H=%.4f bits, Capacity=%.4f\n",
           sys->info.entropy, cp_system_channel_capacity(sys));
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

double cp_matrix_determinant_2x2(double a, double b, double c, double d) {
    return a * d - b * c;
}

double cp_matrix_trace_2x2(double a, double b, double c, double d) {
    (void)b; (void)c;
    return a + d;
}

void cp_matrix_mul(double* C, const double* A, const double* B, int m, int n, int p) {
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < p; j++) {
            C[i * p + j] = 0.0;
            for (int k = 0; k < n; k++) {
                C[i * p + j] += A[i * n + k] * B[k * p + j];
            }
        }
    }
}

double cp_vector_norm(const double* v, int n) {
    double s = 0.0;
    for (int i = 0; i < n; i++) s += v[i] * v[i];
    return sqrt(s);
}

double cp_vector_dot(const double* a, const double* b, int n) {
    double s = 0.0;
    for (int i = 0; i < n; i++) s += a[i] * b[i];
    return s;
}

void cp_vector_scale(double* out, const double* v, double scalar, int n) {
    for (int i = 0; i < n; i++) out[i] = v[i] * scalar;
}

double cp_sigmoid(double x, double k, double x0) {
    return 1.0 / (1.0 + exp(-k * (x - x0)));
}

double cp_clamp(double x, double lo, double hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}
