#ifndef WIENER_FILTER_H
#define WIENER_FILTER_H
#include "wiener_core.h"

/* Wiener Filter -- Optimal Linear Estimation
 * Based on: Wiener (1949) Extrapolation, Interpolation,
 *   and Smoothing of Stationary Time Series
 *   Kolmogorov (1941) Interpolation and Extrapolation
 *
 * L4: Wiener-Hopf equation: R_xx * h = r_xd
 * L4: Wiener-Khinchin: H(w) = S_xd(w) / S_xx(w)
 */

typedef struct {
    double* coeffs;
    int     order;
    double  mse;
    bool    is_causal;
} WCWienerFilter;

typedef struct {
    double* values;
    int     max_lag;
    int     signal_length;
    double  power;
} WCAutocorrelation;

typedef struct {
    double* freq;
    double* psd;
    int     n_bins;
    double  df;
    double  total_power;
} WCPowerSpectrum;

typedef struct {
    double* values;
    int     max_lag;
    double  peak_value;
    int     peak_lag;
} WCCrossCorrelation;

typedef struct {
    double* H_mag;
    double* H_phase;
    int     n_freq;
    double  omega_min;
    double  omega_max;
} WCFreqFilter;

/* Wiener Filter API */
WCWienerFilter* wc_filter_create(int order);
void      wc_filter_free(WCWienerFilter* wf);
int       wc_filter_design_fir(WCWienerFilter* wf, const WCSignal* input, const WCSignal* desired, int order);
int       wc_filter_design_freq(WCWienerFilter* wf, const double* S_xx, const double* S_xd, int n_freq);
int       wc_filter_design_wiener_hopf(WCWienerFilter* wf, const double* autocorr, const double* crosscorr, int order);
WCSignal* wc_filter_apply(const WCWienerFilter* wf, const WCSignal* input);
double    wc_filter_mse(const WCWienerFilter* wf, const WCSignal* input, const WCSignal* desired);
void      wc_filter_print(const WCWienerFilter* wf);

/* Autocorrelation API */
WCAutocorrelation* wc_autocorr_create(int max_lag);
void      wc_autocorr_free(WCAutocorrelation* ac);
int       wc_autocorr_compute(WCAutocorrelation* ac, const WCSignal* sig);
int       wc_autocorr_compute_unbiased(WCAutocorrelation* ac, const WCSignal* sig);
int       wc_autocorr_compute_biased(WCAutocorrelation* ac, const WCSignal* sig);
double    wc_autocorr_get(const WCAutocorrelation* ac, int lag);
void      wc_autocorr_to_toeplitz(const WCAutocorrelation* ac, double* matrix, int dim);
void      wc_autocorr_print(const WCAutocorrelation* ac);

/* Power Spectrum API */
WCPowerSpectrum* wc_psd_create(int n_bins, double df);
void      wc_psd_free(WCPowerSpectrum* ps);
int       wc_psd_periodogram(WCPowerSpectrum* ps, const WCSignal* sig);
int       wc_psd_welch(WCPowerSpectrum* ps, const WCSignal* sig, int window_size, int overlap);
int       wc_psd_from_autocorr(WCPowerSpectrum* ps, const WCAutocorrelation* ac);
double    wc_psd_max_freq(const WCPowerSpectrum* ps);
double    wc_psd_bandwidth(const WCPowerSpectrum* ps);

/* Cross-Correlation API */
WCCrossCorrelation* wc_xcorr_create(int max_lag);
void      wc_xcorr_free(WCCrossCorrelation* xc);
int       wc_xcorr_compute(WCCrossCorrelation* xc, const WCSignal* a, const WCSignal* b);

/* Frequency-Domain Filter API */
WCFreqFilter* wc_freqfilter_create(int n_freq);
void      wc_freqfilter_free(WCFreqFilter* ff);
int       wc_freqfilter_wiener(WCFreqFilter* ff, const double* S_xx, const double* S_xd, int n_freq);
WCSignal* wc_freqfilter_apply(const WCFreqFilter* ff, const WCSignal* sig);

#endif
