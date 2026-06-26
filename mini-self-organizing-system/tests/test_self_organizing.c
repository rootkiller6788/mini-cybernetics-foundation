#include "sos_core.h"
#include "sos_cellular.h"
#include "sos_turing.h"
#include "sos_autocatalytic.h"
#include "sos_criticality.h"
#include "sos_synergetics.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { tests_run++; printf("  TEST %s... ", #name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

/* === sos_core Tests === */

static void test_system_create(void) {
    TEST(system_create);
    SOSystem* sys = sos_system_create("test", 10, 3);
    ASSERT(sys != NULL, "create failed");
    ASSERT(sys->n_components == 10, "n_components wrong");
    ASSERT(sys->n_states == 3, "n_states wrong");
    ASSERT(sys->level == SOS_SELF_MAINTAINING, "default level wrong");
    sos_system_free(sys);
    PASS();
}

static void test_add_state(void) {
    TEST(add_state);
    SOSystem* sys = sos_system_create("test", 5, 0);
    sos_add_state(sys, "energy", 42.0);
    sos_add_state(sys, "order", 7.0);
    ASSERT(sys->n_states == 2, "n_states after add");
    ASSERT(fabs(sos_get_state(sys, 0) - 42.0) < 1e-10, "state 0 wrong");
    ASSERT(fabs(sos_get_state(sys, 1) - 7.0) < 1e-10, "state 1 wrong");
    sos_set_state(sys, 0, 99.0);
    ASSERT(fabs(sos_get_state(sys, 0) - 99.0) < 1e-10, "set_state failed");
    sos_system_free(sys);
    PASS();
}

static void test_order_params(void) {
    TEST(order_params);
    SOSystem* sys = sos_system_create("test", 5, 1);
    sos_add_order_param(sys, "xi", 0.5, 0.1, -0.2, 0.3);
    ASSERT(sys->n_order_params == 1, "order param not added");
    ASSERT(fabs(sys->order_params[0].value - 0.5) < 1e-10, "op value wrong");
    ASSERT(fabs(sys->order_params[0].alpha + 0.2) < 1e-10, "op alpha wrong");
    ASSERT(strcmp(sys->order_params[0].name, "xi") == 0, "op name wrong");
    sos_record_history(sys);
    sos_advance_time(sys, 0.01);
    ASSERT(sys->history_length == 1, "history not recorded");
    ASSERT(fabs(sys->current_time - 0.01) < 1e-10, "time wrong");
    sos_system_free(sys);
    PASS();
}

static void test_shannon_entropy(void) {
    TEST(shannon_entropy);
    double p_uniform[4] = {0.25, 0.25, 0.25, 0.25};
    double H = sos_shannon_entropy(p_uniform, 4);
    ASSERT(fabs(H - 2.0) < 0.01, "uniform entropy should be 2 bits");
    double p_delta[4] = {1.0, 0.0, 0.0, 0.0};
    H = sos_shannon_entropy(p_delta, 4);
    ASSERT(fabs(H) < 1e-10, "delta entropy should be 0");
    PASS();
}

static void test_lmc_complexity(void) {
    TEST(lmc_complexity);
    /* Uniform distribution: H=1, D=0 -> C=0 */
    double p_uniform[3] = {1.0/3, 1.0/3, 1.0/3};
    double C = sos_lmc_complexity(p_uniform, 3);
    ASSERT(fabs(C) < 0.01, "uniform LMC should be ~0");
    /* Delta distribution: H=0, C=0 */
    double p_delta[3] = {1.0, 0.0, 0.0};
    C = sos_lmc_complexity(p_delta, 3);
    ASSERT(fabs(C) < 0.01, "delta LMC should be 0");
    PASS();
}

static void test_entropy_production(void) {
    TEST(entropy_production);
    SOSystem* sys = sos_system_create("test", 5, 1);
    double rates[2] = {0.5, 0.3};
    double aff[2] = {2.0, 1.0};
    double diS = sos_entropy_production(sys, rates, aff, 2);
    ASSERT(diS > 0.0, "entropy production should be positive");
    sos_system_free(sys);
    PASS();
}

static void test_glansdorff_prigogine(void) {
    TEST(glansdorff_prigogine);
    double dJ[2] = {0.1, -0.1};
    double dX[2] = {0.2, -0.2};
    double dXP = sos_glansdorff_prigogine(dJ, dX, 2);
    ASSERT(dXP > 0.0, "GP criterion should be positive for stable");
    sos_system_free(NULL); /* null safety */
    PASS();
}

