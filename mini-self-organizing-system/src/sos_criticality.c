#include "sos_criticality.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===== Bak-Sneppen Model ===== */

BakSneppenModel* bs_create(int n_species) {
    BakSneppenModel* bs = (BakSneppenModel*)calloc(1, sizeof(BakSneppenModel));
    if (!bs) return NULL;
    bs->n_species = n_species;
    bs->fitness = (double*)malloc(n_species * sizeof(double));
    bs->species_id = (int*)malloc(n_species * sizeof(int));
    for (int i = 0; i < n_species; i++) bs->species_id[i] = i;
    bs->threshold = 0.0;
    bs->n_extinctions = 0;
    bs->extinction_sizes = (int*)malloc(10000 * sizeof(int));
    bs->ext_history_len = 0;
    bs->total_time = 0;
    return bs;
}

void bs_free(BakSneppenModel* bs) {
    if (!bs) return;
    free(bs->fitness); free(bs->species_id);
    free(bs->extinction_sizes);
    free(bs);
}

void bs_randomize(BakSneppenModel* bs) {
    if (!bs) return;
    for (int i = 0; i < bs->n_species; i++)
        bs->fitness[i] = (double)rand() / RAND_MAX;
}

int bs_step(BakSneppenModel* bs) {
    /* Find species with minimum fitness */
    if (!bs) return 0;
    int min_idx = 0;
    double min_fit = bs->fitness[0];
    for (int i = 1; i < bs->n_species; i++)
        if (bs->fitness[i] < min_fit) {
            min_fit = bs->fitness[i];
            min_idx = i;
        }
    /* Replace the minimum and its two neighbors with random fitness */
    int avalanche_size = 1;
    bs->fitness[min_idx] = (double)rand() / RAND_MAX;
    bs->n_extinctions++;

    int left = (min_idx - 1 + bs->n_species) % bs->n_species;
    bs->fitness[left] = (double)rand() / RAND_MAX;
    avalanche_size++;

    int right = (min_idx + 1) % bs->n_species;
    bs->fitness[right] = (double)rand() / RAND_MAX;
    avalanche_size++;

    /* Record extinction event */
    if (bs->ext_history_len < 10000) {
        bs->extinction_sizes[bs->ext_history_len++] = avalanche_size;
    }
    bs->threshold = min_fit;
    bs->total_time++;
    return avalanche_size;
}

void bs_run(BakSneppenModel* bs, int n_steps) {
    for (int i = 0; i < n_steps; i++) bs_step(bs);
}

double bs_fitness_threshold(const BakSneppenModel* bs) {
    if (!bs) return 0.0;
    double min_f = bs->fitness[0];
    for (int i = 1; i < bs->n_species; i++)
        if (bs->fitness[i] < min_f) min_f = bs->fitness[i];
    return min_f;
}

int bs_is_critical(const BakSneppenModel* bs) {
    /* The Bak-Sneppen model self-organizes to f_c ~ 0.667.
     * Check if threshold has converged near this value. */
    if (!bs || bs->ext_history_len < 100) return 0;
    double fc = bs_fitness_threshold(bs);
    return (fabs(fc - 0.667) < 0.1) ? 1 : 0;
}

void bs_avalanche_distribution(const BakSneppenModel* bs,
                               double* sizes, double* probs, int n_bins) {
    if (!bs || !sizes || !probs || bs->ext_history_len == 0) return;
    int max_s = 0;
    for (int i = 0; i < bs->ext_history_len; i++)
        if (bs->extinction_sizes[i] > max_s)
            max_s = bs->extinction_sizes[i];
    if (max_s == 0) return;
    for (int b = 0; b < n_bins; b++) {
        double log_low = log(1.0);
        double log_high = log((double)max_s);
        double dlog = (log_high - log_low) / n_bins;
        double bin_low = exp(log_low + b * dlog);
        double bin_high = exp(log_low + (b+1) * dlog);
        sizes[b] = (bin_low + bin_high) / 2.0;
        int count = 0;
        for (int i = 0; i < bs->ext_history_len; i++)
            if (bs->extinction_sizes[i] >= bin_low &&
                bs->extinction_sizes[i] < bin_high) count++;
        probs[b] = (double)count / bs->ext_history_len;
    }
}

/* ===== Forest Fire Model ===== */

ForestFireModel* ff_create(int width, int height, double p_growth, double p_lightning) {
    ForestFireModel* ff = (ForestFireModel*)calloc(1, sizeof(ForestFireModel));
    if (!ff) return NULL;
    ff->width = width; ff->height = height;
    ff->p_growth = p_growth; ff->p_lightning = p_lightning;
    ff->state = (int**)malloc(height * sizeof(int*));
    for (int y = 0; y < height; y++)
        ff->state[y] = (int*)calloc(width, sizeof(int));
    ff->fire_sizes = (int*)malloc(10000 * sizeof(int));
    ff->fire_history_len = 0;
    return ff;
}

