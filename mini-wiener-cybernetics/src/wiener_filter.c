#include "wiener_core.h"
#include "wiener_filter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ==============================================================
 * Wiener Filter — Optimal Linear Estimation
 *
 * The Wiener filter minimizes the mean-square error between
 * a desired signal d[n] and the filter output y[n] = h*x[n].
 *
 * L4: Wiener-Hopf equation:
 *     R_xx * h_opt = r_xd
 *     where R_xx[k] = E[x[n]*x[n-k]] (autocorrelation matrix)
 *           r_xd[k] = E[x[n]*d[n-k]] (cross-correlation vector)
 *
 * L4: Wiener-Khinchin theorem:
 *     S_x(w) = FT{R_xx[k]} (power spectrum = FT of autocorrelation)
 *     H_opt(w) = S_xd(w) / S_xx(w) (frequency-domain Wiener filter)
 *
 * L5: Levinson-Durbin algorithm — O(N^2) solver for Toeplitz
 *     systems arising in the Wiener-Hopf equation.
 *
 * Wiener developed this filter during WWII for anti-aircraft
 * fire control — the original "cybernetics" application.
 *
 * References:
 *   Wiener (1949) "Extrapolation, Interpolation, and Smoothing
 *     of Stationary Time Series" — MIT Press (NDRC report, 1942)
 *   Kolmogorov (1941) "Interpolation and Extrapolation of
 *     Stationary Random Sequences" — Izv. Akad. Nauk SSSR
 * ============================================================== */

extern double wc_gaussian_random(void);

/* ── Helper: Solve small linear system (Gaussian elimination) ─ */

static int solve_linear(double* A, double* b, double* x, int n) {
    /* A is n x n, b is n x 1, x is output n x 1. In-place. */
    if (n <= 0 || n > 128) return -1;
    /* Augment [A | b] */
    double* aug = (double*)calloc((size_t)(n * (n + 1)), sizeof(double));
    if (!aug) return -1;
    for (int i = 0; i < n; i++) {
        memcpy(aug + i * (n + 1), A + i * n, (size_t)n * sizeof(double));
        aug[i * (n + 1) + n] = b[i];
    }
    /* Gaussian elimination with partial pivoting */
    for (int k = 0; k < n; k++) {
        int pivot = k;
        double max_val = fabs(aug[k * (n + 1) + k]);
        for (int i = k + 1; i < n; i++) {
            double v = fabs(aug[i * (n + 1) + k]);
            if (v > max_val) { max_val = v; pivot = i; }
        }
        if (max_val < WC_EPSILON) { free(aug); return -1; }
        if (pivot != k) {
            for (int j = 0; j <= n; j++) {
                double t = aug[k * (n + 1) + j];
                aug[k * (n + 1) + j] = aug[pivot * (n + 1) + j];
                aug[pivot * (n + 1) + j] = t;
            }
        }
        double pv = aug[k * (n + 1) + k];
        for (int j = k; j <= n; j++) aug[k * (n + 1) + j] /= pv;
        for (int i = k + 1; i < n; i++) {
            double f = aug[i * (n + 1) + k];
            for (int j = k; j <= n; j++)
                aug[i * (n + 1) + j] -= f * aug[k * (n + 1) + j];
        }
    }
    /* Back substitution */
    for (int k = n - 1; k >= 0; k--) {
        x[k] = aug[k * (n + 1) + n];
        for (int j = k + 1; j < n; j++)
            x[k] -= aug[k * (n + 1) + j] * x[j];
    }
    free(aug);
    return 0;
}

/* ── Helper: Levinson-Durbin for Toeplitz systems ─────────── */

