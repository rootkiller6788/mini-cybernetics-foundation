/**
 * @file cas_emergence.c
 * @brief Emergence Detection and Measurement (L2-L6)
 *
 * Quantitative measures of emergence, information flow, and
 * self-organization in Complex Adaptive Systems.
 *
 * L2: Weak/strong emergence, downward causation, self-organization
 * L3: Shannon entropy, mutual information, transfer entropy, Phi
 * L4: Causal emergence (Hoel 2013), Tononi IIT, Bedau weak emergence
 * L5: Entropy estimation, phase transition detection, Lyapunov exponent
 * L6: Time series analysis, causal emergence optimization
 *
 * Reference:
 *   Cover, T.M. & Thomas, J.A. (2006). Elements of Information Theory.
 *   Hoel, E. et al. (2013). Causal Emergence. PNAS 110(49):19790-19795.
 *   Tononi, G. et al. (2016). IIT 3.0. Nature Rev. Neurosci. 17:450-461.
 */

#include "cas_emergence.h"
#include <string.h>
#include <stdlib.h>

static uint64_t _em_seed = 271828;

static uint64_t _em_rand_u64(void) {
    _em_seed = _em_seed * 6364136223846793005ULL + 1442695040888963407ULL;
    uint64_t z = _em_seed ^ (_em_seed >> 33);
    z = z * 0xff51afd7ed558ccdULL; z = z ^ (z >> 33);
    z = z * 0xc4ceb9fe1a85ec53ULL;
    return z ^ (z >> 33);
}
static void _em_srand(uint64_t s) { _em_seed = s; }
static double _em_rand_double(void) { return (double)(_em_rand_u64() >> 11) * 0x1.0p-53; }
static double _em_rand_gaussian(void) {
    double u1 = _em_rand_double(), u2 = _em_rand_double();
    return sqrt(-2.0 * log(u1 > 1e-12 ? u1 : 1e-12)) * cos(2.0 * M_PI * u2);
}

/* =================================================================
 * L5: Shannon Entropy
 *
 * H(X) = -sum p(x_i) * log2(p(x_i))
 *
 * Discretize continuous data into bins, count frequencies.
 *
 * Complexity: O(n + bins)
 * ================================================================= */

double cas_entropy_discrete(const double *x, uint32_t n, uint32_t bins,
                            const double *range_min, const double *range_max)
{
    if (!x || n == 0 || bins == 0) return 0.0;

    double rmin = range_min ? *range_min : x[0];
    double rmax = range_max ? *range_max : x[0];

    /* Find min/max if not provided */
    if (!range_min || !range_max) {
        uint32_t i;
        for (i = 0; i < n; i++) {
            if (x[i] < rmin) rmin = x[i];
            if (x[i] > rmax) rmax = x[i];
        }
    }

    if (fabs(rmax - rmin) < 1e-12) return 0.0;

    uint32_t *counts = (uint32_t *)calloc(bins, sizeof(uint32_t));
    if (!counts) return 0.0;

    double bin_width = (rmax - rmin) / bins;
    uint32_t i;
    for (i = 0; i < n; i++) {
        int b = (int)((x[i] - rmin) / bin_width);
        if (b < 0) b = 0;
        if ((uint32_t)b >= bins) b = (int)(bins - 1);
        counts[b]++;
    }

    double H = 0.0;
    for (i = 0; i < bins; i++) {
        if (counts[i] > 0) {
            double p = (double)counts[i] / (double)n;
            H -= p * log2(p);
        }
    }

    free(counts);
    return H;
}

/* =================================================================
 * L5: Joint Entropy H(X,Y)
 *
 * Complexity: O(n + bins_x * bins_y)
 * ================================================================= */