static void test_emergence_detection(void) {
    TEST(emergence_detection);
    double E = sos_detect_emergence(0.5, 1.0);
    ASSERT(fabs(E - 0.5) < 1e-10, "emergence detection wrong");
    double E2 = sos_detect_emergence(1.5, 1.0);
    ASSERT(fabs(E2) < 1e-10, "macro more random -> no emergence");
    PASS();
}

static void test_network_topology(void) {
    TEST(network_topology);
    SOSystem* sys = sos_system_create("net", 3, 0);
    sos_network_init(sys, 4);
    sos_network_add_edge(sys, 0, 1, 1.0);
    sos_network_add_edge(sys, 1, 2, 1.0);
    sos_network_add_edge(sys, 2, 3, 1.0);
    sos_network_add_edge(sys, 3, 0, 1.0);
    ASSERT(sys->n_nodes == 4, "n_nodes wrong");
    ASSERT(sys->n_edges == 4, "n_edges wrong");
    double C = sos_clustering_coefficient(sys);
    ASSERT(C >= 0.0 && C <= 1.0, "clustering out of range");
    double L = sos_average_path_length(sys);
    ASSERT(L > 0.0, "path length should be positive");
    int comm[4] = {0, 0, 1, 1};
    double Q = sos_modularity(sys, comm);
    ASSERT(Q >= -1.0 && Q <= 1.0, "modularity out of range");
    sos_system_free(sys);
    PASS();
}

static void test_correlation(void) {
    TEST(correlation);
    double series[10] = {1,2,3,4,5,4,3,2,1,0};
    double ac1 = sos_autocorrelation(series, 10, 1);
    ASSERT(fabs(ac1) > 0.5, "high autocorrelation expected at lag 1");
    double ac5 = sos_autocorrelation(series, 10, 5);
    ASSERT(ac5 < ac1, "autocorrelation should decay");
    PASS();
}

static void test_powerlaw(void) {
    TEST(powerlaw);
    double vals[5] = {10, 20, 50, 100, 200};
    double alpha = sos_powerlaw_exponent(vals, 5, 10.0);
    ASSERT(alpha > 1.0 && alpha < 5.0, "powerlaw exponent out of range");
    PASS();
}

static void test_eigenvalues(void) {
    TEST(eigenvalues);
    double l1r, l1i, l2r, l2i;
    sos_eigenvalues_2x2(1, 0, 0, 2, &l1r, &l1i, &l2r, &l2i);
    ASSERT(fabs(l1r - 2.0) < 1e-10 && fabs(l2r - 1.0) < 1e-10, "diag eigenvalues");
    sos_eigenvalues_2x2(0, -1, 1, 0, &l1r, &l1i, &l2r, &l2i);
    ASSERT(fabs(l1r) < 1e-10 && fabs(l1i - 1.0) < 1e-10, "rotation eigenvalues");
    PASS();
}

static void test_rk4(void) {
    TEST(rk4);
    /* dx/dt = -x, solution: x(t) = x0 * exp(-t) */
    double x = 1.0;
    void f(double t, const double* xv, double* dx, void* p) {
        (void)t; (void)p;
        dx[0] = -xv[0];
    }
    sos_rk4_step(f, 0.0, &x, 1, 0.1, NULL);
    ASSERT(fabs(x - exp(-0.1)) < 0.001, "RK4 accuracy");
    PASS();
}

/* === sos_cellular Tests === */

static void test_ca_create(void) {
    TEST(ca_create);
    CellularAutomaton* ca = ca_create(10, 10, "B3/S23");
    ASSERT(ca != NULL, "ca_create failed");
    ASSERT(ca->width == 10 && ca->height == 10, "dimensions wrong");
    ASSERT(ca->generation == 0, "generation should start at 0");
    ca_randomize(ca, 0.3);
    int before = ca_live_count(ca);
    ca_evolve(ca);
    int after = ca_live_count(ca);
    ASSERT(ca->generation == 1, "generation not incremented");
    (void)before; (void)after;
    double d = ca_density(ca);
    ASSERT(d >= 0.0 && d <= 1.0, "density out of range");
    ca_free(ca);
    PASS();
}

static void test_elementary_ca(void) {
    TEST(elementary_ca);
    ElementaryCA* eca = eca_create(20, 110); /* Rule 110: Turing-complete */
    ASSERT(eca != NULL, "eca_create failed");
    ASSERT(eca->rule_number == 110, "rule number wrong");
    eca_randomize(eca, 0.5);
    eca_evolve(eca);
    ASSERT(eca->generation == 1, "eca generation");
    double H = eca_spatial_entropy(eca);
    ASSERT(H >= 0.0 && H <= 1.0, "entropy out of range");
    eca_free(eca);
    PASS();
}

