#include "wiener_core.h"
#include "wiener_process.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ==============================================================
 * Wiener Process & Brownian Motion Implementation
 *
 * L3: Mathematical structure of the Wiener process W(t):
 *     1. W(0) = 0 almost surely
 *     2. Independent increments: W(t) - W(s) ~ N(0, t-s)
 *     3. Continuous paths (almost surely)
 *     4. E[W(t)] = 0, Var[W(t)] = t
 *
 * L4: Quadratic variation: [W,W]_t = t (Levy's characterization)
 * L4: Wiener measure on C[0,T] is a Gaussian measure
 *
 * Wiener's use of Brownian motion was foundational to his
 * theory of prediction and filtering in cybernetic systems.
 *
 * References:
 *   Wiener (1923) "Differential-Space" — J. Math. Phys.
 *   Einstein (1905) "On the Motion of Small Particles..."
 *   Ito (1944) "Stochastic Integral" — Proc. Imp. Acad. Tokyo
 * ============================================================== */

/* ── External RNG dependency on core ────────────────── */

extern double wc_gaussian_random(void);

/* ==============================================================
 * WIENER PATH (L3)
 * ============================================================== */

WCWienerPath* wc_wiener_path_create(int n_points, double t_end, int seed) {
    if (n_points < 2 || t_end <= 0.0) return NULL;
    WCWienerPath* wp = (WCWienerPath*)calloc(1, sizeof(WCWienerPath));
    if (!wp) return NULL;
    wp->n_points = n_points;
    wp->t_end    = t_end;
    wp->dt       = t_end / (double)(n_points - 1);
    wp->seed     = seed;
    wp->drift    = 0.0;
    wp->diffusion = 1.0;
    wp->path     = (double*)calloc((size_t)n_points, sizeof(double));
    wp->times    = (double*)calloc((size_t)n_points, sizeof(double));
    if (!wp->path || !wp->times) { wc_wiener_path_free(wp); return NULL; }
    for (int i = 0; i < n_points; i++) wp->times[i] = wp->dt * (double)i;
    return wp;
}

void wc_wiener_path_free(WCWienerPath* wp) {
    if (!wp) return;
    free(wp->path);
    free(wp->times);
    free(wp);
}

void wc_wiener_path_generate(WCWienerPath* wp) {
    if (!wp) return;
    wp->drift    = 0.0;
    wp->diffusion = 1.0;
    /* Seed via core RNG area — use simple LCG */
    static unsigned int seed_state = 0;
    if (wp->seed != 0) seed_state = (unsigned int)wp->seed;
    wp->path[0] = 0.0;
    for (int i = 1; i < wp->n_points; i++) {
        /* W(t+dt) = W(t) + sqrt(dt) * Z,  Z ~ N(0,1) */
        double increment = sqrt(wp->dt) * wc_gaussian_random();
        wp->path[i] = wp->path[i-1] + wp->drift * wp->dt
                    + wp->diffusion * increment;
    }
}

void wc_wiener_path_generate_drifted(WCWienerPath* wp, double drift, double diff) {
    if (!wp) return;
    wp->drift    = drift;
    wp->diffusion = diff;
    wp->path[0] = 0.0;
    for (int i = 1; i < wp->n_points; i++) {
        double increment = diff * sqrt(wp->dt) * wc_gaussian_random();
        wp->path[i] = wp->path[i-1] + drift * wp->dt + increment;
    }
}

double wc_wiener_path_max(const WCWienerPath* wp) {
    if (!wp) return 0.0;
    double m = wp->path[0];
    for (int i = 1; i < wp->n_points; i++)
        if (wp->path[i] > m) m = wp->path[i];
    return m;
}

double wc_wiener_path_min(const WCWienerPath* wp) {
    if (!wp) return 0.0;
    double m = wp->path[0];
    for (int i = 1; i < wp->n_points; i++)
        if (wp->path[i] < m) m = wp->path[i];
    return m;
}

double wc_wiener_path_terminal(const WCWienerPath* wp) {
    if (!wp || wp->n_points == 0) return 0.0;
    return wp->path[wp->n_points - 1];
}

double wc_wiener_path_quadratic_variation(const WCWienerPath* wp) {
    /* L4: Quadratic variation sum (dW)^2 ~ dt * N → dt * N → T */
    if (!wp || wp->n_points < 2) return 0.0;
    double qv = 0.0;
    for (int i = 1; i < wp->n_points; i++) {
        double dw = wp->path[i] - wp->path[i-1];
        qv += dw * dw;
    }
    return qv;
}

double wc_wiener_path_first_hit(const WCWienerPath* wp, double barrier) {
    /* First hitting time: find first index where |W(t)| >= barrier */
    if (!wp) return -1.0;
    for (int i = 0; i < wp->n_points; i++) {
        if (fabs(wp->path[i]) >= barrier)
            return wp->times[i];
    }
    return -1.0;  /* Never hit */
}

void wc_wiener_path_print(const WCWienerPath* wp, const char* label) {
    if (!wp) { printf("%s: NULL\n", label); return; }
    printf("%s: n=%d T=%.2f drift=%.3f diff=%.3f\n",
           label, wp->n_points, wp->t_end, wp->drift, wp->diffusion);
    printf("  terminal=%.4f max=%.4f min=%.4f qvar=%.4f\n",
           wc_wiener_path_terminal(wp),
           wc_wiener_path_max(wp),
           wc_wiener_path_min(wp),
           wc_wiener_path_quadratic_variation(wp));
}

/* ==============================================================
 * WIENER ENSEMBLE STATISTICS (L3, L4)
 *
 * L4: E[W(t)] = 0, Var[W(t)] = t
 * L4: The ensemble verifies the theoretical properties
 *     of the Wiener measure.
 * ============================================================== */

WCWienerEnsemble* wc_wiener_ensemble_create(int n_times, int n_paths,
                                             double t_end, int seed) {
    if (n_times < 2 || n_paths < 1) return NULL;
    WCWienerEnsemble* we = (WCWienerEnsemble*)calloc(1, sizeof(WCWienerEnsemble));
    if (!we) return NULL;
    we->n_times  = n_times;
    we->n_paths  = n_paths;
    we->mean_path = (double*)calloc((size_t)n_times, sizeof(double));
    we->var_path  = (double*)calloc((size_t)n_times, sizeof(double));
    we->skewness  = (double*)calloc((size_t)n_times, sizeof(double));
    we->kurtosis  = (double*)calloc((size_t)n_times, sizeof(double));
    if (!we->mean_path || !we->var_path || !we->skewness || !we->kurtosis) {
        wc_wiener_ensemble_free(we); return NULL;
    }
    return we;
}

void wc_wiener_ensemble_free(WCWienerEnsemble* we) {
    if (!we) return;
    free(we->mean_path);
    free(we->var_path);
    free(we->skewness);
    free(we->kurtosis);
    free(we);
}

void wc_wiener_ensemble_generate(WCWienerEnsemble* we) {
    if (!we) return;
    /* Reset accumulators */
    memset(we->mean_path, 0, (size_t)we->n_times * sizeof(double));
    memset(we->var_path,  0, (size_t)we->n_times * sizeof(double));
    memset(we->skewness,  0, (size_t)we->n_times * sizeof(double));
    memset(we->kurtosis,  0, (size_t)we->n_times * sizeof(double));

    double dt = 1.0 / (double)(we->n_times - 1);
    double* path_i = (double*)calloc((size_t)we->n_times, sizeof(double));
    double* path_i2 = (double*)calloc((size_t)we->n_times, sizeof(double));
    double* path_i3 = (double*)calloc((size_t)we->n_times, sizeof(double));
    double* path_i4 = (double*)calloc((size_t)we->n_times, sizeof(double));
    if (!path_i || !path_i2 || !path_i3 || !path_i4) {
        free(path_i); free(path_i2); free(path_i3); free(path_i4);
        return;
    }

    for (int p = 0; p < we->n_paths; p++) {
        double W = 0.0;
        for (int t = 0; t < we->n_times; t++) {
            if (t == 0) {
                path_i[t]  = 0.0;
                path_i2[t] = 0.0;
                path_i3[t] = 0.0;
                path_i4[t] = 0.0;
                continue;
            }
            W += sqrt(dt) * wc_gaussian_random();
            path_i[t]  = W;
            path_i2[t] = W * W;
            path_i3[t] = W * W * W;
            path_i4[t] = W * W * W * W;
            we->mean_path[t] += path_i[t];
            we->var_path[t]  += path_i2[t];
            we->skewness[t]  += path_i3[t];
            we->kurtosis[t]  += path_i4[t];
        }
    }

    /* Normalize to get statistics */
    double inv_n = 1.0 / (double)we->n_paths;
    for (int t = 1; t < we->n_times; t++) {
        double m1 = we->mean_path[t] * inv_n;
        double m2 = we->var_path[t]  * inv_n;
        double m3 = we->skewness[t]  * inv_n;
        double m4 = we->kurtosis[t]  * inv_n;
        we->mean_path[t] = m1;
        double variance = m2 - m1 * m1;
        we->var_path[t]  = (variance > 0.0) ? variance : 0.0;
        if (variance > WC_EPSILON) {
            double sigma = sqrt(variance);
            we->skewness[t] = (m3 - 3.0 * m1 * variance - m1*m1*m1) / (sigma*sigma*sigma);
            we->kurtosis[t] = (m4 - 4.0*m1*m3 + 6.0*m1*m1*m2 - 3.0*m1*m1*m1*m1)
                            / (variance * variance) - 3.0;
        }
    }

    free(path_i); free(path_i2); free(path_i3); free(path_i4);
}

double wc_wiener_ensemble_mean_at(const WCWienerEnsemble* we, int t_idx) {
    if (!we || t_idx < 0 || t_idx >= we->n_times) return 0.0;
    return we->mean_path[t_idx];
}

double wc_wiener_ensemble_var_at(const WCWienerEnsemble* we, int t_idx) {
    if (!we || t_idx < 0 || t_idx >= we->n_times) return 0.0;
    return we->var_path[t_idx];
}

bool wc_wiener_ensemble_verify(const WCWienerEnsemble* we, double tolerance) {
    /* L4 Verification: E[W(t)] ≈ 0, Var[W(t)] ≈ t */
    if (!we) return false;
    double dt = 1.0 / (double)(we->n_times - 1);
    for (int t = 0; t < we->n_times; t++) {
        double t_val = dt * t;
        if (fabs(we->mean_path[t]) > tolerance) return false;
        if (fabs(we->var_path[t] - t_val) > tolerance) return false;
    }
    return true;
}

/* ==============================================================
 * ITO PROCESS (L3, L8)
 *
 * dX = mu(X,t)dt + sigma(X,t)dW
 *
 * Ito's lemma (L4): df(X) = f'(X)dX + (1/2)f''(X)sigma^2 dt
 * This is the stochastic chain rule, different from ordinary
 * calculus due to the quadratic variation of W(t).
 * ============================================================== */

WCItoProcess* wc_ito_create(double x0, double t0,
                            double (*drift)(double,double,void*),
                            double (*diff)(double,double,void*),
                            void* params) {
    if (!drift || !diff) return NULL;
    WCItoProcess* ip = (WCItoProcess*)calloc(1, sizeof(WCItoProcess));
    if (!ip) return NULL;
    ip->drift_func    = drift;
    ip->diffusion_func = diff;
    ip->params        = params;
    ip->current_x     = x0;
    ip->current_t     = t0;
    return ip;
}

void wc_ito_free(WCItoProcess* ip) { free(ip); }

void wc_ito_step(WCItoProcess* ip, double dt) {
    if (!ip || dt <= 0.0) return;
    double mu = ip->drift_func(ip->current_x, ip->current_t, ip->params);
    double sigma = ip->diffusion_func(ip->current_x, ip->current_t, ip->params);
    double dW = sqrt(dt) * wc_gaussian_random();
    ip->current_x += mu * dt + sigma * dW;
    ip->current_t += dt;
}

WCWienerPath* wc_ito_simulate(WCItoProcess* ip, int n_steps, double dt) {
    if (!ip || n_steps < 1) return NULL;
    WCWienerPath* wp = wc_wiener_path_create(n_steps + 1, dt * n_steps, 0);
    if (!wp) return NULL;
    double x = ip->current_x;
    wp->path[0] = x;
    for (int i = 1; i <= n_steps; i++) {
        double mu = ip->drift_func(x, ip->current_t + dt*(i-1), ip->params);
        double sigma = ip->diffusion_func(x, ip->current_t + dt*(i-1), ip->params);
        x += mu * dt + sigma * sqrt(dt) * wc_gaussian_random();
        wp->path[i] = x;
    }
    return wp;
}

/* ==============================================================
 * GEOMETRIC BROWNIAN MOTION (L3)
 *
 * dS = mu*S*dt + sigma*S*dW
 * Solution: S(t) = S0 * exp((mu - sigma^2/2)t + sigma*W(t))
 *
 * This is the classic model for asset prices and also
 * appears in Wiener's work on multiplicative noise.
 * ============================================================== */

static double gbm_drift(double x, double t, void* params) {
    (void)t;
    double mu = ((double*)params)[0];
    return mu * x;
}

static double gbm_diffusion(double x, double t, void* params) {
    (void)t;
    double sigma = ((double*)params)[1];
    return sigma * x;
}

WCGeometricBM* wc_gbm_create(double mu, double sigma, double S0) {
    WCGeometricBM* gbm = (WCGeometricBM*)calloc(1, sizeof(WCGeometricBM));
    if (!gbm) return NULL;
    gbm->mu    = mu;
    gbm->sigma = sigma;
    gbm->S0    = S0;
    return gbm;
}

void wc_gbm_free(WCGeometricBM* gbm) { free(gbm); }

WCWienerPath* wc_gbm_simulate(const WCGeometricBM* gbm,
                               int n_steps, double t_end, int seed) {
    if (!gbm) return NULL;
    WCWienerPath* wp = wc_wiener_path_create(n_steps + 1, t_end, seed);
    if (!wp) return NULL;
    double dt = t_end / n_steps;
    double S = gbm->S0;
    wp->path[0] = S;
    for (int i = 1; i <= n_steps; i++) {
        double dW = sqrt(dt) * wc_gaussian_random();
        S *= exp((gbm->mu - 0.5 * gbm->sigma * gbm->sigma) * dt
                 + gbm->sigma * dW);
        wp->path[i] = S;
    }
    return wp;
}

/* ==============================================================
 * ORNSTEIN-UHLENBECK PROCESS (L3, L8)
 *
 * dX = theta*(mu - X)*dt + sigma*dW
 * Solution: X(t) = mu + (X0-mu)*exp(-theta*t)
 *                  + sigma*integral(exp(-theta*(t-s)) dW(s))
 *
 * Mean-reverting — key model for homeostatic systems.
 * ============================================================== */

static double ou_drift(double x, double t, void* params) {
    (void)t;
    double theta = ((double*)params)[0];
    double mu    = ((double*)params)[1];
    return theta * (mu - x);
}

static double ou_diffusion(double x, double t, void* params) {
    (void)x; (void)t;
    double sigma = ((double*)params)[2];
    return sigma;
}

WCOrnsteinUhlenbeck* wc_ou_create(double theta, double mu,
                                   double sigma, double X0) {
    WCOrnsteinUhlenbeck* ou = (WCOrnsteinUhlenbeck*)calloc(1, sizeof(WCOrnsteinUhlenbeck));
    if (!ou) return NULL;
    ou->theta = theta;
    ou->mu    = mu;
    ou->sigma = sigma;
    ou->X0    = X0;
    return ou;
}

void wc_ou_free(WCOrnsteinUhlenbeck* ou) { free(ou); }

WCWienerPath* wc_ou_simulate(const WCOrnsteinUhlenbeck* ou,
                              int n_steps, double t_end, int seed) {
    if (!ou) return NULL;
    WCWienerPath* wp = wc_wiener_path_create(n_steps + 1, t_end, seed);
    if (!wp) return NULL;
    double dt = t_end / n_steps;
    double X = ou->X0;
    wp->path[0] = X;
    for (int i = 1; i <= n_steps; i++) {
        double dW = sqrt(dt) * wc_gaussian_random();
        X += ou->theta * (ou->mu - X) * dt + ou->sigma * dW;
        wp->path[i] = X;
    }
    return wp;
}

/* ==============================================================
 * WHITE NOISE (L3)
 *
 * White noise is the generalized derivative of W(t).
 * It has constant power spectral density: S(w) = sigma^2.
 * Strictly speaking, it is a distribution, not a function.
 *
 * Wiener used white noise as the fundamental random input
 * in his filtering and prediction theory.
 * ============================================================== */

WCWhiteNoise* wc_whitenoise_create(double sigma, double dt, int seed) {
    WCWhiteNoise* wn = (WCWhiteNoise*)calloc(1, sizeof(WCWhiteNoise));
    if (!wn) return NULL;
    wn->sigma = sigma;
    wn->dt    = dt;
    wn->seed  = seed;
    wn->custom_dist = NULL;
    return wn;
}

void wc_whitenoise_free(WCWhiteNoise* wn) { free(wn); }

double wc_whitenoise_sample(WCWhiteNoise* wn) {
    if (!wn) return 0.0;
    if (wn->custom_dist)
        return wn->sigma * wn->custom_dist();
    /* Gaussian white noise: variance scales as sigma^2/dt */
    return wn->sigma * wc_gaussian_random() / sqrt(wn->dt);
}

void wc_whitenoise_fill(WCWhiteNoise* wn, double* buffer, int n) {
    if (!wn || !buffer) return;
    for (int i = 0; i < n; i++)
        buffer[i] = wc_whitenoise_sample(wn);
}

WCSignal* wc_whitenoise_signal(WCWhiteNoise* wn, int length) {
    if (!wn) return NULL;
    WCSignal* s = wc_signal_create(length, wn->dt, 0.0);
    if (!s) return NULL;
    wc_whitenoise_fill(wn, s->samples, length);
    return s;
}

/*
 * References:
 * Wiener (1923) Differential-Space. J. Math. Phys. 2:131-174.
 * Einstein (1905) On the motion of small particles... Ann. Phys. 17:549.
 * Ito (1944) Stochastic integral. Proc. Imp. Acad. Tokyo 20:519-524.
 * Uhlenbeck & Ornstein (1930) On the theory of Brownian motion.
 *   Phys. Rev. 36:823-841.
 * Levy (1948) Processus stochastiques et mouvement brownien. Gauthier-Villars.
 */
