#include "adaptive_regulator.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ===== AdaptiveRegulator ===== */
AdaptiveRegulator* adaptive_regulator_create(const char* name, int state_dim, int control_dim) {
    AdaptiveRegulator* reg = (AdaptiveRegulator*)calloc(1, sizeof(AdaptiveRegulator));
    reg->name = strdup(name);
    reg->model_type = MODEL_LINEAR;
    reg->linear_model = NULL;
    reg->model_params = NULL; reg->n_model_params = 0;
    reg->model_fidelity = 0.5; reg->model_uncertainty = 1.0;
    reg->bayesian_belief = NULL;
    reg->n_control_outputs = control_dim;
    reg->setpoint = (double*)calloc(state_dim, sizeof(double));
    reg->current_action = (double*)calloc(control_dim, sizeof(double));
    reg->proportional_gain = 1.0; reg->integral_gain = 0.1; reg->derivative_gain = 0.05;
    reg->integral_error = (double*)calloc(state_dim, sizeof(double));
    reg->prev_error = (double*)calloc(state_dim, sizeof(double));
    reg->use_feedforward = false; reg->feedforward_gain = 0.5;
    reg->adapt_model = false; reg->model_update_rate = 0.1;
    reg->prediction_error = 0.0;
    reg->error_history_capacity = 100;
    reg->error_history = (double*)calloc(reg->error_history_capacity, sizeof(double));
    reg->error_history_len = 0;
    reg->rise_time = 0.0; reg->settling_time = 0.0; reg->overshoot = 0.0;
    reg->steady_state_error = 0.0; reg->control_effort = 0.0;
    return reg;
}

void adaptive_regulator_free(AdaptiveRegulator* reg) {
    if (!reg) return;
    free(reg->name);
    if (reg->linear_model) {
        ashby_matrix_free(reg->linear_model->A);
        ashby_matrix_free(reg->linear_model->B);
        ashby_matrix_free(reg->linear_model->C);
        free(reg->linear_model->process_noise);
        free(reg->linear_model->observation_noise);
        free(reg->linear_model);
    }
    free(reg->model_params);
    if (reg->bayesian_belief) bayesian_belief_free(reg->bayesian_belief);
    free(reg->setpoint); free(reg->current_action);
    free(reg->integral_error); free(reg->prev_error);
    free(reg->error_history); free(reg);
}

void adaptive_regulator_set_model_type(AdaptiveRegulator* reg, InternalModelType type) {
    if (!reg) return;
    reg->model_type = type;
}

void adaptive_regulator_set_linear_model(AdaptiveRegulator* reg, const double* A, const double* B, int n, int m) {
    if (!reg) return;
    if (reg->linear_model) {
        ashby_matrix_free(reg->linear_model->A); ashby_matrix_free(reg->linear_model->B);
        ashby_matrix_free(reg->linear_model->C);
        free(reg->linear_model->process_noise); free(reg->linear_model->observation_noise);
        free(reg->linear_model);
    }
    reg->linear_model = (LinearDynamicalModel*)calloc(1, sizeof(LinearDynamicalModel));
    reg->linear_model->state_dim = n; reg->linear_model->control_dim = m;
    reg->linear_model->observation_dim = n;
    reg->linear_model->A = ashby_matrix_create(n, n);
    reg->linear_model->B = ashby_matrix_create(n, m);
    reg->linear_model->C = ashby_matrix_create(n, n);
    for (int i = 0; i < n*n; i++) reg->linear_model->A->data[i] = A[i];
    for (int i = 0; i < n*m; i++) reg->linear_model->B->data[i] = B[i];
    for (int i = 0; i < n; i++) ashby_matrix_set(reg->linear_model->C, i, i, 1.0);
    reg->linear_model->process_noise = (double*)calloc(n, sizeof(double));
    reg->linear_model->observation_noise = (double*)calloc(n, sizeof(double));
    reg->linear_model->is_learned = false;
}

void adaptive_regulator_set_setpoint(AdaptiveRegulator* reg, const double* setpoint, int n) {
    if (!reg) return;
    memcpy(reg->setpoint, setpoint, n * sizeof(double));
}

void adaptive_regulator_set_gains(AdaptiveRegulator* reg, double Kp, double Ki, double Kd) {
    if (!reg) return;
    reg->proportional_gain = Kp; reg->integral_gain = Ki; reg->derivative_gain = Kd;
}

void adaptive_regulator_compute_error(AdaptiveRegulator* reg, const double* current_state, double* error, int n) {
    if (!reg || !current_state || !error) return;
    for (int i = 0; i < n; i++) error[i] = reg->setpoint[i] - current_state[i];
}

