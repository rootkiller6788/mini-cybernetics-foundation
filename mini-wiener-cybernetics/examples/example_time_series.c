#include "wiener_core.h"
#include "wiener_filter.h"
#include "wiener_prediction.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Example: Wiener Time Series Prediction
 *
 * Demonstrates Wiener's optimal linear prediction theory
 * applied to time series forecasting. Uses AR modeling
 * and the Levinson-Durbin algorithm — the same mathematics
 * Wiener developed for anti-aircraft prediction in WWII.
 *
 * L6: Time series prediction is the canonical problem that
 *     motivated Wiener's cybernetics theory.
 */

int main(void) {
    printf("=== Wiener Time Series Prediction ===\n\n");

    /* Generate synthetic AR(2) process:
     * x[n] = 0.7*x[n-1] - 0.2*x[n-2] + e[n], e[n]~N(0,0.25) */
    int N = 300;
    double* data = (double*)malloc(N * sizeof(double));
    data[0] = 0.0; data[1] = 0.0;
    for (int i = 2; i < N; i++)
        data[i] = 0.7*data[i-1] - 0.2*data[i-2] + 0.5*wc_gaussian_random();

    WCSignal* sig = wc_signal_create_from(data, N, 1.0, 0.0);
    printf("Generated AR(2) process: mean=%.4f var=%.4f\n",
           wc_signal_mean(sig), wc_signal_variance(sig));

    /* Part 1: Autocorrelation Analysis */
    printf("\n--- Part 1: Autocorrelation ---\n");
    WCAutocorrelation* ac = wc_autocorr_create(10);
    wc_autocorr_compute(ac, sig);
    wc_autocorr_print(ac);
    printf("The autocorrelation reveals the process memory.\n");

    /* Part 2: AR Model Fitting via Yule-Walker */
    printf("\n--- Part 2: AR(2) Model (Yule-Walker) ---\n");
    WARModel* ar = wc_ar_create(2);
    int ret = wc_ar_fit_yule_walker(ar, sig);
    if (ret == 0) {
        printf("AR coefficients: a1=%.4f a2=%.4f (true: 0.7, -0.2)\n",
               ar->coeffs[1], ar->coeffs[2]);
        printf("Innovation variance: %.4f (true: 0.25)\n", ar->noise_var);
        printf("AIC=%.2f BIC=%.2f\n", ar->aic, ar->bic);
    }

    /* Part 3: Burg's Method */
    printf("\n--- Part 3: AR(2) Model (Burg's Method) ---\n");
    WARModel* ar_burg = wc_ar_create(2);
    wc_ar_fit_burg(ar_burg, sig);
    printf("AR coefficients (Burg): a1=%.4f a2=%.4f\n",
           ar_burg->coeffs[1], ar_burg->coeffs[2]);
    printf("Innovation variance: %.4f\n", ar_burg->noise_var);

    /* Part 4: k-step ahead prediction */
    printf("\n--- Part 4: Multi-step Prediction ---\n");
    int ahead = 10;
    WCSignal* pred = wc_ar_predict_n(ar, sig, ahead);
    printf("Predicted next %d values:\n", ahead);
    printf("Step  Prediction\n");
    printf("----  ----------\n");
    for (int i = 0; i < ahead; i++)
        printf("%3d   %10.4f\n", i+1, pred->samples[i]);

    /* Part 5: Linear Predictor */
    printf("\n--- Part 5: Optimal Linear Predictor ---\n");
    WCLinearPredictor* lp = wc_lpredictor_create(4);
    wc_lpredictor_design(lp, sig);
    printf("Prediction order: %d\n", lp->order);
    printf("Prediction MSE: %.6f\n", lp->mse);
    printf("Prediction gain: %.2f (= signal_power/MSE, >1 means successful)\n",
           lp->gain);

    /* Test predictor on last few known values */
    printf("True vs predicted (last 5 values):\n");
    for (int i = N-5; i < N; i++) {
        double pred_val = wc_lpredictor_predict(lp, &data[i-lp->order]);
        printf("  t=%d: true=%.4f pred=%.4f err=%.4f\n",
               i, data[i], pred_val, data[i]-pred_val);
    }

    /* Part 6: Model selection via AIC/BIC */
    printf("\n--- Part 6: Model Order Selection ---\n");
    printf("Order  AIC       BIC       NoiseVar\n");
    printf("-----  --------  --------  --------\n");
    for (int p = 1; p <= 5; p++) {
        WARModel* ar_p = wc_ar_create(p);
        wc_ar_fit_burg(ar_p, sig);
        printf("%3d    %8.2f  %8.2f  %8.4f\n",
               p, ar_p->aic, ar_p->bic, ar_p->noise_var);
        wc_ar_free(ar_p);
    }

    /* Part 7: Power spectrum of the process */
    printf("\n--- Part 7: Power Spectrum ---\n");
    WCPowerSpectrum* ps = wc_psd_create(64, 0.01);
    wc_psd_welch(ps, sig, 64, 32);
    printf("Total power: %.4f\n", ps->total_power);
    printf("Peak frequency: %.4f Hz\n", wc_psd_max_freq(ps));
    printf("Half-power bandwidth: %.4f Hz\n", wc_psd_bandwidth(ps));

    wc_signal_free(sig); wc_signal_free(pred);
    wc_autocorr_free(ac); wc_psd_free(ps);
    wc_ar_free(ar); wc_ar_free(ar_burg);
    wc_lpredictor_free(lp);
    free(data);

    printf("\nDone.\n");
    return 0;
}