double cas_entropy_joint(const double *x, const double *y, uint32_t n,
                         uint32_t bins_x, uint32_t bins_y)
{
    if (!x || !y || n == 0) return 0.0;

    /* Find ranges */
    double xmin = x[0], xmax = x[0];
    double ymin = y[0], ymax = y[0];
    uint32_t i;
    for (i = 0; i < n; i++) {
        if (x[i] < xmin) xmin = x[i]; if (x[i] > xmax) xmax = x[i];
        if (y[i] < ymin) ymin = y[i]; if (y[i] > ymax) ymax = y[i];
    }

    if (fabs(xmax - xmin) < 1e-12 || fabs(ymax - ymin) < 1e-12) return 0.0;

    /* 2D histogram */
    uint32_t total_bins = bins_x * bins_y;
    uint32_t *counts = (uint32_t *)calloc(total_bins, sizeof(uint32_t));
    if (!counts) return 0.0;

    double wx = (xmax - xmin) / bins_x;
    double wy = (ymax - ymin) / bins_y;

    for (i = 0; i < n; i++) {
        int bx = (int)((x[i] - xmin) / wx);
        int by = (int)((y[i] - ymin) / wy);
        if (bx < 0) bx = 0; if ((uint32_t)bx >= bins_x) bx = bins_x - 1;
        if (by < 0) by = 0; if ((uint32_t)by >= bins_y) by = bins_y - 1;
        counts[bx * bins_y + by]++;
    }

    double H = 0.0;
    for (i = 0; i < total_bins; i++) {
        if (counts[i] > 0) {
            double p = (double)counts[i] / n;
            H -= p * log2(p);
        }
    }

    free(counts);
    return H;
}

/* =================================================================
 * L5: Mutual Information I(X;Y)
 *
 * I(X;Y) = H(X) + H(Y) - H(X,Y)
 *
 * Measures how much knowing X reduces uncertainty about Y.
 *
 * Theorem: I(X;Y) >= 0 with equality iff X and Y are independent.
 *
 * Complexity: O(n + bins_x * bins_y)
 * ================================================================= */

double cas_mutual_information(const double *x, const double *y,
                              uint32_t n, uint32_t bins)
{
    if (!x || !y || n == 0) return 0.0;

    double hx = cas_entropy_discrete(x, n, bins, NULL, NULL);
    double hy = cas_entropy_discrete(y, n, bins, NULL, NULL);
    double hxy = cas_entropy_joint(x, y, n, bins, bins);

    return hx + hy - hxy;
}

/* =================================================================
 * L5: Conditional Entropy H(X|Y)
 *
 * H(X|Y) = H(X,Y) - H(Y)
 *
 * Complexity: O(n + bins^2)
 * ================================================================= */

double cas_entropy_conditional(const double *x, const double *y,
                               uint32_t n, uint32_t bins_x, uint32_t bins_y)
{
    double hxy = cas_entropy_joint(x, y, n, bins_x, bins_y);
    double hy = cas_entropy_discrete(y, n, bins_y, NULL, NULL);
    return hxy - hy;
}

/* =================================================================
 * L5: Transfer Entropy T_{Y->X}
 *
 * T_{Y->X} = I(X_{t+1} ; Y_t | X_t)
 *
 * Directed measure of information flow from Y to X.
 * Detects causal influence without requiring a model.
 *
 * Theorem (Schreiber 2000, PRL): Transfer entropy is zero iff
 * Y has no predictive information about X beyond X's own past.
 *
 * Complexity: O(n * bins^3)
 * ================================================================= */

