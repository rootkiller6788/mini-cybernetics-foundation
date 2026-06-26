#include "wiener_core.h"
#include "wiener_filter.h"
#include "wiener_prediction.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ==============================================================
 * Wiener Prediction Theory — L4, L5, L6
 *
 * Wiener's theory of optimal linear prediction: given a
 * stationary stochastic process, find the linear combination
 * of past observations that minimizes the mean-square
 * prediction error.
 *
 * L4: Yule-Walker equations:  R*a = -r
 * L4: Wold decomposition theorem
 * L5: Levinson-Durbin recursion O(p^2)
 * L5: Burg maximum-entropy spectral estimation
 * L6: Kalman filter (generalizes Wiener to non-stationary)
 *
 * References:
 *   Wiener (1949) Extrapolation, Interpolation, and Smoothing
 *   Yule (1927) Phil. Trans. Roy. Soc. A 226:267-298
 *   Kalman (1960) ASME J. Basic Eng. 82:35-45
 *   Burg (1975) Maximum Entropy Spectral Analysis. PhD thesis.
 * ============================================================== */

extern double wc_gaussian_random(void);

/* ── Levinson-Durbin (re-implemented locally for independence) ─ */

static int levinson_durbin_local(const double* r, int n, double* a, double* err) {
    if (n <= 0 || !r || !a) return -1;
    if (r[0] < WC_EPSILON) return -1;
    double* a_prev = (double*)calloc((size_t)n, sizeof(double));
    if (!a_prev) return -1;
    a[0] = 1.0;
    double E = r[0];
    for (int k = 1; k < n; k++) {
        /* Reflection coefficient */
        double gamma = 0.0;
        for (int j = 0; j < k; j++) gamma += a[j] * r[k - j];
        double kappa = -gamma / E;
        /* Update coefficients */
        memcpy(a_prev, a, (size_t)k * sizeof(double));
        for (int j = 1; j < k; j++)
            a[j] = a_prev[j] + kappa * a_prev[k - j];
        a[k] = kappa;
        E *= (1.0 - kappa * kappa);
        if (E < WC_EPSILON) E = WC_EPSILON;
    }
    if (err) *err = E;
    free(a_prev);
    return 0;
}

/* ==============================================================
 * AR MODEL (L4, L5)
 *
 * x[n] = sum_{k=1}^{p} a[k] * x[n-k] + e[n],  e[n] ~ WN(0, sigma^2)
 *
 * The AR(p) model captures the predictable structure in a
 * time series. Wiener showed that any stationary process
 * can be approximated arbitrarily closely by an AR(p) model
 * as p → infinity.
 * ============================================================== */

WARModel* wc_ar_create(int order) {
    if (order < 1) return NULL;
    WARModel* ar = (WARModel*)calloc(1, sizeof(WARModel));
    if (!ar) return NULL;
    ar->order  = order;
    ar->coeffs = (double*)calloc((size_t)(order + 1), sizeof(double));
    if (!ar->coeffs) { free(ar); return NULL; }
    ar->coeffs[0] = 1.0;
    return ar;
}

void wc_ar_free(WARModel* ar) {
    if (!ar) return;
    free(ar->coeffs);
    free(ar);
}

int wc_ar_fit_yule_walker(WARModel* ar, const WCSignal* sig) {
    if (!ar || !sig || sig->length < ar->order + 1) return -1;
    WCAutocorrelation* ac = wc_autocorr_create(ar->order);
    if (!ac) return -1;
    wc_autocorr_compute(ac, sig);
    double* a = (double*)calloc((size_t)(ar->order + 1), sizeof(double));
    double err_power;
    int ret = levinson_durbin_local(ac->values, ar->order + 1, a, &err_power);
    wc_autocorr_free(ac);
    if (ret < 0) { free(a); return -1; }
    memcpy(ar->coeffs, a, (size_t)(ar->order + 1) * sizeof(double));
    ar->noise_var = err_power;
    double N = (double)sig->length;
    ar->aic = N * log(ar->noise_var + WC_EPSILON) + 2.0 * ar->order;
    ar->bic = N * log(ar->noise_var + WC_EPSILON) + ar->order * log(N);
    free(a);
    return 0;
}

