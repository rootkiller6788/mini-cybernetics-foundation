#include "wiener_core.h"
#include "wiener_filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Example: Wiener Filter for Signal Denoising
 *
 * Demonstrates the classic Wiener filtering problem:
 * recover a clean sinusoidal signal from noisy measurements.
 * This is the original problem Wiener solved during WWII
 * for anti-aircraft fire control radar signals.
 *
 * L6: Signal denoising is a canonical cybernetics problem
 */

int main(void) {
    printf("=== Wiener Filter: Signal Denoising ===\n\n");

    int N = 200;
    double* clean_data = (double*)malloc(N * sizeof(double));
    double* noisy_data = (double*)malloc(N * sizeof(double));
    for (int i = 0; i < N; i++) {
        clean_data[i] = sin(2.0 * WC_PI * 0.025 * i)
                      + 0.5 * sin(2.0 * WC_PI * 0.05 * i);
    }
    /* Add Gaussian noise */
    for (int i = 0; i < N; i++)
        noisy_data[i] = clean_data[i] + 0.4 * wc_gaussian_random();

    WCSignal* clean = wc_signal_create_from(clean_data, N, 1.0, 0.0);
    WCSignal* noisy = wc_signal_create_from(noisy_data, N, 1.0, 0.0);

    printf("Clean signal: mean=%.4f rms=%.4f energy=%.4f\n",
           wc_signal_mean(clean), wc_signal_rms(clean), wc_signal_energy(clean));
    printf("Noisy signal: mean=%.4f rms=%.4f energy=%.4f\n",
           wc_signal_mean(noisy), wc_signal_rms(noisy), wc_signal_energy(noisy));

    /* Design Wiener filter */
    WCWienerFilter* wf = wc_filter_create(8);
    int ret = wc_filter_design_fir(wf, noisy, clean, 8);
    if (ret < 0) {
        printf("Filter design failed!\n");
    } else {
        wc_filter_print(wf);
        WCSignal* filtered = wc_filter_apply(wf, noisy);
        printf("Filtered signal: mean=%.4f rms=%.4f energy=%.4f\n",
               wc_signal_mean(filtered), wc_signal_rms(filtered),
               wc_signal_energy(filtered));

        /* Compare MSE before and after */
        double mse_noisy = 0.0, mse_filtered = 0.0;
        for (int i = 0; i < N; i++) {
            double e1 = noisy->samples[i] - clean->samples[i];
            double e2 = filtered->samples[i] - clean->samples[i];
            mse_noisy += e1 * e1;
            mse_filtered += e2 * e2;
        }
        mse_noisy /= N;
        mse_filtered /= N;
        printf("\nMSE before filtering: %.6f\n", mse_noisy);
        printf("MSE after filtering:  %.6f\n", mse_filtered);
        printf("Improvement factor:   %.2fx\n",
               mse_noisy / (mse_filtered + 1e-12));
        wc_signal_free(filtered);
    }

    /* Frequency domain approach */
    printf("\n--- Frequency-Domain Wiener Filter ---\n");
    WCAutocorrelation* ac = wc_autocorr_create(16);
    wc_autocorr_compute(ac, noisy);
    WCPowerSpectrum* ps = wc_psd_create(32, 0.01);
    wc_psd_from_autocorr(ps, ac);
    printf("Signal power: %.4f, Max freq: %.4f Hz\n",
           ps->total_power, wc_psd_max_freq(ps));

    WCFreqFilter* ff = wc_freqfilter_create(32);
    /* Design freq-domain Wiener: H = S_clean/S_noisy ≈ S_signal/S_noisy */
    double S_xx[32], S_xd[32];
    for (int k = 0; k < 32; k++) {
        S_xx[k] = ps->psd[k] + 0.16; /* noisy spectrum = signal + noise */
        S_xd[k] = ps->psd[k];        /* cross-spectrum = signal spectrum */
    }
    wc_freqfilter_wiener(ff, S_xx, S_xd, 32);
    WCSignal* ff_out = wc_freqfilter_apply(ff, noisy);
    printf("Freq-filtered rms: %.4f\n", wc_signal_rms(ff_out));

    wc_signal_free(clean); wc_signal_free(noisy);
    wc_signal_free(ff_out);
    wc_filter_free(wf);
    wc_freqfilter_free(ff);
    wc_autocorr_free(ac); wc_psd_free(ps);
    free(clean_data); free(noisy_data);

    printf("\nDone.\n");
    return 0;
}
