#include "cp_variety.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CP_EPS 1e-12

/* ============================================================================
 * Variety & Entropy
 * ============================================================================ */

double cp_variety(int num_states) {
    if (num_states <= 1) return 0.0;
    return log2((double)num_states);
}

double cp_effective_variety(double entropy, bool in_bits) {
    if (entropy < CP_EPS) return 1.0;
    if (in_bits) {
        return pow(2.0, entropy);
    } else {
        return exp(entropy);
    }
}

double cp_entropy(const double* probabilities, int n, bool in_bits) {
    if (!probabilities || n <= 0) return 0.0;
    double h = 0.0;
    for (int i = 0; i < n; i++) {
        if (probabilities[i] > CP_EPS) {
            double lp = in_bits ? log2(probabilities[i]) : log(probabilities[i]);
            h -= probabilities[i] * lp;
        }
    }
    return h;
}

double cp_conditional_entropy(const double* joint_pmf, int nx, int ny, bool in_bits) {
    if (!joint_pmf || nx <= 0 || ny <= 0) return 0.0;
    /* Marginalize to get P(X) */
    double* px = (double*)calloc((size_t)nx, sizeof(double));
    if (!px) return 0.0;
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            px[i] += joint_pmf[i * ny + j];
        }
    }
    /* H(Y|X) = sum_x P(x) * H(Y|X=x) */
    double h_cond = 0.0;
    for (int i = 0; i < nx; i++) {
        if (px[i] < CP_EPS) continue;
        double h_y_given_x = 0.0;
        for (int j = 0; j < ny; j++) {
            double py_given_x = joint_pmf[i * ny + j] / px[i];
            if (py_given_x > CP_EPS) {
                double lp = in_bits ? log2(py_given_x) : log(py_given_x);
                h_y_given_x -= py_given_x * lp;
            }
        }
        h_cond += px[i] * h_y_given_x;
    }
    free(px);
    return h_cond;
}

double cp_mutual_information(const double* joint_pmf, int nx, int ny, bool in_bits) {
    if (!joint_pmf || nx <= 0 || ny <= 0) return 0.0;
    /* Marginalize */
    double* px = (double*)calloc((size_t)nx, sizeof(double));
    double* py = (double*)calloc((size_t)ny, sizeof(double));
    if (!px || !py) { free(px); free(py); return 0.0; }
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            px[i] += joint_pmf[i * ny + j];
            py[j] += joint_pmf[i * ny + j];
        }
    }
    double mi = 0.0;
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            double pxy = joint_pmf[i * ny + j];
            if (pxy > CP_EPS && px[i] > CP_EPS && py[j] > CP_EPS) {
                double ratio = pxy / (px[i] * py[j]);
                if (in_bits)
                    mi += pxy * log2(ratio);
                else
                    mi += pxy * log(ratio);
            }
        }
    }
    free(px); free(py);
    return mi;
}

double cp_ashby_index(double variety_regulator, double variety_disturbance,
                       double channel_capacity) {
    return variety_regulator - variety_disturbance + channel_capacity;
}

/* ============================================================================
 * Control Channel
 * ============================================================================ */

ControlChannel* cp_channel_create(int input_size, int output_size) {
    ControlChannel* ch = (ControlChannel*)calloc(1, sizeof(ControlChannel));
    if (!ch) return NULL;
    ch->input_alphabet_size = input_size;
    ch->output_alphabet_size = output_size;
    int n = input_size * output_size;
    ch->transition_matrix = (double*)calloc((size_t)n, sizeof(double));
    return ch;
}

void cp_channel_free(ControlChannel* ch) {
    if (!ch) return;
    free(ch->transition_matrix);
    free(ch);
}

void cp_channel_set_transition(ControlChannel* ch, int input, int output, double prob) {
    if (!ch || input < 0 || input >= ch->input_alphabet_size ||
        output < 0 || output >= ch->output_alphabet_size) return;
    ch->transition_matrix[input * ch->output_alphabet_size + output] = prob;
}