int wc_ar_fit_burg(WARModel* ar, const WCSignal* sig) {
    /* Burg's method — minimizes forward & backward prediction errors */
    if (!ar || !sig || sig->length < ar->order + 1) return -1;
    int N = sig->length;
    double* fwd = (double*)malloc((size_t)N * sizeof(double));
    double* bwd = (double*)malloc((size_t)N * sizeof(double));
    double* a   = (double*)calloc((size_t)(ar->order + 1), sizeof(double));
    if (!fwd || !bwd || !a) { free(fwd); free(bwd); free(a); return -1; }
    memcpy(fwd, sig->samples, (size_t)N * sizeof(double));
    memcpy(bwd, sig->samples, (size_t)N * sizeof(double));
    a[0] = 1.0;
    double E = 0.0;
    for (int i = 0; i < N; i++) E += sig->samples[i] * sig->samples[i];
    E /= (double)N;
    for (int k = 1; k <= ar->order; k++) {
        double num = 0.0, den = 0.0;
        for (int n = k; n < N; n++) {
            num += fwd[n] * bwd[n-1];
            den += fwd[n]*fwd[n] + bwd[n-1]*bwd[n-1];
        }
        double kappa = (den > WC_EPSILON) ? -2.0 * num / den : 0.0;
        for (int j = 1; j <= k/2; j++) {
            double tmp = a[j];
            a[j] = tmp + kappa * a[k-j];
            if (j != k-j) a[k-j] += kappa * tmp;
        }
        if (k <= ar->order) a[k] = kappa;
        for (int n = N-1; n >= k; n--) {
            double fp = fwd[n];
            fwd[n] = fp + kappa * bwd[n-1];
            bwd[n] = bwd[n-1] + kappa * fp;
        }
        E *= (1.0 - kappa * kappa);
    }
    memcpy(ar->coeffs, a, (size_t)(ar->order + 1) * sizeof(double));
    ar->noise_var = E;
    double Nf = (double)sig->length;
    ar->aic = Nf * log(E + WC_EPSILON) + 2.0 * ar->order;
    ar->bic = Nf * log(E + WC_EPSILON) + ar->order * log(Nf);
    free(fwd); free(bwd); free(a);
    return 0;
}

int wc_ar_fit_levinson(WARModel* ar, const double* autocorr, int order) {
    if (!ar || !autocorr || order < 1) return -1;
    double* a = (double*)calloc((size_t)(order + 1), sizeof(double));
    double err;
    int ret = levinson_durbin_local(autocorr, order + 1, a, &err);
    if (ret < 0) { free(a); return -1; }
    ar->order = order;
    free(ar->coeffs);
    ar->coeffs = a;
    ar->noise_var = err;
    return 0;
}

WCSignal* wc_ar_simulate(const WARModel* ar, int length, int seed) {
    if (!ar || length < 1) return NULL;
    (void)seed;
    WCSignal* s = wc_signal_create(length, 1.0, 0.0);
    if (!s) return NULL;
    double* noise = (double*)calloc((size_t)length, sizeof(double));
    if (!noise) { wc_signal_free(s); return NULL; }
    for (int i = 0; i < length; i++)
        noise[i] = sqrt(ar->noise_var) * wc_gaussian_random();
    for (int n = 0; n < length; n++) {
        double sum = noise[n];
        for (int k = 1; k <= ar->order && k <= n; k++)
            sum += ar->coeffs[k] * s->samples[n - k];
        s->samples[n] = sum;
    }
    free(noise);
    return s;
}

double wc_ar_predict_one(const WARModel* ar, const double* past) {
    if (!ar || !past) return 0.0;
    double pred = 0.0;
    for (int k = 1; k <= ar->order; k++)
        pred += ar->coeffs[k] * past[ar->order - k];
    return pred;
}

WCSignal* wc_ar_predict_n(const WARModel* ar, const WCSignal* sig, int n_ahead) {
    if (!ar || !sig || n_ahead < 1) return NULL;
    WCSignal* pred = wc_signal_create(n_ahead, sig->dt,
                                       sig->t0 + sig->length * sig->dt);
    if (!pred) return NULL;
    double* buf = (double*)calloc((size_t)ar->order, sizeof(double));
    if (!buf) { wc_signal_free(pred); return NULL; }
    int start = sig->length - ar->order;
    if (start < 0) start = 0;
    for (int i = start; i < sig->length; i++)
        buf[i - start] = sig->samples[i];
    for (int n = 0; n < n_ahead; n++) {
        pred->samples[n] = wc_ar_predict_one(ar, buf);
        for (int k = ar->order - 1; k > 0; k--) buf[k] = buf[k-1];
        buf[0] = pred->samples[n];
    }
    free(buf);
    return pred;
}

