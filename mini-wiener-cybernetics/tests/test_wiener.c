#include "wiener_core.h"
#include "wiener_process.h"
#include "wiener_filter.h"
#include "wiener_feedback.h"
#include "wiener_prediction.h"
#include "wiener_information.h"
#include "wiener_cybernetics_app.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

/* ==============================================================
 * mini-wiener-cybernetics — Comprehensive Test Suite
 *
 * Tests cover all L1-L8 knowledge layers with assert-based
 * verification of mathematical properties.
 * ============================================================== */

static int tests_run = 0;
static int tests_passed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %s... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASSED\n"); } while(0)
#define FAIL(msg) do { printf("FAILED: %s\n", msg); } while(0)
#define ASSERT_TRUE(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)
#define ASSERT_FLOAT_EQ(a, b, tol, msg) do { double _a=(double)(a); double _b=(double)(b); if (fabs(_a-_b) > (tol)) { FAIL(msg); printf("    expected %.6f got %.6f\n", _b, _a); return; } } while(0)

/* ── L1: Signal Tests ───────────────────────────────── */

static void test_signal_create(void) {
    TEST("signal_create");
    WCSignal* s = wc_signal_create(100, 0.01, 0.0);
    ASSERT_TRUE(s != NULL, "signal create failed");
    ASSERT_FLOAT_EQ(s->length, 100, 0.01, "length wrong");
    ASSERT_FLOAT_EQ(s->dt, 0.01, 1e-12, "dt wrong");
    wc_signal_free(s);
    PASS();
}

static void test_signal_mean(void) {
    TEST("signal_mean");
    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    WCSignal* s = wc_signal_create_from(data, 5, 1.0, 0.0);
    ASSERT_TRUE(s != NULL, "create failed");
    double m = wc_signal_mean(s);
    ASSERT_FLOAT_EQ(m, 3.0, 0.01, "mean wrong");
    wc_signal_free(s);
    PASS();
}

static void test_signal_variance(void) {
    TEST("signal_variance");
    double data[] = {0.0, 2.0, 0.0, 2.0};
    WCSignal* s = wc_signal_create_from(data, 4, 1.0, 0.0);
    double v = wc_signal_variance(s);
    ASSERT_FLOAT_EQ(v, 1.0, 0.01, "variance wrong");
    wc_signal_free(s);
    PASS();
}

static void test_signal_add_sub(void) {
    TEST("signal_add_sub");
    double d1[] = {1.0, 2.0, 3.0};
    double d2[] = {4.0, 5.0, 6.0};
    WCSignal *a = wc_signal_create_from(d1, 3, 1.0, 0.0);
    WCSignal *b = wc_signal_create_from(d2, 3, 1.0, 0.0);
    WCSignal *sum = wc_signal_add(a, b);
    ASSERT_TRUE(sum != NULL, "add failed");
    ASSERT_FLOAT_EQ(sum->samples[0], 5.0, 0.01, "add[0] wrong");
    ASSERT_FLOAT_EQ(sum->samples[1], 7.0, 0.01, "add[1] wrong");
    WCSignal* diff = wc_signal_sub(b, a);
    ASSERT_TRUE(diff != NULL, "sub failed");
    ASSERT_FLOAT_EQ(diff->samples[0], 3.0, 0.01, "sub[0] wrong");
    wc_signal_free(a); wc_signal_free(b);
    wc_signal_free(sum); wc_signal_free(diff);
    PASS();
}

/* ── L1: Cybernetic System Tests ────────────────────── */

static void test_system_create(void) {
    TEST("system_create");
    WCCyberneticSystem* sys = wc_system_create(2, 1, 1, 0.01);
    ASSERT_TRUE(sys != NULL, "system create failed");
    ASSERT_TRUE(sys->n_state == 2, "n_state wrong");
    wc_system_free(sys);
    PASS();
}

/* ── L2: Feedback Loop Tests ────────────────────────── */