void adaptive_regulator_pid_action(AdaptiveRegulator* reg, const double* error, double dt, double* action, int n) {
    if (!reg || !error || !action) return;
    for (int i = 0; i < n; i++) {
        double P = reg->proportional_gain * error[i];
        reg->integral_error[i] += error[i] * dt;
        double I = reg->integral_gain * reg->integral_error[i];
        double D = 0.0;
        if (dt > 1e-10) D = reg->derivative_gain * (error[i] - reg->prev_error[i]) / dt;
        action[i] = P + I + D;
        reg->prev_error[i] = error[i];
    }
}

void adaptive_regulator_feedforward_action(AdaptiveRegulator* reg, const double* current_state, const double* disturbance_estimate, double* action, int n) {
    if (!reg || !action) return;
    /* Feedforward: use model to predict required action */
    if (reg->linear_model && reg->use_feedforward) {
        double* desired_state = reg->setpoint;
        /* Inverse: u = B^+ * (x_des - A*x) */
        for (int i = 0; i < n; i++) {
            double pred_evol = 0.0;
            for (int j = 0; j < reg->linear_model->state_dim; j++)
                pred_evol += ashby_matrix_get(reg->linear_model->A, i, j) * current_state[j];
            action[i] = reg->feedforward_gain * (desired_state[i] - pred_evol);
            if (disturbance_estimate) action[i] -= reg->feedforward_gain * disturbance_estimate[i];
        }
    }
}

void adaptive_regulator_total_action(AdaptiveRegulator* reg, const double* current_state, const double* disturbance_estimate, double dt, double* action, int n) {
    if (!reg) return;
    double* error = (double*)calloc(n, sizeof(double));
    double* ff_action = (double*)calloc(n, sizeof(double));
    adaptive_regulator_compute_error(reg, current_state, error, n);
    adaptive_regulator_pid_action(reg, error, dt, action, n);
    adaptive_regulator_feedforward_action(reg, current_state, disturbance_estimate, ff_action, n);
    for (int i = 0; i < n; i++) {
        action[i] += ff_action[i];
        reg->current_action[i] = action[i];
    }
    /* Update error history */
    double rmse = 0.0;
    for (int i = 0; i < n; i++) rmse += error[i] * error[i];
    rmse = sqrt(rmse / n);
    if (reg->error_history_len < reg->error_history_capacity)
        reg->error_history[reg->error_history_len++] = rmse;
    free(error); free(ff_action);
}

void adaptive_regulator_predict_forward(AdaptiveRegulator* reg, const double* current_state, const double* action, double* predicted_next, int n) {
    if (!reg || !reg->linear_model) return;
    /* x(t+1) = A*x(t) + B*u(t) */
    double* Ax = (double*)calloc(n, sizeof(double));
    double* Bu = (double*)calloc(n, sizeof(double));
    ashby_matrix_vec_mul(reg->linear_model->A, current_state, Ax);
    ashby_matrix_vec_mul(reg->linear_model->B, action, Bu);
    for (int i = 0; i < n; i++) predicted_next[i] = Ax[i] + Bu[i];
    free(Ax); free(Bu);
}

void adaptive_regulator_predict_inverse(AdaptiveRegulator* reg, const double* current_state, const double* desired_next, double* required_action, int n) {
    if (!reg || !reg->linear_model) return;
    /* u = B^+ * (x_next - A*x), using pseudoinverse via transpose */
    double* Ax = (double*)calloc(n, sizeof(double));
    ashby_matrix_vec_mul(reg->linear_model->A, current_state, Ax);
    double* delta = (double*)malloc(n * sizeof(double));
    for (int i = 0; i < n; i++) delta[i] = desired_next[i] - Ax[i];
    /* Simple inverse: if B is square and diagonal-dominant */
    for (int i = 0; i < n && i < reg->linear_model->control_dim; i++) {
        double b_ii = ashby_matrix_get(reg->linear_model->B, i, i);
        required_action[i] = (fabs(b_ii) > 1e-10) ? delta[i] / b_ii : 0.0;
    }
    free(Ax); free(delta);
}

void adaptive_regulator_update_model(AdaptiveRegulator* reg, const double* state_before, const double* action, const double* state_after, double dt) {
    if (!reg || !reg->linear_model || !reg->adapt_model) return;
    double lr = reg->model_update_rate;
    int n = reg->linear_model->state_dim;
    int m = reg->linear_model->control_dim;
    /* Simple gradient descent on A and B to minimize prediction error */
    double* predicted = (double*)malloc(n * sizeof(double));
    adaptive_regulator_predict_forward(reg, state_before, action, predicted, n);
    for (int i = 0; i < n; i++) {
        double err = state_after[i] - predicted[i];
        /* Update A: dA_ij = lr * err_i * state_j */
        for (int j = 0; j < n; j++) {
            double a_old = ashby_matrix_get(reg->linear_model->A, i, j);
            double da = lr * err * state_before[j];
            ashby_matrix_set(reg->linear_model->A, i, j, a_old + da);
        }
        /* Update B: dB_ij = lr * err_i * action_j */
        for (int j = 0; j < m; j++) {
            double b_old = ashby_matrix_get(reg->linear_model->B, i, j);
            double db = lr * err * action[j];
            ashby_matrix_set(reg->linear_model->B, i, j, b_old + db);
        }
    }
    reg->linear_model->is_learned = true;
    reg->prediction_error = adaptive_regulator_model_prediction_error(reg, predicted, state_after, n);
    (void)dt;
    free(predicted);
}

