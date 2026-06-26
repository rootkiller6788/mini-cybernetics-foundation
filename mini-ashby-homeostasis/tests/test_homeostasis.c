#include "ashby_homeostasis.h"
#include "essential_variables.h"
#include "homeostat_model.h"
#include "ultrastability.h"
#include "variety.h"
#include "adaptive_regulator.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define EPS 0.001

static int tests_run = 0;
static int tests_passed = 0;
#define TEST(name) do { tests_run++; printf("  %s... ", name); } while(0)
#define OK() do { tests_passed++; printf("PASSED\n"); } while(0)
#define FAIL(msg) do { printf("FAILED: %s\n", msg); } while(0)
#define ASSERT_EQ(a,b) do { if((a)!=(b)){FAIL("assertion");return;} } while(0)
#define ASSERT_NEAR(a,b,tol) do { if(fabs((a)-(b))>(tol)){printf("FAILED: %.6f != %.6f\n",a,b);return;} } while(0)
#define ASSERT_TRUE(x) do { if(!(x)){FAIL(#x);return;} } while(0)

/* ===== L1: Essential Variables Definitions ===== */
static void test_ev_create(void) {
    TEST("EV set creation");
    EssentialVariableSet* evs = ashby_ev_set_create();
    ASSERT_TRUE(evs != NULL);
    ASSERT_EQ(evs->n_variables, 0);
    ASSERT_EQ(evs->status, HOMEOSTASIS_SAFE);
    ashby_ev_set_free(evs);
    OK();
}

static void test_ev_add(void) {
    TEST("EV addition with bounds");
    EssentialVariableSet* evs = ashby_ev_set_create();
    int idx = ashby_ev_add(evs, "temperature", 35.0, 42.0, true);
    ASSERT_EQ(idx, 0);
    ASSERT_EQ(evs->n_variables, 1);
    ASSERT_NEAR(evs->variables[0].lower_bound, 35.0, EPS);
    ASSERT_NEAR(evs->variables[0].upper_bound, 42.0, EPS);
    ASSERT_TRUE(evs->variables[0].is_critical);
    ashby_ev_set_free(evs);
    OK();
}

static void test_ev_status_check(void) {
    TEST("EV status: safe/warning/critical/violated/dead");
    EssentialVariableSet* evs = ashby_ev_set_create();
    ashby_ev_add(evs, "safe_var", 0.0, 10.0, false);  /* non-critical */
    ashby_ev_add(evs, "crit_var", 0.0, 10.0, true);   /* critical */
    /* Both at midpoints: SAFE */
    ashby_ev_update(evs, 0, 5.0, 0.1);
    ashby_ev_update(evs, 1, 5.0, 0.1);
    ASSERT_EQ(ashby_ev_check_status(evs), HOMEOSTASIS_SAFE);
    /* Non-critical variable in critical zone (>8 but <10): CRITICAL */
    ashby_ev_update(evs, 0, 9.0, 0.1);
    ASSERT_EQ(ashby_ev_check_status(evs), HOMEOSTASIS_CRITICAL);
    /* Non-critical variable violated but critical variable safe */
    ashby_ev_update(evs, 0, 5.0, 0.1); /* restore */
    ashby_ev_update(evs, 1, 11.0, 0.1); /* critical violated -> DEAD */
    ASSERT_EQ(ashby_ev_check_status(evs), HOMEOSTASIS_DEAD);
    ashby_ev_set_free(evs);
    OK();
}

static void test_ev_deviation(void) {
    TEST("EV deviation from setpoint");
    EssentialVariableSet* evs = ashby_ev_set_create();
    ashby_ev_add(evs, "x", 0.0, 10.0, false);
    ashby_ev_update(evs, 0, 5.0, 0.1);
    ASSERT_NEAR(ashby_ev_deviation(evs), 0.0, EPS);
    ashby_ev_update(evs, 0, 7.5, 0.1);
    ASSERT_NEAR(ashby_ev_deviation(evs), 0.5, EPS);
    ashby_ev_set_free(evs);
    OK();
}