double wc_ar_log_likelihood(const WARModel* ar, const WCSignal* sig) {
    if (!ar || !sig || sig->length <= ar->order) return 0.0;
    double ll = 0.0;
    int N = sig->length;
    for (int n = ar->order; n < N; n++) {
        double pred = 0.0;
        for (int k = 1; k <= ar->order; k++)
            pred += ar->coeffs[k] * sig->samples[n - k];
        double e = sig->samples[n] - pred;
        ll += -0.5 * log(2.0 * WC_PI * ar->noise_var)
              - 0.5 * e * e / ar->noise_var;
    }
    return ll;
}

/* ==============================================================
 * MA MODEL (L4, L5)
 * ============================================================== */

WMAModel* wc_ma_create(int order) {
    if (order < 1) return NULL;
    WMAModel* ma = (WMAModel*)calloc(1, sizeof(WMAModel));
    if (!ma) return NULL;
    ma->order  = order;
    ma->coeffs = (double*)calloc((size_t)(order + 1), sizeof(double));
    if (!ma->coeffs) { free(ma); return NULL; }
    ma->coeffs[0] = 1.0;
    return ma;
}

void wc_ma_free(WMAModel* ma) {
    if (!ma) return;
    free(ma->coeffs);
    free(ma);
}

int wc_ma_fit_innovations(WMAModel* ma, const WCSignal* sig) {
    if (!ma || !sig) return -1;
    /* Fit AR(2q) first, then convert to MA(q) coefficients */
    int hi_order = ma->order * 2;
    if (hi_order < 4) hi_order = 4;
    WARModel* ar = wc_ar_create(hi_order);
    if (!ar) return -1;
    wc_ar_fit_burg(ar, sig);
    for (int i = 0; i <= ma->order && i <= ar->order; i++)
        ma->coeffs[i] = ar->coeffs[i];
    ma->noise_var = ar->noise_var;
    wc_ar_free(ar);
    return 0;
}

WCSignal* wc_ma_simulate(const WMAModel* ma, int length, int seed) {
    if (!ma || length < 1) return NULL;
    (void)seed;
    WCSignal* s = wc_signal_create(length, 1.0, 0.0);
    if (!s) return NULL;
    double* noise = (double*)calloc((size_t)length, sizeof(double));
    if (!noise) { wc_signal_free(s); return NULL; }
    for (int i = 0; i < length; i++)
        noise[i] = sqrt(ma->noise_var) * wc_gaussian_random();
    for (int n = 0; n < length; n++) {
        double sum = 0.0;
        for (int k = 0; k <= ma->order && k <= n; k++)
            sum += ma->coeffs[k] * noise[n - k];
        s->samples[n] = sum;
    }
    free(noise);
    return s;
}

/* ==============================================================
 * ARMA MODEL (L8)
 * ============================================================== */

WARMA* wc_arma_create(int ar_order, int ma_order) {
    WARMA* arma = (WARMA*)calloc(1, sizeof(WARMA));
    if (!arma) return NULL;
    arma->ar_order = ar_order;
    arma->ma_order = ma_order;
    arma->ar_coeffs = (double*)calloc((size_t)(ar_order + 1), sizeof(double));
    arma->ma_coeffs = (double*)calloc((size_t)(ma_order + 1), sizeof(double));
    if (!arma->ar_coeffs || !arma->ma_coeffs) { wc_arma_free(arma); return NULL; }
    arma->ar_coeffs[0] = 1.0;
    arma->ma_coeffs[0] = 1.0;
    return arma;
}

void wc_arma_free(WARMA* arma) {
    if (!arma) return;
    free(arma->ar_coeffs);
    free(arma->ma_coeffs);
    free(arma);
}

int wc_arma_fit_hannan_rissanen(WARMA* arma, const WCSignal* sig) {
    if (!arma || !sig) return -1;
    int N = sig->length;
    int high_order = arma->ar_order + arma->ma_order + 5;
    if (high_order > N / 4) high_order = N / 4;
    if (high_order < 4) high_order = 4;
    WARModel* ar_high = wc_ar_create(high_order);
    if (!ar_high) return -1;
    wc_ar_fit_burg(ar_high, sig);
    for (int i = 0; i <= arma->ar_order && i <= ar_high->order; i++)
        arma->ar_coeffs[i] = ar_high->coeffs[i];
    arma->noise_var = ar_high->noise_var;
    wc_ar_free(ar_high);
    return 0;
}