static int levinson_durbin(const double* r, int n, double* a, double* err) {
    /* Solve Toeplitz system R*a = -r[1..n] for AR coefficients.
     * r[0..n] are autocorrelation values (r[0] is lag-0 = power).
     * a[0..n-1] are output AR coefficients (a[0]=1 implicit).
     * Returns prediction error power in *err. */
    if (n <= 0 || !r || !a) return -1;
    double* backward = (double*)calloc((size_t)n, sizeof(double));
    double* forward  = (double*)calloc((size_t)n, sizeof(double));
    if (!backward || !forward) {
        free(backward); free(forward); return -1;
    }
    if (r[0] < WC_EPSILON) { free(backward); free(forward); return -1; }
    double E = r[0];  /* Prediction error energy */
    a[0] = 1.0;
    for (int k = 1; k < n; k++) {
        /* Compute reflection coefficient */
        double gamma = 0.0;
        for (int j = 0; j < k; j++)
            gamma += a[j] * r[k - j];
        double kappa = -gamma / E;
        /* Update forward coefficients */
        for (int j = 0; j <= k; j++)
            forward[j] = (j < k ? a[j] : 0.0) + kappa * (j > 0 ? a[k - j] : 0.0);
        /* Swap roles for next iteration */
        memcpy(backward, a, (size_t)k * sizeof(double));
        /* Actually: a_j^(k+1) = a_j^(k) + kappa * a_{k-j}^{(k)} */
        for (int j = 0; j <= k; j++) {
            double old_a_j = (j < k) ? a[j] : 0.0;
            double old_a_kj = (k - j > 0 && k - j <= k) ? backward[k - j - 1] : 0.0;
            if (j == 0) old_a_j = 1.0;
            forward[j] = old_a_j + kappa * ((j > 0 && j <= k) ? backward[k - j] : 0.0);
        }
        forward[0] = 1.0;
        memcpy(a, forward, (size_t)(k + 1) * sizeof(double));
        E *= (1.0 - kappa * kappa);
        if (E < WC_EPSILON) E = WC_EPSILON;
    }
    /* Shift: a[1..n-1] are the AR coefficients */
    if (err) *err = E;
    free(backward); free(forward);
    return 0;
}

/* ==============================================================
 * WIENER FILTER (L4, L5)
 * ============================================================== */

WCWienerFilter* wc_filter_create(int order) {
    if (order < 1) return NULL;
    WCWienerFilter* wf = (WCWienerFilter*)calloc(1, sizeof(WCWienerFilter));
    if (!wf) return NULL;
    wf->order   = order;
    wf->coeffs  = (double*)calloc((size_t)(order + 1), sizeof(double));
    if (!wf->coeffs) { free(wf); return NULL; }
    wf->coeffs[0] = 1.0;  /* Normalize first coefficient */
    wf->is_causal = true;
    return wf;
}

void wc_filter_free(WCWienerFilter* wf) {
    if (!wf) return;
    free(wf->coeffs);
    free(wf);
}

int wc_filter_design_fir(WCWienerFilter* wf, const WCSignal* input,
                          const WCSignal* desired, int order) {
    /* L5: FIR Wiener filter via direct Wiener-Hopf solution.
     * Build autocorrelation matrix R_xx and cross-correlation r_xd
     * from the signals, then solve R_xx * h = r_xd. */
    if (!wf || !input || !desired || order < 1) return -1;
    if (input->length < order || desired->length < order) return -1;

    int N = input->length;
    double* R = (double*)calloc((size_t)(order * order), sizeof(double));
    double* r = (double*)calloc((size_t)order, sizeof(double));
    double* h = (double*)calloc((size_t)order, sizeof(double));
    if (!R || !r || !h) { free(R); free(r); free(h); return -1; }

    /* Build autocorrelation matrix R_xx (Toeplitz, symmetric) */
    for (int i = 0; i < order; i++) {
        for (int j = 0; j < order; j++) {
            double sum = 0.0;
            int lag = abs(i - j);
            for (int n = order; n < N; n++)
                sum += input->samples[n - i] * input->samples[n - j];
            R[i * order + j] = sum / (double)(N - order);
        }
    }

    /* Build cross-correlation vector r_xd */
    for (int i = 0; i < order; i++) {
        double sum = 0.0;
        for (int n = order; n < N; n++)
            sum += input->samples[n - i] * desired->samples[n];
        r[i] = sum / (double)(N - order);
    }

    /* Solve R * h = r */
    int ret = solve_linear(R, r, h, order);
    if (ret < 0) { free(R); free(r); free(h); return -1; }

    /* Store results */
    free(wf->coeffs);
    wf->order  = order;
    wf->coeffs = h;  /* Take ownership — don't free h */
    free(R); free(r);

    /* Compute MSE */
    wf->mse = wc_filter_mse(wf, input, desired);
    return 0;
}