static void test_ev_proximity(void) {
    TEST("EV proximity to boundary");
    EssentialVariableSet* evs = ashby_ev_set_create();
    ashby_ev_add(evs, "x", 0.0, 10.0, false);
    ashby_ev_update(evs, 0, 5.0, 0.1);
    ASSERT_NEAR(ashby_ev_proximity_to_boundary(evs, 0), 1.0, EPS);
    ashby_ev_update(evs, 0, 2.0, 0.1);
    ASSERT_NEAR(ashby_ev_proximity_to_boundary(evs, 0), 0.4, EPS);
    ashby_ev_set_free(evs);
    OK();
}

static void test_ev_survivable(void) {
    TEST("EV survivability check");
    EssentialVariableSet* evs = ashby_ev_set_create();
    ashby_ev_add(evs, "critical_var", 0.0, 10.0, true);
    ashby_ev_add(evs, "noncritical_var", 0.0, 10.0, false);
    ashby_ev_update(evs, 0, 5.0, 0.1);
    ashby_ev_update(evs, 1, 15.0, 0.1); /* noncritical violated */
    ASSERT_TRUE(ashby_ev_is_survivable(evs));
    ashby_ev_update(evs, 0, 15.0, 0.1); /* critical violated */
    ASSERT_TRUE(!ashby_ev_is_survivable(evs));
    ashby_ev_set_free(evs);
    OK();
}

static void test_ev_urgency(void) {
    TEST("EV urgency computation");
    EssentialVariable* ev = (EssentialVariable*)calloc(1, sizeof(EssentialVariable));
    ev->name = strdup("test");
    ev->lower_bound = 0.0; ev->upper_bound = 10.0;
    ev->warning_lower = 2.0; ev->warning_upper = 8.0;
    ev->current_value = 5.0; ev->rate_of_change = 0.0;
    ASSERT_NEAR(ashby_ev_urgency(ev), 0.0, EPS);
    ev->current_value = 1.5; ev->rate_of_change = -0.5;
    double urgency = ashby_ev_urgency(ev);
    ASSERT_TRUE(urgency > 0.5);
    free(ev->name); free(ev);
    OK();
}

static void test_ev_multi_ops(void) {
    TEST("Multi-EV operations");
    EssentialVariableSet* evs = ashby_ev_set_create();
    ashby_ev_add(evs, "a", 0.0, 10.0, false);
    ashby_ev_add(evs, "b", 0.0, 20.0, false);
    double vals[] = {7.0, 15.0};
    ashby_ev_set_synchronize(evs, vals, 2);
    ASSERT_NEAR(evs->variables[0].current_value, 7.0, EPS);
    ASSERT_NEAR(evs->variables[1].current_value, 15.0, EPS);
    double vol = ashby_ev_set_survival_volume(evs);
    ASSERT_NEAR(vol, 200.0, EPS);
    int worst = ashby_ev_set_worst_variable(evs);
    ASSERT_TRUE(worst >= 0);
    ashby_ev_set_free(evs);
    OK();
}

/* ===== L2: Environment ===== */
static void test_environment_create(void) {
    TEST("Environment creation");
    AshbyEnvironment* env = ashby_env_create("TestEnv", ENV_CHALLENGING);
    ASSERT_TRUE(env != NULL);
    ASSERT_EQ(env->classification, ENV_CHALLENGING);
    ashby_env_free(env);
    OK();
}

static void test_environment_disturbance(void) {
    TEST("Environment disturbance generation");
    AshbyEnvironment* env = ashby_env_create("Test", ENV_BENIGN);
    double params[] = {1.0, 0.5, 0.0}; /* amp=1, freq=0.5, phase=0 */
    ashby_env_add_disturbance(env, DISTURBANCE_SINUSOIDAL, params, 3, 0.0);
    double v0 = ashby_env_disturbance_value(env, 0, 0.0);
    ASSERT_NEAR(v0, 0.0, EPS);
    double v1 = ashby_env_disturbance_value(env, 0, 1.0);
    ASSERT_NEAR(v1, sin(2.0*3.14159265*0.5*1.0), EPS);
    ashby_env_free(env);
    OK();
}