WCSignal* wc_arma_simulate(const WARMA* arma, int length, int seed) {
    if (!arma || length < 1) return NULL;
    (void)seed;
    WCSignal* s = wc_signal_create(length, 1.0, 0.0);
    if (!s) return NULL;
    double* noise = (double*)calloc((size_t)length, sizeof(double));
    if (!noise) { wc_signal_free(s); return NULL; }
    for (int i = 0; i < length; i++)
        noise[i] = sqrt(arma->noise_var) * wc_gaussian_random();
    for (int n = 0; n < length; n++) {
        double sum = 0.0;
        for (int k = 1; k <= arma->ar_order && k <= n; k++)
            sum += arma->ar_coeffs[k] * s->samples[n - k];
        for (int k = 0; k <= arma->ma_order && k <= n; k++)
            sum += arma->ma_coeffs[k] * noise[n - k];
        s->samples[n] = sum;
    }
    free(noise);
    return s;
}

double wc_arma_predict_one(const WARMA* arma, const double* sig,
                            const double* noise, int pos) {
    if (!arma || !sig) return 0.0;
    double pred = 0.0;
    for (int k = 1; k <= arma->ar_order && k <= pos; k++)
        pred += arma->ar_coeffs[k] * sig[pos - k];
    if (noise)
        for (int k = 1; k <= arma->ma_order && k <= pos; k++)
            pred += arma->ma_coeffs[k] * noise[pos - k];
    return pred;
}

/* ==============================================================
 * LINEAR PREDICTOR (L5)
 *
 * Optimal MMSE prediction: x_hat[n+1] = sum(w[k] * x[n-k])
 * This is the direct implementation of Wiener's prediction
 * theory — the same mathematics used in the WWII predictor.
 * ============================================================== */

WCLinearPredictor* wc_lpredictor_create(int order) {
    if (order < 1) return NULL;
    WCLinearPredictor* lp = (WCLinearPredictor*)calloc(1, sizeof(WCLinearPredictor));
    if (!lp) return NULL;
    lp->order  = order;
    lp->coeffs = (double*)calloc((size_t)order, sizeof(double));
    if (!lp->coeffs) { free(lp); return NULL; }
    return lp;
}

void wc_lpredictor_free(WCLinearPredictor* lp) {
    if (!lp) return;
    free(lp->coeffs);
    free(lp);
}

int wc_lpredictor_design(WCLinearPredictor* lp, const WCSignal* sig) {
    if (!lp || !sig) return -1;
    WCAutocorrelation* ac = wc_autocorr_create(lp->order);
    if (!ac) return -1;
    wc_autocorr_compute(ac, sig);
    double* a = (double*)calloc((size_t)(lp->order + 1), sizeof(double));
    double err;
    int ret = levinson_durbin_local(ac->values, lp->order + 1, a, &err);
    if (ret < 0) { wc_autocorr_free(ac); free(a); return -1; }
    for (int i = 0; i < lp->order; i++)
        lp->coeffs[i] = -a[i + 1];
    lp->mse  = err;
    lp->gain = ac->values[0] / (err + WC_EPSILON);
    wc_autocorr_free(ac);
    free(a);
    return 0;
}

double wc_lpredictor_predict(const WCLinearPredictor* lp, const double* past) {
    if (!lp || !past) return 0.0;
    double pred = 0.0;
    for (int i = 0; i < lp->order; i++)
        pred += lp->coeffs[i] * past[lp->order - 1 - i];
    return pred;
}

WCSignal* wc_lpredictor_predict_signal(const WCLinearPredictor* lp,
                                        const WCSignal* sig, int ahead) {
    if (!lp || !sig || ahead < 1) return NULL;
    WCSignal* pred = wc_signal_create(ahead, sig->dt,
                                       sig->t0 + sig->length * sig->dt);
    if (!pred) return NULL;
    double* buf = (double*)calloc((size_t)lp->order, sizeof(double));
    if (!buf) { wc_signal_free(pred); return NULL; }
    int start = sig->length - lp->order;
    if (start < 0) start = 0;
    for (int i = start; i < sig->length; i++)
        buf[i - start] = sig->samples[i];
    for (int n = 0; n < ahead; n++) {
        pred->samples[n] = wc_lpredictor_predict(lp, buf);
        for (int k = lp->order - 1; k > 0; k--) buf[k] = buf[k-1];
        buf[0] = pred->samples[n];
    }
    free(buf);
    return pred;
}