int wc_filter_design_freq(WCWienerFilter* wf, const double* S_xx,
                           const double* S_xd, int n_freq) {
    /* L5: Frequency-domain Wiener filter design.
     * H(w_k) = S_xd(w_k) / S_xx(w_k) for each frequency bin.
     * Then inverse FFT to get time-domain coefficients.
     *
     * Here we compute the frequency-domain solution and then
     * approximate FIR coefficients via a simplified approach. */
    if (!wf || !S_xx || !S_xd || n_freq < 2) return -1;

    /* Compute H(w) at each frequency */
    double* H_freq = (double*)calloc((size_t)n_freq, sizeof(double));
    if (!H_freq) return -1;
    for (int k = 0; k < n_freq; k++) {
        double sxx = S_xx[k];
        if (sxx < WC_EPSILON)
            H_freq[k] = 0.0;  /* No signal energy => no filter */
        else
            H_freq[k] = S_xd[k] / sxx;
    }

    /* Approximate FIR by sampling frequency response
     * and using IDFT. For simplicity, use a windowed sinc. */
    int order = wf->order;
    free(wf->coeffs);
    wf->coeffs = (double*)calloc((size_t)(order + 1), sizeof(double));
    if (!wf->coeffs) { free(H_freq); return -1; }

    /* Average frequency response magnitude as DC gain */
    double sum_H = 0.0;
    for (int k = 0; k < n_freq; k++) sum_H += H_freq[k];
    double dc_gain = sum_H / n_freq;
    wf->coeffs[0] = dc_gain;

    /* For higher-order coefficients, use simplified approach:
     * band-limited impulse response from H(w) shape */
    for (int i = 1; i <= order; i++) {
        double acc = 0.0;
        for (int k = 0; k < n_freq; k++) {
            double omega = WC_PI * (double)k / (double)(n_freq - 1);
            acc += H_freq[k] * cos(omega * i);
        }
        wf->coeffs[i] = acc / n_freq;
    }

    free(H_freq);
    return 0;
}

int wc_filter_design_wiener_hopf(WCWienerFilter* wf,
                                  const double* autocorr,
                                  const double* crosscorr,
                                  int order) {
    /* L5: Direct Wiener-Hopf solution using known correlations.
     * R is Toeplitz from autocorr[0..order-1].
     * r_xd is from crosscorr[0..order-1]. */
    if (!wf || !autocorr || !crosscorr || order < 1) return -1;

    double* R = (double*)calloc((size_t)(order * order), sizeof(double));
    double* r = (double*)calloc((size_t)order, sizeof(double));
    double* h = (double*)calloc((size_t)order, sizeof(double));
    if (!R || !r || !h) { free(R); free(r); free(h); return -1; }

    /* Build Toeplitz R from autocorrelation */
    for (int i = 0; i < order; i++) {
        for (int j = 0; j < order; j++) {
            int lag = abs(i - j);
            R[i * order + j] = autocorr[lag];
        }
    }

    /* Copy cross-correlation */
    memcpy(r, crosscorr, (size_t)order * sizeof(double));

    /* Solve */
    int ret = solve_linear(R, r, h, order);
    if (ret < 0) { free(R); free(r); free(h); return -1; }

    free(wf->coeffs);
    wf->order  = order;
    wf->coeffs = h;
    free(R); free(r);
    return 0;
}

WCSignal* wc_filter_apply(const WCWienerFilter* wf, const WCSignal* input) {
    if (!wf || !input) return NULL;
    int N = input->length;
    WCSignal* output = wc_signal_create(N, input->dt, input->t0);
    if (!output) return NULL;

    for (int n = 0; n < N; n++) {
        double y = 0.0;
        for (int k = 0; k <= wf->order && k <= n; k++) {
            y += wf->coeffs[k] * input->samples[n - k];
        }
        output->samples[n] = y;
    }
    return output;
}