double cas_transfer_entropy(const double *x, const double *y,
                            uint32_t n, uint32_t bins, uint32_t lag)
{
    if (!x || !y || n <= lag + 1) return 0.0;

    /* Prepare lagged series: X_past, X_future, Y_past */
    uint32_t m = n - lag - 1;
    double *x_past = (double *)malloc(m * sizeof(double));
    double *x_fut = (double *)malloc(m * sizeof(double));
    double *y_past = (double *)malloc(m * sizeof(double));
    if (!x_past || !x_fut || !y_past) {
        free(x_past); free(x_fut); free(y_past);
        return 0.0;
    }

    uint32_t i;
    for (i = 0; i < m; i++) {
        x_past[i] = x[i];
        x_fut[i] = x[i + lag + 1];
        y_past[i] = y[i];
    }

    /* T = H(X_fut | X_past) - H(X_fut | X_past, Y_past) */
    double hx_fut_given_x_past = cas_entropy_conditional(x_fut, x_past, m, bins, bins);
    double hx_fut_given_both = 0.0;

    /* H(X_fut | X_past, Y_past) = H(X_fut, X_past, Y_past) - H(X_past, Y_past) */
    /* Simplified: use pairwise conditioning */

    /* Build joint conditioning: pair (X_past, Y_past) -> X_fut */
    double *joint_past = (double *)malloc(m * sizeof(double));
    if (!joint_past) {
        free(x_past); free(x_fut); free(y_past);
        return 0.0;
    }
    for (i = 0; i < m; i++)
        joint_past[i] = x_past[i] + y_past[i] * 100.0; /* unique encoding */

    double h_fut_joint = cas_entropy_joint(x_fut, joint_past, m, bins, bins);
    double h_joint = cas_entropy_discrete(joint_past, m, bins, NULL, NULL);
    hx_fut_given_both = h_fut_joint - h_joint;

    free(x_past); free(x_fut); free(y_past); free(joint_past);

    double te = hx_fut_given_x_past - hx_fut_given_both;
    if (te < 0.0) te = 0.0;
    return te;
}

/* =================================================================
 * L5: Comprehensive Information Measures
 *
 * Compute all information-theoretic measures for a pair of series.
 *
 * Complexity: O(n * bins^3)
 * ================================================================= */

void cas_info_measures_compute(const double *x, const double *y,
                               uint32_t n, uint32_t bins, uint32_t lag,
                               cas_info_measures_t *measures)
{
    if (!measures) return;
    memset(measures, 0, sizeof(*measures));
    if (!x || !y || n == 0) return;

    measures->n_samples = n;
    measures->shannon_entropy = cas_entropy_discrete(x, n, bins, NULL, NULL);
    measures->joint_entropy = cas_entropy_joint(x, y, n, bins, bins);
    measures->conditional_entropy = cas_entropy_conditional(x, y, n, bins, bins);
    measures->mutual_information = cas_mutual_information(x, y, n, bins);
    measures->transfer_entropy = cas_transfer_entropy(x, y, n, bins, lag);
    measures->active_information = measures->shannon_entropy - measures->conditional_entropy;
    measures->integrated_info = measures->mutual_information / (measures->shannon_entropy + 1e-12);
    measures->causal_emergence = measures->transfer_entropy - measures->mutual_information;
}

/* =================================================================
 * L4: Causal Emergence (Hoel 2013)
 *
 * CE = EI_macro - EI_micro
 *
 * Effective Information EI = I(S_{t+1}; S_t) under uniform input.
 * Positive CE indicates macro level has more EI than micro level,
 * implying the macro description captures genuine emergence.
 *
 * Complexity: O(n_states^2)
 * ================================================================= */