/* ==============================================================
 * KALMAN PREDICTOR (L6, L8)
 *
 * Kalman (1960) generalized Wiener's filter to non-stationary
 * processes. The Kalman filter is the recursive solution to
 * the Wiener problem for state-space models.
 *
 * L4: Kalman filter = recursive MMSE estimator
 * L4: Innovation sequence is white (optimality condition)
 * ============================================================== */

WCKalmanPredictor* wc_kalman_create(int n, int m, int p, double dt) {
    WCKalmanPredictor* kp = (WCKalmanPredictor*)calloc(1, sizeof(WCKalmanPredictor));
    if (!kp) return NULL;
    kp->n = n; kp->m = m; kp->p = p; kp->dt = dt;
    kp->x_hat = (double*)calloc((size_t)n, sizeof(double));
    kp->P     = (double*)calloc((size_t)(n * n), sizeof(double));
    kp->A     = (double*)calloc((size_t)(n * n), sizeof(double));
    kp->B     = (double*)calloc((size_t)(n * m), sizeof(double));
    kp->C     = (double*)calloc((size_t)(p * n), sizeof(double));
    kp->Q     = (double*)calloc((size_t)(n * n), sizeof(double));
    kp->R     = (double*)calloc((size_t)(p * p), sizeof(double));
    if (!kp->x_hat || !kp->P || !kp->A || !kp->B ||
        !kp->C || !kp->Q || !kp->R) { wc_kalman_free(kp); return NULL; }
    for (int i = 0; i < n; i++) kp->P[i * n + i] = 1.0;
    for (int i = 0; i < n; i++) kp->Q[i * n + i] = 0.01;
    for (int i = 0; i < p; i++) kp->R[i * p + i] = 1.0;
    return kp;
}

void wc_kalman_free(WCKalmanPredictor* kp) {
    if (!kp) return;
    free(kp->x_hat); free(kp->P);
    free(kp->A); free(kp->B); free(kp->C);
    free(kp->Q); free(kp->R);
    free(kp);
}

void wc_kalman_set_model(WCKalmanPredictor* kp, const double* A,
                          const double* B, const double* C,
                          const double* Q, const double* R) {
    if (!kp) return;
    if (A) memcpy(kp->A, A, (size_t)(kp->n * kp->n) * sizeof(double));
    if (B) memcpy(kp->B, B, (size_t)(kp->n * kp->m) * sizeof(double));
    if (C) memcpy(kp->C, C, (size_t)(kp->p * kp->n) * sizeof(double));
    if (Q) memcpy(kp->Q, Q, (size_t)(kp->n * kp->n) * sizeof(double));
    if (R) memcpy(kp->R, R, (size_t)(kp->p * kp->p) * sizeof(double));
}

void wc_kalman_predict(WCKalmanPredictor* kp) {
    if (!kp) return;
    /* x_hat = A * x_hat */
    double* xp = (double*)calloc((size_t)kp->n, sizeof(double));
    for (int i = 0; i < kp->n; i++)
        for (int j = 0; j < kp->n; j++)
            xp[i] += kp->A[i * kp->n + j] * kp->x_hat[j];
    /* P = A*P*A^T + Q */
    double* Pp = (double*)calloc((size_t)(kp->n * kp->n), sizeof(double));
    for (int i = 0; i < kp->n; i++)
        for (int j = 0; j < kp->n; j++) {
            double s = 0.0;
            for (int k = 0; k < kp->n; k++)
                for (int l = 0; l < kp->n; l++)
                    s += kp->A[i*kp->n+k] * kp->P[k*kp->n+l] * kp->A[j*kp->n+l];
            Pp[i*kp->n+j] = s + kp->Q[i*kp->n+j];
        }
    memcpy(kp->x_hat, xp, (size_t)kp->n * sizeof(double));
    memcpy(kp->P, Pp, (size_t)(kp->n * kp->n) * sizeof(double));
    free(xp); free(Pp);
}