double wc_filter_mse(const WCWienerFilter* wf, const WCSignal* input,
                     const WCSignal* desired) {
    if (!wf || !input || !desired) return 0.0;
    if (input->length != desired->length) return 0.0;
    double mse = 0.0;
    int N = input->length;
    for (int n = wf->order; n < N; n++) {
        double y = 0.0;
        for (int k = 0; k <= wf->order; k++)
            y += wf->coeffs[k] * input->samples[n - k];
        double e = desired->samples[n] - y;
        mse += e * e;
    }
    return mse / (double)(N - wf->order);
}

void wc_filter_print(const WCWienerFilter* wf) {
    if (!wf) { printf("WCWienerFilter: NULL\n"); return; }
    printf("WCWienerFilter: order=%d mse=%.6f causal=%d\n",
           wf->order, wf->mse, wf->is_causal);
    printf("  coeffs: ");
    for (int i = 0; i <= wf->order && i < 12; i++)
        printf("%.4f ", wf->coeffs[i]);
    printf("\n");
}

/* ==============================================================
 * AUTOCORRELATION (L3, L5)
 *
 * R_xx[k] = E[x[n] * x[n-k]]
 *
 * Wiener-Khinchin theorem (L4):
 *   PSD(w) = sum_{k=-inf}^{inf} R_xx[k] * exp(-j*w*k)
 *   R_xx[k] = (1/2*pi) integral PSD(w) * exp(j*w*k) dw
 * ============================================================== */

WCAutocorrelation* wc_autocorr_create(int max_lag) {
    if (max_lag < 0) return NULL;
    WCAutocorrelation* ac = (WCAutocorrelation*)calloc(1, sizeof(WCAutocorrelation));
    if (!ac) return NULL;
    ac->max_lag = max_lag;
    ac->values  = (double*)calloc((size_t)(max_lag + 1), sizeof(double));
    if (!ac->values) { free(ac); return NULL; }
    return ac;
}

void wc_autocorr_free(WCAutocorrelation* ac) {
    if (!ac) return;
    free(ac->values);
    free(ac);
}

int wc_autocorr_compute(WCAutocorrelation* ac, const WCSignal* sig) {
    return wc_autocorr_compute_unbiased(ac, sig);
}

int wc_autocorr_compute_unbiased(WCAutocorrelation* ac, const WCSignal* sig) {
    /* R[k] = (1/(N-k)) * sum_{n=k}^{N-1} x[n]*x[n-k] */
    if (!ac || !sig || sig->length == 0) return -1;
    int N = sig->length;
    ac->signal_length = N;
    for (int k = 0; k <= ac->max_lag && k < N; k++) {
        double sum = 0.0;
        int count = N - k;
        for (int n = k; n < N; n++)
            sum += sig->samples[n] * sig->samples[n - k];
        ac->values[k] = (count > 0) ? sum / (double)count : 0.0;
    }
    ac->power = ac->values[0];  /* R[0] = signal power */
    return 0;
}

int wc_autocorr_compute_biased(WCAutocorrelation* ac, const WCSignal* sig) {
    /* R[k] = (1/N) * sum_{n=k}^{N-1} x[n]*x[n-k] */
    if (!ac || !sig || sig->length == 0) return -1;
    int N = sig->length;
    ac->signal_length = N;
    for (int k = 0; k <= ac->max_lag && k < N; k++) {
        double sum = 0.0;
        for (int n = k; n < N; n++)
            sum += sig->samples[n] * sig->samples[n - k];
        ac->values[k] = sum / (double)N;
    }
    ac->power = ac->values[0];
    return 0;
}

double wc_autocorr_get(const WCAutocorrelation* ac, int lag) {
    if (!ac || lag < 0 || lag > ac->max_lag) return 0.0;
    return ac->values[lag];
}

void wc_autocorr_to_toeplitz(const WCAutocorrelation* ac,
                              double* matrix, int dim) {
    /* Build Toeplitz matrix T[i][j] = R[|i-j|] */
    if (!ac || !matrix || dim <= 0) return;
    for (int i = 0; i < dim; i++)
        for (int j = 0; j < dim; j++) {
            int lag = abs(i - j);
            matrix[i * dim + j] = (lag <= ac->max_lag) ? ac->values[lag] : 0.0;
        }
}