double cp_channel_compute_capacity(ControlChannel* ch, int max_iter, double tol) {
    if (!ch || max_iter < 1) return 0.0;
    int nx = ch->input_alphabet_size;
    int ny = ch->output_alphabet_size;

    /* Blahut-Arimoto algorithm */
    double* px = (double*)malloc((size_t)nx * sizeof(double));
    double* qx = (double*)malloc((size_t)nx * sizeof(double));
    if (!px || !qx) { free(px); free(qx); return 0.0; }

    /* Initialize uniform distribution */
    for (int i = 0; i < nx; i++) px[i] = 1.0 / nx;

    for (int iter = 0; iter < max_iter; iter++) {
        /* Compute q(x|y) = px(x) * P(y|x) / sum_x' px(x') * P(y|x') */
        for (int i = 0; i < nx; i++) {
            qx[i] = 0.0;
            for (int j = 0; j < ny; j++) {
                double sum_p = 0.0;
                for (int k = 0; k < nx; k++) {
                    sum_p += px[k] * ch->transition_matrix[k * ny + j];
                }
                if (sum_p > CP_EPS) {
                    qx[i] += ch->transition_matrix[i * ny + j] *
                             log2(ch->transition_matrix[i * ny + j] / sum_p);
                }
            }
            if (qx[i] < -100.0) qx[i] = -100.0;
            if (qx[i] > 100.0)  qx[i] = 100.0;
            qx[i] = exp(qx[i]);
        }
        double sum_q = 0.0;
        for (int i = 0; i < nx; i++) sum_q += qx[i];
        if (sum_q < CP_EPS) break;

        double max_diff = 0.0;
        for (int i = 0; i < nx; i++) {
            double new_px = qx[i] / sum_q;
            double diff = fabs(new_px - px[i]);
            if (diff > max_diff) max_diff = diff;
            px[i] = new_px;
        }
        if (max_diff < tol) break;
    }

    /* Compute capacity = I(X;Y) with current px */
    double capacity = 0.0;
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            double pxy = px[i] * ch->transition_matrix[i * ny + j];
            if (pxy < CP_EPS) continue;
            double py = 0.0;
            for (int k = 0; k < nx; k++) {
                py += px[k] * ch->transition_matrix[k * ny + j];
            }
            if (py > CP_EPS) {
                capacity += pxy * log2(pxy / (px[i] * py));
            }
        }
    }
    ch->capacity = capacity;
    free(px); free(qx);
    return capacity;
}

double cp_channel_equivocation(const ControlChannel* ch) {
    if (!ch || ch->input_alphabet_size == 0) return 0.0;
    /* H(X|Y) assuming uniform input distribution */
    int nx = ch->input_alphabet_size;
    int ny = ch->output_alphabet_size;
    double uniform = 1.0 / nx;
    double h_cond = 0.0;
    for (int j = 0; j < ny; j++) {
        double py = 0.0;
        for (int i = 0; i < nx; i++) {
            py += uniform * ch->transition_matrix[i * ny + j];
        }
        double hy_given_y = 0.0;
        for (int i = 0; i < nx; i++) {
            double p_xy = uniform * ch->transition_matrix[i * ny + j];
            double p_x_given_y = (py > CP_EPS) ? p_xy / py : 0.0;
            if (p_x_given_y > CP_EPS)
                hy_given_y -= p_x_given_y * log2(p_x_given_y);
        }
        h_cond += py * hy_given_y;
    }
    return h_cond;
}

double cp_channel_error_probability(const ControlChannel* ch) {
    if (!ch) return 0.0;
    int nx = ch->input_alphabet_size;
    int ny = ch->output_alphabet_size;
    double err = 0.0;
    for (int i = 0; i < nx; i++) {
        double row_sum = 0.0;
        for (int j = 0; j < ny; j++) {
            row_sum += ch->transition_matrix[i * ny + j];
            if (i != j) err += ch->transition_matrix[i * ny + j];
        }
        if (row_sum > 1.0 + CP_EPS || row_sum < 1.0 - CP_EPS) {
            /* not a proper stochastic matrix ? penalize */
        }
    }
    return err / nx;
}

/* ============================================================================
 * Disturbance Ensemble
 * ============================================================================ */

DisturbanceEnsemble* cp_disturbance_ensemble_create(int n) {
    DisturbanceEnsemble* de = (DisturbanceEnsemble*)calloc(1, sizeof(DisturbanceEnsemble));
    if (!de) return NULL;
    de->n_disturbances = n;
    de->probabilities = (double*)calloc((size_t)n, sizeof(double));
    return de;
}

void cp_disturbance_ensemble_free(DisturbanceEnsemble* de) {
    if (!de) return;
    free(de->probabilities);
    free(de);
}