void ff_free(ForestFireModel* ff) {
    if (!ff) return;
    for (int y = 0; y < ff->height; y++) free(ff->state[y]);
    free(ff->state); free(ff->fire_sizes);
    free(ff);
}

int ff_step(ForestFireModel* ff) {
    if (!ff) return 0;
    int fire_size = 0;
    /* Lightning strikes: random trees start burning */
    for (int y = 0; y < ff->height; y++)
        for (int x = 0; x < ff->width; x++)
            if (ff->state[y][x] == 1 && (double)rand()/RAND_MAX < ff->p_lightning) {
                ff->state[y][x] = 2;
                fire_size++;
            }
    /* Fire spread: burning trees ignite neighbors */
    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    int changed = (fire_size > 0);
    while (changed) {
        changed = 0;
        for (int y = 0; y < ff->height; y++)
            for (int x = 0; x < ff->width; x++)
                if (ff->state[y][x] == 2) {
                    for (int d = 0; d < 4; d++) {
                        int nx = x+dirs[d][0], ny = y+dirs[d][1];
                        if (nx>=0 && nx<ff->width && ny>=0 && ny<ff->height)
                            if (ff->state[ny][nx] == 1) {
                                ff->state[ny][nx] = 2;
                                fire_size++; changed = 1;
                            }
                    }
                }
    }
    /* Burn out: burning -> empty */
    for (int y = 0; y < ff->height; y++)
        for (int x = 0; x < ff->width; x++)
            if (ff->state[y][x] == 2) {
                ff->state[y][x] = 0;
                ff->n_burned++;
            }
    /* Tree growth */
    ff->n_trees = 0; ff->n_burning = 0;
    for (int y = 0; y < ff->height; y++)
        for (int x = 0; x < ff->width; x++) {
            if (ff->state[y][x] == 0 && (double)rand()/RAND_MAX < ff->p_growth) {
                ff->state[y][x] = 1;
            }
            if (ff->state[y][x] == 1) ff->n_trees++;
            if (ff->state[y][x] == 2) ff->n_burning++;
        }
    if (fire_size > 0 && ff->fire_history_len < 10000)
        ff->fire_sizes[ff->fire_history_len++] = fire_size;
    return fire_size;
}

void ff_run(ForestFireModel* ff, int n_steps) {
    for (int i = 0; i < n_steps; i++) ff_step(ff);
}

void ff_fire_distribution(const ForestFireModel* ff,
                          double* sizes, double* probs, int n_bins) {
    if (!ff || !sizes || !probs || ff->fire_history_len == 0) return;
    int max_s = 0;
    for (int i = 0; i < ff->fire_history_len; i++)
        if (ff->fire_sizes[i] > max_s) max_s = ff->fire_sizes[i];
    if (max_s == 0) return;
    double log_min = 0.0, log_max = log((double)max_s);
    double dlog = (log_max - log_min) / n_bins;
    for (int b = 0; b < n_bins; b++) {
        double low = exp(log_min + b*dlog), high = exp(log_min + (b+1)*dlog);
        sizes[b] = (low + high)/2.0;
        int count = 0;
        for (int i = 0; i < ff->fire_history_len; i++)
            if (ff->fire_sizes[i] >= low && ff->fire_sizes[i] < high) count++;
        probs[b] = (double)count / ff->fire_history_len;
    }
}

/* ===== SOC Analysis ===== */

int soc_test_powerlaw(const double* values, int n, double x_min,
                      double* p_value) {
    /* Clauset-Shalizi-Newman (2009) power-law test.
     * Uses KS statistic between empirical and fitted power-law CDF.
     * Simplified: compute exponent, generate synthetic data,
     * compute KS distance. Returns 1 if power-law plausible. */
    if (!values || n < 50) { if(p_value) *p_value = 0.0; return 0; }
    /* Compute MLE exponent alpha */
    double sum_log = 0.0; int cnt = 0;
    for (int i = 0; i < n; i++)
        if (values[i] >= x_min) { sum_log += log(values[i]/x_min); cnt++; }
    if (cnt < 10) { if(p_value) *p_value = 0.0; return 0; }
    double alpha = 1.0 + (double)cnt / sum_log;
    /* Compute empirical CDF and theoretical CDF */
    double* sorted = (double*)malloc(cnt * sizeof(double));
    int idx = 0;
    for (int i = 0; i < n; i++)
        if (values[i] >= x_min) sorted[idx++] = values[i];
    /* Simple bubble sort for small sets */
    for (int i = 0; i < cnt-1; i++)
        for (int j = 0; j < cnt-1-i; j++)
            if (sorted[j] > sorted[j+1]) {
                double tmp = sorted[j]; sorted[j] = sorted[j+1]; sorted[j+1] = tmp;
            }
    /* KS statistic = max |empirical CDF - theoretical CDF| */
    double ks_max = 0.0;
    for (int i = 0; i < cnt; i++) {
        double ecdf = (double)(i+1) / cnt;
        double tcdf = 1.0 - pow(x_min / sorted[i], alpha - 1.0);
        double diff = fabs(ecdf - tcdf);
        if (diff > ks_max) ks_max = diff;
    }
    free(sorted);
    /* Approximate p-value: p ~ exp(-2 * n * KS^2) (simplified) */
    if (p_value) {
        double stat = 2.0 * n * ks_max * ks_max;
        *p_value = exp(-stat);
    }
    return (*p_value > 0.1) ? 1 : 0;
}