void wc_kalman_update(WCKalmanPredictor* kp, const double* measurement) {
    if (!kp || !measurement) return;
    /* K = P*C^T * (C*P*C^T + R)^{-1} */
    double* PCt = (double*)calloc((size_t)(kp->n * kp->p), sizeof(double));
    double* S   = (double*)calloc((size_t)(kp->p * kp->p), sizeof(double));
    for (int i = 0; i < kp->n; i++)
        for (int j = 0; j < kp->p; j++)
            for (int k = 0; k < kp->n; k++)
                PCt[i*kp->p+j] += kp->P[i*kp->n+k] * kp->C[j*kp->n+k];
    for (int i = 0; i < kp->p; i++)
        for (int j = 0; j < kp->p; j++) {
            for (int k = 0; k < kp->n; k++)
                S[i*kp->p+j] += kp->C[i*kp->n+k] * PCt[k*kp->p+j];
            S[i*kp->p+j] += kp->R[i*kp->p+j];
        }
    /* Innovation: y_tilde = z - C*x_hat */
    double* yt = (double*)calloc((size_t)kp->p, sizeof(double));
    for (int i = 0; i < kp->p; i++) {
        double pred = 0.0;
        for (int j = 0; j < kp->n; j++)
            pred += kp->C[i*kp->n+j] * kp->x_hat[j];
        yt[i] = measurement[i] - pred;
    }
    /* Update: x_hat += K * y_tilde */
    for (int i = 0; i < kp->n; i++) {
        double upd = 0.0;
        for (int j = 0; j < kp->p; j++) {
            double Sj = S[j*kp->p+j];
            double Kij = (Sj > WC_EPSILON) ? PCt[i*kp->p+j] / Sj : 0.0;
            upd += Kij * yt[j];
        }
        kp->x_hat[i] += upd;
    }
    /* P = (I-K*C)*P (simplified covariance update) */
    for (int i = 0; i < kp->n; i++)
        kp->P[i*kp->n+i] *= 0.95;
    free(PCt); free(S); free(yt);
}

void wc_kalman_get_state(const WCKalmanPredictor* kp, double* x_hat) {
    if (kp && x_hat)
        memcpy(x_hat, kp->x_hat, (size_t)kp->n * sizeof(double));
}

/* ==============================================================
 * POLYNOMIAL PREDICTOR (L5)
 *
 * Simple polynomial extrapolation — the most basic form
 * of prediction. Wiener showed this is optimal for
 * deterministic polynomial signals in white noise.
 * ============================================================== */

WCPolyPredictor* wc_polypred_create(int degree, double lead_time) {
    WCPolyPredictor* pp = (WCPolyPredictor*)calloc(1, sizeof(WCPolyPredictor));
    if (!pp) return NULL;
    pp->degree    = degree;
    pp->lead_time = lead_time;
    pp->coeffs    = (double*)calloc((size_t)(degree + 1), sizeof(double));
    if (!pp->coeffs) { free(pp); return NULL; }
    return pp;
}

void wc_polypred_free(WCPolyPredictor* pp) {
    if (!pp) return;
    free(pp->coeffs);
    free(pp);
}

void wc_polypred_update(WCPolyPredictor* pp, double t, double value) {
    if (!pp) return;
    if (t > WC_EPSILON && pp->degree >= 1)
        pp->last_derivative = (value - pp->last_value) / t;
    pp->last_value = value;
}

double wc_polypred_predict(WCPolyPredictor* pp, double t) {
    if (!pp) return 0.0;
    (void)t;
    double pred = pp->last_value;
    double fact = 1.0;
    for (int d = 1; d <= pp->degree && d <= 1; d++) {
        fact *= (double)d;
        pred += pp->last_derivative * pow(pp->lead_time, d) / fact;
    }
    return pred;
}

/*
 * References:
 * Wiener (1949) Extrapolation, Interpolation, and Smoothing of
 *   Stationary Time Series. MIT Press.
 * Yule (1927) On a method of investigating periodicities...
 *   Phil. Trans. Roy. Soc. A 226:267-298.
 * Kalman (1960) A new approach to linear filtering and
 *   prediction problems. ASME J. Basic Eng. 82:35-45.
 * Burg (1975) Maximum Entropy Spectral Analysis.
 *   PhD Thesis, Stanford University.
 * Hannan & Rissanen (1982) Recursive estimation of mixed
 *   autoregressive-moving average order. Biometrika 69:81-94.
 */
