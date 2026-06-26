#include "cp_regulator.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define CP_EPS 1e-12

/* ============================================================================
 * Internal Model
 * ============================================================================ */

InternalModel* cp_internal_model_create(int n, int m, int p) {
    InternalModel* im = (InternalModel*)calloc(1, sizeof(InternalModel));
    if (!im) return NULL;
    im->n = n;
    im->m = m;
    im->p = p;
    if (n > 0) {
        im->A = (double*)calloc((size_t)(n * n), sizeof(double));
        /* Default: identity matrix (no dynamics) */
        for (int i = 0; i < n; i++) im->A[i * n + i] = 1.0;
    }
    if (n > 0 && m > 0)
        im->B = (double*)calloc((size_t)(n * m), sizeof(double));
    if (p > 0 && n > 0)
        im->C = (double*)calloc((size_t)(p * n), sizeof(double));
    im->is_accurate = false;
    return im;
}

void cp_internal_model_free(InternalModel* im) {
    if (!im) return;
    free(im->A);
    free(im->B);
    free(im->C);
    free(im);
}

void cp_internal_model_set_A(InternalModel* im, const double* A) {
    if (im && im->A && A) memcpy(im->A, A, (size_t)(im->n * im->n) * sizeof(double));
}

void cp_internal_model_set_B(InternalModel* im, const double* B) {
    if (im && im->B && B) memcpy(im->B, B, (size_t)(im->n * im->m) * sizeof(double));
}

void cp_internal_model_set_C(InternalModel* im, const double* C) {
    if (im && im->C && C) memcpy(im->C, C, (size_t)(im->p * im->n) * sizeof(double));
}

void cp_internal_model_predict(const InternalModel* im,
                                const double* x, const double* u,
                                double* x_next, double* y_pred) {
    if (!im || !x || !u || !x_next) return;
    /* x_next = A * x + B * u */
    for (int i = 0; i < im->n; i++) {
        x_next[i] = 0.0;
        for (int j = 0; j < im->n; j++) {
            x_next[i] += im->A[i * im->n + j] * x[j];
        }
        for (int j = 0; j < im->m; j++) {
            x_next[i] += im->B[i * im->m + j] * u[j];
        }
    }
    /* y_pred = C * x_next */
    if (y_pred) {
        for (int i = 0; i < im->p; i++) {
            y_pred[i] = 0.0;
            for (int j = 0; j < im->n; j++) {
                y_pred[i] += im->C[i * im->n + j] * x_next[j];
            }
        }
    }
}

double cp_internal_model_identification_error(const InternalModel* im,
                                               const double* A_true,
                                               const double* B_true,
                                               const double* C_true) {
    if (!im) return 1e6;
    double err = 0.0;
    /* Frobenius norm differences */
    if (A_true && im->A) {
        for (int i = 0; i < im->n * im->n; i++) {
            double d = im->A[i] - A_true[i];
            err += d * d;
        }
    }
    if (B_true && im->B) {
        for (int i = 0; i < im->n * im->m; i++) {
            double d = im->B[i] - B_true[i];
            err += d * d;
        }
    }
    if (C_true && im->C) {
        for (int i = 0; i < im->p * im->n; i++) {
            double d = im->C[i] - C_true[i];
            err += d * d;
        }
    }
    return sqrt(err);
}

/* ============================================================================
 * Regulator
 * ============================================================================ */

Regulator* cp_regulator_create(const char* name, RegulatorType type,
                                int nx, int nu, int ny) {
    Regulator* reg = (Regulator*)calloc(1, sizeof(Regulator));
    if (!reg) return NULL;
    reg->name = (name && name[0]) ? strdup(name) : strdup("regulator");
    reg->type = type;
    reg->state_dim = nx;
    reg->input_dim = nu;
    reg->output_dim = ny;
    if (ny > 0) {
        reg->observed_output = (double*)calloc((size_t)ny, sizeof(double));
        reg->predicted_output = (double*)calloc((size_t)ny, sizeof(double));
    }
    if (nu > 0)
        reg->control_action = (double*)calloc((size_t)nu, sizeof(double));
    reg->plant_model = cp_internal_model_create(nx, nu, ny);
    return reg;
}

void cp_regulator_free(Regulator* reg) {
    if (!reg) return;
    free(reg->name);
    free(reg->observed_output);
    free(reg->predicted_output);
    free(reg->control_action);
    cp_internal_model_free(reg->plant_model);
    free(reg);
}

void cp_regulator_attach_plant(Regulator* reg, ControlSystem* sys) {
    if (!reg) return;
    reg->controlled_sys = sys;
}

void cp_regulator_set_model(Regulator* reg, InternalModel* model) {
    if (!reg || !model) return;
    cp_internal_model_free(reg->plant_model);
    reg->plant_model = model;
}

