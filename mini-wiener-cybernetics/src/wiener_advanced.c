#include "wiener_core.h"
#include "wiener_process.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ==============================================================
 * Advanced Wiener Cybernetics Topics — L8
 *
 * Advanced extensions of Wiener's cybernetics:
 * Nonlinear stochastic systems, Wiener-Volterra series,
 * generalized harmonic analysis, and stochastic resonance.
 *
 * L8 Topics:
 *   - Random walk properties and first passage times
 *   - Nonlinear stochastic dynamics
 *   - Multiplicative noise systems
 *   - Stochastic resonance phenomenon
 *   - Fokker-Planck equation approximation
 *
 * References:
 *   Wiener (1958) Nonlinear Problems in Random Theory
 *   Stratonovich (1963) Topics in the Theory of Random Noise
 *   Benzi et al. (1981) Stochastic resonance. J. Phys. A
 * ============================================================== */

extern double wc_gaussian_random(void);

/* ==============================================================
 * RANDOM WALK ANALYSIS (L8)
 *
 * Wiener process as the continuous limit of random walks.
 * Donsker's invariance principle connects discrete random
 * walks to the Wiener process.
 *
 * L4: Central limit theorem → Wiener measure
 * L4: Law of the iterated logarithm for W(t)
 * ============================================================== */

/* First passage time distribution for Brownian motion with drift:
 * P(tau_a <= t) = 2*Phi(-|a-mu*t|/(sigma*sqrt(t))) + ...
 * This is the Bachelier-Levy formula. */

double wc_first_passage_probability(double barrier, double drift,
                                     double sigma, double t) {
    if (t <= 0.0 || sigma < WC_EPSILON) return 0.0;
    double d1 = (barrier - drift * t) / (sigma * sqrt(t));
    double d2 = (-barrier - drift * t) / (sigma * sqrt(t));
    double p = wc_erf_approx(d1 / sqrt(2.0)) * 0.5 + 0.5;
    if (drift > WC_EPSILON)
        p += exp(2.0 * drift * barrier / (sigma * sigma))
           * (wc_erf_approx(d2 / sqrt(2.0)) * 0.5 + 0.5);
    return p;
}

/* Reflection principle: P(max_{s<=t} W(s) >= a) = 2*P(W(t) >= a) */
double wc_running_max_probability(double level, double t) {
    if (t <= 0.0) return 0.0;
    double z = level / sqrt(t);
    return 2.0 * (1.0 - (wc_erf_approx(z / sqrt(2.0)) * 0.5 + 0.5));
}

/* Arcsin law for Brownian motion:
 * P(time spent positive <= x*t) = (2/pi)*arcsin(sqrt(x)) */
double wc_arcsin_probability(double x) {
    if (x < 0.0) x = 0.0;
    if (x > 1.0) x = 1.0;
    return 2.0 * asin(sqrt(x)) / WC_PI;
}

/* ==============================================================
 * WIENER-VOLTERRA SERIES (L8)
 *
 * Wiener extended his linear filter theory to nonlinear
 * systems using Volterra series expansions. The Wiener
 * G-functional expansion represents nonlinear systems
 * as orthogonal polynomial functionals of the input.
 *
 * y(t) = G0 + G1[x(t)] + G2[x(t)] + ...
 *
 * where G_k are k-th order Wiener functionals.
 * ============================================================== */

/* Second-order Volterra kernel evaluation:
 * y(t) = h0 + integral(h1(tau)*x(t-tau) dtau)
 *       + integral(h2(tau1,tau2)*x(t-tau1)*x(t-tau2) dtau1 dtau2) */
double wc_volterra_second_order(const double* x, int n,
                                 const double* h1, int h1_len,
                                 const double* h2, int h2_len,
                                 int t_idx) {
    if (!x || !h1 || !h2 || t_idx < 0 || t_idx >= n) return 0.0;
    double y = 0.0;
    /* First-order (linear) term */
    for (int tau = 0; tau < h1_len && tau <= t_idx; tau++)
        y += h1[tau] * x[t_idx - tau];
    /* Second-order (quadratic) term */
    for (int tau1 = 0; tau1 < h2_len && tau1 <= t_idx; tau1++)
        for (int tau2 = 0; tau2 < h2_len && tau2 <= t_idx; tau2++)
            y += h2[tau1 * h2_len + tau2]
               * x[t_idx - tau1] * x[t_idx - tau2];
    return y;
}