/* ===== L3: System Dynamics ===== */
static void test_system_create(void) {
    TEST("AshbySystem creation");
    AshbySystem* sys = ashby_system_create("TestSys", 4);
    ASSERT_TRUE(sys != NULL);
    ASSERT_EQ(sys->n_states, 4);
    ASSERT_EQ(sys->regulator_type, REGULATOR_ERROR_DRIVEN);
    ashby_system_free(sys);
    OK();
}

static void test_system_step(void) {
    TEST("AshbySystem single step");
    AshbySystem* sys = ashby_system_create("Test", 2);
    ashby_ev_add(sys->essential_vars, "x", -1.0, 1.0, false);
    ashby_ev_add(sys->essential_vars, "y", -1.0, 1.0, false);
    sys->state_vector[0] = 0.5; sys->state_vector[1] = 0.3;
    ashby_system_set_regulator(sys, REGULATOR_ERROR_DRIVEN);
    ashby_system_step(sys);
    ASSERT_TRUE(sys->time > 0.0);
    ashby_system_free(sys);
    OK();
}

static void test_system_homeostatic_check(void) {
    TEST("Homeostatic state check");
    AshbySystem* sys = ashby_system_create("Test", 2);
    ashby_ev_add(sys->essential_vars, "x", -1.0, 1.0, false);
    sys->state_vector[0] = 0.0; sys->state_vector[1] = 0.0;
    ashby_system_step(sys);
    ASSERT_TRUE(ashby_system_is_homeostatic(sys));
    sys->state_vector[0] = 5.0;
    ashby_ev_update(sys->essential_vars, 0, 5.0, 0.1);
    ASSERT_TRUE(!ashby_system_is_homeostatic(sys));
    ashby_system_free(sys);
    OK();
}

static void test_system_regulation_modes(void) {
    TEST("Regulation mode switching");
    AshbySystem* sys = ashby_system_create("Test", 3);
    ashby_ev_add(sys->essential_vars, "x", -1.0, 1.0, false);
    ashby_ev_add(sys->essential_vars, "y", -1.0, 1.0, false);
    ashby_ev_add(sys->essential_vars, "z", -1.0, 1.0, false);
    RegulatorType modes[] = {REGULATOR_FIXED, REGULATOR_ERROR_DRIVEN,
        REGULATOR_ADAPTIVE, REGULATOR_PREDICTIVE, REGULATOR_LEARNING};
    for (int i = 0; i < 5; i++) {
        ashby_system_set_regulator(sys, modes[i]);
        ASSERT_EQ(sys->regulator_type, modes[i]);
        ashby_system_step(sys);
    }
    ashby_system_free(sys);
    OK();
}

/* ===== L4: Homeostat Model ===== */
static void test_homeostat_unit_create(void) {
    TEST("HomeostatUnit creation");
    HomeostatUnit* unit = homeostat_unit_create(0, 25);
    ASSERT_TRUE(unit != NULL);
    ASSERT_EQ(unit->unit_id, 0);
    ASSERT_EQ(unit->n_positions, 25);
    ASSERT_NEAR(unit->resistance, 100.0, EPS);
    homeostat_unit_free(unit);
    OK();
}

static void test_homeostat_unit_dynamics(void) {
    TEST("HomeostatUnit dynamics integration");
    HomeostatUnit* unit = homeostat_unit_create(0, 10);
    unit->needle_angle = 0.1;
    for (int i = 0; i < 100; i++) homeostat_unit_dynamics(unit, 0.001);
    ASSERT_TRUE(fabs(unit->needle_angle) < 10.0); /* should not diverge */
    homeostat_unit_free(unit);
    OK();
}