double adaptive_regulator_model_prediction_error(AdaptiveRegulator* reg, const double* predicted, const double* actual, int n) {
    if (!reg) return 0.0;
    double mse = 0.0;
    for (int i = 0; i < n; i++) {
        double err = actual[i] - predicted[i];
        mse += err * err;
    }
    return sqrt(mse / n);
}

/* ===== Bayesian Belief ===== */
BayesianParameterBelief* bayesian_belief_create(int n_params) {
    BayesianParameterBelief* b = (BayesianParameterBelief*)calloc(1, sizeof(BayesianParameterBelief));
    b->n_params = n_params;
    b->param_means = (double*)calloc(n_params, sizeof(double));
    b->param_vars = (double*)malloc(n_params * sizeof(double));
    for (int i = 0; i < n_params; i++) b->param_vars[i] = 1.0;
    /* Covariance matrix: diagonal by default */
    b->param_cov = (double**)malloc(n_params * sizeof(double*));
    for (int i = 0; i < n_params; i++) {
        b->param_cov[i] = (double*)calloc(n_params, sizeof(double));
        b->param_cov[i][i] = 1.0;
    }
    b->prior_strength = 1.0; b->learning_rate = 0.1;
    return b;
}

void bayesian_belief_free(BayesianParameterBelief* b) {
    if (!b) return;
    free(b->param_means); free(b->param_vars);
    for (int i = 0; i < b->n_params; i++) free(b->param_cov[i]);
    free(b->param_cov); free(b);
}

void bayesian_belief_initialize(BayesianParameterBelief* b, const double* prior_means, const double* prior_vars, double prior_strength) {
    if (!b) return;
    memcpy(b->param_means, prior_means, b->n_params * sizeof(double));
    memcpy(b->param_vars, prior_vars, b->n_params * sizeof(double));
    b->prior_strength = prior_strength;
    for (int i = 0; i < b->n_params; i++) {
        for (int j = 0; j < b->n_params; j++) b->param_cov[i][j] = 0.0;
        b->param_cov[i][i] = prior_vars[i];
    }
}

void bayesian_belief_update(BayesianParameterBelief* b, const double* observation, const double* predicted, const double* jacobian, int n_obs) {
    if (!b) return;
    /* Simplified Kalman-like update for each parameter independently */
    double lr = b->learning_rate;
    for (int i = 0; i < b->n_params; i++) {
        double grad = 0.0;
        for (int k = 0; k < n_obs; k++) {
            double innov = observation[k] - predicted[k];
            grad += innov * jacobian[k * b->n_params + i];
        }
        double update = lr * grad;
        b->param_means[i] += update;
        b->param_vars[i] *= (1.0 - lr * 0.5);
        if (b->param_vars[i] < 1e-6) b->param_vars[i] = 1e-6;
    }
}

double bayesian_belief_uncertainty(const BayesianParameterBelief* b) {
    if (!b) return 0.0;
    double trace = 0.0;
    for (int i = 0; i < b->n_params; i++) trace += b->param_vars[i];
    return trace;
}

void bayesian_belief_sample(BayesianParameterBelief* b, double* sample) {
    if (!b) return;
    for (int i = 0; i < b->n_params; i++) {
        double u1 = (double)rand() / RAND_MAX;
        double u2 = (double)rand() / RAND_MAX;
        double z = sqrt(-2.0 * log(u1 + 1e-15)) * cos(6.2831853 * u2);
        sample[i] = b->param_means[i] + z * sqrt(b->param_vars[i]);
    }
}

void bayesian_belief_print(const BayesianParameterBelief* b) {
    if (!b) return;
    printf("Bayesian Belief: %d params, uncertainty=%.4f\n", b->n_params, bayesian_belief_uncertainty(b));
    for (int i = 0; i < b->n_params; i++)
        printf("  param[%d]: mean=%.4f, var=%.6f\n", i, b->param_means[i], b->param_vars[i]);
}

