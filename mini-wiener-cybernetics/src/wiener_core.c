#include "wiener_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ==============================================================
 * Wiener Cybernetics — Core Implementation
 *
 * Implements Norbert Wiener's foundational cybernetics concepts:
 * signals, feedback loops, information measures, homeostasis,
 * and goal-directed behavior.
 *
 * References:
 *   Wiener (1948) "Cybernetics" — MIT Press
 *   Wiener (1950) "The Human Use of Human Beings"
 *   Rosenblueth, Wiener, Bigelow (1943) "Behavior, Purpose and
 *     Teleology" — Philosophy of Science 10(1)
 * ============================================================== */

/* ── Internal RNG (LCG for reproducibility) ──────────── */

static unsigned int wc_rng_state = 123456789;

static void wc_rng_seed(unsigned int seed) {
    wc_rng_state = seed;
}

static unsigned int wc_rng_next(void) {
    wc_rng_state = wc_rng_state * 1103515245 + 12345;
    return wc_rng_state;
}

static double wc_rng_uniform(void) {
    return (double)(wc_rng_next() & 0x7FFFFFFF) / (double)0x80000000;
}

/* ── Gaussian Random Number (Box-Muller transform) ───── */

double wc_gaussian_random(void) {
    double u1 = wc_rng_uniform();
    double u2 = wc_rng_uniform();
    if (u1 < 1e-12) u1 = 1e-12;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * WC_PI * u2);
}

void wc_generate_noise(double* buffer, int n, double sigma) {
    if (!buffer || n <= 0) return;
    for (int i = 0; i < n; i++) {
        buffer[i] = sigma * wc_gaussian_random();
    }
}

/* ── Special Functions ────────────────────────────────── */

double wc_sinc(double x) {
    if (fabs(x) < WC_EPSILON) return 1.0;
    double arg = WC_PI * x;
    return sin(arg) / arg;
}

double wc_erf_approx(double x) {
    /* Rational approximation, max error 1.5e-7 (Abramowitz & Stegun 7.1.26) */
    double sign = (x >= 0.0) ? 1.0 : -1.0;
    double ax = fabs(x);
    double t = 1.0 / (1.0 + 0.3275911 * ax);
    double t2 = t * t;
    double t3 = t2 * t;
    double t4 = t3 * t;
    double t5 = t4 * t;
    double poly = 1.0 - (0.254829592 * t - 0.284496736 * t2
               + 1.421413741 * t3 - 1.453152027 * t4
               + 1.061405429 * t5) * exp(-ax * ax);
    return sign * poly;
}

/* ── Utility Functions ────────────────────────────────── */

double wc_max(double a, double b) { return (a > b) ? a : b; }
double wc_min(double a, double b) { return (a < b) ? a : b; }
bool   wc_is_zero(double x)      { return fabs(x) < WC_EPSILON; }
bool   wc_is_equal(double a, double b) { return fabs(a - b) < WC_EPSILON; }

/* ==============================================================
 * SIGNAL IMPLEMENTATION
 * Wiener: "Information is a name for the content of what is
 * exchanged with the outer world as we adjust to it."
 * Signals are the carriers of information.
 * ============================================================== */

/* ── Signal Lifecycle ────────────────────────────────── */

WCSignal* wc_signal_create(int length, double dt, double t0) {
    if (length <= 0) return NULL;
    WCSignal* s = (WCSignal*)calloc(1, sizeof(WCSignal));
    if (!s) return NULL;
    s->length   = length;
    s->capacity = length;
    s->dt       = dt;
    s->t0       = t0;
    s->samples  = (double*)calloc((size_t)length, sizeof(double));
    if (!s->samples) { free(s); return NULL; }
    return s;
}

WCSignal* wc_signal_create_from(const double* data, int length, double dt, double t0) {
    WCSignal* s = wc_signal_create(length, dt, t0);
    if (s && data) memcpy(s->samples, data, (size_t)length * sizeof(double));
    return s;
}

WCSignal* wc_signal_clone(const WCSignal* s) {
    if (!s) return NULL;
    WCSignal* c = wc_signal_create(s->length, s->dt, s->t0);
    if (c) memcpy(c->samples, s->samples, (size_t)s->length * sizeof(double));
    return c;
}

void wc_signal_free(WCSignal* s) {
    if (!s) return;
    free(s->samples);
    free(s);
}