static void test_homeostat_unit_stability(void) {
    TEST("HomeostatUnit stability check");
    HomeostatUnit* unit = homeostat_unit_create(0, 10);
    unit->needle_angle = 0.05; unit->stable_angle_center = 0.0;
    ASSERT_TRUE(homeostat_unit_check_stability(unit, 0.1));
    unit->needle_angle = 0.5;
    ASSERT_TRUE(!homeostat_unit_check_stability(unit, 0.1));
    homeostat_unit_free(unit);
    OK();
}

static void test_homeostat_unit_step(void) {
    TEST("HomeostatUnit parameter step (uniselector)");
    HomeostatUnit* unit = homeostat_unit_create(0, 25);
    (void)unit->resistance; /* reference for context */
    homeostat_unit_step_parameter(unit);
    ASSERT_EQ(unit->step_count, 1);
    /* After step, some parameter should have changed (not necessarily R) */
    homeostat_unit_free(unit);
    OK();
}

static void test_homeostat_system_create(void) {
    TEST("HomeostatSystem creation");
    HomeostatSystem* hs = homeostat_system_create(4, 20);
    ASSERT_TRUE(hs != NULL);
    ASSERT_EQ(hs->n_units, 4);
    ASSERT_EQ(hs->time, 0.0);
    homeostat_system_free(hs);
    OK();
}

static void test_homeostat_system_step(void) {
    TEST("HomeostatSystem dynamics");
    HomeostatSystem* hs = homeostat_system_create(4, 10);
    double coupling[16] = {0};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (i != j) coupling[i*4+j] = -0.01;
    homeostat_system_set_connectivity(hs, coupling, 4);
    for (int i = 0; i < 50; i++) homeostat_system_step(hs, 0.01);
    ASSERT_TRUE(hs->time > 0.0);
    ASSERT_TRUE(hs->total_energy >= 0.0);
    homeostat_system_free(hs);
    OK();
}

static void test_homeostat_disturbance(void) {
    TEST("Homeostat disturbance application");
    HomeostatSystem* hs = homeostat_system_create(4, 10);
    double vel_before = hs->units[0].needle_velocity;
    homeostat_system_apply_disturbance(hs, 0, 1.0);
    ASSERT_TRUE(fabs(hs->units[0].needle_velocity - vel_before) > EPS);
    homeostat_system_free(hs);
    OK();
}

static void test_homeostat_energy(void) {
    TEST("Homeostat energy computation");
    HomeostatSystem* hs = homeostat_system_create(2, 5);
    hs->units[0].needle_velocity = 2.0;
    hs->units[0].coil_current = 1.0;
    hs->units[0].needle_angle = 0.5;
    double Ek = homeostat_energy_kinetic(hs);
    double Em = homeostat_energy_magnetic(hs);
    double Ep = homeostat_energy_potential(hs);
    ASSERT_TRUE(Ek > 0.0);
    ASSERT_TRUE(Em > 0.0);
    ASSERT_TRUE(Ep > 0.0);
    double Ploss = homeostat_power_loss(hs);
    ASSERT_TRUE(Ploss > 0.0);
    homeostat_system_free(hs);
    OK();
}

/* ===== L5: Ultrastability ===== */
static void test_param_field_create(void) {
    TEST("ParameterField creation");
    ParameterField* pf = param_field_create("test_field", 4, 100);
    ASSERT_TRUE(pf != NULL);
    ASSERT_EQ(pf->n_params, 4);
    ASSERT_EQ(pf->n_candidates, 100);
    param_field_free(pf);
    OK();
}

static void test_param_field_step(void) {
    TEST("ParameterField step-function");
    ParameterField* pf = param_field_create("test", 3, 50);
    double mins[] = {0, 0, 0}, maxs[] = {10, 10, 10};
    param_field_initialize_random(pf, mins, maxs);
    double* before = (double*)malloc(3 * sizeof(double));
    memcpy(before, pf->values, 3 * sizeof(double));
    param_field_step(pf);
    ASSERT_EQ(pf->n_tried, 1);
    free(before);
    param_field_free(pf);
    OK();
}

static void test_ultrastable_system_create(void) {
    TEST("UltrastableSystem creation");
    UltrastableSystem* us = ultrastable_system_create(ULTRASTABLE_SINGLE);
    ASSERT_TRUE(us != NULL);
    ASSERT_EQ(us->level, ULTRASTABLE_SINGLE);
    ultrastable_system_free(us);
    OK();
}