/* ===== Performance Analysis ===== */
void adaptive_regulator_evaluate_performance(AdaptiveRegulator* reg, double dt, double total_time) {
    if (!reg) return;
    double* step_response = reg->error_history;
    int n = reg->error_history_len;
    if (n < 2) return;
    /* Rise time: first time error < 10% of initial error */
    double initial_err = step_response[0];
    if (initial_err < 1e-10) { reg->rise_time = 0.0; reg->settling_time = 0.0; reg->overshoot = 0.0; reg->steady_state_error = 0.0; return; }
    double target_10 = 0.1 * initial_err;
    double target_5 = 0.05 * initial_err;
    reg->rise_time = -1.0;
    reg->settling_time = -1.0;
    reg->overshoot = 0.0;
    for (int i = 0; i < n; i++) {
        if (reg->rise_time < 0 && step_response[i] < target_10) reg->rise_time = i * dt;
        /* Overshoot: max error beyond 0 in the other direction */
        if (step_response[i] < -reg->overshoot) reg->overshoot = -step_response[i];
    }
    /* Settling time: last time error goes outside 5% band */
    for (int i = n - 1; i >= 0; i--) {
        if (fabs(step_response[i]) > target_5) {
            reg->settling_time = (i < n - 1) ? (i + 1) * dt : total_time;
            break;
        }
    }
    reg->steady_state_error = step_response[n - 1];
    /* Control effort: sum of squared actions */
    double effort = 0.0;
    for (int i = 0; i < reg->n_control_outputs; i++)
        effort += reg->current_action[i] * reg->current_action[i];
    reg->control_effort = effort;
    (void)total_time;
}

double adaptive_regulator_cost(const AdaptiveRegulator* reg) {
    if (!reg || reg->error_history_len == 0) return 0.0;
    double cost = 0.0;
    for (int i = 0; i < reg->error_history_len; i++) {
        double e = reg->error_history[i];
        cost += e * e;
    }
    return cost / reg->error_history_len;
}

double adaptive_regulator_model_fidelity_score(AdaptiveRegulator* reg, const double* actual_params, int n) {
    if (!reg || !reg->linear_model || !actual_params) return 0.0;
    /* Compare learned model A matrix to actual parameters */
    double diff_sum = 0.0;
    double norm_sum = 0.0;
    for (int i = 0; i < n && i < reg->linear_model->state_dim; i++) {
        double a_ii = ashby_matrix_get(reg->linear_model->A, i, i);
        double diff = a_ii - actual_params[i];
        diff_sum += diff * diff;
        norm_sum += actual_params[i] * actual_params[i];
    }
    if (norm_sum < 1e-10) return 0.0;
    return 1.0 - sqrt(diff_sum / norm_sum);
}

void adaptive_regulator_print(const AdaptiveRegulator* reg) {
    if (!reg) return;
    printf("=== Adaptive Regulator: %s ===\n", reg->name);
    printf("Model type: %d, fidelity: %.2f, uncertainty: %.2f\n", reg->model_type, reg->model_fidelity, reg->model_uncertainty);
    printf("Gains: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", reg->proportional_gain, reg->integral_gain, reg->derivative_gain);
    printf("Feedforward: %s (gain=%.2f), Adapt: %s (rate=%.3f)\n",
           reg->use_feedforward ? "on" : "off", reg->feedforward_gain,
           reg->adapt_model ? "on" : "off", reg->model_update_rate);
    printf("Performance: rise=%.3f, settle=%.3f, overshoot=%.2f%%, SSE=%.4f, cost=%.4f\n",
           reg->rise_time, reg->settling_time, reg->overshoot * 100.0,
           reg->steady_state_error, adaptive_regulator_cost(reg));
}

/* ===== Conant-Ashby Theorem ===== */
double conant_ashby_isomorphism_measure(const AdaptiveRegulator* reg, const double* system_params, int n_sys_params) {
    if (!reg || !reg->model_params || !system_params) return 0.0;
    /* Measure the structural similarity between regulator model and system */
    double dot = 0.0, norm_m = 0.0, norm_s = 0.0;
    int n = (reg->n_model_params < n_sys_params) ? reg->n_model_params : n_sys_params;
    for (int i = 0; i < n; i++) {
        dot += reg->model_params[i] * system_params[i];
        norm_m += reg->model_params[i] * reg->model_params[i];
        norm_s += system_params[i] * system_params[i];
    }
    double denom = sqrt(norm_m * norm_s);
    if (denom < 1e-10) return 0.0;
    return dot / denom;
}

bool conant_ashby_regulator_quality_check(const AdaptiveRegulator* reg, double regulation_error, double model_error, double threshold) {
    if (!reg) return false;
    /* The Conant-Ashby theorem implies that model error bounds regulation error */
    return model_error < threshold && regulation_error < 2.0 * model_error;
}