double wc_signal_get(const WCSignal* s, int i) {
    if (!s || i < 0 || i >= s->length) return 0.0;
    return s->samples[i];
}

void wc_signal_set(WCSignal* s, int i, double val) {
    if (s && i >= 0 && i < s->length)
        s->samples[i] = val;
}

/* ── Signal Statistics ───────────────────────────────── */

double wc_signal_mean(const WCSignal* s) {
    if (!s || s->length == 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < s->length; i++) sum += s->samples[i];
    return sum / (double)s->length;
}

double wc_signal_variance(const WCSignal* s) {
    if (!s || s->length == 0) return 0.0;
    double mean = wc_signal_mean(s);
    double sum_sq = 0.0;
    for (int i = 0; i < s->length; i++) {
        double d = s->samples[i] - mean;
        sum_sq += d * d;
    }
    return sum_sq / (double)s->length;
}

double wc_signal_rms(const WCSignal* s) {
    if (!s || s->length == 0) return 0.0;
    double sum_sq = 0.0;
    for (int i = 0; i < s->length; i++)
        sum_sq += s->samples[i] * s->samples[i];
    return sqrt(sum_sq / (double)s->length);
}

double wc_signal_energy(const WCSignal* s) {
    if (!s) return 0.0;
    double e = 0.0;
    for (int i = 0; i < s->length; i++)
        e += s->samples[i] * s->samples[i];
    return e * s->dt;
}

double wc_signal_power(const WCSignal* s) {
    if (!s || s->length == 0) return 0.0;
    double duration = s->length * s->dt;
    return (duration > WC_EPSILON) ? wc_signal_energy(s) / duration : 0.0;
}

/* ── Signal Operations ───────────────────────────────── */

WCSignal* wc_signal_add(const WCSignal* a, const WCSignal* b) {
    if (!a || !b || a->length != b->length) return NULL;
    WCSignal* r = wc_signal_create(a->length, a->dt, a->t0);
    if (!r) return NULL;
    for (int i = 0; i < a->length; i++)
        r->samples[i] = a->samples[i] + b->samples[i];
    return r;
}

WCSignal* wc_signal_sub(const WCSignal* a, const WCSignal* b) {
    if (!a || !b || a->length != b->length) return NULL;
    WCSignal* r = wc_signal_create(a->length, a->dt, a->t0);
    if (!r) return NULL;
    for (int i = 0; i < a->length; i++)
        r->samples[i] = a->samples[i] - b->samples[i];
    return r;
}

WCSignal* wc_signal_mul(const WCSignal* a, double scalar) {
    if (!a) return NULL;
    WCSignal* r = wc_signal_create(a->length, a->dt, a->t0);
    if (!r) return NULL;
    for (int i = 0; i < a->length; i++)
        r->samples[i] = a->samples[i] * scalar;
    return r;
}

WCSignal* wc_signal_convolve(const WCSignal* a, const WCSignal* b) {
    if (!a || !b) return NULL;
    int out_len = a->length + b->length - 1;
    WCSignal* r = wc_signal_create(out_len, a->dt, a->t0);
    if (!r) return NULL;
    for (int n = 0; n < out_len; n++) {
        double sum = 0.0;
        for (int k = 0; k < a->length; k++) {
            int j = n - k;
            if (j >= 0 && j < b->length)
                sum += a->samples[k] * b->samples[j];
        }
        r->samples[n] = sum;
    }
    return r;
}

WCSignal* wc_signal_derivative(const WCSignal* s) {
    if (!s || s->length < 2) return NULL;
    WCSignal* r = wc_signal_create(s->length - 1, s->dt,
                                    s->t0 + 0.5 * s->dt);
    if (!r) return NULL;
    for (int i = 0; i < s->length - 1; i++)
        r->samples[i] = (s->samples[i+1] - s->samples[i]) / s->dt;
    return r;
}

WCSignal* wc_signal_integral(const WCSignal* s) {
    if (!s || s->length < 1) return NULL;
    WCSignal* r = wc_signal_create(s->length, s->dt, s->t0);
    if (!r) return NULL;
    r->samples[0] = 0.0;
    for (int i = 1; i < s->length; i++)
        r->samples[i] = r->samples[i-1] + s->samples[i] * s->dt;
    return r;
}

void wc_signal_print(const WCSignal* s, const char* label) {
    if (!s) { printf("%s: NULL\n", label); return; }
    printf("%s: n=%d dt=%.4f t0=%.4f", label, s->length, s->dt, s->t0);
    printf(" mean=%.6f rms=%.6f energy=%.4f\n",
           wc_signal_mean(s), wc_signal_rms(s), wc_signal_energy(s));
}