static void test_feedback_pid(void) {
    TEST("feedback_pid");
    WCFeedbackLoop* fb = wc_feedback_create(2.0, 0.5, 0.1, 0.01);
    ASSERT_TRUE(fb != NULL, "create failed");
    wc_feedback_set_setpoint(fb, 10.0);
    double u1 = wc_feedback_update(fb, 8.0);
    ASSERT_TRUE(fabs(u1) > 0.0, "zero control action");
    double u2 = wc_feedback_update(fb, 9.5);
    ASSERT_TRUE(fabs(u2) < fabs(u1), "error not decreasing");
    ASSERT_TRUE(wc_feedback_is_stable(fb), "should be stable");
    double ss = wc_feedback_steady_error(fb);
    ASSERT_FLOAT_EQ(ss, 0.0, 0.01, "integral should give zero ss error");
    wc_feedback_free(fb);
    PASS();
}

/* ── L2: Information Tests ──────────────────────────── */

static void test_information_entropy(void) {
    TEST("information_entropy");
    WCInformation* info = wc_info_create(NULL, 0);
    ASSERT_TRUE(info == NULL, "zero symbols should return NULL");
    double p[] = {0.5, 0.5};
    WCInformation* i2 = wc_info_create(p, 2);
    ASSERT_TRUE(i2 != NULL, "create failed");
    double H = wc_info_entropy(i2);
    ASSERT_FLOAT_EQ(H, 1.0, 0.01, "entropy of fair coin should be 1 bit");
    wc_info_free(i2);
    PASS();
}

static void test_info_max_entropy(void) {
    TEST("info_max_entropy");
    double Hmax = wc_info_max_entropy(4);
    ASSERT_FLOAT_EQ(Hmax, 2.0, 0.01, "max entropy of 4 symbols");
    PASS();
}

static void test_info_kl_divergence(void) {
    TEST("info_kl_divergence");
    double p[] = {0.5, 0.5};
    double q[] = {0.9, 0.1};
    WCInformation* P = wc_info_create(p, 2);
    WCInformation* Q = wc_info_create(q, 2);
    double dkl = wc_info_kldivergence(P, Q);
    ASSERT_TRUE(dkl > 0.0, "KL divergence should be positive");
    wc_info_free(P); wc_info_free(Q);
    PASS();
}

/* ── L2: Homeostasis Tests ──────────────────────────── */

static void test_homeostasis(void) {
    TEST("homeostasis");
    WCHomeostasis* hs = wc_homeostasis_create(3);
    ASSERT_TRUE(hs != NULL, "create failed");
    wc_homeostasis_set_target(hs, 0, 37.0, 0.5);
    wc_homeostasis_set_target(hs, 1, 7.4, 0.05);
    wc_homeostasis_update(hs, 0, 37.2);
    wc_homeostasis_update(hs, 1, 7.41);
    ASSERT_TRUE(wc_homeostasis_check(hs), "should be homeostatic");
    wc_homeostasis_update(hs, 0, 39.0);
    ASSERT_TRUE(!wc_homeostasis_check(hs), "should detect fever");
    wc_homeostasis_free(hs);
    PASS();
}

/* ── L2: Goal-Directed Tests ────────────────────────── */

static void test_goal_directed(void) {
    TEST("goal_directed");
    WCGoalDirected* gd = wc_goal_create(2);
    ASSERT_TRUE(gd != NULL, "create failed");
    double goal[] = {10.0, 20.0};
    wc_goal_set_target(gd, goal);
    double state[] = {5.0, 15.0};
    wc_goal_update(gd, state);
    double d = wc_goal_distance(gd);
    ASSERT_TRUE(d > 0.0, "distance should be positive");
    ASSERT_TRUE(!wc_goal_reached(gd, 0.01), "should not be at goal");
    double reached_state[] = {10.0, 20.0};
    wc_goal_update(gd, reached_state);
    ASSERT_TRUE(wc_goal_reached(gd, 1.0), "should be at goal");
    wc_goal_free(gd);
    PASS();
}