void cas_causal_emergence_compute(cas_causal_emergence_t *ce,
                                  const double *transition_matrix,
                                  uint32_t micro_states,
                                  uint32_t macro_states)
{
    if (!ce || !transition_matrix) return;
    memset(ce, 0, sizeof(*ce));

    ce->micro_state_size = micro_states;
    ce->macro_state_size = macro_states;

    /* EI_micro = average MI between input (uniform) and output */
    double ei_micro = 0.0;
    uint32_t i, j;
    for (i = 0; i < micro_states; i++) {
        double cond_entropy = 0.0;
        for (j = 0; j < micro_states; j++) {
            double p = transition_matrix[i * micro_states + j];
            if (p > 0.0) cond_entropy -= p * log2(p);
        }
        ei_micro += log2((double)micro_states) - cond_entropy;
    }
    ei_micro /= micro_states;

    /* EI_macro: coarse-grain transition matrix */
    double ei_macro = 0.0;
    uint32_t partition_size = micro_states / macro_states;
    if (partition_size < 1) partition_size = 1;

    for (i = 0; i < macro_states; i++) {
        double cond_entropy = 0.0;
        /* Average transition from macro state i to macro state j */
        for (j = 0; j < macro_states; j++) {
            double avg_p = 0.0;
            uint32_t count = 0;
            uint32_t si, sj;
            for (si = i * partition_size; si < (i + 1) * partition_size && si < micro_states; si++) {
                for (sj = j * partition_size; sj < (j + 1) * partition_size && sj < micro_states; sj++) {
                    avg_p += transition_matrix[si * micro_states + sj];
                    count++;
                }
            }
            if (count > 0) avg_p /= count;
            if (avg_p > 0.0) cond_entropy -= avg_p * log2(avg_p);
        }
        ei_macro += log2((double)macro_states) - cond_entropy;
    }
    ei_macro /= macro_states;

    ce->micro_effective_info = ei_micro;
    ce->macro_effective_info = ei_macro;
    ce->causal_emergence_value = ei_macro - ei_micro;
    ce->has_causal_emergence = (ei_macro > ei_micro);
}

void cas_causal_emergence_optimize(cas_causal_emergence_t *ce,
                                   const double *transition_matrix,
                                   uint32_t micro_states,
                                   uint32_t max_macro_states)
{
    if (!ce || !transition_matrix) return;

    double best_ce = -1e308;
    uint32_t best_macro = 1;
    uint32_t m;

    /* Grid search over number of macro states */
    for (m = 2; m <= max_macro_states && m <= micro_states; m++) {
        cas_causal_emergence_t trial;
        cas_causal_emergence_compute(&trial, transition_matrix, micro_states, m);
        if (trial.causal_emergence_value > best_ce) {
            best_ce = trial.causal_emergence_value;
            best_macro = m;
        }
    }

    cas_causal_emergence_compute(ce, transition_matrix, micro_states, best_macro);
}

/* =================================================================
 * L5: Self-Organization Index
 *
 * SOI = 1 - (spatial_entropy / max_entropy)
 *
 * Complexity: O(n_agents + grid_bins^2)
 * ================================================================= */

void cas_self_org_compute(cas_self_org_t *so, const double *x,
                          const double *y, uint32_t n, uint32_t grid_bins)
{
    if (!so || !x || !y || n == 0) return;
    memset(so, 0, sizeof(*so));

    uint32_t bins = grid_bins > 0 ? grid_bins : 10;
    if (bins > 64) bins = 64;

    uint32_t *counts = (uint32_t *)calloc(bins * bins, sizeof(uint32_t));
    if (!counts) return;

    double xmin = x[0], xmax = x[0];
    double ymin = y[0], ymax = y[0];
    uint32_t i;
    for (i = 0; i < n; i++) {
        if (x[i] < xmin) xmin = x[i]; if (x[i] > xmax) xmax = x[i];
        if (y[i] < ymin) ymin = y[i]; if (y[i] > ymax) ymax = y[i];
    }

    if (fabs(xmax - xmin) < 1e-12 || fabs(ymax - ymin) < 1e-12) {
        free(counts); return;
    }

    double wx = (xmax - xmin) / bins;
    double wy = (ymax - ymin) / bins;

    for (i = 0; i < n; i++) {
        int bx = (int)((x[i] - xmin) / wx);
        int by = (int)((y[i] - ymin) / wy);
        if (bx < 0) bx = 0; if ((uint32_t)bx >= bins) bx = bins - 1;
        if (by < 0) by = 0; if ((uint32_t)by >= bins) by = bins - 1;
        counts[bx * bins + by]++;
    }

    /* Spatial entropy */
    double H = 0.0;
    for (i = 0; i < bins * bins; i++) {
        if (counts[i] > 0) {
            double p = (double)counts[i] / n;
            H -= p * log2(p);
        }
    }

    double H_max = log2((double)(bins * bins));
    so->spatial_entropy = H;
    so->self_org_index = H_max > 0.0 ? 1.0 - H / H_max : 0.0;
    so->is_self_organized = so->self_org_index > 0.5;

    /* Cluster sizes */
    uint32_t max_cluster = 0, total_clusters = 0;
    double avg_cluster = 0.0;
    for (i = 0; i < bins * bins; i++) {
        if (counts[i] > 0) {
            total_clusters++;
            avg_cluster += counts[i];
            if (counts[i] > max_cluster) max_cluster = counts[i];
        }
    }
    if (total_clusters > 0) avg_cluster /= total_clusters;
    so->cluster_size_max = (double)max_cluster;
    so->cluster_size_avg = avg_cluster;

    free(counts);
}