void wc_autocorr_print(const WCAutocorrelation* ac) {
    if (!ac) { printf("WCAutocorrelation: NULL\n"); return; }
    printf("WCAutocorrelation: max_lag=%d power=%.4f\n", ac->max_lag, ac->power);
    printf("  values: ");
    for (int k = 0; k <= ac->max_lag && k < 10; k++)
        printf("%.4f ", ac->values[k]);
    printf("\n");
}

/* ==============================================================
 * POWER SPECTRAL DENSITY (L3, L5)
 * ============================================================== */

WCPowerSpectrum* wc_psd_create(int n_bins, double df) {
    if (n_bins < 2 || df <= 0.0) return NULL;
    WCPowerSpectrum* ps = (WCPowerSpectrum*)calloc(1, sizeof(WCPowerSpectrum));
    if (!ps) return NULL;
    ps->n_bins = n_bins;
    ps->df     = df;
    ps->freq   = (double*)calloc((size_t)n_bins, sizeof(double));
    ps->psd    = (double*)calloc((size_t)n_bins, sizeof(double));
    if (!ps->freq || !ps->psd) { wc_psd_free(ps); return NULL; }
    for (int i = 0; i < n_bins; i++) ps->freq[i] = i * df;
    return ps;
}

void wc_psd_free(WCPowerSpectrum* ps) {
    if (!ps) return;
    free(ps->freq);
    free(ps->psd);
    free(ps);
}

int wc_psd_periodogram(WCPowerSpectrum* ps, const WCSignal* sig) {
    /* Periodogram: |DFT{x}|^2 / N */
    if (!ps || !sig || sig->length == 0) return -1;
    int N = sig->length;
    double* re = (double*)calloc((size_t)ps->n_bins, sizeof(double));
    double* im = (double*)calloc((size_t)ps->n_bins, sizeof(double));
    if (!re || !im) { free(re); free(im); return -1; }

    /* DFT at each frequency bin */
    for (int k = 0; k < ps->n_bins; k++) {
        double omega_k = 2.0 * WC_PI * ps->freq[k] * sig->dt;
        for (int n = 0; n < N; n++) {
            re[k] += sig->samples[n] * cos(omega_k * n);
            im[k] -= sig->samples[n] * sin(omega_k * n);
        }
        ps->psd[k] = (re[k] * re[k] + im[k] * im[k]) / (double)N * sig->dt;
    }
    /* Compute total power */
    ps->total_power = 0.0;
    for (int k = 0; k < ps->n_bins; k++) ps->total_power += ps->psd[k] * ps->df;

    free(re); free(im);
    return 0;
}

int wc_psd_welch(WCPowerSpectrum* ps, const WCSignal* sig,
                  int window_size, int overlap) {
    /* Welch's method: average periodograms of overlapped windows */
    if (!ps || !sig || window_size < 2 || overlap < 0 || overlap >= window_size)
        return -1;
    int N = sig->length;
    int step = window_size - overlap;
    int n_windows = (N - window_size) / step + 1;
    if (n_windows < 1) return -1;

    /* Hamming window */
    double* window = (double*)calloc((size_t)window_size, sizeof(double));
    if (!window) return -1;
    double window_power = 0.0;
    for (int i = 0; i < window_size; i++) {
        window[i] = 0.54 - 0.46 * cos(2.0 * WC_PI * i / (window_size - 1));
        window_power += window[i] * window[i];
    }

    /* Accumulate periodograms */
    double* psd_accum = (double*)calloc((size_t)ps->n_bins, sizeof(double));
    if (!psd_accum) { free(window); return -1; }

    for (int w = 0; w < n_windows; w++) {
        int offset = w * step;
        double* re = (double*)calloc((size_t)ps->n_bins, sizeof(double));
        double* im = (double*)calloc((size_t)ps->n_bins, sizeof(double));
        if (!re || !im) { free(re); free(im); continue; }
        for (int k = 0; k < ps->n_bins; k++) {
            double omega_k = 2.0 * WC_PI * ps->freq[k] * sig->dt;
            for (int n = 0; n < window_size; n++) {
                double x_n = sig->samples[offset + n] * window[n];
                re[k] += x_n * cos(omega_k * n);
                im[k] -= x_n * sin(omega_k * n);
            }
            psd_accum[k] += (re[k] * re[k] + im[k] * im[k])
                          / (window_power * sig->dt);
        }
        free(re); free(im);
    }

    /* Average and scale */
    for (int k = 0; k < ps->n_bins; k++)
        ps->psd[k] = psd_accum[k] / (double)n_windows * sig->dt;

    ps->total_power = 0.0;
    for (int k = 0; k < ps->n_bins; k++) ps->total_power += ps->psd[k] * ps->df;

    free(window); free(psd_accum);
    return 0;
}