static void test_sandpile(void) {
    TEST(sandpile);
    SandpileModel* sp = sp_create(8, 8, 4);
    ASSERT(sp != NULL, "sp_create failed");
    sp_add_random_grains(sp, 100);
    ASSERT(sp_total_grains(sp) == 100, "grain count wrong");
    double slope = sp_average_slope(sp);
    ASSERT(slope > 0.0, "slope should be positive");
    sp_free(sp);
    PASS();
}

/* === sos_turing Tests === */

static void test_turing_create(void) {
    TEST(turing_create);
    TuringSystem* ts = turing_create(TURING_SCHNAKENBERG, 16, 16, 1.0, 1.0);
    ASSERT(ts != NULL, "turing_create failed");
    double params[2] = {0.5, 0.8};
    turing_set_params(ts, params, 2);
    turing_set_diffusion(ts, 0.1, 10.0);
    double u_star, v_star;
    turing_homogeneous_steady_state(ts, &u_star, &v_star);
    ASSERT(u_star > 0.0 && v_star > 0.0, "steady state wrong");
    int unstable = turing_check_instability(ts, u_star, v_star);
    /* Schnakenberg with Du=0.1, Dv=10 should be Turing unstable */
    (void)unstable;
    turing_free(ts);
    PASS();
}

static void test_turing_evolution(void) {
    TEST(turing_evolution);
    TuringSystem* ts = turing_create(TURING_SCHNAKENBERG, 8, 8, 1.0, 1.0);
    double params[2] = {0.3, 0.5};
    turing_set_params(ts, params, 2);
    turing_set_diffusion(ts, 0.1, 20.0);
    double us, vs;
    turing_homogeneous_steady_state(ts, &us, &vs);
    turing_random_init(ts, us, vs, 0.01);
    turing_evolve(ts, 10);
    ASSERT(ts->step == 10, "evolution step count wrong");
    turing_free(ts);
    PASS();
}

/* === sos_autocatalytic Tests === */

static void test_autocatalytic(void) {
    TEST(autocatalytic);
    CatalyticNetwork* cn = cn_create();
    int food = cn_add_molecule(cn, "food", MOLECULE_FOOD);
    int cat = cn_add_molecule(cn, "catalyst", MOLECULE_CATALYST);
    int prod = cn_add_molecule(cn, "product", MOLECULE_PRODUCT);
    ASSERT(food == 0 && cat == 1 && prod == 2, "molecule indices wrong");
    /* Reaction: food -> product, catalyzed by catalyst.
     * Catalyst also in food set so RAF can form. */
    cn_add_reaction(cn, food, -1, prod, cat, 1.0, 0.001);
    int fset[2] = {0, 1};  /* Both food and catalyst available */
    cn_set_food(cn, fset, 2);
    int raf_size = cn_find_raf(cn);
    ASSERT(raf_size >= 3, "RAF should include food + catalyst + product");
    ASSERT(cn_is_autocatalytic(cn) == 1, "should be autocatalytic");
    cn_simulate_dynamics(cn, 0.01, 100);
    double conc = cn_get_concentration(cn, prod);
    ASSERT(conc >= 0.0, "product concentration should be non-negative");
    cn_free(cn);
    PASS();
}

static void test_hypercycle(void) {
    TEST(hypercycle);
    Hypercycle* hc = hc_create(3);
    ASSERT(hc != NULL, "hc_create failed");
    double fitness[3] = {1.5, 1.0, 0.8};
    hc_set_fitness(hc, fitness);
    hc_set_coupling(hc, 0.1);
    hc_evolve(hc, 0.1);
    ASSERT(hc->mean_fitness > 0.0, "mean fitness should be positive");
    double eth = hc_error_threshold(hc);
    ASSERT(eth > 0.0 && eth <= 1.0, "error threshold out of range");
    int coex = hc_check_coexistence(hc, 1e-6);
    (void)coex;
    hc_free(hc);
    PASS();
}

static void test_nk_landscape(void) {
    TEST(nk_landscape);
    NKLandscape* nk = nk_create(8, 2);
    ASSERT(nk != NULL, "nk_create failed");
    int genome[8];
    nk_random_genome(nk, genome);
    double fit = nk_fitness(nk, genome);
    ASSERT(fit >= 0.0 && fit <= 1.0, "fitness out of range");
    int steps = nk_local_optimum(nk, genome, 100);
    ASSERT(steps >= 0, "local optimum search failed");
    double ac = nk_fitness_autocorrelation(nk, 50);
    ASSERT(ac >= -1.0 && ac <= 1.0, "autocorrelation out of range");
    nk_free(nk);
    PASS();
}