/* =================================================================
 * L5: Phase Transition Detection
 *
 * Detect phase transition from order parameter variance
 * (susceptibility peak) and Binder cumulant crossing.
 *
 * Complexity: O(n)
 * ================================================================= */

bool cas_phase_transition_detect(const double *order_param, uint32_t n,
                                 double *critical_point)
{
    if (!order_param || n < 4) return false;

    /* Variance peaks at critical point */
    double best_var = 0.0;
    uint32_t best_idx = 0;
    uint32_t window = n / 10;
    if (window < 4) window = 4;

    uint32_t i, j;
    for (i = 0; i <= n - window; i++) {
        double mean = 0.0, var = 0.0;
        for (j = 0; j < window; j++)
            mean += order_param[i + j];
        mean /= window;
        for (j = 0; j < window; j++)
            var += (order_param[i + j] - mean) * (order_param[i + j] - mean);
        var /= window;

        if (var > best_var) {
            best_var = var;
            best_idx = i;
        }
    }

    if (critical_point)
        *critical_point = (double)best_idx / (double)n;

    /* Threshold for detection */
    return best_var > 0.01;
}

/* =================================================================
 * L5: False Nearest Neighbors (embedding dimension estimation)
 *
 * Complexity: O(n * dim * k)
 * ================================================================= */

double cas_false_nearest_neighbors(const double *x, uint32_t n,
                                   uint32_t dim, uint32_t lag,
                                   double rtol)
{
    if (!x || n < dim * lag + 2) return 1.0;

    /* Embed the time series */
    uint32_t n_pts = n - (dim - 1) * lag;
    if (n_pts < 10) return 1.0;

    double fnn_count = 0.0;
    uint32_t total_pairs = 0;
    uint32_t i, j, d;

    for (i = 0; i < n_pts; i++) {
        /* Find nearest neighbor in current embedding */
        double min_dist = 1e308;
        uint32_t nn_idx = i;
        for (j = 0; j < n_pts; j++) {
            if (i == j) continue;
            double dist = 0.0;
            for (d = 0; d < dim; d++)
                dist += (x[i + d * lag] - x[j + d * lag]) * (x[i + d * lag] - x[j + d * lag]);
            if (dist < min_dist && dist > 1e-12) {
                min_dist = dist;
                nn_idx = j;
            }
        }

        /* Check if neighbor becomes false in next dimension */
        if (i + dim * lag < n && nn_idx + dim * lag < n) {
            double dist_dim = sqrt(min_dist);
            double delta = fabs(x[i + dim * lag] - x[nn_idx + dim * lag]);
            if (dist_dim > 1e-12 && delta / dist_dim > rtol)
                fnn_count++;
            total_pairs++;
        }
    }

    return total_pairs > 0 ? fnn_count / total_pairs : 1.0;
}