/* ==============================================================
 * CYBERNETIC SYSTEM (L1-L2)
 * Wiener's central abstraction: a system receives inputs
 * (information), processes them through internal state, and
 * produces outputs (action). Control and communication are
 * unified in this framework.
 * ============================================================== */

WCCyberneticSystem* wc_system_create(int n_state, int n_input,
                                      int n_output, double dt) {
    if (n_state < 0 || n_input < 0 || n_output < 0) return NULL;
    WCCyberneticSystem* sys = (WCCyberneticSystem*)calloc(1, sizeof(WCCyberneticSystem));
    if (!sys) return NULL;
    sys->n_state  = n_state;
    sys->n_input  = n_input;
    sys->n_output = n_output;
    sys->dt       = dt;
    sys->t        = 0.0;
    if (n_state  > 0) sys->state    = (double*)calloc((size_t)n_state,  sizeof(double));
    if (n_input  > 0) sys->input    = (double*)calloc((size_t)n_input,  sizeof(double));
    if (n_output > 0) sys->output   = (double*)calloc((size_t)n_output, sizeof(double));
    if (n_output > 0) sys->feedback = (double*)calloc((size_t)n_output, sizeof(double));
    return sys;
}

void wc_system_free(WCCyberneticSystem* sys) {
    if (!sys) return;
    free(sys->state);
    free(sys->input);
    free(sys->output);
    free(sys->feedback);
    free(sys);
}

void wc_system_set_state(WCCyberneticSystem* sys, const double* state) {
    if (sys && state && sys->n_state > 0)
        memcpy(sys->state, state, (size_t)sys->n_state * sizeof(double));
}

void wc_system_get_state(const WCCyberneticSystem* sys, double* state) {
    if (sys && state && sys->n_state > 0)
        memcpy(state, sys->state, (size_t)sys->n_state * sizeof(double));
}

void wc_system_set_input(WCCyberneticSystem* sys, const double* input) {
    if (sys && input && sys->n_input > 0)
        memcpy(sys->input, input, (size_t)sys->n_input * sizeof(double));
}

void wc_system_get_output(const WCCyberneticSystem* sys, double* output) {
    if (sys && output && sys->n_output > 0)
        memcpy(output, sys->output, (size_t)sys->n_output * sizeof(double));
}

void wc_system_step(WCCyberneticSystem* sys) {
    if (!sys) return;
    if (sys->n_output > 0 && sys->n_state > 0) {
        int nc = (sys->n_state < sys->n_output) ? sys->n_state : sys->n_output;
        memcpy(sys->output, sys->state, (size_t)nc * sizeof(double));
    }
    if (sys->feedback && sys->n_output > 0)
        memcpy(sys->feedback, sys->output, (size_t)sys->n_output * sizeof(double));
    sys->t += sys->dt;
}

void wc_system_simulate(WCCyberneticSystem* sys, int steps,
                         double* state_history, double* output_history) {
    if (!sys || steps <= 0) return;
    for (int s = 0; s < steps; s++) {
        if (state_history)
            memcpy(state_history + (size_t)s * sys->n_state,
                   sys->state, (size_t)sys->n_state * sizeof(double));
        wc_system_step(sys);
        if (output_history)
            memcpy(output_history + (size_t)s * sys->n_output,
                   sys->output, (size_t)sys->n_output * sizeof(double));
    }
}

void wc_system_print(const WCCyberneticSystem* sys) {
    if (!sys) { printf("WCCyberneticSystem: NULL\n"); return; }
    printf("WCCyberneticSystem: n_state=%d n_in=%d n_out=%d dt=%.4f t=%.4f\n",
           sys->n_state, sys->n_input, sys->n_output, sys->dt, sys->t);
}

/* ==============================================================
 * FEEDBACK LOOP (L2, L4)
 * Negative feedback is the fundamental mechanism of cybernetic
 * regulation. Wiener and Rosenblueth showed that purposeful
 * behavior = behavior controlled by negative feedback.
 *
 * L4 Theorem: A feedback loop with integral action drives
 * steady-state error to zero (internal model principle).
 * ============================================================== */