static void test_ultrastable_trigger(void) {
    TEST("Ultrastable trigger mechanism");
    UltrastableSystem* us = ultrastable_system_create(ULTRASTABLE_SINGLE);
    us->field_primary = param_field_create("main", 2, 20);
    double mins[] = {0, 0}, maxs[] = {1, 1};
    param_field_initialize_random(us->field_primary, mins, maxs);
    ultrastable_system_set_trigger(us, 0.5, 0.2);
    ASSERT_TRUE(ultrastable_system_should_step(us, 0.8));
    ASSERT_TRUE(!ultrastable_system_should_step(us, 0.3));
    ASSERT_TRUE(ultrastable_system_should_stop(us, 0.1));
    ASSERT_TRUE(!ultrastable_system_should_stop(us, 0.5));
    ultrastable_system_free(us);
    OK();
}

/* ===== L6: Variety & Ashby's Law ===== */
static void test_variety_space_create(void) {
    TEST("VarietySpace creation");
    VarietySpace* vs = variety_space_create(100, 0.01);
    ASSERT_TRUE(vs != NULL);
    ASSERT_EQ(vs->n_states, 0);
    variety_space_free(vs);
    OK();
}

static void test_variety_computation(void) {
    TEST("Variety computation (Shannon entropy)");
    VarietySpace* vs = variety_space_create(10, 0.01);
    variety_space_set_uniform(vs, 4);
    variety_space_compute(vs);
    ASSERT_NEAR(vs->total_variety, 2.0, EPS); /* log2(4) = 2 */
    variety_space_free(vs);
    OK();
}

static void test_variety_channel(void) {
    TEST("VarietyChannel and Ashby's Law");
    VarietySpace* vd = variety_space_create(10, 0.01);
    variety_space_set_uniform(vd, 8); /* V(D) = 3 bits */
    variety_space_compute(vd);
    VarietySpace* vr = variety_space_create(10, 0.01);
    variety_space_set_uniform(vr, 4); /* V(R) = 2 bits */
    variety_space_compute(vr);
    VarietySpace* ve = variety_space_create(10, 0.01);
    variety_space_set_uniform(ve, 2); /* V(E) = 1 bit */
    variety_space_compute(ve);
    VarietyChannel* vc = variety_channel_create();
    variety_channel_set_disturbance(vc, vd);
    variety_channel_set_regulation(vc, vr);
    variety_channel_set_essential(vc, ve);
    variety_channel_evaluate(vc);
    ASSERT_TRUE(vc->variety_blocked >= 0.0);
    ASSERT_TRUE(vc->regulatory_efficiency >= 0.0);
    double req_min = variety_requisite_minimum(vd->total_variety, ve->total_variety);
    ASSERT_NEAR(req_min, 2.0, EPS); /* 3 - 1 = 2 */
    variety_channel_free(vc);
    variety_space_free(vd);
    variety_space_free(vr);
    variety_space_free(ve);
    OK();
}

static void test_variety_estimation(void) {
    TEST("Variety estimation from data");
    double samples[] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
                        1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0};
    double V = variety_estimate_from_samples(samples, 20, 0.2);
    ASSERT_TRUE(V > 0.0);
    double V_adapt = variety_estimate_adaptive(samples, 20, 10);
    ASSERT_TRUE(V_adapt > 0.0);
    OK();
}

static void test_variety_engineering(void) {
    TEST("Variety engineering strategies");
    double V_in = 10.0;
    double V_filtered = variety_attenuation_by_filtering(V_in, 0.8);
    ASSERT_TRUE(V_filtered < V_in);
    double V_buffered = variety_attenuation_by_buffering(V_in, 5.0);
    ASSERT_TRUE(V_buffered < V_in);
    double V_pred = variety_attenuation_by_prediction(V_in, 0.9);
    ASSERT_TRUE(V_pred < V_in);
    double amp_needed = variety_amplification_required(20.0, 10.0);
    ASSERT_NEAR(amp_needed, 2.0, EPS);
    OK();
}