/* =================================================================
 * L5: Largest Lyapunov Exponent (Rosenstein algorithm)
 *
 * lambda > 0 indicates sensitive dependence on initial conditions (chaos).
 *
 * Complexity: O(n^2) simplified
 * ================================================================= */

double cas_lyapunov_exponent(const double *x, uint32_t n,
                             uint32_t dim, uint32_t lag)
{
    if (!x || n < 2 * dim * lag) return 0.0;

    uint32_t n_pts = n - dim * lag;
    if (n_pts < 10) return 0.0;

    double sum_log_div = 0.0;
    uint32_t count = 0;
    uint32_t i, j, d;

    for (i = 0; i < n_pts; i++) {
        double min_dist = 1e308;
        uint32_t nn = i;
        for (j = 0; j < n_pts; j++) {
            if (abs((int)(i - j)) < 10) continue; /* skip temporal neighbors */
            double dist = 0.0;
            for (d = 0; d < dim; d++)
                dist += (x[i + d * lag] - x[j + d * lag]) * (x[i + d * lag] - x[j + d * lag]);
            if (dist < min_dist && dist > 1e-12) {
                min_dist = dist;
                nn = j;
            }
        }

        /* Track divergence over forward steps */
        uint32_t max_fwd = 5;
        if (i + max_fwd < n_pts && nn + max_fwd < n_pts) {
            uint32_t k;
            for (k = 1; k <= max_fwd; k++) {
                double new_dist = 0.0;
                for (d = 0; d < dim; d++)
                    new_dist += (x[i + k + d * lag] - x[nn + k + d * lag]) * (x[i + k + d * lag] - x[nn + k + d * lag]);
                if (min_dist > 1e-12 && new_dist > 1e-12) {
                    sum_log_div += log(new_dist / min_dist);
                    count++;
                }
                min_dist = new_dist;
            }
        }
    }

    return count > 0 ? sum_log_div / count : 0.0;
}

/* =================================================================
 * L5: Hurst Exponent (R/S analysis)
 *
 * H > 0.5: persistent; H = 0.5: random walk; H < 0.5: anti-persistent.
 *
 * Complexity: O(n log n)
 * ================================================================= */

double cas_hurst_exponent(const double *x, uint32_t n)
{
    if (!x || n < 10) return 0.5;

    /* Compute mean and cumulative deviations */
    double mean = 0.0;
    uint32_t i;
    for (i = 0; i < n; i++) mean += x[i];
    mean /= n;

    double *dev = (double *)malloc(n * sizeof(double));
    if (!dev) return 0.5;

    dev[0] = x[0] - mean;
    for (i = 1; i < n; i++)
        dev[i] = dev[i - 1] + x[i] - mean;

    /* R/S for different segment lengths */
    double sum_h = 0.0;
    uint32_t n_scales = 0;
    uint32_t seg_len;

    for (seg_len = n / 10; seg_len <= n / 2; seg_len += n / 20) {
        if (seg_len < 10) continue;
        uint32_t n_segs = n / seg_len;
        double rs_avg = 0.0;
        uint32_t s;
        for (s = 0; s < n_segs; s++) {
            double R = dev[s * seg_len + seg_len - 1] - dev[s * seg_len];
            /* S = std dev within segment */
            double S = 0.0;
            uint32_t j;
            for (j = s * seg_len; j < (s + 1) * seg_len; j++)
                S += (x[j] - mean) * (x[j] - mean);
            S = sqrt(S / seg_len);
            if (S > 1e-12) rs_avg += R / S;
        }
        rs_avg /= n_segs;

        sum_h += log(rs_avg) / log((double)seg_len);
        n_scales++;
    }

    free(dev);
    return n_scales > 0 ? sum_h / n_scales : 0.5;
}

/* =================================================================
 * L5: Nonlinearity Detection (Surrogate Data Method)
 *
 * Compare prediction error on original data vs phase-randomized surrogates.
 * If original is significantly more predictable, dynamics are nonlinear.
 *
 * Complexity: O(n_surrogates * n * dim)
 * ================================================================= */