WCFeedbackLoop* wc_feedback_create(double kp, double ki, double kd, double dt) {
    WCFeedbackLoop* fb = (WCFeedbackLoop*)calloc(1, sizeof(WCFeedbackLoop));
    if (!fb) return NULL;
    fb->kp = kp;
    fb->ki = ki;
    fb->kd = kd;
    fb->dt = dt;
    fb->setpoint       = 0.0;
    fb->integral_limit = 1e6;
    fb->is_active      = true;
    return fb;
}

void wc_feedback_free(WCFeedbackLoop* fb) {
    free(fb);
}

void wc_feedback_set_setpoint(WCFeedbackLoop* fb, double sp) {
    if (fb) fb->setpoint = sp;
}

double wc_feedback_update(WCFeedbackLoop* fb, double measurement) {
    if (!fb || !fb->is_active) return 0.0;
    double prev_err = fb->error;
    fb->error = fb->setpoint - measurement;
    /* Integral with anti-windup */
    fb->integral += fb->error * fb->dt;
    if (fb->integral >  fb->integral_limit) fb->integral =  fb->integral_limit;
    if (fb->integral < -fb->integral_limit) fb->integral = -fb->integral_limit;
    /* Derivative with noise filtering (first-order lag) */
    double raw_deriv = (fb->error - prev_err) / fb->dt;
    fb->derivative = fb->derivative * 0.7 + raw_deriv * 0.3;
    /* PID control law */
    fb->control = fb->kp * fb->error
                + fb->ki * fb->integral
                + fb->kd * fb->derivative;
    fb->gain = (fabs(fb->error) > WC_EPSILON)
               ? fabs(fb->control / fb->error) : fb->kp;
    return fb->control;
}

void wc_feedback_reset(WCFeedbackLoop* fb) {
    if (!fb) return;
    fb->error      = 0.0;
    fb->integral   = 0.0;
    fb->derivative = 0.0;
    fb->control    = 0.0;
}

bool wc_feedback_is_stable(const WCFeedbackLoop* fb) {
    if (!fb) return false;
    return (fb->kp > -1.0) && (fb->ki >= 0.0) && (fb->kd >= 0.0);
}

double wc_feedback_steady_error(const WCFeedbackLoop* fb) {
    if (!fb) return 0.0;
    if (fb->ki > WC_EPSILON) return 0.0;
    return fb->error;
}

double wc_feedback_overshoot(const WCFeedbackLoop* fb) {
    if (!fb) return 0.0;
    double wn   = sqrt(fb->kp + 1.0);
    double zeta = (fb->kd + fb->ki * 0.1) / (2.0 * wn + WC_EPSILON);
    if (zeta >= 1.0) return 0.0;
    return 100.0 * exp(-WC_PI * zeta / sqrt(1.0 - zeta * zeta));
}

double wc_feedback_settling_time(const WCFeedbackLoop* fb, double tolerance) {
    if (!fb) return 1e9;
    double wn   = sqrt(fb->kp + 1.0);
    double zeta = (fb->kd + fb->ki * 0.1) / (2.0 * wn + WC_EPSILON);
    if (zeta < WC_EPSILON || wn < WC_EPSILON) return 1e9;
    return -log(tolerance) / (zeta * wn);
}

/* ==============================================================
 * INFORMATION MEASURES (L2, L3, L4)
 *
 * Wiener (1948, Ch.III): "The amount of information... is the
 * negative of the quantity usually defined as entropy."
 *
 * L4: Information = negative entropy.
 *     I(X) = -sum(p_i * log2(p_i)) = H(X)
 * L4: Channel capacity (Shannon-Hartley, also in Wiener):
 *     C = B * log2(1 + SNR)
 * ============================================================== */

WCInformation* wc_info_create(const double* prob_dist, int n_symbols) {
    if (n_symbols <= 0) return NULL;
    WCInformation* info = (WCInformation*)calloc(1, sizeof(WCInformation));
    if (!info) return NULL;
    info->n_symbols = n_symbols;
    info->prob_dist = (double*)calloc((size_t)n_symbols, sizeof(double));
    if (!info->prob_dist) { free(info); return NULL; }
    if (prob_dist)
        memcpy(info->prob_dist, prob_dist, (size_t)n_symbols * sizeof(double));
    return info;
}

void wc_info_free(WCInformation* info) {
    if (!info) return;
    free(info->prob_dist);
    free(info);
}

double wc_info_entropy(const WCInformation* info) {
    if (!info || !info->prob_dist) return 0.0;
    double H = 0.0;
    for (int i = 0; i < info->n_symbols; i++) {
        double p = info->prob_dist[i];
        if (p > WC_EPSILON) H -= p * log2(p);
    }
    return H;
}