/* ===== L7: Adaptive Regulator ===== */
static void test_adaptive_regulator_create(void) {
    TEST("AdaptiveRegulator creation");
    AdaptiveRegulator* reg = adaptive_regulator_create("test", 3, 2);
    ASSERT_TRUE(reg != NULL);
    ASSERT_EQ(reg->n_control_outputs, 2);
    adaptive_regulator_free(reg);
    OK();
}

static void test_pid_action(void) {
    TEST("PID control action");
    AdaptiveRegulator* reg = adaptive_regulator_create("pid_test", 2, 2);
    adaptive_regulator_set_gains(reg, 1.0, 0.1, 0.05);
    double setpoint[] = {1.0, 0.0};
    adaptive_regulator_set_setpoint(reg, setpoint, 2);
    double state[] = {0.0, 0.0};
    double error[2], action[2];
    adaptive_regulator_compute_error(reg, state, error, 2);
    ASSERT_NEAR(error[0], 1.0, EPS);
    adaptive_regulator_pid_action(reg, error, 0.1, action, 2);
    ASSERT_TRUE(action[0] > 0.0); /* positive correction */
    adaptive_regulator_free(reg);
    OK();
}

static void test_model_prediction(void) {
    TEST("Linear model forward prediction");
    AdaptiveRegulator* reg = adaptive_regulator_create("model_test", 2, 1);
    double A[] = {0.9, 0.0, 0.0, 0.9};
    double B[] = {1.0, 0.5};
    adaptive_regulator_set_linear_model(reg, A, B, 2, 1);
    double state[] = {1.0, 2.0};
    double action[] = {0.5};
    double predicted[2];
    adaptive_regulator_predict_forward(reg, state, action, predicted, 2);
    ASSERT_NEAR(predicted[0], 1.0*0.9 + 0.5*1.0, EPS);
    ASSERT_NEAR(predicted[1], 2.0*0.9 + 0.5*0.5, EPS);
    adaptive_regulator_free(reg);
    OK();
}

static void test_bayesian_belief(void) {
    TEST("Bayesian parameter belief");
    BayesianParameterBelief* b = bayesian_belief_create(3);
    double means[] = {1.0, 2.0, 3.0};
    double vars[] = {0.1, 0.2, 0.3};
    bayesian_belief_initialize(b, means, vars, 1.0);
    ASSERT_NEAR(b->param_means[0], 1.0, EPS);
    double unc = bayesian_belief_uncertainty(b);
    ASSERT_NEAR(unc, 0.6, EPS);
    double sample[3];
    bayesian_belief_sample(b, sample);
    bayesian_belief_free(b);
    OK();
}

static void test_conant_ashby(void) {
    TEST("Conant-Ashby isomorphism measure");
    AdaptiveRegulator* reg = adaptive_regulator_create("ca_test", 2, 2);
    reg->n_model_params = 3;
    reg->model_params = (double*)malloc(3 * sizeof(double));
    reg->model_params[0] = 0.9; reg->model_params[1] = 0.1; reg->model_params[2] = 0.5;
    double sys_params[] = {0.9, 0.1, 0.5};
    double iso = conant_ashby_isomorphism_measure(reg, sys_params, 3);
    ASSERT_NEAR(iso, 1.0, EPS);
    bool quality = conant_ashby_regulator_quality_check(reg, 0.01, 0.01, 0.1);
    ASSERT_TRUE(quality);
    adaptive_regulator_free(reg);
    OK();
}