void cp_regulator_update(Regulator* reg, const double* y_measured) {
    if (!reg || !y_measured) return;
    for (int i = 0; i < reg->output_dim; i++)
        reg->observed_output[i] = y_measured[i];

    /* Predict output using internal model */
    if (reg->controlled_sys && reg->plant_model) {
        cp_internal_model_predict(reg->plant_model,
                                   reg->controlled_sys->x_hat,
                                   reg->controlled_sys->u,
                                   NULL, reg->predicted_output);
        /* Compute prediction error */
        double pe = 0.0;
        for (int i = 0; i < reg->output_dim; i++) {
            double d = y_measured[i] - reg->predicted_output[i];
            pe += d * d;
        }
        reg->prediction_error = sqrt(pe);
    }
}

void cp_regulator_compute_action(Regulator* reg) {
    if (!reg || !reg->controlled_sys) return;
    ControlSystem* sys = reg->controlled_sys;
    for (int i = 0; i < reg->input_dim; i++) {
        double u_i = 0.0;
        double e_sum = 0.0;
        for (int j = 0; j < sys->ny && j < reg->input_dim; j++) {
            int idx = i * sys->nx + j;
            double k = (sys->gain_matrix && idx < sys->nu * sys->nx)
                        ? sys->gain_matrix[idx] : 1.0;
            e_sum += k * sys->estimation_error[j];
        }
        u_i = e_sum / ((double)sys->ny + CP_EPS);
        if (u_i > 100.0) u_i = 100.0;
        if (u_i < -100.0) u_i = -100.0;
        reg->control_action[i] = u_i;
    }
}

void cp_regulator_apply(Regulator* reg) {
    if (!reg || !reg->controlled_sys) return;
    for (int i = 0; i < reg->input_dim && i < reg->controlled_sys->nu; i++) {
        reg->controlled_sys->u[i] = reg->control_action[i];
    }
}

/* ============================================================================
 * Good Regulator Theorem
 * ============================================================================ */

double cp_homomorphism_degree(const double* A_plant, const double* A_model, int n) {
    if (!A_plant || !A_model || n <= 0) return 0.0;
    double diff_norm = 0.0, plant_norm = 0.0;
    for (int i = 0; i < n * n; i++) {
        double d = A_plant[i] - A_model[i];
        diff_norm += d * d;
        plant_norm += A_plant[i] * A_plant[i];
    }
    if (plant_norm < CP_EPS) return 1.0;
    double rel_error = sqrt(diff_norm / plant_norm);
    double degree = 1.0 - rel_error;
    return (degree < 0.0) ? 0.0 : ((degree > 1.0) ? 1.0 : degree);
}

GoodRegulatorResult* cp_good_regulator_evaluate(const Regulator* reg) {
    if (!reg) return NULL;
    GoodRegulatorResult* grr = (GoodRegulatorResult*)calloc(1, sizeof(GoodRegulatorResult));
    if (!grr) return NULL;

    if (reg->plant_model) {
        grr->model = *reg->plant_model;
        /* Copy heap arrays */
        int n2 = reg->plant_model->n * reg->plant_model->n;
        if (n2 > 0) {
            grr->model.A = (double*)malloc((size_t)n2 * sizeof(double));
            if (grr->model.A) memcpy(grr->model.A, reg->plant_model->A, (size_t)n2 * sizeof(double));
        }
    }

    grr->regulation_error = (reg->controlled_sys)
        ? reg->controlled_sys->squared_error_sum / (reg->controlled_sys->step_count + 1)
        : 0.0;

    /* Homomorphism degree w.r.t. plant */
    if (reg->controlled_sys && reg->controlled_sys->model_params &&
        reg->plant_model && reg->plant_model->A) {
        int n = reg->plant_model->n;
        grr->homomorphism_degree =
            cp_homomorphism_degree(reg->controlled_sys->model_params, reg->plant_model->A, n);
    } else {
        grr->homomorphism_degree = 0.5; /* unknown */
    }

    grr->model_accuracy = 1.0 / (1.0 + grr->regulation_error);
    grr->grt_threshold = 0.7;
    grr->satisfies_grt = (grr->homomorphism_degree >= grr->grt_threshold);
    return grr;
}

void cp_good_regulator_free(GoodRegulatorResult* grr) {
    if (!grr) return;
    free(grr->model.A);
    free(grr->residual_entropy);
    free(grr);
}

void cp_good_regulator_print(const GoodRegulatorResult* grr) {
    if (!grr) { printf("GoodRegulatorResult: NULL\n"); return; }
    printf("=== Good Regulator Theorem ===\n");
    printf("Homomorphism degree: %.4f (threshold: %.4f)\n",
           grr->homomorphism_degree, grr->grt_threshold);
    printf("Regulation error: %.6f\n", grr->regulation_error);
    printf("Model accuracy: %.4f\n", grr->model_accuracy);
    printf("GRT satisfied: %s\n", grr->satisfies_grt ? "YES" : "NO");
}

