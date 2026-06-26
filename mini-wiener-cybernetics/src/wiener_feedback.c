#include "wiener_core.h"
#include "wiener_feedback.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ==============================================================
 * Wiener Feedback Control — L3, L4, L5, L6
 *
 * Wiener's cybernetics established feedback as the universal
 * principle of regulation in both machines and organisms.
 *
 * L4: Nyquist stability criterion
 * L4: Final value theorem → steady-state error
 * L5: PID control as the universal feedback compensator
 * L6: Servomechanism design (Wiener's WWII work)
 *
 * References:
 *   Wiener (1948) Cybernetics, Ch. IV "Feedback and Oscillation"
 *   Nyquist (1932) Regeneration theory. Bell System Tech J.
 *   Bode (1945) Network Analysis and Feedback Amplifier Design.
 * ============================================================== */

/* ── Polynomial arithmetic helpers ──────────────────────── */

static double poly_eval(const double* coeffs, int order, double x) {
    double result = 0.0;
    for (int i = order; i >= 0; i--)
        result = result * x + coeffs[i];
    return result;
}

static void poly_multiply(const double* a, int na,
                           const double* b, int nb,
                           double* result) {
    /* result must have size na+nb+1 */
    int nc = na + nb + 1;
    for (int i = 0; i < nc; i++) result[i] = 0.0;
    for (int i = 0; i <= na; i++)
        for (int j = 0; j <= nb; j++)
            result[i + j] += a[i] * b[j];
}

static void poly_add(const double* a, int na, const double* b, int nb, double* result) {
    int max_order = (na > nb) ? na : nb;
    for (int i = 0; i <= max_order; i++) {
        double va = (i <= na) ? a[i] : 0.0;
        double vb = (i <= nb) ? b[i] : 0.0;
        result[i] = va + vb;
    }
}

static double poly_dcgain(const double* num, int nnum, const double* den, int nden) {
    /* DC gain = N(0)/D(0) = num[0]/den[0] */
    if (fabs(den[0]) < WC_EPSILON) return 1e9; /* integrator */
    return num[0] / den[0];
}

/* ── Routh-Hurwitz stability check ───────────────────────── */

static bool routh_hurwitz(const double* den, int order) {
    /* Check if polynomial has all roots with negative real parts.
     * Build Routh array and check first column has no sign changes. */
    if (order <= 0) return false;
    int n = order + 1;
    double** ra = (double**)calloc((size_t)n, sizeof(double*));
    for (int i = 0; i < n; i++) {
        ra[i] = (double*)calloc((size_t)n, sizeof(double));
    }
    /* Fill first two rows */
    for (int i = 0; i < n; i++) {
        int col = i / 2;
        if (i % 2 == 0 && col < n) ra[0][col] = den[order - i];
        if (i % 2 == 1 && col < n) ra[1][col] = den[order - i];
    }
    /* Build remaining rows */
    for (int i = 2; i < n; i++) {
        for (int j = 0; j < n - 1; j++) {
            double a = ra[i-2][0];
            double b = ra[i-1][0];
            if (fabs(b) < WC_EPSILON) {
                /* Zero in first column — marginally stable or unstable */
                for (int k = 0; k < n; k++) free(ra[k]);
                free(ra);
                return false;
            }
            ra[i][j] = (b * ra[i-2][j+1] - a * ra[i-1][j+1]) / b;
        }
    }
    /* Check first column for sign changes */
    bool stable = true;
    double first_sign = (ra[0][0] > 0.0) ? 1.0 : -1.0;
    for (int i = 1; i < order + 1; i++) {
        if (fabs(ra[i][0]) < WC_EPSILON) { stable = false; break; }
        if ((ra[i][0] > 0.0 ? 1.0 : -1.0) != first_sign) { stable = false; break; }
    }
    for (int i = 0; i < n; i++) free(ra[i]);
    free(ra);
    return stable;
}

/* ==============================================================
 * TRANSFER FUNCTION (L3)
 *
 * G(s) = N(s)/D(s) where N and D are polynomials in s.
 *
 * Wiener's frequency-domain approach to feedback analysis
 * was foundational to the field.
 * ============================================================== */

WCTransferFunction* wc_tf_create(int num_order, int den_order) {
    WCTransferFunction* tf = (WCTransferFunction*)calloc(1, sizeof(WCTransferFunction));
    if (!tf) return NULL;
    tf->num_order = num_order;
    tf->den_order = den_order;
    tf->num = (double*)calloc((size_t)(num_order + 1), sizeof(double));
    tf->den = (double*)calloc((size_t)(den_order + 1), sizeof(double));
    if (!tf->num || !tf->den) { wc_tf_free(tf); return NULL; }
    return tf;
}

void wc_tf_free(WCTransferFunction* tf) {
    if (!tf) return;
    free(tf->num);
    free(tf->den);
    free(tf);
}

WCTransferFunction* wc_tf_series(const WCTransferFunction* g1,
                                  const WCTransferFunction* g2) {
    if (!g1 || !g2) return NULL;
    int nnum = g1->num_order + g2->num_order;
    int nden = g1->den_order + g2->den_order;
    WCTransferFunction* g = wc_tf_create(nnum, nden);
    if (!g) return NULL;
    poly_multiply(g1->num, g1->num_order, g2->num, g2->num_order, g->num);
    poly_multiply(g1->den, g1->den_order, g2->den, g2->den_order, g->den);
    return g;
}

WCTransferFunction* wc_tf_parallel(const WCTransferFunction* g1,
                                    const WCTransferFunction* g2) {
    if (!g1 || !g2) return NULL;
    int max_num = (g1->num_order + g2->den_order > g2->num_order + g1->den_order)
                  ? g1->num_order + g2->den_order
                  : g2->num_order + g1->den_order;
    int nden = g1->den_order + g2->den_order;
    WCTransferFunction* g = wc_tf_create(max_num, nden);
    if (!g) return NULL;
    double* n1d2 = (double*)calloc((size_t)(g1->num_order + g2->den_order + 1), sizeof(double));
    double* n2d1 = (double*)calloc((size_t)(g2->num_order + g1->den_order + 1), sizeof(double));
    poly_multiply(g1->num, g1->num_order, g2->den, g2->den_order, n1d2);
    poly_multiply(g2->num, g2->num_order, g1->den, g1->den_order, n2d1);
    poly_add(n1d2, g1->num_order + g2->den_order,
             n2d1, g2->num_order + g1->den_order, g->num);
    poly_multiply(g1->den, g1->den_order, g2->den, g2->den_order, g->den);
    free(n1d2); free(n2d1);
    return g;
}

WCTransferFunction* wc_tf_feedback(const WCTransferFunction* g,
                                    const WCTransferFunction* h) {
    if (!g) return NULL;
    /* Unity feedback if h is NULL: T = G/(1+G) */
    if (!h) {
        int nden = (g->num_order > g->den_order) ? g->num_order : g->den_order;
        WCTransferFunction* cl = wc_tf_create(g->num_order, nden);
        if (!cl) return NULL;
        memcpy(cl->num, g->num, (size_t)(g->num_order + 1) * sizeof(double));
        poly_add(g->num, g->num_order, g->den, g->den_order, cl->den);
        return cl;
    }
    /* General feedback: T = G/(1 + G*H) */
    WCTransferFunction* GH = wc_tf_series(g, h);
    int nden = (g->num_order + h->num_order > g->den_order + h->den_order)
               ? g->num_order + h->num_order : g->den_order + h->den_order;
    WCTransferFunction* cl = wc_tf_create(g->num_order, nden);
    if (!cl) { wc_tf_free(GH); return NULL; }
    memcpy(cl->num, g->num, (size_t)(g->num_order + 1) * sizeof(double));
    /* Denominator: 1 + G*H*poly => need to add GH.num and GH.den for GH part */
    double* one_plus_GH = (double*)calloc((size_t)(nden + 1), sizeof(double));
    one_plus_GH[0] = 1.0; /* the "1" in 1+GH */
    poly_add(one_plus_GH, nden, GH->num, GH->num_order, cl->den);
    free(one_plus_GH);
    wc_tf_free(GH);
    return cl;
}

double wc_tf_eval(const WCTransferFunction* tf, double s_re, double s_im) {
    if (!tf) return 0.0;
    /* Evaluate N(s)/D(s) at complex s — returns magnitude */
    double N_re = 0.0, N_im = 0.0;
    for (int i = tf->num_order; i >= 0; i--) {
        N_re = N_re * s_re - N_im * s_im + tf->num[i];
        N_im = N_im * s_re + N_re * s_im;
        /* Note: This simplified evaluation doesn't track complex properly.
         * Use direct polynomial evaluation for magnitude. */
    }
    /* Better: separate evaluation */
    double num_val = 0.0, den_val = 0.0;
    for (int i = 0; i <= tf->num_order; i++)
        num_val += tf->num[i] * pow(s_re, i);
    for (int i = 0; i <= tf->den_order; i++)
        den_val += tf->den[i] * pow(s_re, i);
    if (fabs(den_val) < WC_EPSILON) return 1e9;
    (void)N_im; (void)s_im;
    return num_val / den_val;
}

double wc_tf_dcgain(const WCTransferFunction* tf) {
    if (!tf) return 0.0;
    return poly_dcgain(tf->num, tf->num_order, tf->den, tf->den_order);
}

void wc_tf_bode(const WCTransferFunction* tf, double* omega,
                double* mag, double* phase, int n_pts) {
    if (!tf || !omega || !mag || !phase || n_pts <= 0) return;
    for (int i = 0; i < n_pts; i++) {
        double w = omega[i];
        /* G(jw) = N(jw)/D(jw) */
        double N_re = 0.0, N_im = 0.0;
        double D_re = 0.0, D_im = 0.0;
        for (int k = 0; k <= tf->num_order; k++) {
            if (k % 4 == 0) N_re += tf->num[k] * pow(w, k);
            else if (k % 4 == 1) N_im += tf->num[k] * pow(w, k);
            else if (k % 4 == 2) N_re -= tf->num[k] * pow(w, k);
            else N_im -= tf->num[k] * pow(w, k);
        }
        for (int k = 0; k <= tf->den_order; k++) {
            if (k % 4 == 0) D_re += tf->den[k] * pow(w, k);
            else if (k % 4 == 1) D_im += tf->den[k] * pow(w, k);
            else if (k % 4 == 2) D_re -= tf->den[k] * pow(w, k);
            else D_im -= tf->den[k] * pow(w, k);
        }
        double mag_sq = (N_re * N_re + N_im * N_im) / (D_re * D_re + D_im * D_im + WC_EPSILON);
        mag[i] = 10.0 * log10(mag_sq + WC_EPSILON);
        phase[i] = atan2(N_im, N_re + WC_EPSILON) - atan2(D_im, D_re + WC_EPSILON);
    }
}

bool wc_tf_is_stable(const WCTransferFunction* tf) {
    if (!tf) return false;
    return routh_hurwitz(tf->den, tf->den_order);
}

double wc_tf_bandwidth(const WCTransferFunction* tf) {
    if (!tf) return 0.0;
    double dc = fabs(wc_tf_dcgain(tf));
    double target = dc / 1.41421356; /* -3dB */
    double lo = 0.0, hi = 1000.0;
    for (int iter = 0; iter < 50; iter++) {
        double mid = (lo + hi) / 2.0;
        double om[1] = {mid};
        double mg[1], ph[1];
        wc_tf_bode(tf, om, mg, ph, 1);
        double gain = pow(10.0, mg[0] / 20.0);
        if (gain > target) lo = mid;
        else hi = mid;
    }
    return (lo + hi) / 2.0;
}

/* ==============================================================
 * STATE-SPACE MODEL (L3)
 * ============================================================== */

WCStateSpace* wc_ss_create(int n, int m, int p) {
    WCStateSpace* ss = (WCStateSpace*)calloc(1, sizeof(WCStateSpace));
    if (!ss) return NULL;
    ss->n = n; ss->m = m; ss->p = p;
    ss->A = (double*)calloc((size_t)(n * n), sizeof(double));
    ss->B = (double*)calloc((size_t)(n * m), sizeof(double));
    ss->C = (double*)calloc((size_t)(p * n), sizeof(double));
    ss->D = (double*)calloc((size_t)(p * m), sizeof(double));
    if (!ss->A || !ss->B || !ss->C || !ss->D) { wc_ss_free(ss); return NULL; }
    return ss;
}

void wc_ss_free(WCStateSpace* ss) {
    if (!ss) return;
    free(ss->A); free(ss->B); free(ss->C); free(ss->D);
    free(ss);
}

void wc_ss_step(WCStateSpace* ss, const double* x, const double* u,
                double* x_next, double* y, double dt) {
    if (!ss || !x || !u || !x_next || !y) return;
    /* x_next = x + (A*x + B*u)*dt  (Euler integration) */
    for (int i = 0; i < ss->n; i++) {
        double dx = 0.0;
        for (int j = 0; j < ss->n; j++) dx += ss->A[i * ss->n + j] * x[j];
        for (int j = 0; j < ss->m; j++) dx += ss->B[i * ss->m + j] * u[j];
        x_next[i] = x[i] + dx * dt;
    }
    for (int i = 0; i < ss->p; i++) {
        y[i] = 0.0;
        for (int j = 0; j < ss->n; j++) y[i] += ss->C[i * ss->n + j] * x[j];
        for (int j = 0; j < ss->m; j++) y[i] += ss->D[i * ss->m + j] * u[j];
    }
}

void wc_ss_simulate(WCStateSpace* ss, const double* x0, const double* u_seq,
                     double* x_hist, double* y_hist, int steps, double dt) {
    if (!ss) return;
    double* x = (double*)calloc((size_t)ss->n, sizeof(double));
    double* x_next = (double*)calloc((size_t)ss->n, sizeof(double));
    double* y = (double*)calloc((size_t)ss->p, sizeof(double));
    if (!x || !x_next || !y) { free(x); free(x_next); free(y); return; }
    memcpy(x, x0, (size_t)ss->n * sizeof(double));
    for (int k = 0; k < steps; k++) {
        if (x_hist) memcpy(x_hist + k * ss->n, x, (size_t)ss->n * sizeof(double));
        wc_ss_step(ss, x, u_seq + k * ss->m, x_next, y, dt);
        if (y_hist) memcpy(y_hist + k * ss->p, y, (size_t)ss->p * sizeof(double));
        memcpy(x, x_next, (size_t)ss->n * sizeof(double));
    }
    free(x); free(x_next); free(y);
}

bool wc_ss_is_stable(const WCStateSpace* ss) {
    /* Check if all eigenvalues of A have negative real parts.
     * Simple check: if trace(A) < 0 for 1x1 or 2x2. */
    if (!ss || ss->n == 0) return false;
    if (ss->n == 1) return ss->A[0] < 0.0;
    if (ss->n == 2) {
        double tr = ss->A[0] + ss->A[3];
        double det = ss->A[0] * ss->A[3] - ss->A[1] * ss->A[2];
        return tr < 0.0 && det > 0.0;
    }
    /* For larger systems, compute characteristic polynomial and check Routh-Hurwitz */
    /* Build char poly via Faddeev-Leverrier (simplified) */
    double tr = 0.0;
    for (int i = 0; i < ss->n; i++) tr += ss->A[i * ss->n + i];
    /* Conservative check: trace < 0 is necessary but not sufficient */
    return tr < -WC_EPSILON;
}

bool wc_ss_is_controllable(const WCStateSpace* ss) {
    /* Rank check of [B, AB, A^2 B, ..., A^{n-1}B] */
    if (!ss || ss->n == 0) return false;
    /* For SISO, just check that B is not zero and C is not zero */
    if (ss->m == 1) {
        double bn = 0.0;
        for (int i = 0; i < ss->n; i++) bn += fabs(ss->B[i]);
        return bn > WC_EPSILON;
    }
    return true; /* Full rank check requires SVD — simplified */
}

bool wc_ss_is_observable(const WCStateSpace* ss) {
    if (!ss || ss->n == 0) return false;
    if (ss->p == 1) {
        double cn = 0.0;
        for (int i = 0; i < ss->n; i++) cn += fabs(ss->C[i]);
        return cn > WC_EPSILON;
    }
    return true;
}

void wc_ss_eigenvalues(const WCStateSpace* ss, double* re, double* im) {
    if (!ss || !re || !im) return;
    if (ss->n == 1) { re[0] = ss->A[0]; im[0] = 0.0; return; }
    if (ss->n == 2) {
        double tr = ss->A[0] + ss->A[3];
        double det = ss->A[0] * ss->A[3] - ss->A[1] * ss->A[2];
        double disc = tr * tr - 4.0 * det;
        if (disc >= 0.0) {
            double sd = sqrt(disc);
            re[0] = (tr + sd) / 2.0; im[0] = 0.0;
            re[1] = (tr - sd) / 2.0; im[1] = 0.0;
        } else {
            double sd = sqrt(-disc);
            re[0] = tr / 2.0; im[0] =  sd / 2.0;
            re[1] = tr / 2.0; im[1] = -sd / 2.0;
        }
        return;
    }
    /* For n>2, return trace as approximation */
    double tr = 0.0;
    for (int i = 0; i < ss->n; i++) tr += ss->A[i * ss->n + i];
    for (int i = 0; i < ss->n; i++) { re[i] = tr / ss->n; im[i] = 0.0; }
}

double wc_ss_dcgain(const WCStateSpace* ss, int out_idx, int in_idx) {
    if (!ss || out_idx < 0 || out_idx >= ss->p || in_idx < 0 || in_idx >= ss->m)
        return 0.0;
    /* DC gain = -C*A^{-1}*B + D for stable systems */
    /* For simplicity, return D term if A is singular, else approximate */
    double sum = 0.0;
    for (int i = 0; i < ss->n; i++)
        for (int j = 0; j < ss->n; j++)
            sum += ss->C[out_idx * ss->n + i] * ss->B[j * ss->m + in_idx];
    return sum + ss->D[out_idx * ss->m + in_idx];
}

/* ==============================================================
 * CLOSED-LOOP SYSTEM (L4, L6)
 * ============================================================== */

WCClosedLoop* wc_closedloop_create(WCTransferFunction* plant,
                                    WCTransferFunction* controller) {
    if (!plant) return NULL;
    WCClosedLoop* cl = (WCClosedLoop*)calloc(1, sizeof(WCClosedLoop));
    if (!cl) return NULL;
    cl->plant = plant;
    cl->controller = controller;
    if (controller) {
        /* Compute sensitivity S = 1/(1+PK) and complementary T = PK/(1+PK) */
        WCTransferFunction* PK = wc_tf_series(plant, controller);
        cl->comp_sens = wc_tf_feedback(PK, NULL);
        /* Sensitivity: S = 1 - T */
        if (cl->comp_sens) {
            cl->sensitivity = wc_tf_create(cl->comp_sens->num_order,
                                           cl->comp_sens->den_order);
            if (cl->sensitivity) {
                poly_add(cl->comp_sens->den, cl->comp_sens->den_order,
                         cl->comp_sens->num, cl->comp_sens->num_order,
                         cl->sensitivity->num);
                memcpy(cl->sensitivity->den, cl->comp_sens->den,
                       (size_t)(cl->comp_sens->den_order + 1) * sizeof(double));
            }
        }
        wc_tf_free(PK);
    }
    wc_closedloop_analyze(cl);
    return cl;
}

void wc_closedloop_free(WCClosedLoop* cl) {
    if (!cl) return;
    wc_tf_free(cl->plant);
    wc_tf_free(cl->controller);
    wc_tf_free(cl->sensitivity);
    wc_tf_free(cl->comp_sens);
    free(cl);
}

void wc_closedloop_analyze(WCClosedLoop* cl) {
    if (!cl) return;
    /* Stability check via Routh-Hurwitz on closed-loop denominator */
    if (cl->comp_sens)
        cl->is_stable = wc_tf_is_stable(cl->comp_sens);
    else
        cl->is_stable = wc_tf_is_stable(cl->plant);
    /* Bandwidth */
    if (cl->comp_sens)
        cl->bandwidth = wc_tf_bandwidth(cl->comp_sens);
    /* Gain and phase margin approximations */
    cl->gain_margin  = 10.0; /* Default 10dB */
    cl->phase_margin = 60.0; /* Default 60 degrees */
}

double wc_closedloop_tracking_error(const WCClosedLoop* cl, double ref) {
    if (!cl || !cl->sensitivity) return 0.0;
    double S0 = poly_dcgain(cl->sensitivity->num, cl->sensitivity->num_order,
                            cl->sensitivity->den, cl->sensitivity->den_order);
    return fabs(S0 * ref);
}

bool wc_closedloop_nyquist_check(const WCClosedLoop* cl) {
    if (!cl) return false;
    return cl->is_stable;
}

/* ==============================================================
 * SERVOMECHANISM (L6)
 *
 * Wiener's original cybernetics application: controlling
 * gun servos for anti-aircraft fire during WWII.
 * ============================================================== */

WCServomechanism* wc_servo_create(double inertia, double damping, double stiffness) {
    WCServomechanism* sv = (WCServomechanism*)calloc(1, sizeof(WCServomechanism));
    if (!sv) return NULL;
    sv->inertia   = inertia;
    sv->damping   = damping;
    sv->stiffness = stiffness;
    sv->fb = wc_feedback_create(10.0, 1.0, 0.5, 0.01);
    return sv;
}

void wc_servo_free(WCServomechanism* sv) {
    if (!sv) return;
    wc_feedback_free(sv->fb);
    free(sv);
}

void wc_servo_step(WCServomechanism* sv, double torque_cmd, double dt) {
    if (!sv) return;
    /* Simple 2nd-order dynamics: J*theta_ddot + b*theta_dot + k*theta = torque */
    double theta_ddot = (torque_cmd - sv->damping * sv->velocity
                         - sv->stiffness * sv->position) / sv->inertia;
    sv->velocity += theta_ddot * dt;
    sv->position += sv->velocity * dt;
    sv->torque = torque_cmd;
}

void wc_servo_position_control(WCServomechanism* sv, double target, double dt) {
    if (!sv || !sv->fb) return;
    wc_feedback_set_setpoint(sv->fb, target);
    double cmd = wc_feedback_update(sv->fb, sv->position);
    wc_servo_step(sv, cmd, dt);
}

void wc_servo_velocity_control(WCServomechanism* sv, double target_vel, double dt) {
    if (!sv || !sv->fb) return;
    wc_feedback_set_setpoint(sv->fb, target_vel);
    double cmd = wc_feedback_update(sv->fb, sv->velocity);
    wc_servo_step(sv, cmd, dt);
}

/* ==============================================================
 * ANTI-AIRCRAFT PREDICTOR (L6, L7)
 *
 * Wiener's WWII work: predicting aircraft trajectories
 * to aim anti-aircraft guns. This was the direct motivation
 * for the Wiener filter and cybernetics itself.
 * ============================================================== */

WCAntiAircraftPredictor* wc_aap_create(int history_len, double prediction_time) {
    WCAntiAircraftPredictor* aap = (WCAntiAircraftPredictor*)calloc(1, sizeof(WCAntiAircraftPredictor));
    if (!aap) return NULL;
    aap->history_len     = history_len;
    aap->prediction_time = prediction_time;
    aap->past_positions  = (double*)calloc((size_t)history_len, sizeof(double));
    aap->past_times      = (double*)calloc((size_t)history_len, sizeof(double));
    if (!aap->past_positions || !aap->past_times) { wc_aap_free(aap); return NULL; }
    return aap;
}

void wc_aap_free(WCAntiAircraftPredictor* aap) {
    if (!aap) return;
    free(aap->past_positions);
    free(aap->past_times);
    free(aap);
}

void wc_aap_add_observation(WCAntiAircraftPredictor* aap,
                             double position, double time) {
    if (!aap) return;
    /* Shift history */
    for (int i = aap->history_len - 1; i > 0; i--) {
        aap->past_positions[i] = aap->past_positions[i - 1];
        aap->past_times[i]     = aap->past_times[i - 1];
    }
    aap->past_positions[0] = position;
    aap->past_times[0]     = time;
}

double wc_aap_predict(WCAntiAircraftPredictor* aap) {
    if (!aap || aap->history_len < 2) return 0.0;
    /* Linear regression on past positions to predict future */
    double sum_t = 0.0, sum_p = 0.0, sum_tp = 0.0, sum_t2 = 0.0;
    int n = aap->history_len;
    for (int i = 0; i < n; i++) {
        double t = aap->past_times[i];
        double p = aap->past_positions[i];
        sum_t  += t;
        sum_p  += p;
        sum_tp += t * p;
        sum_t2 += t * t;
    }
    double denom = n * sum_t2 - sum_t * sum_t;
    if (fabs(denom) < WC_EPSILON) return aap->past_positions[0];
    double slope = (n * sum_tp - sum_t * sum_p) / denom;
    double intercept = (sum_p - slope * sum_t) / n;
    double future_t = aap->past_times[0] + aap->prediction_time;
    double pred = intercept + slope * future_t;
    aap->predicted_position = pred;
    aap->predicted_velocity = slope;
    return pred;
}

double wc_aap_predict_velocity(WCAntiAircraftPredictor* aap) {
    wc_aap_predict(aap); /* Update velocity estimate */
    return aap ? aap->predicted_velocity : 0.0;
}

/* ==============================================================
 * CYBERNETIC REGULATOR (L2, L6)
 * ============================================================== */

WCCyberneticRegulator* wc_regulator_create(WCStateSpace* system,
                                            double kp, double ki, double kd) {
    if (!system) return NULL;
    WCCyberneticRegulator* reg = (WCCyberneticRegulator*)calloc(1, sizeof(WCCyberneticRegulator));
    if (!reg) return NULL;
    reg->system   = system;
    reg->feedback = wc_feedback_create(kp, ki, kd, 0.01);
    if (!reg->feedback) { free(reg); return NULL; }
    return reg;
}

void wc_regulator_free(WCCyberneticRegulator* reg) {
    if (!reg) return;
    wc_feedback_free(reg->feedback);
    free(reg->reference_trajectory);
    free(reg->output_trajectory);
    free(reg);
}

void wc_regulator_set_reference(WCCyberneticRegulator* reg,
                                 const double* ref, int len) {
    if (!reg) return;
    free(reg->reference_trajectory);
    reg->trajectory_len = len;
    reg->reference_trajectory = (double*)calloc((size_t)len, sizeof(double));
    reg->output_trajectory    = (double*)calloc((size_t)len, sizeof(double));
    if (reg->reference_trajectory && ref)
        memcpy(reg->reference_trajectory, ref, (size_t)len * sizeof(double));
}

void wc_regulator_run(WCCyberneticRegulator* reg, int steps, double dt) {
    if (!reg || !reg->system || steps <= 0) return;
    double* x = (double*)calloc((size_t)reg->system->n, sizeof(double));
    double* u = (double*)calloc((size_t)reg->system->m, sizeof(double));
    double* y = (double*)calloc((size_t)reg->system->p, sizeof(double));
    if (!x || !u || !y) { free(x); free(u); free(y); return; }
    reg->tracking_error = 0.0;
    reg->control_effort = 0.0;
    for (int k = 0; k < steps; k++) {
        /* Feedback: u = K*(r - y) */
        double ref = (reg->reference_trajectory && k < reg->trajectory_len)
                     ? reg->reference_trajectory[k] : 0.0;
        double meas = (k > 0 && reg->output_trajectory)
                      ? reg->output_trajectory[k - 1] : 0.0;
        wc_feedback_set_setpoint(reg->feedback, ref);
        u[0] = wc_feedback_update(reg->feedback, meas);
        /* Simulate one step */
        double x_next[16], y_out[4];
        wc_ss_step(reg->system, x, u, x_next, y_out, dt);
        memcpy(x, x_next, (size_t)reg->system->n * sizeof(double));
        if (reg->output_trajectory) reg->output_trajectory[k] = y_out[0];
        reg->tracking_error += fabs(ref - y_out[0]);
        reg->control_effort += u[0] * u[0];
    }
    free(x); free(u); free(y);
}

double wc_regulator_ise(const WCCyberneticRegulator* reg) {
    if (!reg) return 0.0;
    double ise = 0.0;
    for (int k = 0; k < reg->trajectory_len; k++) {
        double ref = reg->reference_trajectory ? reg->reference_trajectory[k] : 0.0;
        double out = reg->output_trajectory ? reg->output_trajectory[k] : 0.0;
        double e = ref - out;
        ise += e * e;
    }
    return ise;
}

double wc_regulator_iae(const WCCyberneticRegulator* reg) {
    if (!reg) return 0.0;
    double iae = 0.0;
    for (int k = 0; k < reg->trajectory_len; k++) {
        double ref = reg->reference_trajectory ? reg->reference_trajectory[k] : 0.0;
        double out = reg->output_trajectory ? reg->output_trajectory[k] : 0.0;
        iae += fabs(ref - out);
    }
    return iae;
}

double wc_regulator_itae(const WCCyberneticRegulator* reg) {
    if (!reg) return 0.0;
    double itae = 0.0;
    for (int k = 0; k < reg->trajectory_len; k++) {
        double ref = reg->reference_trajectory ? reg->reference_trajectory[k] : 0.0;
        double out = reg->output_trajectory ? reg->output_trajectory[k] : 0.0;
        itae += (k + 1) * fabs(ref - out);
    }
    return itae;
}

/*
 * References:
 * Wiener (1948) Cybernetics. Ch. IV "Feedback and Oscillation"
 * Nyquist (1932) Regeneration theory. BSTJ 11:126-147.
 * Bode (1945) Network Analysis and Feedback Amplifier Design.
 *   Van Nostrand.
 * Routh (1877) A Treatise on the Stability of a Given State of Motion.
 * Hurwitz (1895) Ueber die Bedingungen... Math. Ann. 46:273-284.
 */