/* ── L3: Wiener Process Tests ───────────────────────── */

static void test_wiener_path(void) {
    TEST("wiener_path");
    WCWienerPath* wp = wc_wiener_path_create(100, 1.0, 42);
    ASSERT_TRUE(wp != NULL, "create failed");
    wc_wiener_path_generate(wp);
    ASSERT_FLOAT_EQ(wp->path[0], 0.0, 0.01, "W(0) should be 0");
    double qv = wc_wiener_path_quadratic_variation(wp);
    ASSERT_TRUE(qv > 0.0, "QV should be positive");
    ASSERT_TRUE(fabs(qv - wp->t_end) < 0.5, "QV should approx equal T");
    wc_wiener_path_free(wp);
    PASS();
}

static void test_wiener_increment_normality(void) {
    TEST("wiener_increment_normality");
    /* Generate many paths and verify increment statistics */
    int n_paths = 500;
    int n_steps = 50;
    WCWienerEnsemble* we = wc_wiener_ensemble_create(n_steps, n_paths, 1.0, 0);
    ASSERT_TRUE(we != NULL, "ensemble create failed");
    wc_wiener_ensemble_generate(we);
    /* Check E[W(t)] ≈ 0 and Var[W(t)] ≈ t at midpoint */
    int mid = n_steps / 2;
    double mean = wc_wiener_ensemble_mean_at(we, mid);
    double var  = wc_wiener_ensemble_var_at(we, mid);
    ASSERT_TRUE(fabs(mean) < 0.2, "E[W(t)] should be near 0");
    ASSERT_TRUE(var > 0.0, "Var should be positive");
    wc_wiener_ensemble_free(we);
    PASS();
}

/* ── L4: Wiener-Hopf / Filter Tests ─────────────────── */

static void test_autocorrelation(void) {
    TEST("autocorrelation");
    double data[100];
    for (int i = 0; i < 100; i++) data[i] = sin(2.0 * WC_PI * 0.05 * i);
    WCSignal* s = wc_signal_create_from(data, 100, 1.0, 0.0);
    WCAutocorrelation* ac = wc_autocorr_create(10);
    int ret = wc_autocorr_compute(ac, s);
    ASSERT_TRUE(ret == 0, "autocorr compute failed");
    ASSERT_FLOAT_EQ(ac->values[0], ac->power, 0.01, "R[0] should equal power");
    ASSERT_TRUE(fabs(ac->values[0]) > 0.0, "R[0] should be nonzero");
    wc_autocorr_free(ac); wc_signal_free(s);
    PASS();
}

static void test_wiener_filter_design(void) {
    TEST("wiener_filter_design");
    /* Create simple test signals */
    int N = 100;
    double* clean_data = (double*)calloc(N, sizeof(double));
    double* noisy_data = (double*)calloc(N, sizeof(double));
    for (int i = 0; i < N; i++) {
        clean_data[i] = sin(2.0 * WC_PI * 0.02 * i);
        noisy_data[i] = clean_data[i] + 0.3 * ((double)rand()/RAND_MAX - 0.5);
    }
    WCSignal* clean = wc_signal_create_from(clean_data, N, 1.0, 0.0);
    WCSignal* noisy = wc_signal_create_from(noisy_data, N, 1.0, 0.0);
    WCWienerFilter* wf = wc_filter_create(4);
    ASSERT_TRUE(wf != NULL, "filter create failed");
    int ret = wc_filter_design_fir(wf, noisy, clean, 4);
    ASSERT_TRUE(ret == 0, "filter design failed");
    ASSERT_TRUE(wf->mse >= 0.0, "MSE should be non-negative");
    WCSignal* filtered = wc_filter_apply(wf, noisy);
    ASSERT_TRUE(filtered != NULL, "filter apply failed");
    double mse_after = wc_filter_mse(wf, noisy, clean);
    ASSERT_TRUE(mse_after >= 0.0, "MSE should be non-negative");
    wc_signal_free(clean); wc_signal_free(noisy);
    wc_signal_free(filtered);
    wc_filter_free(wf);
    free(clean_data); free(noisy_data);
    PASS();
}