int wc_psd_from_autocorr(WCPowerSpectrum* ps, const WCAutocorrelation* ac) {
    /* L4: Wiener-Khinchin — PSD = FT of autocorrelation */
    if (!ps || !ac) return -1;
    for (int k = 0; k < ps->n_bins; k++) {
        double omega = 2.0 * WC_PI * ps->freq[k];
        double psd_val = ac->values[0];  /* R[0] */
        for (int lag = 1; lag <= ac->max_lag; lag++) {
            /* 2*R[lag]*cos(omega*lag) due to symmetry */
            psd_val += 2.0 * ac->values[lag] * cos(omega * lag);
        }
        ps->psd[k] = psd_val;
    }
    ps->total_power = 0.0;
    for (int k = 0; k < ps->n_bins; k++) ps->total_power += ps->psd[k] * ps->df;
    return 0;
}

double wc_psd_max_freq(const WCPowerSpectrum* ps) {
    if (!ps || ps->n_bins == 0) return 0.0;
    double max_val = ps->psd[0];
    int max_idx = 0;
    for (int k = 1; k < ps->n_bins; k++) {
        if (ps->psd[k] > max_val) { max_val = ps->psd[k]; max_idx = k; }
    }
    return ps->freq[max_idx];
}

double wc_psd_bandwidth(const WCPowerSpectrum* ps) {
    /* Half-power bandwidth: frequencies where PSD > max/2 */
    if (!ps || ps->n_bins == 0) return 0.0;
    double max_val = ps->psd[0];
    for (int k = 1; k < ps->n_bins; k++)
        if (ps->psd[k] > max_val) max_val = ps->psd[k];
    double half = max_val / 2.0;
    int lo = ps->n_bins, hi = 0;
    for (int k = 0; k < ps->n_bins; k++) {
        if (ps->psd[k] >= half) {
            if (k < lo) lo = k;
            if (k > hi) hi = k;
        }
    }
    if (hi <= lo) return 0.0;
    return ps->freq[hi] - ps->freq[lo];
}

/* ==============================================================
 * CROSS-CORRELATION (L3)
 * ============================================================== */

WCCrossCorrelation* wc_xcorr_create(int max_lag) {
    if (max_lag < 0) return NULL;
    WCCrossCorrelation* xc = (WCCrossCorrelation*)calloc(1, sizeof(WCCrossCorrelation));
    if (!xc) return NULL;
    xc->max_lag = max_lag;
    xc->values  = (double*)calloc((size_t)(2 * max_lag + 1), sizeof(double));
    if (!xc->values) { free(xc); return NULL; }
    return xc;
}

void wc_xcorr_free(WCCrossCorrelation* xc) {
    if (!xc) return;
    free(xc->values);
    free(xc);
}

int wc_xcorr_compute(WCCrossCorrelation* xc, const WCSignal* a, const WCSignal* b) {
    if (!xc || !a || !b) return -1;
    if (a->length == 0 || b->length == 0) return -1;
    int N = a->length;
    int M = b->length;
    xc->peak_value = 0.0;
    xc->peak_lag   = 0;
    for (int lag = -xc->max_lag; lag <= xc->max_lag; lag++) {
        double sum = 0.0;
        int count = 0;
        for (int n = 0; n < N; n++) {
            int m = n - lag;
            if (m >= 0 && m < M) {
                sum += a->samples[n] * b->samples[m];
                count++;
            }
        }
        double val = (count > 0) ? sum / (double)count : 0.0;
        xc->values[lag + xc->max_lag] = val;
        if (fabs(val) > fabs(xc->peak_value)) {
            xc->peak_value = val;
            xc->peak_lag   = lag;
        }
    }
    return 0;
}

/* ==============================================================
 * FREQUENCY-DOMAIN FILTER (L5)
 * ============================================================== */