double wc_info_max_entropy(int n_symbols) {
    return (n_symbols > 0) ? log2((double)n_symbols) : 0.0;
}

double wc_info_kldivergence(const WCInformation* p, const WCInformation* q) {
    if (!p || !q || p->n_symbols != q->n_symbols) return 0.0;
    double dkl = 0.0;
    for (int i = 0; i < p->n_symbols; i++) {
        if (p->prob_dist[i] > WC_EPSILON && q->prob_dist[i] > WC_EPSILON)
            dkl += p->prob_dist[i] * log2(p->prob_dist[i] / q->prob_dist[i]);
    }
    return dkl;
}

double wc_info_mutual_info(const WCInformation* joint,
                            const WCInformation* marg_a,
                            const WCInformation* marg_b) {
    if (!joint || !marg_a || !marg_b) return 0.0;
    double mi = 0.0;
    for (int i = 0; i < joint->n_symbols; i++) {
        int a_idx = i / marg_b->n_symbols;
        int b_idx = i % marg_b->n_symbols;
        if (a_idx >= marg_a->n_symbols || b_idx >= marg_b->n_symbols) continue;
        double p_ab = joint->prob_dist[i];
        double p_a  = marg_a->prob_dist[a_idx];
        double p_b  = marg_b->prob_dist[b_idx];
        if (p_ab > WC_EPSILON && p_a > WC_EPSILON && p_b > WC_EPSILON)
            mi += p_ab * log2(p_ab / (p_a * p_b));
    }
    return mi;
}

double wc_info_channel_capacity(const WCInformation* info, double snr, double bw) {
    (void)info;
    return bw * log2(1.0 + snr);
}

/* ==============================================================
 * ENTROPY BUDGET (L3)
 *
 * Wiener emphasized the thermodynamic cost of information:
 * obtaining information requires an expenditure of negentropy.
 * ============================================================== */

WCEntropyBudget* wc_entropy_create(double temp, double bw) {
    WCEntropyBudget* eb = (WCEntropyBudget*)calloc(1, sizeof(WCEntropyBudget));
    if (!eb) return NULL;
    eb->temperature = temp;
    eb->bandwidth   = bw;
    return eb;
}

void wc_entropy_free(WCEntropyBudget* eb) { free(eb); }

void wc_entropy_update(WCEntropyBudget* eb, double signal_power, double noise_power) {
    if (!eb) return;
    eb->signal_power = signal_power;
    eb->noise_power  = noise_power;
    double snr = signal_power / (noise_power + WC_EPSILON);
    eb->entropy_rate     = eb->bandwidth * log2(1.0 + snr);
    eb->information_flow = eb->entropy_rate;
}

double wc_entropy_rate(const WCEntropyBudget* eb) {
    return eb ? eb->entropy_rate : 0.0;
}

double wc_entropy_snr(const WCEntropyBudget* eb) {
    if (!eb || eb->noise_power < WC_EPSILON) return 1e9;
    return eb->signal_power / eb->noise_power;
}

double wc_entropy_capacity(const WCEntropyBudget* eb) {
    if (!eb) return 0.0;
    double snr = eb->signal_power / (eb->noise_power + WC_EPSILON);
    return eb->bandwidth * log2(1.0 + snr);
}

/* ==============================================================
 * HOMEOSTASIS (L1, L2)
 *
 * Homeostasis is the property of cybernetic systems to maintain
 * internal stability. Wiener discussed this as the biological
 * analog of negative feedback control.
 * ============================================================== */

WCHomeostasis* wc_homeostasis_create(int n_vars) {
    if (n_vars <= 0) return NULL;
    WCHomeostasis* hs = (WCHomeostasis*)calloc(1, sizeof(WCHomeostasis));
    if (!hs) return NULL;
    hs->n_vars = n_vars;
    hs->regulated_vars  = (double*)calloc((size_t)n_vars, sizeof(double));
    hs->setpoints       = (double*)calloc((size_t)n_vars, sizeof(double));
    hs->tolerances      = (double*)calloc((size_t)n_vars, sizeof(double));
    hs->current_values  = (double*)calloc((size_t)n_vars, sizeof(double));
    if (!hs->regulated_vars || !hs->setpoints ||
        !hs->tolerances || !hs->current_values) {
        wc_homeostasis_free(hs); return NULL;
    }
    for (int i = 0; i < n_vars; i++) hs->tolerances[i] = 0.1;
    hs->is_homeostatic = true;
    return hs;
}