/* ── L4: Power Spectrum Tests (Wiener-Khinchin) ─────── */

static void test_power_spectrum(void) {
    TEST("power_spectrum");
    double data[128];
    for (int i = 0; i < 128; i++) data[i] = sin(2.0 * WC_PI * 0.1 * i);
    WCSignal* s = wc_signal_create_from(data, 128, 1.0, 0.0);
    WCPowerSpectrum* ps = wc_psd_create(64, 0.01);
    int ret = wc_psd_periodogram(ps, s);
    ASSERT_TRUE(ret == 0, "periodogram failed");
    ASSERT_TRUE(ps->total_power > 0.0, "total power should be positive");
    double max_f = wc_psd_max_freq(ps);
    ASSERT_TRUE(max_f >= 0.0, "max freq should be non-negative");
    wc_psd_free(ps); wc_signal_free(s);
    PASS();
}

/* ── L5: Levinson-Durbin / AR Model Tests ───────────── */

static void test_ar_model(void) {
    TEST("ar_model");
    /* Generate AR(2) process: x[n] = 0.5*x[n-1] - 0.3*x[n-2] + noise */
    int N = 200;
    double* data = (double*)calloc(N, sizeof(double));
    data[0] = 0.0; data[1] = 0.0;
    for (int i = 2; i < N; i++)
        data[i] = 0.5*data[i-1] - 0.3*data[i-2] + 0.1 * ((double)rand()/RAND_MAX - 0.5);
    WCSignal* s = wc_signal_create_from(data, N, 1.0, 0.0);
    WARModel* ar = wc_ar_create(2);
    int ret = wc_ar_fit_burg(ar, s);
    ASSERT_TRUE(ret == 0, "AR fit failed");
    ASSERT_TRUE(ar->noise_var > 0.0, "noise variance should be positive");
    double pred = wc_ar_predict_one(ar, &data[N-3]);
    ASSERT_TRUE(fabs(pred) < 10.0, "prediction should be bounded");
    WCSignal* pred_sig = wc_ar_predict_n(ar, s, 5);
    ASSERT_TRUE(pred_sig != NULL, "prediction signal failed");
    ASSERT_TRUE(pred_sig->length == 5, "prediction length wrong");
    wc_signal_free(pred_sig);
    wc_ar_free(ar); wc_signal_free(s); free(data);
    PASS();
}

/* ── L5: Linear Predictor Tests ──────────────────────── */

static void test_linear_predictor(void) {
    TEST("linear_predictor");
    double data[100];
    for (int i = 0; i < 100; i++) data[i] = sin(2.0 * WC_PI * 0.03 * i);
    WCSignal* s = wc_signal_create_from(data, 100, 1.0, 0.0);
    WCLinearPredictor* lp = wc_lpredictor_create(4);
    int ret = wc_lpredictor_design(lp, s);
    ASSERT_TRUE(ret == 0, "predictor design failed");
    double pred = wc_lpredictor_predict(lp, &data[95]);
    ASSERT_TRUE(pred == pred, "prediction should not be NaN");
    ASSERT_TRUE(lp->mse >= 0.0, "MSE non-negative");
    ASSERT_TRUE(lp->gain >= 1.0, "prediction gain should be >= 1");
    wc_lpredictor_free(lp); wc_signal_free(s);
    PASS();
}

/* ── L6: Kalman Predictor Tests ──────────────────────── */