/* Wiener G-functional kernel orthogonalization (simplified) */
void wc_wiener_g_functional(const double* input, int n, int order,
                             double* output) {
    /* Simplified: Use Gram-Schmidt orthogonalized polynomial
     * functionals of the input. For order=1: G1 = linear filter.
     * For order=2: G2 = quadratic - mean(quadratic). */
    if (!input || !output || n <= 0) return;
    for (int i = 0; i < n; i++) {
        output[i] = input[i];  /* G1: identity */
        if (order >= 2) {
            /* G2: Hermite polynomial H2(x) = x^2 - 1 */
            output[i] += (input[i] * input[i] - 1.0) * 0.1;
        }
        if (order >= 3) {
            /* G3: Hermite polynomial H3(x) = x^3 - 3x */
            output[i] += (input[i]*input[i]*input[i] - 3.0*input[i]) * 0.01;
        }
    }
}

/* ==============================================================
 * STOCHASTIC RESONANCE (L8)
 *
 * A counterintuitive phenomenon where adding noise to a
 * nonlinear system can enhance the detection of a weak
 * periodic signal. This has applications in neural coding,
 * climate modeling, and signal processing.
 *
 * L8: Benzi et al. (1981) discovered SR in climate models
 * L8: Gammaitoni et al. (1998) review in Rev. Mod. Phys.
 * ============================================================== */

/* Bistable potential: V(x) = x^4/4 - x^2/2
 * With periodic forcing and noise:
 * dx = (x - x^3 + A*cos(omega*t))dt + sigma*dW */
double wc_stochastic_resonance_simulate(double A, double omega,
                                         double sigma, double dt,
                                         int n_steps, double* output) {
    if (!output || n_steps < 1) return 0.0;
    double x = 0.0;
    double snr = 0.0;
    for (int i = 0; i < n_steps; i++) {
        double t = i * dt;
        double drift = x - x*x*x + A * cos(omega * t);
        double dW = sigma * sqrt(dt) * wc_gaussian_random();
        x += drift * dt + dW;
        output[i] = x;
        snr += output[i] * cos(omega * t);
    }
    return snr / n_steps;
}

/* Signal-to-noise ratio for stochastic resonance */
double wc_stochastic_resonance_snr(const double* output, int n,
                                    double omega, double dt) {
    if (!output || n < 2) return 0.0;
    /* Compute power at forcing frequency omega */
    double re = 0.0, im = 0.0;
    for (int i = 0; i < n; i++) {
        double t = i * dt;
        re += output[i] * cos(omega * t);
        im -= output[i] * sin(omega * t);
    }
    double signal_power = (re*re + im*im) / n;
    double total_power = 0.0;
    for (int i = 0; i < n; i++) total_power += output[i] * output[i];
    total_power /= n;
    double noise_power = total_power - signal_power;
    if (noise_power < WC_EPSILON) return 0.0;
    return 10.0 * log10(signal_power / noise_power);
}

/* ==============================================================
 * GENERALIZED HARMONIC ANALYSIS (L8)
 *
 * Wiener's (1930) "Generalized Harmonic Analysis" extended
 * Fourier analysis to non-periodic, non-integrable functions.
 * This was foundational to his later work on time series.
 *
 * Key insight: The autocorrelation function R(tau) and the
 * integrated spectrum S(w) form a Fourier transform pair
 * even for signals that are not square-integrable.
 *
 * L4: Wiener-Khinchin theorem (generalized)
 * ============================================================== */

/* Generalized autocorrelation for a non-stationary process:
 * R(t, tau) = E[x(t+tau/2) * x(t-tau/2)]
 *
 * For stationary processes, R depends only on tau.
 * For cyclostationary processes, R is periodic in t. */
double wc_generalized_autocorrelation(const double* x, int n,
                                       int t, int tau) {
    if (!x || n <= 0) return 0.0;
    int idx1 = t + tau / 2;
    int idx2 = t - tau / 2;
    if (idx1 < 0 || idx1 >= n || idx2 < 0 || idx2 >= n) return 0.0;
    return x[idx1] * x[idx2];
}

/* Wigner-Ville time-frequency distribution (L8):
 * W(t,w) = integral(x(t+tau/2)*x(t-tau/2)*exp(-j*w*tau) dtau)
 * This is a quadratic time-frequency representation that
 * generalizes the spectrogram. */
void wc_wigner_ville(const double* x, int n, int t,
                      double* freq, double* wv, int n_freq) {
    if (!x || !freq || !wv || n_freq <= 0) return;
    for (int k = 0; k < n_freq; k++) {
        double omega = freq[k];
        double re = 0.0;
        int max_tau = (n - abs(t)) / 2;
        if (max_tau > 32) max_tau = 32;
        for (int tau = -max_tau; tau <= max_tau; tau++) {
            double r = wc_generalized_autocorrelation(x, n, t, tau);
            re += r * cos(omega * tau);
        }
        wv[k] = re / (2.0 * max_tau + 1.0);
    }
}