void cas_nonlinearity_detect(cas_nonlinearity_t *nl, const double *x,
                             uint32_t n, uint32_t n_surrogates,
                             uint64_t *seed)
{
    if (!nl || !x || n < 20) return;
    memset(nl, 0, sizeof(*nl));
    if (seed) _em_srand(*seed);

    /* Original prediction error via simple nearest-neighbor predictor */
    double original_error = 0.0;
    uint32_t i;
    for (i = 10; i < n; i++) {
        original_error += fabs(x[i] - x[i - 1]);
    }
    original_error /= (n - 10);

    /* Generate surrogates and compute prediction errors */
    double surrogate_error_sum = 0.0;
    uint32_t s;
    for (s = 0; s < n_surrogates; s++) {
        /* Phase-randomized surrogate via FFT (simplified: shuffle phases) */
        double *surrogate = (double *)malloc(n * sizeof(double));
        if (!surrogate) continue;
        memcpy(surrogate, x, n * sizeof(double));

        /* Random phase shuffling: swap segments */
        uint32_t seg = n / 4;
        uint32_t i1 = _em_rand_u64() % (n - seg);
        uint32_t i2 = _em_rand_u64() % (n - seg);
        uint32_t j;
        for (j = 0; j < seg; j++) {
            double tmp = surrogate[i1 + j];
            surrogate[i1 + j] = surrogate[i2 + j];
            surrogate[i2 + j] = tmp;
        }

        double surr_error = 0.0;
        for (i = 10; i < n; i++)
            surr_error += fabs(surrogate[i] - surrogate[i - 1]);
        surr_error /= (n - 10);

        surrogate_error_sum += surr_error;
        free(surrogate);
    }

    nl->surrogate_error = n_surrogates > 0 ? surrogate_error_sum / n_surrogates : 0.0;
    nl->nonlinearity_score = nl->surrogate_error > 1e-12
        ? fabs(original_error - nl->surrogate_error) / nl->surrogate_error : 0.0;
    nl->is_nonlinear = nl->nonlinearity_score > 0.1;
    nl->is_chaotic = (cas_lyapunov_exponent(x, n, 3, 1) > 0.01);

    if (seed) *seed = _em_seed;
}

/* =================================================================
 * L5: Time Series Functions
 * ================================================================= */

void cas_ts_init(cas_time_series_t *ts, uint32_t capacity)
{
    if (!ts) return;
    memset(ts, 0, sizeof(*ts));
    ts->capacity = capacity < CAS_TIME_SERIES_MAX ? capacity : CAS_TIME_SERIES_MAX;
    ts->data = (double *)malloc(ts->capacity * sizeof(double));
    ts->length = 0;
}

void cas_ts_append(cas_time_series_t *ts, double value)
{
    if (!ts || !ts->data) return;
    if (ts->length < ts->capacity)
        ts->data[ts->length++] = value;
}

void cas_ts_analyze(cas_time_series_t *ts)
{
    if (!ts || !ts->data || ts->length == 0) return;

    double sum = 0.0, sum2 = 0.0, sum3 = 0.0, sum4 = 0.0;
    uint32_t i, n = ts->length;
    for (i = 0; i < n; i++) {
        sum += ts->data[i];
        sum2 += ts->data[i] * ts->data[i];
        sum3 += ts->data[i] * ts->data[i] * ts->data[i];
        sum4 += ts->data[i] * ts->data[i] * ts->data[i] * ts->data[i];
    }
    ts->mean = sum / n;
    double var = sum2 / n - ts->mean * ts->mean;
    ts->variance = var;
    ts->skewness = var > 1e-12 ? (sum3/n - 3*ts->mean*var - ts->mean*ts->mean*ts->mean) / (var*sqrt(var)) : 0.0;
    ts->kurtosis = var > 1e-12 ? (sum4/n - 4*ts->mean*sum3/n + 6*ts->mean*ts->mean*sum2/n - 3*ts->mean*ts->mean*ts->mean*ts->mean) / (var*var) - 3.0 : 0.0;
    ts->hurst_exponent = cas_hurst_exponent(ts->data, n);
    ts->lyapunov_exponent = cas_lyapunov_exponent(ts->data, n, 3, 1);
    ts->has_long_memory = ts->hurst_exponent > 0.55;
}