void wc_homeostasis_free(WCHomeostasis* hs) {
    if (!hs) return;
    free(hs->regulated_vars);
    free(hs->setpoints);
    free(hs->tolerances);
    free(hs->current_values);
    free(hs);
}

void wc_homeostasis_set_target(WCHomeostasis* hs, int idx,
                                double setpoint, double tol) {
    if (!hs || idx < 0 || idx >= hs->n_vars) return;
    hs->setpoints[idx]  = setpoint;
    hs->tolerances[idx] = tol;
}

void wc_homeostasis_update(WCHomeostasis* hs, int idx, double value) {
    if (!hs || idx < 0 || idx >= hs->n_vars) return;
    hs->current_values[idx] = value;
    hs->regulated_vars[idx] = value - hs->setpoints[idx];
    hs->allostatic_load    += fabs(hs->regulated_vars[idx]);
    hs->is_homeostatic      = wc_homeostasis_check(hs);
}

bool wc_homeostasis_check(const WCHomeostasis* hs) {
    if (!hs) return false;
    for (int i = 0; i < hs->n_vars; i++) {
        if (fabs(hs->regulated_vars[i]) > hs->tolerances[i])
            return false;
    }
    return true;
}

double wc_homeostasis_deviation(const WCHomeostasis* hs) {
    if (!hs) return 0.0;
    double s = 0.0;
    for (int i = 0; i < hs->n_vars; i++)
        s += hs->regulated_vars[i] * hs->regulated_vars[i];
    return sqrt(s);
}

/* ==============================================================
 * GOAL-DIRECTED BEHAVIOR (L1, L2)
 *
 * Wiener & Bigelow (1943): Purposeful behavior is behavior
 * that is controlled by negative feedback. A goal-seeking
 * system continuously reduces the error between its current
 * state and a goal state.
 * ============================================================== */

WCGoalDirected* wc_goal_create(int n_dims) {
    if (n_dims <= 0) return NULL;
    WCGoalDirected* gd = (WCGoalDirected*)calloc(1, sizeof(WCGoalDirected));
    if (!gd) return NULL;
    gd->n_dimensions  = n_dims;
    gd->goal_state    = (double*)calloc((size_t)n_dims, sizeof(double));
    gd->current_state = (double*)calloc((size_t)n_dims, sizeof(double));
    if (!gd->goal_state || !gd->current_state) { wc_goal_free(gd); return NULL; }
    return gd;
}

void wc_goal_free(WCGoalDirected* gd) {
    if (!gd) return;
    free(gd->goal_state);
    free(gd->current_state);
    free(gd);
}

void wc_goal_set_target(WCGoalDirected* gd, const double* goal) {
    if (gd && goal)
        memcpy(gd->goal_state, goal, (size_t)gd->n_dimensions * sizeof(double));
}

void wc_goal_update(WCGoalDirected* gd, const double* state) {
    if (!gd || !state) return;
    memcpy(gd->current_state, state, (size_t)gd->n_dimensions * sizeof(double));
    gd->goal_error = 0.0;
    for (int i = 0; i < gd->n_dimensions; i++) {
        double d = gd->current_state[i] - gd->goal_state[i];
        gd->goal_error += d * d;
    }
    gd->goal_error    = sqrt(gd->goal_error);
    gd->approach_rate = gd->goal_error;
    gd->goal_reached  = (gd->goal_error < 0.01);
}

double wc_goal_distance(const WCGoalDirected* gd) {
    return gd ? gd->goal_error : 0.0;
}

bool wc_goal_reached(const WCGoalDirected* gd, double tolerance) {
    if (!gd) return false;
    return gd->goal_error < tolerance;
}

/*
 * References:
 * Wiener (1948) Cybernetics. MIT Press. (Ch. I-IV core concepts)
 * Wiener (1950) The Human Use of Human Beings. Houghton Mifflin.
 * Rosenblueth, Wiener, Bigelow (1943) Behavior, Purpose and Teleology.
 *   Philosophy of Science 10(1):18-24.
 * Ashby (1956) An Introduction to Cybernetics. Chapman & Hall.
 * Shannon (1948) A Mathematical Theory of Communication.
 *   Bell System Technical Journal 27:379-423, 623-656.
 */