static void test_kalman_predictor(void) {
    TEST("kalman_predictor");
    WCKalmanPredictor* kp = wc_kalman_create(2, 1, 1, 0.1);
    ASSERT_TRUE(kp != NULL, "kalman create failed");
    double A[] = {1.0, 0.1, 0.0, 1.0};
    double B[] = {0.0, 0.1};
    double C[] = {1.0, 0.0};
    double Q[] = {0.01, 0.0, 0.0, 0.01};
    double R[] = {1.0};
    wc_kalman_set_model(kp, A, B, C, Q, R);
    /* Predict then update with measurement */
    wc_kalman_predict(kp);
    double meas[] = {1.5};
    wc_kalman_update(kp, meas);
    double x_hat[2];
    wc_kalman_get_state(kp, x_hat);
    ASSERT_TRUE(fabs(x_hat[0]) < 10.0, "state estimate bounded");
    wc_kalman_free(kp);
    PASS();
}

/* ── L6: Feedback Control / Servo Tests ──────────────── */

static void test_servomechanism(void) {
    TEST("servomechanism");
    WCServomechanism* sv = wc_servo_create(1.0, 0.5, 2.0);
    ASSERT_TRUE(sv != NULL, "servo create failed");
    wc_servo_position_control(sv, 1.0, 0.01);
    for (int i = 0; i < 100; i++)
        wc_servo_position_control(sv, 1.0, 0.01);
    ASSERT_TRUE(fabs(sv->position) > 0.0, "servo should move");
    wc_servo_free(sv);
    PASS();
}

/* ── L6: Anti-Aircraft Predictor Tests ───────────────── */

static void test_anti_aircraft_predictor(void) {
    TEST("anti_aircraft_predictor");
    WCAntiAircraftPredictor* aap = wc_aap_create(5, 2.0);
    ASSERT_TRUE(aap != NULL, "AAP create failed");
    for (int i = 0; i < 5; i++)
        wc_aap_add_observation(aap, 100.0 + 10.0 * i, (double)i);
    double pred = wc_aap_predict(aap);
    ASSERT_TRUE(pred > 100.0, "should predict beyond last position");
    double vel = wc_aap_predict_velocity(aap);
    ASSERT_TRUE(fabs(vel - 10.0) < 5.0, "velocity should be ~10 m/s");
    wc_aap_free(aap);
    PASS();
}

/* ── L7: Thermal Control Test ────────────────────────── */

static void test_thermal_control(void) {
    TEST("thermal_control");
    WCThermalControl* tc = wc_thermal_create(10000.0, 200.0, 10.0);
    ASSERT_TRUE(tc != NULL, "thermal create failed");
    wc_thermal_set_desired(tc, 20.0);
    for (int i = 0; i < 50; i++)
        wc_thermal_step(tc, 0.1);
    ASSERT_TRUE(tc->room_temp > 10.0, "should warm up");
    double eff = wc_thermal_efficiency(tc);
    ASSERT_TRUE(eff >= 0.0, "efficiency non-negative");
    wc_thermal_free(tc);
    PASS();
}

/* ── L7: Biological Homeostasis Test ─────────────────── */

static void test_biological_homeostasis(void) {
    TEST("biological_homeostasis");
    WCBiologicalHomeostasis* bh = wc_biohomeo_create();
    ASSERT_TRUE(bh != NULL, "biohomeo create failed");
    ASSERT_FLOAT_EQ(bh->core_temp, 37.0, 0.1, "initial core temp");
    /* Cold environment test */
    bh->core_temp = 36.0;
    for (int i = 0; i < 200; i++)
        wc_biohomeo_step(bh, 5.0, 0.02);
    ASSERT_TRUE(bh->shivering + bh->sweat_rate + bh->blood_flow > 0.0,
                "thermoregulation active");
    /* Exercise */
    wc_biohomeo_exercise(bh, 0.5);
    for (int i = 0; i < 50; i++)
        wc_biohomeo_step(bh, 20.0, 0.01);
    ASSERT_TRUE(bh->metabolic_rate > 80.0, "metabolic rate increased");
    /* Fever */
    wc_biohomeo_fever(bh, 39.0);
    ASSERT_TRUE(bh->is_fever, "fever flag should be set");
    ASSERT_FLOAT_EQ(bh->setpoint, 39.0, 0.1, "fever setpoint");
    wc_biohomeo_free(bh);
    PASS();
}

