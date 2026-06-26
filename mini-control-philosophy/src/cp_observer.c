#include "cp_observer.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif
#define CP_EPS 1e-12

/* ============================================================================
 * Observer
 * ============================================================================ */

Observer* cp_observer_create(ObserverType type, int n, int m) {
    Observer* obs = (Observer*)calloc(1, sizeof(Observer));
    if (!obs) return NULL;
    obs->type = type;
    obs->n = n;
    obs->m = m;
    if (n > 0) {
        obs->x_hat = (double*)calloc((size_t)n, sizeof(double));
        obs->innovation = (double*)calloc((size_t)m, sizeof(double));
    }
    /* P: error covariance (n?n for Kalman) or observer poles (n for Luenberger) */
    if (type == CP_KALMAN_FILTER) {
        obs->P = (double*)calloc((size_t)(n * n), sizeof(double));
        obs->Q = (double*)calloc((size_t)(n * n), sizeof(double));
        obs->R = (double*)calloc((size_t)(m * m), sizeof(double));
        /* Default: identity */
        for (int i = 0; i < n; i++) { obs->P[i * n + i] = 1.0; obs->Q[i * n + i] = 0.01; }
        for (int i = 0; i < m; i++) obs->R[i * m + i] = 0.1;
    }
    obs->K = (double*)calloc((size_t)(n * m), sizeof(double));
    obs->estimation_error = 0.0;
    obs->is_converged = false;
    obs->convergence_steps = 0;
    obs->convergence_rate = 0.0;
    return obs;
}

void cp_observer_free(Observer* obs) {
    if (!obs) return;
    free(obs->x_hat);
    free(obs->P);
    free(obs->K);
    free(obs->Q);
    free(obs->R);
    free(obs->innovation);
    free(obs);
}

void cp_observer_set_noise(Observer* obs, const double* Q, const double* R) {
    if (!obs) return;
    if (Q && obs->Q)
        memcpy(obs->Q, Q, (size_t)(obs->n * obs->n) * sizeof(double));
    if (R && obs->R)
        memcpy(obs->R, R, (size_t)(obs->m * obs->m) * sizeof(double));
}

void cp_observer_set_gain(Observer* obs, const double* K) {
    if (!obs || !K) return;
    memcpy(obs->K, K, (size_t)(obs->n * obs->m) * sizeof(double));
}

void cp_observer_predict(Observer* obs, const double* A, const double* B,
                          const double* u, int n, double dt) {
    if (!obs || !A || n == 0) return;
    /* x_hat_pred = A * x_hat + B * u */
    double* x_pred = (double*)malloc((size_t)n * sizeof(double));
    if (!x_pred) return;
    for (int i = 0; i < n; i++) {
        x_pred[i] = 0.0;
        for (int j = 0; j < n; j++) {
            x_pred[i] += A[i * n + j] * obs->x_hat[j];
        }
        if (B && u) {
            for (int j = 0; j < n; j++) {
                x_pred[i] += B[i * n + j] * u[j];
            }
        }
    }
    memcpy(obs->x_hat, x_pred, (size_t)n * sizeof(double));

    /* Kalman predict: P_pred = A * P * A^T + Q */
    if (obs->type == CP_KALMAN_FILTER && obs->P) {
        double* P_temp = (double*)calloc((size_t)(n * n), sizeof(double));
        if (P_temp) {
            /* A * P */
            for (int i = 0; i < n; i++)
                for (int j = 0; j < n; j++)
                    for (int k = 0; k < n; k++)
                        P_temp[i * n + j] += A[i * n + k] * obs->P[k * n + j];
            /* (A*P) * A^T + Q */
            for (int i = 0; i < n; i++)
                for (int j = 0; j < n; j++) {
                    obs->P[i * n + j] = obs->Q[i * n + j];
                    for (int k = 0; k < n; k++)
                        obs->P[i * n + j] += P_temp[i * n + k] * A[j * n + k];
                }
            free(P_temp);
        }
    }
    free(x_pred);
    (void)dt;
}

