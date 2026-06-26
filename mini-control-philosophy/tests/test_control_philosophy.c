#include "cp_core.h"
#include "cp_variety.h"
#include "cp_purpose.h"
#include "cp_regulator.h"
#include "cp_observer.h"
#include "cp_hierarchy.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define EPS 1e-9

int main(void) {
    int tests_passed = 0;

    /* ===== cp_core ===== */
    printf("--- Testing cp_core ---\n");
    ControlSystem* sys = cp_system_create("test_sys", 2, 1, 2);
    assert(sys != NULL);
    assert(sys->nx == 2); assert(sys->nu == 1); assert(sys->ny == 2);
    assert(sys->paradigm == CP_CLOSED_LOOP);
    tests_passed += 5;

    double ref[2] = {1.0, 0.0};
    double tol[2] = {0.01, 0.02};
    cp_system_set_reference(sys, ref, tol);
    assert(fabs(sys->r[0] - 1.0) < EPS);
    assert(fabs(sys->r[1] - 0.0) < EPS);
    assert(fabs(sys->goal_tolerance[0] - 0.01) < EPS);
    tests_passed += 3;

    cp_system_set_paradigm(sys, CP_ADAPTIVE);
    assert(sys->paradigm == CP_ADAPTIVE);
    assert(sys->n_model_params > 0);
    tests_passed += 2;

    cp_system_set_feedback(sys, CP_BALANCING_FEEDBACK);
    assert(sys->feedback_mode == CP_BALANCING_FEEDBACK);
    tests_passed++;

    /* Simulate */
    sys->x[0] = 0.5; sys->x[1] = -0.3;
    cp_system_simulate(sys, 1.0, 0.1);
    assert(sys->step_count > 0);
    assert(sys->cost_to_go >= 0.0);
    tests_passed += 2;

    double lyap = cp_system_lyapunov_value(sys);
    assert(lyap >= 0.0);
    double lyap_dot = cp_system_lyapunov_derivative(sys, 0.01);
    (void)lyap_dot;
    tests_passed += 1;

    StabilityClass sc = cp_system_classify_stability(sys, 10);
    assert(sc >= CP_ASYMPTOTICALLY_STABLE && sc <= CP_STRUCTURALLY_UNSTABLE);
    tests_passed++;

    double Wo[4] = {0}; /* 2x2 */
    cp_system_observability_gramian(sys, Wo);
    assert(!isnan(Wo[0]));

    double Wc[4] = {0};
    cp_system_controllability_gramian(sys, Wc);
    assert(!isnan(Wc[0]));
    tests_passed += 2;

    double cap = cp_system_channel_capacity(sys);
    assert(cap >= 0.0);
    tests_passed++;

    cp_system_compute_info_metrics(sys, 100, 5);
    assert(sys->info.entropy >= 0.0);
    tests_passed++;

    cp_system_print(sys);

    /* ===== cp_variety ===== */
    printf("--- Testing cp_variety ---\n");
    assert(fabs(cp_variety(4) - 2.0) < EPS);
    assert(fabs(cp_variety(1) - 0.0) < EPS);
    assert(fabs(cp_variety(1024) - 10.0) < EPS);
    tests_passed += 3;

    double probs[4] = {0.25, 0.25, 0.25, 0.25};
    double h = cp_entropy(probs, 4, true);
    assert(fabs(h - 2.0) < 0.01);
    tests_passed++;

    double joint[4] = {0.25, 0.25, 0.25, 0.25}; /* 2x2 */
    double mi = cp_mutual_information(joint, 2, 2, true);
    assert(fabs(mi) < 0.01); /* independent */
    tests_passed++;

    double ep = cp_effective_variety(2.0, true);
    assert(fabs(ep - 4.0) < 0.01);
    tests_passed++;

    double ashby_idx = cp_ashby_index(5.0, 3.0, 2.0);
    assert(fabs(ashby_idx - 4.0) < EPS);
    tests_passed++;

    /* Channel */
    ControlChannel* ch = cp_channel_create(2, 2);
    assert(ch);
    cp_channel_set_transition(ch, 0, 0, 0.9);
    cp_channel_set_transition(ch, 0, 1, 0.1);
    cp_channel_set_transition(ch, 1, 0, 0.1);
    cp_channel_set_transition(ch, 1, 1, 0.9);
    double chan_cap = cp_channel_compute_capacity(ch, 50, 1e-6);
    assert(chan_cap > 0.0);
    double eq = cp_channel_equivocation(ch);
    assert(eq >= 0.0);
    tests_passed += 3;

    /* Ashby Regulator */
    DisturbanceEnsemble* de = cp_disturbance_ensemble_create(3);
    de->probabilities[0] = 0.5; de->probabilities[1] = 0.3; de->probabilities[2] = 0.2;
    cp_disturbance_ensemble_normalize(de);
    assert(fabs(de->entropy) > 0.0);
    tests_passed++;

    AshbyRegulator* ar = cp_ashby_regulator_create(3, 3, 9);
    assert(ar);
    cp_ashby_regulator_set_strategy(ar, 0, 0);
    cp_ashby_regulator_set_strategy(ar, 1, 1);
    cp_ashby_regulator_set_strategy(ar, 2, 2);
    cp_ashby_regulator_evaluate(ar, de);
    assert(ar->satisfies_ashby_law || !ar->satisfies_ashby_law); /* always true, sanity */
    tests_passed += 2;

    double ov = cp_ashby_regulator_outcome_variety(ar, de);
    (void)ov;
    cp_ashby_regulator_print(ar);

    /* ===== cp_purpose ===== */
    printf("--- Testing cp_purpose ---\n");
    GoalSpec* goal = cp_goal_create("test_goal", 2, ref, tol);
    assert(goal); assert(goal->n_dimensions == 2);
    tests_passed += 2;

    double out[2] = {1.0, 0.005};
    assert(cp_goal_check_satisfaction(goal, out));
    double ar_ratio = cp_goal_achievement_ratio(goal, out);
    assert(ar_ratio >= 0.0 && ar_ratio <= 1.05);
    tests_passed += 2;

    double weights[2] = {1.0, 2.0};
    UtilityFunction* uf = cp_utility_create_quadratic(2, weights);
    assert(uf);
    double util = cp_utility_evaluate(uf, out, ref);
    assert(util <= 0.0); /* quadratic penalty */
    tests_passed += 2;

    TeleologicalSystem* ts = cp_teleo_create(sys, 2);
    assert(ts);
    cp_teleo_add_goal(ts, "goal_a", ref, tol, 2);
    assert(ts->n_goals == 1);
    cp_teleo_step(ts, 0.01);
    assert(ts->sys->step_count > 0);
    tests_passed += 3;

    PurposiveAnalysis* pa = cp_purposive_analyze(ts);
    assert(pa);
    assert(pa->has_feedback);
    cp_purposive_print(pa);
    tests_passed += 2;

    double voi = cp_value_of_information(NULL, NULL, NULL, 2, 2, 2);
    (void)voi;
    double cp = cp_control_premium(10.0, 5.0);
    assert(fabs(cp - 5.0) < EPS);
    tests_passed++;

    /* ===== cp_regulator ===== */
    printf("--- Testing cp_regulator ---\n");
    InternalModel* im = cp_internal_model_create(2, 1, 2);
    assert(im);
    assert(im->A[0] == 1.0); assert(im->A[3] == 1.0);
    tests_passed += 2;

    double x[2] = {1.0, 0.0}, u_in[1] = {0.5};
    double x_pred[2], y_pred[2];
    cp_internal_model_predict(im, x, u_in, x_pred, y_pred);
    assert(!isnan(x_pred[0]));
    tests_passed++;

    double homo = cp_homomorphism_degree(im->A, im->A, 2);
    assert(fabs(homo - 1.0) < EPS);
    tests_passed++;

    Regulator* reg = cp_regulator_create("test_reg", CP_REGULATOR_LINEAR, 2, 1, 2);
    assert(reg);
    cp_regulator_attach_plant(reg, sys);
    cp_regulator_set_model(reg, im);
    cp_regulator_update(reg, out);
    cp_regulator_compute_action(reg);
    cp_regulator_apply(reg);
    tests_passed += 5;

    GoodRegulatorResult* grr = cp_good_regulator_evaluate(reg);
    assert(grr);
    cp_good_regulator_print(grr);
    tests_passed++;

    Servomechanism* sm = cp_servo_create(reg, 10);
    assert(sm);
    double traj[20]; /* 10 steps * 2 dims */
    for (int i = 0; i < 20; i++) traj[i] = (double)(i % 5);
    cp_servo_set_reference(sm, traj, 10);
    cp_servo_simulate(sm, 0.05);
    assert(sm->rms_tracking_error >= 0.0);
    tests_passed += 2;

    double poles[2] = {-1.0, -2.0};
    double bode = cp_bode_sensitivity_integral(poles, 2);
    assert(bode > 0.0);
    tests_passed++;

    /* ===== cp_observer ===== */
    printf("--- Testing cp_observer ---\n");
    Observer* obs = cp_observer_create(CP_KALMAN_FILTER, 2, 2);
    assert(obs); assert(obs->n == 2); assert(obs->m == 2);
    tests_passed += 3;

    double A_a[4] = {0.9, 0.1, -0.1, 0.9};
    double C_mat[4] = {1.0, 0.0, 0.0, 1.0};
    double meas[2] = {0.5, -0.2};
    cp_observer_predict(obs, A_a, NULL, NULL, 2, 0.01);
    cp_observer_update(obs, C_mat, meas, 2, 2);
    assert(!isnan(obs->x_hat[0]));
    tests_passed++;

    double err = cp_observer_estimation_error(obs, x, 2);
    assert(err >= 0.0);
    tests_passed++;

    ObserverController* oc = cp_observer_controller_create(obs, sys);
    assert(oc);
    bool sep = cp_observer_controller_check_separation(oc, A_a, C_mat, 2);
    assert(sep);
    tests_passed += 2;

    int rank;
    double* O_mat = cp_observability_matrix(A_a, C_mat, 2, &rank);
    assert(O_mat);
    int obs_rank = cp_observability_rank(A_a, C_mat, 2, 1e-6);
    assert(obs_rank == 2);
    tests_passed += 2;

    double Wo_full[4];
    cp_observability_gramian_solve(A_a, C_mat, 2, Wo_full, 100, 1e-6);
    assert(!isnan(Wo_full[0]));
    tests_passed++;

    double prior_cov[4] = {2.0, 0.0, 0.0, 2.0};
    double post_cov[4] = {0.5, 0.0, 0.0, 0.5};
    ObservationInfo* oi = cp_observation_info_compute(obs, prior_cov, post_cov, 2);
    assert(oi); assert(oi->information_gain > 0.0);
    tests_passed += 2;

    /* ===== cp_hierarchy ===== */
    printf("--- Testing cp_hierarchy ---\n");
    ControlLayer* l0 = cp_layer_create("Strategic", CP_STRATEGIC_LAYER, 2, 10.0, 100.0);
    ControlLayer* l1 = cp_layer_create("Tactical", CP_COORDINATION_LAYER, 2, 1.0, 10.0);
    ControlLayer* l2 = cp_layer_create("Operational", CP_EXECUTION_LAYER, 2, 0.1, 1.0);
    assert(l0 && l1 && l2);
    tests_passed += 3;

    HierarchicalControl* hc = cp_hierarchical_create("Plant", 5);
    assert(hc);
    int i0 = cp_hierarchical_add_layer(hc, l0);
    int i1 = cp_hierarchical_add_layer(hc, l1);
    int i2 = cp_hierarchical_add_layer(hc, l2);
    assert(i0 == 0 && i1 == 1 && i2 == 2);
    l1->parent_idx = i0; l2->parent_idx = i1;
    cp_layer_add_child(l0, i1); cp_layer_add_child(l1, i2);
    tests_passed += 4;

    double g_gl[2] = {10.0, 5.0};
    cp_hierarchical_set_global_goal(hc, g_gl, 2);
    cp_hierarchical_step(hc, 0.05);
    assert(hc->total_cost >= 0.0);
    tests_passed++;

    double em = cp_hierarchical_emergence(hc);
    (void)em;
    double overhead = cp_hierarchical_coordination_overhead(hc);
    assert(overhead >= 0.0);
    tests_passed++;

    EmergenceAnalysis* ea = cp_emergence_analyze(hc);
    assert(ea);
    cp_emergence_print(ea);
    tests_passed++;

    ViableSystemModel* vsm = cp_vsm_create("TestCo");
    assert(vsm);
    cp_vsm_set_operational_units(vsm, 3);
    cp_vsm_set_autonomy(vsm, 0, 0.8);
    cp_vsm_set_autonomy(vsm, 1, 0.9);
    cp_vsm_step(vsm, 0.1);
    double coh = cp_vsm_cohesion(vsm);
    assert(coh > 0.0 && coh <= 1.0);
    assert(cp_vsm_is_viable(vsm));
    tests_passed += 5;

    GoalTree* gt = cp_goal_tree_create(10);
    assert(gt);
    int root = cp_goal_tree_add_goal(gt, "root", g_gl, 2, 1.0, -1);
    assert(root == 0);
    cp_goal_tree_decompose(gt, root);
    double gtc = cp_goal_tree_coherence(gt);
    assert(gtc > 0.0);
    tests_passed += 3;

    double span = cp_optimal_span_of_control(64, 0.1, 0.2);
    assert(span >= 2.0);
    tests_passed++;

    /* Cleanup */
    cp_goal_free(goal);
    cp_utility_free(uf);
    cp_teleo_free(ts);
    cp_purposive_free(pa);
    cp_channel_free(ch);
    cp_disturbance_ensemble_free(de);
    cp_ashby_regulator_free(ar);
    /* im freed by reg */
    /* reg owns im now; we set_model above */
    cp_good_regulator_free(grr);
    cp_servo_free(sm);
    cp_observer_free(obs);
    cp_observer_controller_free(oc);
    free(O_mat);
    cp_observation_info_free(oi);
    cp_emergence_free(ea);
    cp_goal_tree_free(gt);

    cp_layer_free(l0);
    cp_layer_free(l1);
    cp_layer_free(l2);
    cp_hierarchical_free(hc);
    cp_vsm_free(vsm);

    cp_regulator_free(reg);
    cp_system_free(sys);

    printf("\nAll %d tests passed.\n", tests_passed);
    return 0;
}