double soc_noise_exponent(const double* time_series, int n, double dt) {
    /* Compute 1/f^alpha exponent by log-log regression of power spectrum. */
    if (!time_series || n < 16) return 0.0;
    int nf = n/2 + 1;
    double* psd = (double*)malloc(nf * sizeof(double));
    /* Simple periodogram */
    for (int k = 0; k < nf; k++) {
        double re = 0.0, im = 0.0;
        for (int t = 0; t < n; t++) {
            double theta = 2.0 * M_PI * k * t / n;
            re += time_series[t] * cos(theta);
            im -= time_series[t] * sin(theta);
        }
        psd[k] = (re*re + im*im) / n;
    }
    /* Log-log regression on frequencies 1..nf/2 (skip DC) */
    int start = 1, end = nf/2;
    double sx=0, sy=0, sxy=0, sxx=0;
    int count = 0;
    for (int i = start; i < end; i++) {
        if (psd[i] < 1e-15) continue;
        double f = (double)i / (n * dt);
        double lx = log(f), ly = log(psd[i]);
        sx += lx; sy += ly; sxy += lx*ly; sxx += lx*lx;
        count++;
    }
    free(psd);
    if (count < 4) return 0.0;
    double denom = count*sxx - sx*sx;
    if (fabs(denom) < 1e-15) return 0.0;
    double slope = (count*sxy - sx*sy) / denom;
    return -slope; /* P ~ f^{-alpha}, slope = -alpha */
}

double soc_finite_size_scaling(const double* mean_sizes,
                               const double* system_sizes, int n) {
    /* <s>(L) ~ L^{gamma}. Fit gamma by log-log regression. */
    if (!mean_sizes || !system_sizes || n < 2) return 0.0;
    double sx=0, sy=0, sxy=0, sxx=0;
    for (int i = 0; i < n; i++) {
        double lx = log(system_sizes[i]);
        double ly = log(mean_sizes[i]);
        sx += lx; sy += ly; sxy += lx*ly; sxx += lx*lx;
    }
    double denom = n*sxx - sx*sx;
    if (fabs(denom) < 1e-15) return 0.0;
    return (n*sxy - sx*sy) / denom;
}

double soc_avalanche_shape_collapse(const double** avalanche_profiles,
                                    const double* durations,
                                    int n_avalanches, int max_duration) {
    /* Sethna et al. (2001): V(t,T) ~ T^{gamma-1} * F(t/T)
     * Compute collapse quality as variance of scaled profiles.
     * Lower variance = better collapse. */
    if (!avalanche_profiles || !durations || n_avalanches < 2) return 0.0;
    /* Estimate gamma from mean peak velocity scaling */
    double sx=0, sy=0, sxy=0, sxx=0;
    for (int a = 0; a < n_avalanches; a++) {
        double lT = log(durations[a]);
        double peak = 0.0;
        for (int t = 0; t < max_duration; t++)
            if (avalanche_profiles[a][t] > peak)
                peak = avalanche_profiles[a][t];
        if (peak < 1e-15) continue;
        double ly = log(peak);
        sx += lT; sy += ly; sxy += lT*ly; sxx += lT*lT;
    }
    double gamma_minus_1 = (n_avalanches*sxy - sx*sy) / (n_avalanches*sxx - sx*sx + 1e-15);
    /* Compute variance of binned scaled profiles */
    int n_bins = 20;
    double* bin_mean = (double*)calloc(n_bins, sizeof(double));
    double* bin_var = (double*)calloc(n_bins, sizeof(double));
    int* bin_count = (int*)calloc(n_bins, sizeof(int));
    for (int a = 0; a < n_avalanches; a++) {
        for (int t = 0; t < max_duration && (double)t/durations[a] < 1.0; t++) {
            double scaled_t = (double)t / durations[a];
            int bin = (int)(scaled_t * n_bins);
            if (bin >= 0 && bin < n_bins) {
                double scaled_v = avalanche_profiles[a][t] / pow(durations[a], gamma_minus_1);
                bin_mean[bin] += scaled_v;
                bin_var[bin] += scaled_v * scaled_v;
                bin_count[bin]++;
            }
        }
    }
    double total_var = 0.0;
    int valid_bins = 0;
    for (int b = 0; b < n_bins; b++) {
        if (bin_count[b] > 1) {
            double mean = bin_mean[b] / bin_count[b];
            double var = bin_var[b]/bin_count[b] - mean*mean;
            total_var += var;
            valid_bins++;
        }
    }
    free(bin_mean); free(bin_var); free(bin_count);
    /* Collapse quality = 1 / variance (higher is better) */
    return (valid_bins > 0 && total_var > 1e-15) ? 1.0/total_var : 0.0;
}