/* ===== L8: Utility Functions ===== */
static void test_matrix_ops(void) {
    TEST("Matrix operations");
    AHMatrix* a = ashby_matrix_create(2, 3);
    ashby_matrix_set(a, 0, 0, 1.0);
    ashby_matrix_set(a, 0, 1, 2.0);
    ashby_matrix_set(a, 0, 2, 3.0);
    ashby_matrix_set(a, 1, 0, 4.0);
    ashby_matrix_set(a, 1, 1, 5.0);
    ashby_matrix_set(a, 1, 2, 6.0);
    ASSERT_NEAR(ashby_matrix_get(a, 0, 1), 2.0, EPS);
    double x[] = {1.0, 1.0, 1.0};
    double y[2] = {0, 0};
    ashby_matrix_vec_mul(a, x, y);
    ASSERT_NEAR(y[0], 6.0, EPS);
    ASSERT_NEAR(y[1], 15.0, EPS);
    ashby_matrix_free(a);
    OK();
}

static void test_timeseries_ops(void) {
    TEST("TimeSeries operations");
    AHTimeSeries* ts = ashby_timeseries_create(1);
    for (int i = 0; i < 100; i++) ashby_timeseries_record(ts, (double)i);
    ASSERT_EQ(ts->length, 100);
    ASSERT_NEAR(ashby_timeseries_mean(ts), 49.5, 1.0);
    double var = ashby_timeseries_variance(ts);
    ASSERT_TRUE(var > 0.0);
    double ac = ashby_timeseries_autocorrelation(ts, 1);
    ASSERT_TRUE(ac > 0.9);
    int crossings = ashby_timeseries_crossing_count(ts, 49.5);
    ASSERT_EQ(crossings, 1);
    ashby_timeseries_free(ts);
    OK();
}

static void test_utility_funcs(void) {
    TEST("General utility functions");
    ASSERT_NEAR(ashby_clamp(5.0, 0.0, 3.0), 3.0, EPS);
    ASSERT_NEAR(ashby_clamp(-1.0, 0.0, 3.0), 0.0, EPS);
    ASSERT_NEAR(ashby_linear_map(5.0, 0.0, 10.0, 0.0, 100.0), 50.0, EPS);
    ASSERT_NEAR(ashby_sigmoid_01(0.0, 1.0), 0.5, EPS);
    double* ls = ashby_linspace(0.0, 10.0, 5);
    ASSERT_NEAR(ls[0], 0.0, EPS);
    ASSERT_NEAR(ls[4], 10.0, EPS);
    free(ls);
    double v[] = {3.0, 4.0};
    ashby_normalize_vector(v, 2);
    ASSERT_NEAR(v[0]*v[0] + v[1]*v[1], 1.0, EPS);
    OK();
}

/* ===== Run all ===== */
int main(void) {
    printf("=== mini-ashby-homeostasis Test Suite ===\n\n");

    printf("L1: Essential Variables\n");
    test_ev_create();
    test_ev_add();
    test_ev_status_check();
    test_ev_deviation();
    test_ev_proximity();
    test_ev_survivable();
    test_ev_urgency();
    test_ev_multi_ops();

    printf("\nL2: Environment\n");
    test_environment_create();
    test_environment_disturbance();

    printf("\nL3: System Dynamics\n");
    test_system_create();
    test_system_step();
    test_system_homeostatic_check();
    test_system_regulation_modes();

    printf("\nL4: Homeostat Model\n");
    test_homeostat_unit_create();
    test_homeostat_unit_dynamics();
    test_homeostat_unit_stability();
    test_homeostat_unit_step();
    test_homeostat_system_create();
    test_homeostat_system_step();
    test_homeostat_disturbance();
    test_homeostat_energy();

    printf("\nL5: Ultrastability\n");
    test_param_field_create();
    test_param_field_step();
    test_ultrastable_system_create();
    test_ultrastable_trigger();

    printf("\nL6: Variety & Ashby's Law\n");
    test_variety_space_create();
    test_variety_computation();
    test_variety_channel();
    test_variety_estimation();
    test_variety_engineering();

    printf("\nL7: Adaptive Regulation\n");
    test_adaptive_regulator_create();
    test_pid_action();
    test_model_prediction();
    test_bayesian_belief();
    test_conant_ashby();

    printf("\nL8: Utilities\n");
    test_matrix_ops();
    test_timeseries_ops();
    test_utility_funcs();

    printf("\n========================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("========================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}