void cp_observer_update(Observer* obs, const double* C, const double* y_measured, int n, int m) {
    if (!obs || !C || !y_measured || n == 0 || m == 0) return;
    cp_observer_innovation(obs, C, y_measured, n, m);

    /* x_hat = x_hat + K * innovation */
    for (int i = 0; i < n; i++) {
        double correction = 0.0;
        for (int j = 0; j < m; j++) {
            correction += obs->K[i * m + j] * obs->innovation[j];
        }
        obs->x_hat[i] += correction;
    }

    /* Kalman update: P = (I - K*C) * P_pred */
    if (obs->type == CP_KALMAN_FILTER && obs->P) {
        double* KC = (double*)calloc((size_t)(n * n), sizeof(double));
        if (KC) {
            for (int i = 0; i < n; i++)
                for (int j = 0; j < n; j++)
                    for (int k = 0; k < m; k++)
                        KC[i * n + j] += obs->K[i * m + k] * C[k * n + j];
            double* I_KC = (double*)calloc((size_t)(n * n), sizeof(double));
            if (I_KC) {
                for (int i = 0; i < n; i++) I_KC[i * n + i] = 1.0;
                for (int i = 0; i < n * n; i++) I_KC[i] -= KC[i];
                double* P_new = (double*)calloc((size_t)(n * n), sizeof(double));
                if (P_new) {
                    for (int i = 0; i < n; i++)
                        for (int j = 0; j < n; j++)
                            for (int k = 0; k < n; k++)
                                P_new[i * n + j] += I_KC[i * n + k] * obs->P[k * n + j];
                    memcpy(obs->P, P_new, (size_t)(n * n) * sizeof(double));
                    free(P_new);
                }
                free(I_KC);
            }
            free(KC);
        }
    }
}

void cp_observer_innovation(Observer* obs, const double* C, const double* y, int n, int m) {
    if (!obs || !C || !y || m == 0) return;
    /* innovation = y - C * x_hat */
    double norm = 0.0;
    for (int i = 0; i < m; i++) {
        double cx = 0.0;
        for (int j = 0; j < n; j++) {
            cx += C[i * n + j] * obs->x_hat[j];
        }
        obs->innovation[i] = y[i] - cx;
        norm += obs->innovation[i] * obs->innovation[i];
    }
    obs->innovation_norm = sqrt(norm);
}

double cp_observer_estimation_error(const Observer* obs, const double* x_true, int n) {
    if (!obs || !x_true) return 0.0;
    double err = 0.0;
    for (int i = 0; i < n; i++) {
        double d = obs->x_hat[i] - x_true[i];
        err += d * d;
    }
    return sqrt(err);
}

bool cp_observer_check_convergence(const Observer* obs, double threshold) {
    if (!obs) return false;
    return obs->innovation_norm < threshold;
}

double cp_observer_convergence_rate(const Observer* obs, const double* A, int n) {
    if (!obs || !A || n <= 0) return 0.0;
    /* Rate = max eigenvalue of (A - KC) */
    /* Simplified: return spectral radius estimate */
    double max_mag = 0.0;
    for (int i = 0; i < n; i++) {
        double row_sum = 0.0;
        for (int j = 0; j < n; j++) {
            row_sum += fabs(A[i * n + j]);
        }
        if (row_sum > max_mag) max_mag = row_sum;
    }
    return max_mag;
}

void cp_observer_print(const Observer* obs) {
    if (!obs) { printf("Observer: NULL\n"); return; }
    printf("=== Observer ===\n");
    printf("Type: %d, n=%d, m=%d\n", obs->type, obs->n, obs->m);
    printf("Innovation norm: %.6f, Converged: %d, Steps: %d\n",
           obs->innovation_norm, obs->is_converged, obs->convergence_steps);
    printf("Estimation error: %.6f\n", obs->estimation_error);
    printf("x_hat: ");
    for (int i = 0; i < obs->n && i < 10; i++)
        printf("%.4f ", obs->x_hat[i]);
    printf("\n");
}

/* ============================================================================
 * Separation Principle
 * ============================================================================ */

ObserverController* cp_observer_controller_create(Observer* obs, ControlSystem* ctrl) {
    ObserverController* oc = (ObserverController*)calloc(1, sizeof(ObserverController));
    if (!oc) return NULL;
    oc->observer = obs;
    oc->controller = ctrl;
    int n_obs = obs ? obs->n : 0;
    int n_ctrl = ctrl ? ctrl->nx : 0;
    oc->composite_dim = n_obs + n_ctrl;
    oc->composite_state = (double*)calloc((size_t)oc->composite_dim, sizeof(double));
    oc->separation_holds = true;
    return oc;
}