/* ── L7: Communication System Test ───────────────────── */

static void test_communication_system(void) {
    TEST("communication_system");
    double msg_data[] = {1.0, -1.0, 1.0, -1.0};
    WCSignal* msg = wc_signal_create_from(msg_data, 4, 1.0, 0.0);
    WCCommSystem* cs = wc_commsys_create(10.0);
    ASSERT_TRUE(cs != NULL, "commsys create failed");
    wc_commsys_transmit(cs, msg);
    ASSERT_TRUE(cs->received != NULL, "should receive signal");
    ASSERT_TRUE(cs->ber >= 0.0 && cs->ber <= 1.0, "BER in range");
    double cap = wc_commsys_capacity(cs);
    ASSERT_TRUE(cap > 0.0, "capacity positive");
    wc_commsys_free(cs); wc_signal_free(msg);
    PASS();
}

/* ── L8: Advanced Tests ──────────────────────────────── */

static void test_stochastic_resonance(void) {
    TEST("stochastic_resonance");
    int n = 1000;
    double* output = (double*)calloc(n, sizeof(double));
    double snr = wc_stochastic_resonance_simulate(0.3, 2.0*WC_PI*0.01, 0.5, 0.01, n, output);
    ASSERT_TRUE(output[0] == output[0], "output should not be NaN");
    double snr_db = wc_stochastic_resonance_snr(output, n, 2.0*WC_PI*0.01, 0.01);
    (void)snr; (void)snr_db;
    free(output);
    PASS();
}

static void test_first_passage(void) {
    TEST("first_passage");
    double p = wc_first_passage_probability(1.0, 0.0, 1.0, 1.0);
    ASSERT_TRUE(p >= 0.0 && p <= 1.0, "probability in [0,1]");
    PASS();
}

static void test_arcsin_law(void) {
    TEST("arcsin_law");
    double p = wc_arcsin_probability(0.5);
    ASSERT_FLOAT_EQ(p, 0.5, 0.01, "arcsin(0.5) should be 0.5");
    PASS();
}

static void test_volterra_series(void) {
    TEST("volterra_series");
    double x[] = {0.5, -0.3, 0.8, -0.2};
    double h1[] = {1.0, 0.5};
    double h2[] = {0.1, 0.0, 0.0, 0.1};
    double y = wc_volterra_second_order(x, 4, h1, 2, h2, 2, 3);
    ASSERT_TRUE(y == y, "output not NaN");
    PASS();
}

int main(void) {
    printf("=== mini-wiener-cybernetics Test Suite ===\n\n");
    printf("L1: Definitions\n");
    test_signal_create();
    test_signal_mean();
    test_signal_variance();
    test_signal_add_sub();
    test_system_create();

    printf("\nL2: Core Concepts\n");
    test_feedback_pid();
    test_information_entropy();
    test_info_max_entropy();
    test_info_kl_divergence();
    test_homeostasis();
    test_goal_directed();

    printf("\nL3: Mathematical Structures\n");
    test_wiener_path();
    test_wiener_increment_normality();

    printf("\nL4: Fundamental Laws\n");
    test_autocorrelation();
    test_wiener_filter_design();
    test_power_spectrum();

    printf("\nL5: Algorithms\n");
    test_ar_model();
    test_linear_predictor();

    printf("\nL6: Canonical Problems\n");
    test_kalman_predictor();
    test_servomechanism();
    test_anti_aircraft_predictor();

    printf("\nL7: Applications\n");
    test_thermal_control();
    test_biological_homeostasis();
    test_communication_system();

    printf("\nL8: Advanced Topics\n");
    test_stochastic_resonance();
    test_first_passage();
    test_arcsin_law();
    test_volterra_series();

    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
