#include "variety.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ===== VarietySpace ===== */
VarietySpace* variety_space_create(int capacity, double epsilon) {
    VarietySpace* vs = (VarietySpace*)calloc(1, sizeof(VarietySpace));
    vs->capacity = capacity;
    vs->epsilon = epsilon > 0.0 ? epsilon : 0.01;
    vs->state_ids = (int*)malloc(capacity * sizeof(int));
    vs->state_probs = (double*)malloc(capacity * sizeof(double));
    vs->n_states = 0;
    vs->total_variety = 0.0; vs->effective_variety = 0.0;
    return vs;
}

void variety_space_free(VarietySpace* vs) {
    if (!vs) return;
    free(vs->state_ids); free(vs->state_probs); free(vs);
}

void variety_space_add_state(VarietySpace* vs, int state_id, double prob) {
    if (!vs || vs->n_states >= vs->capacity) return;
    vs->state_ids[vs->n_states] = state_id;
    vs->state_probs[vs->n_states] = prob;
    vs->n_states++;
}

void variety_space_set_uniform(VarietySpace* vs, int n_states) {
    if (!vs || n_states > vs->capacity) return;
    vs->n_states = n_states;
    double p = 1.0 / n_states;
    for (int i = 0; i < n_states; i++) {
        vs->state_ids[i] = i;
        vs->state_probs[i] = p;
    }
}

void variety_space_compute(VarietySpace* vs) {
    if (!vs) return;
    double sum_p = 0.0;
    for (int i = 0; i < vs->n_states; i++) sum_p += vs->state_probs[i];
    if (sum_p < 1e-15) { vs->total_variety = 0.0; vs->effective_variety = 0.0; return; }
    /* Normalize */
    double* p = (double*)malloc(vs->n_states * sizeof(double));
    for (int i = 0; i < vs->n_states; i++) p[i] = vs->state_probs[i] / sum_p;
    /* Shannon entropy = variety in bits */
    double H = 0.0; int effective_n = 0;
    for (int i = 0; i < vs->n_states; i++) {
        if (p[i] > 1e-15) H -= p[i] * log2(p[i]);
        if (p[i] > vs->epsilon) effective_n++;
    }
    vs->total_variety = H;
    vs->effective_variety = log2((double)effective_n + 1e-10);
    free(p);
}

double variety_space_entropy(const VarietySpace* vs) { return vs ? vs->total_variety : 0.0; }
double variety_space_effective_n(const VarietySpace* vs) { return vs ? vs->effective_variety : 0.0; }

double variety_space_redundancy(const VarietySpace* vs) {
    if (!vs || vs->n_states <= 1) return 0.0;
    double max_H = log2((double)vs->n_states);
    if (max_H < 1e-10) return 0.0;
    return 1.0 - vs->total_variety / max_H;
}

void variety_space_merge(VarietySpace* dest, const VarietySpace* src) {
    if (!dest || !src) return;
    for (int i = 0; i < src->n_states && dest->n_states < dest->capacity; i++)
        variety_space_add_state(dest, src->state_ids[i], src->state_probs[i]);
}

void variety_space_print(const VarietySpace* vs) {
    if (!vs) return;
    printf("VarietySpace: %d states, V=%.4f bits, V_eff=%.4f bits, redundancy=%.2f%%\n",
           vs->n_states, vs->total_variety, vs->effective_variety,
           variety_space_redundancy(vs) * 100.0);
}

/* ===== Variety Estimation ===== */
double variety_estimate_from_samples(const double* samples, int n, double bin_width) {
    if (!samples || n < 2 || bin_width <= 0.0) return 0.0;
    double xmin = samples[0], xmax = samples[0];
    for (int i = 0; i < n; i++) {
        if (samples[i] < xmin) xmin = samples[i];
        if (samples[i] > xmax) xmax = samples[i];
    }
    double range = xmax - xmin;
    if (range < 1e-10) return 0.0;
    int n_bins = (int)(range / bin_width) + 1;
    if (n_bins > 1000) n_bins = 1000;
    int* counts = (int*)calloc(n_bins, sizeof(int));
    for (int i = 0; i < n; i++) {
        int b = (int)((samples[i] - xmin) / bin_width);
        if (b < 0) b = 0;
        if (b >= n_bins) b = n_bins - 1;
        counts[b]++;
    }
    double H = 0.0;
    for (int b = 0; b < n_bins; b++) {
        double p = (double)counts[b] / n;
        if (p > 1e-15) H -= p * log2(p);
    }
    free(counts);
    return H;
}