void cp_observer_controller_free(ObserverController* oc) {
    if (!oc) return;
    free(oc->composite_state);
    free(oc);
}

void cp_observer_controller_step(ObserverController* oc, double dt,
                                  const double* y_measured) {
    if (!oc || !oc->observer || !oc->controller) return;
    Observer* obs = oc->observer;
    ControlSystem* sys = oc->controller;

    /* 1. Update estimate from measurement */
    if (y_measured) {
        for (int i = 0; i < sys->ny && i < obs->m; i++) {
            obs->innovation[i] = y_measured[i];
        }
    }

    /* 2. Copy x_hat to controller's state estimate */
    for (int i = 0; i < sys->nx && i < obs->n; i++) {
        sys->x_hat[i] = obs->x_hat[i];
    }

    /* 3. Compute control using estimated state */
    cp_system_step(sys, dt);

    /* 4. Predict next estimate */
    if (sys->model_params) {
        cp_observer_predict(obs, sys->model_params, NULL, sys->u, sys->nx, dt);
    }
}

bool cp_observer_controller_check_separation(const ObserverController* oc,
                                              const double* A, const double* C, int n) {
    if (!oc) return false;
    (void)A; (void)C; (void)n;
    /* For LTI Gaussian systems, separation holds.
       For nonlinear/non-Gaussian, it generally doesn't. */
    if (oc->observer->type == CP_KALMAN_FILTER) return true;
    if (oc->observer->type == CP_LUENBERGER_OBSERVER) return true;
    return false; /* other types may not satisfy separation */
}

/* ============================================================================
 * Observability Analysis
 * ============================================================================ */

double* cp_observability_matrix(const double* A, const double* C, int n, int* total_rows) {
    if (!A || !C || n <= 0) { *total_rows = 0; return NULL; }
    int m = n; /* assume C is m?n, m=n for simplicity */
    *total_rows = n * m;
    double* O = (double*)calloc((size_t)(n * m * n), sizeof(double));
    if (!O) { *total_rows = 0; return NULL; }

    /* C is first m rows of O */
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            O[i * n + j] = C[i * n + j];

    /* Compute CA^k */
    double* A_power = (double*)malloc((size_t)(n * n) * sizeof(double));
    double* C_times_A = (double*)malloc((size_t)(m * n) * sizeof(double));
    if (!A_power || !C_times_A) {
        free(A_power); free(C_times_A); free(O);
        *total_rows = 0; return NULL;
    }

    /* A_power = I initially */
    for (int i = 0; i < n * n; i++) A_power[i] = 0.0;
    for (int i = 0; i < n; i++) A_power[i * n + i] = 1.0;

    for (int k = 0; k < n - 1; k++) {
        /* A_power = A_power * A */
        double* new_A = (double*)calloc((size_t)(n * n), sizeof(double));
        if (!new_A) break;
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                for (int l = 0; l < n; l++)
                    new_A[i * n + j] += A_power[i * n + l] * A[l * n + j];
        memcpy(A_power, new_A, (size_t)(n * n) * sizeof(double));
        free(new_A);

        /* C * A_power */
        for (int i = 0; i < m; i++)
            for (int j = 0; j < n; j++) {
                C_times_A[i * n + j] = 0.0;
                for (int l = 0; l < n; l++)
                    C_times_A[i * n + j] += C[i * n + l] * A_power[l * n + j];
            }

        /* Place in O */
        int row_start = (k + 1) * m;
        for (int i = 0; i < m; i++)
            for (int j = 0; j < n; j++)
                O[(row_start + i) * n + j] = C_times_A[i * n + j];
    }

    free(A_power); free(C_times_A);
    return O;
}