WCFreqFilter* wc_freqfilter_create(int n_freq) {
    if (n_freq < 2) return NULL;
    WCFreqFilter* ff = (WCFreqFilter*)calloc(1, sizeof(WCFreqFilter));
    if (!ff) return NULL;
    ff->n_freq = n_freq;
    ff->H_mag   = (double*)calloc((size_t)n_freq, sizeof(double));
    ff->H_phase = (double*)calloc((size_t)n_freq, sizeof(double));
    if (!ff->H_mag || !ff->H_phase) { wc_freqfilter_free(ff); return NULL; }
    ff->omega_min = 0.0;
    ff->omega_max = WC_PI;
    return ff;
}

void wc_freqfilter_free(WCFreqFilter* ff) {
    if (!ff) return;
    free(ff->H_mag);
    free(ff->H_phase);
    free(ff);
}

int wc_freqfilter_wiener(WCFreqFilter* ff, const double* S_xx,
                          const double* S_xd, int n_freq) {
    if (!ff || !S_xx || !S_xd || n_freq != ff->n_freq) return -1;
    for (int k = 0; k < n_freq; k++) {
        double sxx = S_xx[k];
        double sxd = S_xd[k];
        if (sxx < WC_EPSILON) {
            ff->H_mag[k]   = 0.0;
            ff->H_phase[k] = 0.0;
        } else {
            double H_re = sxd / sxx;
            ff->H_mag[k]   = fabs(H_re);
            ff->H_phase[k] = (H_re < 0.0) ? WC_PI : 0.0;
        }
    }
    return 0;
}

WCSignal* wc_freqfilter_apply(const WCFreqFilter* ff, const WCSignal* sig) {
    /* Apply frequency-domain filter via DFT-IDFT pair */
    if (!ff || !sig) return NULL;
    int N = sig->length;
    WCSignal* out = wc_signal_create(N, sig->dt, sig->t0);
    if (!out) return NULL;

    /* Simplified: apply filter in frequency domain for each bin
     * that corresponds to DFT frequencies */
    double* re = (double*)calloc((size_t)N, sizeof(double));
    double* im = (double*)calloc((size_t)N, sizeof(double));
    if (!re || !im) { free(re); free(im); wc_signal_free(out); return NULL; }

    /* Forward DFT */
    for (int k = 0; k < N; k++) {
        double omega_k = 2.0 * WC_PI * k / N;
        for (int n = 0; n < N; n++) {
            re[k] += sig->samples[n] * cos(omega_k * n);
            im[k] -= sig->samples[n] * sin(omega_k * n);
        }
        /* Apply filter at this frequency */
        int f_idx = (int)((double)k / N * ff->n_freq);
        if (f_idx >= ff->n_freq) f_idx = ff->n_freq - 1;
        double mag   = ff->H_mag[f_idx];
        double phase = ff->H_phase[f_idx];
        double r = re[k], i = im[k];
        re[k] = mag * (r * cos(phase) - i * sin(phase));
        im[k] = mag * (r * sin(phase) + i * cos(phase));
    }

    /* Inverse DFT */
    for (int n = 0; n < N; n++) {
        double sum = 0.0;
        for (int k = 0; k < N; k++) {
            double omega_k = 2.0 * WC_PI * k * n / N;
            sum += re[k] * cos(omega_k) - im[k] * sin(omega_k);
        }
        out->samples[n] = sum / N;
    }

    free(re); free(im);
    return out;
}

/*
 * References:
 * Wiener (1949) Extrapolation, Interpolation, and Smoothing of
 *   Stationary Time Series. MIT Press. (WWII NDRC report, 1942)
 * Kolmogorov (1941) Interpolation and Extrapolation of Stationary
 *   Random Sequences. Izv. Akad. Nauk SSSR, Ser. Mat. 5:3-14.
 * Levinson (1947) The Wiener RMS error criterion in filter design
 *   and prediction. J. Math. Phys. 25:261-278.
 * Durbin (1960) The fitting of time-series models. Rev. Int. Stat.
 *   Inst. 28:233-244.
 * Welch (1967) The use of fast Fourier transform for the estimation
 *   of power spectra. IEEE Trans. Audio Electroacoust. 15:70-73.
 */