double variety_estimate_from_histogram(const int* counts, int n_bins, int total) {
    if (!counts || n_bins <= 0 || total <= 0) return 0.0;
    double H = 0.0;
    for (int i = 0; i < n_bins; i++) {
        double p = (double)counts[i] / total;
        if (p > 1e-15) H -= p * log2(p);
    }
    return H;
}

double variety_estimate_adaptive(const double* samples, int n, int max_bins) {
    if (!samples || n < 2) return 0.0;
    double* sorted = (double*)malloc(n * sizeof(double));
    memcpy(sorted, samples, n * sizeof(double));
    /* Simple sort */
    for (int i = 0; i < n - 1; i++)
        for (int j = i + 1; j < n; j++)
            if (sorted[i] > sorted[j]) { double t = sorted[i]; sorted[i] = sorted[j]; sorted[j] = t; }
    double range = sorted[n-1] - sorted[0];
    if (range < 1e-10) { free(sorted); return 0.0; }
    int bins = (n < max_bins) ? n : max_bins;
    double bw = range / bins;
    int* counts = (int*)calloc(bins, sizeof(int));
    for (int i = 0; i < n; i++) {
        int b = (int)((sorted[i] - sorted[0]) / bw);
        if (b < 0) b = 0;
        if (b >= bins) b = bins - 1;
        counts[b]++;
    }
    double H = variety_estimate_from_histogram(counts, bins, n);
    free(sorted); free(counts);
    return H;
}

double variety_log_dimension(const double* samples, int n, int embedding_dim, double radius) {
    if (!samples || n < 2 || radius <= 0.0) return 0.0;
    /* Correlation dimension estimator: count pairs within radius */
    int count = 0; int total_pairs = n * (n - 1) / 2;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double dist_sq = 0.0;
            for (int d = 0; d < embedding_dim && i + d < n && j + d < n; d++) {
                double diff = samples[i + d] - samples[j + d];
                dist_sq += diff * diff;
            }
            if (sqrt(dist_sq) < radius) count++;
        }
    }
    double corr_sum = (double)count / total_pairs;
    if (corr_sum < 1e-15) return 0.0;
    return log2(corr_sum) / log2(radius + 1e-10);
}

/* ===== VarietyChannel ===== */
VarietyChannel* variety_channel_create(void) {
    VarietyChannel* vc = (VarietyChannel*)calloc(1, sizeof(VarietyChannel));
    vc->disturbance = NULL; vc->regulation = NULL; vc->essential_var = NULL;
    vc->channel_capacity = 1.0; vc->variety_blocked = 0.0;
    vc->regulatory_efficiency = 0.0; vc->satisfies_requisite = false;
    return vc;
}

void variety_channel_free(VarietyChannel* vc) {
    if (!vc) return;
    if (vc->disturbance) variety_space_free(vc->disturbance);
    if (vc->regulation) variety_space_free(vc->regulation);
    if (vc->essential_var) variety_space_free(vc->essential_var);
    free(vc);
}

void variety_channel_set_disturbance(VarietyChannel* vc, const VarietySpace* vs) {
    if (!vc || !vs) return;
    if (vc->disturbance) variety_space_free(vc->disturbance);
    vc->disturbance = variety_space_create(vs->capacity, vs->epsilon);
    for (int i = 0; i < vs->n_states; i++)
        variety_space_add_state(vc->disturbance, vs->state_ids[i], vs->state_probs[i]);
    variety_space_compute(vc->disturbance);
}

void variety_channel_set_regulation(VarietyChannel* vc, const VarietySpace* vs) {
    if (!vc || !vs) return;
    if (vc->regulation) variety_space_free(vc->regulation);
    vc->regulation = variety_space_create(vs->capacity, vs->epsilon);
    for (int i = 0; i < vs->n_states; i++)
        variety_space_add_state(vc->regulation, vs->state_ids[i], vs->state_probs[i]);
    variety_space_compute(vc->regulation);
}