void cp_disturbance_ensemble_set_probability(DisturbanceEnsemble* de, int idx, double p) {
    if (!de || idx < 0 || idx >= de->n_disturbances) return;
    de->probabilities[idx] = p;
}

void cp_disturbance_ensemble_normalize(DisturbanceEnsemble* de) {
    if (!de) return;
    double sum = 0.0;
    for (int i = 0; i < de->n_disturbances; i++) sum += de->probabilities[i];
    if (sum < CP_EPS) {
        double uniform = 1.0 / de->n_disturbances;
        for (int i = 0; i < de->n_disturbances; i++) de->probabilities[i] = uniform;
        sum = 1.0;
    }
    for (int i = 0; i < de->n_disturbances; i++) de->probabilities[i] /= sum;
    de->entropy = cp_entropy(de->probabilities, de->n_disturbances, true);
    de->variety = de->entropy;
}

double cp_disturbance_ensemble_variety(const DisturbanceEnsemble* de) {
    if (!de) return 0.0;
    return de->variety;
}

/* ============================================================================
 * Ashby Regulator
 * ============================================================================ */

AshbyRegulator* cp_ashby_regulator_create(int n_disturbances, int n_responses, int n_outcomes) {
    AshbyRegulator* reg = (AshbyRegulator*)calloc(1, sizeof(AshbyRegulator));
    if (!reg) return NULL;
    reg->n_disturbance_types = n_disturbances;
    reg->n_response_types = n_responses;
    reg->n_outcome_types = n_outcomes;
    reg->strategy_table = (int*)malloc((size_t)n_disturbances * sizeof(int));
    if (reg->strategy_table) {
        for (int i = 0; i < n_disturbances; i++) reg->strategy_table[i] = 0;
    }
    return reg;
}

void cp_ashby_regulator_free(AshbyRegulator* reg) {
    if (!reg) return;
    free(reg->strategy_table);
    free(reg);
}

void cp_ashby_regulator_set_strategy(AshbyRegulator* reg, int disturbance, int response) {
    if (!reg || disturbance < 0 || disturbance >= reg->n_disturbance_types ||
        response < 0 || response >= reg->n_response_types) return;
    reg->strategy_table[disturbance] = response;
}

void cp_ashby_regulator_evaluate(AshbyRegulator* reg, const DisturbanceEnsemble* de) {
    if (!reg || !de) return;
    /* Count distinct responses used */
    int* used = (int*)calloc((size_t)reg->n_response_types, sizeof(int));
    if (!used) return;
    for (int d = 0; d < reg->n_disturbance_types; d++) {
        int r = reg->strategy_table[d];
        if (r >= 0 && r < reg->n_response_types) used[r] = 1;
    }
    double distinct_responses = 0.0;
    for (int r = 0; r < reg->n_response_types; r++) distinct_responses += used[r];
    free(used);

    /* Weight by disturbance probability */
    double v_r = log2(distinct_responses > 1.0 ? distinct_responses : 2.0);
    double v_d = (de->variety > CP_EPS) ? de->variety : log2((double)de->n_disturbances);

    reg->variety_disturbance = v_d;
    reg->variety_response = v_r;
    reg->requisite_ratio = (v_d > CP_EPS) ? v_r / v_d : 1e6;
    reg->satisfies_ashby_law = (v_r >= v_d);
}

double cp_ashby_regulator_outcome_variety(const AshbyRegulator* reg,
                                           const DisturbanceEnsemble* de) {
    if (!reg || !de) return 0.0;
    /* Count distinct (disturbance, response) pairs */
    int n_d = reg->n_disturbance_types;
    int n_r = reg->n_response_types;
    int n_outcomes = n_d * n_r;
    if (n_outcomes > 100000) n_outcomes = 100000;
    int* outcome_seen = (int*)calloc((size_t)n_outcomes, sizeof(int));
    if (!outcome_seen) return 0.0;
    for (int d = 0; d < n_d; d++) {
        int r = reg->strategy_table[d];
        int outcome_idx = d * n_r + r;
        if (outcome_idx < n_outcomes) outcome_seen[outcome_idx] = 1;
    }
    double distinct = 0.0;
    for (int i = 0; i < n_outcomes; i++) distinct += outcome_seen[i];
    free(outcome_seen);
    double vo = log2(distinct > 1.0 ? distinct : 2.0);
    return vo;
}