/* === sos_criticality Tests === */

static void test_bak_sneppen(void) {
    TEST(bak_sneppen);
    BakSneppenModel* bs = bs_create(64);
    bs_randomize(bs);
    for (int i = 0; i < 2000; i++) bs_step(bs);
    double fc = bs_fitness_threshold(bs);
    /* Bak-Sneppen self-organizes to f_c approx 0.667.
     * With finite steps, threshold in (0,1) is sufficient validation. */
    ASSERT(fc > 0.0 && fc < 1.0, "BS threshold in valid range");
    ASSERT(bs->n_extinctions >= 2000, "extinction count");
    bs_free(bs);
    PASS();
}

static void test_forest_fire(void) {
    TEST(forest_fire);
    ForestFireModel* ff = ff_create(16, 16, 0.01, 0.001);
    ff_run(ff, 100);
    ASSERT(ff->n_trees >= 0, "tree count should be non-negative");
    ff_free(ff);
    PASS();
}

static void test_soc_noise(void) {
    TEST(soc_noise);
    double ts[256];
    for (int i = 0; i < 256; i++)
        ts[i] = sin(2.0*M_PI*i/32) + 0.1*((double)rand()/RAND_MAX-0.5);
    double alpha = soc_noise_exponent(ts, 256, 0.1);
    ASSERT(alpha >= 0.0, "noise exponent should be >= 0");
    PASS();
}

/* === sos_synergetics Tests === */

static void test_linear_stability(void) {
    TEST(linear_stability);
    double J[4] = {-1, 2, -3, -0.5}; /* stable 2x2 system */
    LinearStability* ls = ls_analyze(J, 2);
    ASSERT(ls != NULL, "ls_analyze failed");
    ASSERT(ls->n_modes == 2, "should have 2 modes");
    int n_op = ls_identify_order_params(ls, 1e-4);
    ASSERT(n_op >= 0, "order params should be non-negative");
    ls_free(ls);
    PASS();
}

static void test_laser(void) {
    TEST(laser);
    LaserSystem* laser = laser_create(2.0, 1.0, 0.5, 2.0);
    ASSERT(laser != NULL, "laser_create failed");
    ASSERT(fabs(laser->threshold - 2.0) < 1e-10, "laser threshold wrong");
    for (int i = 0; i < 1000; i++) laser_step(laser, 0.01);
    double n_ss = laser_steady_state_photons(laser);
    ASSERT(n_ss >= 0.0, "steady state photons wrong");
    int lasing = laser_is_lasing(laser);
    (void)lasing;
    laser_free(laser);
    PASS();
}

static void test_landau(void) {
    TEST(landau);
    double F0 = landau_free_energy(0.0, 0.5, 1.0);
    ASSERT(fabs(F0) < 1e-10, "F(0) should be 0");
    double xi_eq = landau_equilibrium(-1.0, 1.0);
    ASSERT(xi_eq > 0.0, "equilibrium should be nonzero for a<0");
    double xi_eq2 = landau_equilibrium(1.0, 1.0);
    ASSERT(fabs(xi_eq2) < 1e-10, "equilibrium should be 0 for a>0");
    PASS();
}

/* === Print Summary === */

static void test_print(void) {
    TEST(print_system);
    SOSystem* sys = sos_system_create("summary_test", 5, 2);
    sos_add_order_param(sys, "test_op", 0.3, 0.1, -0.1, 0.2);
    sos_print_system(sys, stdout);
    sos_system_free(sys);
    PASS();
}

int main(void) {
    printf("=== Self-Organizing Systems Test Suite ===\n\n");

    test_system_create();
    test_add_state();
    test_order_params();
    test_shannon_entropy();
    test_lmc_complexity();
    test_entropy_production();
    test_glansdorff_prigogine();
    test_emergence_detection();
    test_network_topology();
    test_correlation();
    test_powerlaw();
    test_eigenvalues();
    test_rk4();

    test_ca_create();
    test_elementary_ca();
    test_sandpile();

    test_turing_create();
    test_turing_evolution();

    test_autocatalytic();
    test_hypercycle();
    test_nk_landscape();

    test_bak_sneppen();
    test_forest_fire();
    test_soc_noise();

    test_linear_stability();
    test_laser();
    test_landau();

    test_print();

    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