/* ==============================================================
 * FOKKER-PLANCK EQUATION (L8)
 *
 * The Fokker-Planck equation describes the time evolution
 * of the probability density of a stochastic process:
 *
 * dp/dt = -d/dx[mu(x)*p] + (1/2)*d^2/dx^2[sigma^2(x)*p]
 *
 * For the Wiener process: dp/dt = (1/2)*d^2p/dx^2 (heat equation)
 * ============================================================== */

/* Forward Kolmogorov (Fokker-Planck) equation solver
 * for 1D diffusion on a grid. Crank-Nicolson semi-implicit. */
int wc_fokker_planck_step(double* p, int n_x, double dx, double dt,
                            double (*drift)(double,void*),
                            double (*diffusion)(double,void*),
                            void* params) {
    if (!p || n_x < 3 || dx <= 0.0 || dt <= 0.0 || !drift || !diffusion)
        return -1;
    /* Explicit Euler for demonstration:
     * p_i^{n+1} = p_i^n + dt*(...)*/
    double* p_new = (double*)calloc((size_t)n_x, sizeof(double));
    if (!p_new) return -1;
    for (int i = 1; i < n_x - 1; i++) {
        double x = i * dx;
        double mu  = drift(x, params);
        double sig = diffusion(x, params);
        double sig2 = sig * sig;
        /* Drift term: -d/dx[mu*p] */
        double drift_term = -(mu * p[i+1] - mu * p[i-1]) / (2.0 * dx);
        /* Diffusion term: (1/2)*d^2/dx^2[sigma^2*p] */
        double diff_term = sig2 * (p[i+1] - 2.0*p[i] + p[i-1]) / (dx * dx) * 0.5;
        p_new[i] = p[i] + dt * (drift_term + diff_term);
        if (p_new[i] < 0.0) p_new[i] = 0.0;
    }
    /* Boundary conditions: zero flux */
    p_new[0] = p_new[1];
    p_new[n_x-1] = p_new[n_x-2];
    /* Normalize */
    double sum = 0.0;
    for (int i = 0; i < n_x; i++) sum += p_new[i];
    if (sum > WC_EPSILON)
        for (int i = 0; i < n_x; i++) p_new[i] /= sum;
    memcpy(p, p_new, (size_t)n_x * sizeof(double));
    free(p_new);
    return 0;
}

/* ==============================================================
 * ENTROPY RATE OF STOCHASTIC PROCESSES (L8)
 *
 * Kolmogorov-Sinai entropy rate for a stochastic process:
 * h = lim_{n->inf} H(X_n | X_1,...,X_{n-1})
 *
 * For a Gaussian process with power spectrum S(w):
 * h = (1/2)*log(2*pi*e) + (1/4*pi)*integral(log(S(w))) dw
 * ============================================================== */

double wc_entropy_rate_gaussian(const double* spectrum, int n_freq,
                                 double df) {
    if (!spectrum || n_freq < 1 || df <= 0.0) return 0.0;
    /* Wiener's entropy rate formula for Gaussian processes */
    double integral = 0.0;
    for (int i = 0; i < n_freq; i++) {
        if (spectrum[i] > WC_EPSILON)
            integral += log(spectrum[i]) * df;
    }
    double base = 0.5 * log(2.0 * WC_PI * WC_E);
    return base + integral / (4.0 * WC_PI);
}

/* ==============================================================
 * LYAPUNOV EXPONENT FOR STOCHASTIC SYSTEMS (L8)
 *
 * Measures the average exponential rate of divergence of
 * nearby trajectories. For stochastic systems, this
 * generalizes to the almost-sure Lyapunov exponent.
 * ============================================================== */

double wc_lyapunov_stochastic(const double* trajectory, int n,
                               double (*drift_deriv)(double,void*),
                               void* params) {
    if (!trajectory || n < 2 || !drift_deriv) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < n; i++)
        sum += log(fabs(drift_deriv(trajectory[i], params)) + WC_EPSILON);
    return sum / n;
}

/*
 * References:
 * Wiener (1930) Generalized harmonic analysis. Acta Math. 55:117-258.
 * Wiener (1958) Nonlinear Problems in Random Theory. MIT Press.
 * Stratonovich (1963) Topics in the Theory of Random Noise. Gordon & Breach.
 * Benzi, Parisi, Sutera, Vulpiani (1981) The mechanism of stochastic
 *   resonance. J. Phys. A: Math. Gen. 14:L453-L457.
 * Gammaitoni, Hanggi, Jung, Marchesoni (1998) Stochastic resonance.
 *   Rev. Mod. Phys. 70:223-287.
 * Fokker (1914) Die mittlere Energie rotierender elektrischer Dipole...
 *   Ann. Phys. 348:810-820.
 * Planck (1917) Uber einen Satz der statistischen Dynamik...
 *   Sitzungsber. Preuss. Akad. Wiss. 24:324-341.
 */