double cas_ts_autocorrelation(const cas_time_series_t *ts, uint32_t lag)
{
    if (!ts || !ts->data || lag >= ts->length) return 0.0;

    double mean = ts->mean;
    double num = 0.0, den = 0.0;
    uint32_t i;
    for (i = 0; i < ts->length - lag; i++)
        num += (ts->data[i] - mean) * (ts->data[i + lag] - mean);
    for (i = 0; i < ts->length; i++)
        den += (ts->data[i] - mean) * (ts->data[i] - mean);
    return den > 1e-12 ? num / den : 0.0;
}

double cas_ts_partial_autocorr(const cas_time_series_t *ts, uint32_t lag)
{
    /* Simplified PACF via Durbin-Levinson (1st order approx) */
    double acf1 = cas_ts_autocorrelation(ts, 1);
    double acf_k = cas_ts_autocorrelation(ts, lag);
    if (lag == 1) return acf1;
    return (acf_k - acf1 * acf1) / (1.0 - acf1 * acf1);
}

void cas_ts_detrend(cas_time_series_t *ts)
{
    if (!ts || !ts->data || ts->length < 2) return;

    /* Linear detrend */
    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    uint32_t i, n = ts->length;
    for (i = 0; i < n; i++) {
        sx += i;
        sy += ts->data[i];
        sxx += (double)i * i;
        sxy += i * ts->data[i];
    }
    double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx + 1e-12);
    double intercept = (sy - slope * sx) / n;

    for (i = 0; i < n; i++)
        ts->data[i] -= (slope * i + intercept);

    cas_ts_analyze(ts);
}

void cas_ts_destroy(cas_time_series_t *ts)
{
    if (!ts) return;
    free(ts->data);
    ts->data = NULL;
    ts->length = 0;
    ts->capacity = 0;
}

/* =================================================================
 * L5: Integrated Information for CAS population
 *
 * Simplified Phi measure using population correlation structure.
 *
 * Complexity: O(n_agents^2)
 * ================================================================= */

double cas_integrated_information(const cas_population_t *pop)
{
    if (!pop || pop->size < 2) return 0.0;

    /* Phi ~ total correlation among agents */
    double total_mi = 0.0;
    uint32_t i, j;
    uint32_t n_pairs = 0;

    for (i = 0; i < pop->size; i++) {
        uint32_t dim = pop->agents[i].model.state_dim;
        for (j = i + 1; j < pop->size; j++) {
            uint32_t dim2 = pop->agents[j].model.state_dim;
            if (dim == 0 || dim2 == 0) continue;

            double dot = 0.0;
            uint32_t d, mdim = dim < dim2 ? dim : dim2;
            for (d = 0; d < mdim; d++)
                dot += pop->agents[i].model.state[d] * pop->agents[j].model.state[d];
            total_mi += fabs(dot) / mdim;
            n_pairs++;
        }
    }

    return n_pairs > 0 ? total_mi / n_pairs : 0.0;
}

/* =================================================================
 * L5: Causal Density of Network
 *
 * Fraction of causal interactions among possible pairs.
 *
 * Complexity: O(n^2)
 * ================================================================= */

double cas_causal_density(const cas_network_t *net)
{
    if (!net || net->num_nodes < 2) return 0.0;

    uint32_t max_edges = net->num_nodes * (net->num_nodes - 1) / 2;
    return max_edges > 0 ? (double)net->num_edges / max_edges : 0.0;
}