void variety_channel_set_essential(VarietyChannel* vc, const VarietySpace* vs) {
    if (!vc || !vs) return;
    if (vc->essential_var) variety_space_free(vc->essential_var);
    vc->essential_var = variety_space_create(vs->capacity, vs->epsilon);
    for (int i = 0; i < vs->n_states; i++)
        variety_space_add_state(vc->essential_var, vs->state_ids[i], vs->state_probs[i]);
    variety_space_compute(vc->essential_var);
}

void variety_channel_evaluate(VarietyChannel* vc) {
    if (!vc || !vc->disturbance || !vc->regulation || !vc->essential_var) return;
    double Vd = vc->disturbance->total_variety;
    double Vr = vc->regulation->total_variety;
    double Ve = vc->essential_var->total_variety;
    vc->variety_blocked = Vd - Ve;
    if (vc->variety_blocked < 0.0) vc->variety_blocked = 0.0;
    vc->regulatory_efficiency = (Vd > 0.0) ? vc->variety_blocked / Vd : 0.0;
    vc->satisfies_requisite = (Vr >= Vd - Ve) || (Vr + Ve >= Vd);
    vc->channel_capacity = Vr;
}

bool variety_channel_check_requisite(const VarietyChannel* vc) {
    return vc ? vc->satisfies_requisite : false;
}

double variety_channel_excess(const VarietyChannel* vc) {
    if (!vc || !vc->regulation || !vc->disturbance) return 0.0;
    return vc->regulation->total_variety - vc->disturbance->total_variety;
}

void variety_channel_print(const VarietyChannel* vc) {
    if (!vc) return;
    printf("=== Variety Channel ===\n");
    printf("V(D)=%.4f, V(R)=%.4f, V(E)=%.4f\n",
           vc->disturbance ? vc->disturbance->total_variety : 0.0,
           vc->regulation ? vc->regulation->total_variety : 0.0,
           vc->essential_var ? vc->essential_var->total_variety : 0.0);
    printf("Blocked: %.4f, Efficiency: %.2f%%, Requisite satisfied: %s\n",
           vc->variety_blocked, vc->regulatory_efficiency * 100.0,
           vc->satisfies_requisite ? "YES" : "NO");
}

/* ===== Ashby's Law Computations ===== */
double variety_requisite_minimum(double V_disturbance, double V_essential_target) {
    double needed = V_disturbance - V_essential_target;
    return needed > 0.0 ? needed : 0.0;
}

double variety_regulation_effectiveness(double V_regulator, double V_disturbance) {
    if (V_disturbance < 1e-10) return 1.0;
    double eff = V_regulator / V_disturbance;
    return eff > 1.0 ? 1.0 : eff;
}

double variety_surplus(const VarietyChannel* vc) {
    if (!vc) return 0.0;
    return variety_channel_excess(vc);
}

double variety_amplification_needed(const VarietyChannel* vc) {
    if (!vc || !vc->regulation || !vc->disturbance) return 0.0;
    double needed = vc->disturbance->total_variety;
    double have = vc->regulation->total_variety;
    if (have < 1e-10) return needed;
    return needed / have;
}

/* ===== Information-Theoretic Connections ===== */
double variety_to_channel_capacity(double variety, double bandwidth, double snr_db) {
    double snr_linear = pow(10.0, snr_db / 10.0);
    double shannon = bandwidth * log2(1.0 + snr_linear);
    return (variety < shannon) ? variety : shannon;
}

double variety_conditional_entropy(const VarietySpace* j, const double* conditional_probs, int n_conditions) {
    if (!j) return 0.0;
    double H_cond = 0.0;
    for (int c = 0; c < n_conditions; c++) {
        double p_cond = conditional_probs[c];
        if (p_cond < 1e-15) continue;
        for (int i = 0; i < j->n_states; i++) {
            double p_ji = j->state_probs[i] * p_cond;
            if (p_ji > 1e-15) H_cond -= p_ji * log2(p_ji);
        }
    }
    return H_cond;
}

double variety_mutual_information_from_channel(const VarietyChannel* vc) {
    if (!vc || !vc->disturbance || !vc->essential_var) return 0.0;
    return vc->disturbance->total_variety - vc->essential_var->total_variety;
}

double variety_rate_distortion(double input_variety, double* distortion_matrix, int n_input, int n_output) {
    /* Simplified: variety loss due to distortion */
    double avg_distortion = 0.0;
    int count = 0;
    for (int i = 0; i < n_input; i++)
        for (int j = 0; j < n_output; j++) {
            avg_distortion += distortion_matrix[i * n_output + j];
            count++;
        }
    avg_distortion /= (count + 1);
    return input_variety * (1.0 - avg_distortion / (1.0 + avg_distortion));
}