void cp_ashby_regulator_print(const AshbyRegulator* reg) {
    if (!reg) { printf("AshbyRegulator: NULL\n"); return; }
    printf("=== Ashby Regulator ===\n");
    printf("Disturbances: %d, Responses: %d, Outcomes: %d\n",
           reg->n_disturbance_types, reg->n_response_types, reg->n_outcome_types);
    printf("Strategy Table:\n");
    for (int d = 0; d < reg->n_disturbance_types; d++) {
        printf("  D%d ? R%d\n", d, reg->strategy_table[d]);
    }
    printf("V(D)=%.4f, V(R)=%.4f, V(R)/V(D)=%.4f\n",
           reg->variety_disturbance, reg->variety_response, reg->requisite_ratio);
    printf("Ashby Law satisfied: %s\n", reg->satisfies_ashby_law ? "YES" : "NO");
}

/* ============================================================================
 * Transfer Entropy
 * ============================================================================ */

double cp_transfer_entropy_discrete(const int* source, const int* target,
                                     int n, int lag, int alphabet_size) {
    if (!source || !target || n <= lag || alphabet_size < 2) return 0.0;

    /* Build histograms for P(target_future, target_past, source_past) */
    int total_bins = alphabet_size * alphabet_size * alphabet_size;
    double* joint = (double*)calloc((size_t)total_bins, sizeof(double));
    double* cond = (double*)calloc((size_t)(alphabet_size * alphabet_size), sizeof(double));
    if (!joint || !cond) { free(joint); free(cond); return 0.0; }

    int count = 0;
    for (int t = 0; t < n - lag; t++) {
        int tf = target[t + lag];
        int tp = target[t];
        int sp = source[t];
        if (tf < 0 || tf >= alphabet_size) continue;
        if (tp < 0 || tp >= alphabet_size) continue;
        if (sp < 0 || sp >= alphabet_size) continue;

        int idx = tf * alphabet_size * alphabet_size + tp * alphabet_size + sp;
        joint[idx] += 1.0;
        cond[tp * alphabet_size + sp] += 1.0;
        count++;
    }

    if (count == 0) { free(joint); free(cond); return 0.0; }

    /* Normalize */
    for (int i = 0; i < total_bins; i++) joint[i] /= count;
    for (int i = 0; i < alphabet_size * alphabet_size; i++) cond[i] /= count;

    /* TE = sum P(tf, tp, sp) * log( P(tf|tp, sp) / P(tf|tp) ) */
    double te = 0.0;
    for (int tf = 0; tf < alphabet_size; tf++) {
        for (int tp = 0; tp < alphabet_size; tp++) {
            for (int sp = 0; sp < alphabet_size; sp++) {
                int idx = tf * alphabet_size * alphabet_size + tp * alphabet_size + sp;
                double p_joint = joint[idx];
                if (p_joint < CP_EPS) continue;

                /* P(tf, tp) */
                double p_tf_tp = 0.0;
                for (int sp2 = 0; sp2 < alphabet_size; sp2++) {
                    int idx2 = tf * alphabet_size * alphabet_size + tp * alphabet_size + sp2;
                    p_tf_tp += joint[idx2];
                }

                /* P(tp) */
                double p_tp = 0.0;
                for (int tf2 = 0; tf2 < alphabet_size; tf2++) {
                    int idx2 = tf2 * alphabet_size * alphabet_size + tp * alphabet_size + sp;
                    p_tp += joint[idx2];
                }

                /* P(tf|tp, sp) */
                double p_cond_cond = cond[tp * alphabet_size + sp];
                double p_tf_given_tp_sp = (p_cond_cond > CP_EPS)
                    ? p_joint / p_cond_cond : 0.0;

                /* P(tf|tp) */
                double p_tf_given_tp = (p_tp > CP_EPS)
                    ? p_tf_tp / p_tp : 0.0;

                if (p_tf_given_tp_sp > CP_EPS && p_tf_given_tp > CP_EPS) {
                    te += p_joint * log2(p_tf_given_tp_sp / p_tf_given_tp);
                }
            }
        }
    }

    free(joint); free(cond);
    return te;
}

double cp_information_bottleneck(double i_xu, double i_uy, double beta) {
    return i_xu - beta * i_uy;
}