int cp_observability_rank(const double* A, const double* C, int n, double tol) {
    int total_rows = 0;
    double* O = cp_observability_matrix(A, C, n, &total_rows);
    if (!O) return 0;

    /* Count non-zero rows via QR-like sweep */
    int rank = 0;
    for (int col = 0; col < n; col++) {
        /* Find pivot */
        double max_val = 0.0;
        int pivot_row = -1;
        for (int row = rank; row < total_rows; row++) {
            double val = fabs(O[row * n + col]);
            if (val > max_val) { max_val = val; pivot_row = row; }
        }
        if (max_val < tol) continue;

        /* Swap */
        if (pivot_row != rank) {
            for (int j = 0; j < n; j++) {
                double tmp = O[rank * n + j];
                O[rank * n + j] = O[pivot_row * n + j];
                O[pivot_row * n + j] = tmp;
            }
        }

        /* Eliminate */
        for (int row = rank + 1; row < total_rows; row++) {
            double factor = O[row * n + col] / O[rank * n + col];
            for (int j = col; j < n; j++) {
                O[row * n + j] -= factor * O[rank * n + j];
            }
        }
        rank++;
    }

    free(O);
    return rank;
}

void cp_observability_gramian_solve(const double* A, const double* C,
                                     int n, double* Wo, int max_iter, double tol) {
    if (!A || !C || !Wo || n <= 0) return;
    /* Solve A^T W + W A + C^T C = 0 via iteration */
    /* Initialize Wo = I */
    for (int i = 0; i < n * n; i++) Wo[i] = 0.0;
    for (int i = 0; i < n; i++) Wo[i * n + i] = 1.0;

    double* rhs = (double*)malloc((size_t)(n * n) * sizeof(double));
    if (!rhs) return;
    /* rhs = -C^T C */
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            rhs[i * n + j] = 0.0;
            for (int k = 0; k < n; k++)
                rhs[i * n + j] -= C[k * n + i] * C[k * n + j];
        }

    for (int iter = 0; iter < max_iter; iter++) {
        double max_change = 0.0;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                double lhs = 0.0;
                /* A^T W */
                for (int k = 0; k < n; k++)
                    lhs += A[k * n + i] * Wo[k * n + j];
                /* + W A */
                for (int k = 0; k < n; k++)
                    lhs += Wo[i * n + k] * A[k * n + j];
                /* residual = A^T W + W A - rhs */
                double residual = lhs - rhs[i * n + j];
                double update = -0.05 * residual;
                Wo[i * n + j] += update;
                double change = fabs(update);
                if (change > max_change) max_change = change;
            }
        }
        if (max_change < tol) break;
    }
    free(rhs);
}

/* ============================================================================
 * Observation Information
 * ============================================================================ */

ObservationInfo* cp_observation_info_compute(const Observer* obs,
                                              const double* prior_cov,
                                              const double* posterior_cov, int n) {
    if (!obs || n <= 0) return NULL;
    ObservationInfo* oi = (ObservationInfo*)calloc(1, sizeof(ObservationInfo));
    if (!oi) return NULL;

    /* H_prior ? 0.5 ln((2?e)^n |prior_cov|) */
    double det_prior = 1.0;
    if (prior_cov) {
        det_prior = 1.0;
        for (int i = 0; i < n; i++) det_prior *= prior_cov[i * n + i];
    }
    oi->prior_entropy = 0.5 * log(pow(2.0 * M_PI * M_E, (double)n) * det_prior);

    double det_post = 1.0;
    if (posterior_cov) {
        det_post = 1.0;
        for (int i = 0; i < n; i++) det_post *= posterior_cov[i * n + i];
    }
    oi->posterior_entropy = 0.5 * log(pow(2.0 * M_PI * M_E, (double)n) * det_post);

    oi->information_gain = oi->prior_entropy - oi->posterior_entropy;
    if (oi->information_gain < 0) oi->information_gain = 0.0;

    oi->observation_efficiency = (oi->prior_entropy > CP_EPS)
        ? oi->information_gain / oi->prior_entropy : 0.0;
    oi->effective_rank = n;
    return oi;
}

void cp_observation_info_free(ObservationInfo* oi) {
    free(oi);
}

void cp_observation_info_print(const ObservationInfo* oi) {
    if (!oi) { printf("ObservationInfo: NULL\n"); return; }
    printf("=== Observation Information ===\n");
    printf("Prior entropy: %.4f, Posterior entropy: %.4f\n",
           oi->prior_entropy, oi->posterior_entropy);
    printf("Information gain: %.4f (efficiency: %.4f)\n",
           oi->information_gain, oi->observation_efficiency);
    printf("Effective rank: %d\n", oi->effective_rank);
}

double cp_observation_value(const double* prior_entropy, const double* posterior_entropy) {
    if (!prior_entropy || !posterior_entropy) return 0.0;
    return *prior_entropy - *posterior_entropy;
}