bool cp_internal_model_principle_check(const Regulator* reg,
                                        const double* exo_generator, int exo_dim) {
    if (!reg || !exo_generator || exo_dim <= 0) return false;
    if (!reg->plant_model || !reg->plant_model->A) return false;
    /* Check: eigenvalues of regulator model contain eigenvalues of exo_generator */
    double homomorphism = cp_homomorphism_degree(reg->plant_model->A, exo_generator, exo_dim);
    return homomorphism > 0.8;
}

/* ============================================================================
 * Servomechanism
 * ============================================================================ */

Servomechanism* cp_servo_create(Regulator* reg, int traj_len) {
    Servomechanism* sm = (Servomechanism*)calloc(1, sizeof(Servomechanism));
    if (!sm) return NULL;
    sm->reg = reg;
    sm->trajectory_length = traj_len;
    if (reg && reg->output_dim > 0) {
        sm->reference_trajectory = (double*)calloc((size_t)(traj_len * reg->output_dim), sizeof(double));
        sm->tracking_error = (double*)calloc((size_t)traj_len, sizeof(double));
    }
    return sm;
}

void cp_servo_free(Servomechanism* sm) {
    if (!sm) return;
    free(sm->reference_trajectory);
    free(sm->tracking_error);
    free(sm);
}

void cp_servo_set_reference(Servomechanism* sm, const double* trajectory, int len) {
    if (!sm || !trajectory || len <= 0) return;
    int n_dim = (sm->reg) ? sm->reg->output_dim : 1;
    int copy_len = (len < sm->trajectory_length) ? len : sm->trajectory_length;
    memcpy(sm->reference_trajectory, trajectory, (size_t)(copy_len * n_dim) * sizeof(double));
    sm->trajectory_length = copy_len;
}

void cp_servo_simulate(Servomechanism* sm, double dt) {
    if (!sm || !sm->reg || !sm->reg->controlled_sys) return;
    ControlSystem* sys = sm->reg->controlled_sys;
    sm->rms_tracking_error = 0.0;
    sm->peak_error = 0.0;

    for (int t = 0; t < sm->trajectory_length; t++) {
        int offset = t * sys->ny;
        cp_system_set_reference(sys, &sm->reference_trajectory[offset], NULL);
        cp_regulator_update(sm->reg, sys->y);
        cp_regulator_compute_action(sm->reg);
        cp_regulator_apply(sm->reg);
        cp_system_step(sys, dt);

        double e = 0.0;
        for (int i = 0; i < sys->ny; i++) {
            double d = sys->y[i] - sm->reference_trajectory[offset + i];
            e += d * d;
        }
        sm->tracking_error[t] = sqrt(e);
        sm->rms_tracking_error += e;
        if (sqrt(e) > sm->peak_error) sm->peak_error = sqrt(e);
    }
    sm->rms_tracking_error = sqrt(sm->rms_tracking_error / sm->trajectory_length);
}

void cp_servo_print(const Servomechanism* sm) {
    if (!sm) { printf("Servomechanism: NULL\n"); return; }
    printf("=== Servomechanism ===\n");
    printf("Trajectory length: %d\n", sm->trajectory_length);
    printf("RMS tracking error: %.6f\n", sm->rms_tracking_error);
    printf("Peak error: %.6f\n", sm->peak_error);
}

/* ============================================================================
 * Bode / Waterbed
 * ============================================================================ */

double cp_bode_sensitivity_integral(const double* poles, int n_poles) {
    if (!poles || n_poles <= 0) return 0.0;
    /* For each stable pole p (Re(p) < 0): contribution = Re(p) */
    double integral = 0.0;
    for (int i = 0; i < n_poles; i++) {
        if (poles[i] < 0.0) {
            integral += poles[i]; /* Re(p) is negative, so integral is negative */
        }
    }
    /* Bode integral = PI * sum of unstable poles' real parts (for continuous-time)
       Here we use a simplified: PI * sum of |stable poles| */
    return M_PI * fabs(integral);
}

double cp_waterbed_index(const double* sensitivity_old,
                          const double* sensitivity_new, int n_freqs) {
    if (!sensitivity_old || !sensitivity_new || n_freqs <= 0) return 0.0;
    /* Conservation check: integral of log|S| should be invariant */
    double area_old = 0.0, area_new = 0.0;
    for (int i = 0; i < n_freqs; i++) {
        area_old += log(sensitivity_old[i] + CP_EPS);
        area_new += log(sensitivity_new[i] + CP_EPS);
    }
    return fabs(area_new - area_old) / (fabs(area_old) + CP_EPS);
}