/* ===== Variety Engineering ===== */
double variety_attenuation_by_filtering(double input_variety, double filter_selectivity) {
    double s = (filter_selectivity > 1.0) ? 1.0 : (filter_selectivity < 0.0 ? 0.0 : filter_selectivity);
    return input_variety * (1.0 - s * 0.5);
}

double variety_attenuation_by_buffering(double input_variety, double buffer_capacity) {
    return input_variety / (1.0 + buffer_capacity * 0.1);
}

double variety_attenuation_by_prediction(double input_variety, double prediction_accuracy) {
    double acc = (prediction_accuracy > 1.0) ? 1.0 : (prediction_accuracy < 0.0 ? 0.0 : prediction_accuracy);
    return input_variety * (1.0 - acc * 0.8);
}

double variety_amplification_required(double target_variety, double current_variety) {
    if (current_variety < 1e-10) return target_variety;
    double ratio = target_variety / current_variety;
    return ratio > 1.0 ? ratio : 1.0;
}

/* ===== VarietyPipeline ===== */
VarietyPipeline* variety_pipeline_create(int n_stages) {
    VarietyPipeline* vp = (VarietyPipeline*)calloc(1, sizeof(VarietyPipeline));
    vp->n_stages = n_stages;
    vp->stages = (VarietyChannel**)calloc(n_stages, sizeof(VarietyChannel*));
    vp->stage_attenuation = (double*)calloc(n_stages, sizeof(double));
    vp->total_attenuation = 0.0; vp->bottleneck_variety = 1e100; vp->bottleneck_stage = -1;
    return vp;
}

void variety_pipeline_free(VarietyPipeline* vp) {
    if (!vp) return;
    for (int i = 0; i < vp->n_stages; i++)
        if (vp->stages[i]) variety_channel_free(vp->stages[i]);
    free(vp->stages); free(vp->stage_attenuation); free(vp);
}

void variety_pipeline_set_stage(VarietyPipeline* vp, int stage_idx, VarietyChannel* vc) {
    if (!vp || stage_idx < 0 || stage_idx >= vp->n_stages) return;
    vp->stages[stage_idx] = vc;
}

void variety_pipeline_analyze(VarietyPipeline* vp) {
    if (!vp) return;
    vp->total_attenuation = 0.0;
    vp->bottleneck_variety = 1e100; vp->bottleneck_stage = -1;
    for (int i = 0; i < vp->n_stages; i++) {
        if (!vp->stages[i]) continue;
        variety_channel_evaluate(vp->stages[i]);
        vp->stage_attenuation[i] = vp->stages[i]->variety_blocked;
        vp->total_attenuation += vp->stages[i]->variety_blocked;
        if (vp->stages[i]->regulation && vp->stages[i]->regulation->total_variety < vp->bottleneck_variety) {
            vp->bottleneck_variety = vp->stages[i]->regulation->total_variety;
            vp->bottleneck_stage = i;
        }
    }
}

double variety_pipeline_residual(const VarietyPipeline* vp) {
    if (!vp || vp->n_stages == 0) return 0.0;
    if (!vp->stages[0] || !vp->stages[0]->disturbance) return 0.0;
    double initial = vp->stages[0]->disturbance->total_variety;
    return initial - vp->total_attenuation;
}

bool variety_pipeline_feasible(const VarietyPipeline* vp) {
    if (!vp) return false;
    for (int i = 0; i < vp->n_stages; i++)
        if (vp->stages[i] && !vp->stages[i]->satisfies_requisite) return false;
    return true;
}

int variety_pipeline_find_bottleneck(const VarietyPipeline* vp) {
    return vp ? vp->bottleneck_stage : -1;
}

void variety_pipeline_print(const VarietyPipeline* vp) {
    if (!vp) return;
    printf("=== Variety Pipeline: %d stages ===\n", vp->n_stages);
    printf("Total attenuation: %.4f, Residual: %.4f, Feasible: %s\n",
           vp->total_attenuation, variety_pipeline_residual(vp),
           variety_pipeline_feasible(vp) ? "YES" : "NO");
    printf("Bottleneck at stage %d (V=%.4f)\n", vp->bottleneck_stage, vp->bottleneck_variety);
}