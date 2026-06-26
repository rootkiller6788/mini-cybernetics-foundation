/**
 * @file test_models.c
 * @brief Tests for chemoton and hypercycle models.
 *
 * Because chemoton.c and hypercycle.c define functions with static linkage
 * in their own compilation units, we test them by including the source.
 * This tests the public API functions of each model.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

/* Include the chemoton implementation directly to access its functions.
 * We need the type definitions and function declarations. */
typedef struct {
    double metabolites[8];
    int n_metabolites;
    double nutrient;
    double waste;
    double template_concentration;
    double monomers[4];
    int n_monomers;
    double template_polymerase;
    double membrane_precursors;
    double membrane_area;
    double membrane_permeability;
    double encapsulated_volume;
    double metabolic_efficiency;
    double template_replication_rate;
    double membrane_growth_rate;
    double decay_rate;
    double nutrient_influx;
    double total_energy;
    double organization_measure;
} chemoton_t;

void chemoton_init(chemoton_t *che);
void chemoton_integrate_rk4(chemoton_t *che, double dt);
int chemoton_check_autopoiesis(const chemoton_t *che);
int chemoton_find_steady_state(chemoton_t *che, double max_time, double tolerance);
void chemoton_bifurcation_scan(chemoton_t *che, const char *param_name,
                                double param_start, double param_end,
                                int n_points, double *results);
void chemoton_print_state(const chemoton_t *che);

/* Hypercycle types and functions */
#define HC_MAX_SPECIES 20

typedef struct {
    int n_species;
    double concentrations[HC_MAX_SPECIES];
    double rate_constants[HC_MAX_SPECIES];
    double decay_rates[HC_MAX_SPECIES];
    double dilution_flow;
    double total_concentration;
    double fitness[HC_MAX_SPECIES];
    double mean_fitness;
    int is_quasispecies;
} hypercycle_t;

void hypercycle_init(hypercycle_t *hc, int n_species);
void hypercycle_integrate_rk4(hypercycle_t *hc, double dt);
void hypercycle_ode(const hypercycle_t *hc, double *dxdt);
int hypercycle_find_fixed_point(const hypercycle_t *hc,
                                 double *fixed_point, int *is_stable);
void hypercycle_error_threshold(int n_components, double fitness_ratio,
                                 double *q_min, double *threshold);
void hypercycle_compute_quasispecies(hypercycle_t *hc);
void hypercycle_print_state(const hypercycle_t *hc);

/* Chemoton tests */
static void test_chemoton_init(void)
{
    TEST("chemoton_init");
    chemoton_t che;
    chemoton_init(&che);
    CHECK(che.n_metabolites == 4, "default 4 metabolites");
    CHECK(che.n_monomers == 2, "default 2 monomers");
    CHECK(che.nutrient > 0.0, "nutrient initialized");
    CHECK(che.metabolic_efficiency > 0.0, "metabolic efficiency set");
    PASS();
}

static void test_chemoton_integrate(void)
{
    TEST("chemoton_integrate_rk4");
    chemoton_t che;
    chemoton_init(&che);

    chemoton_integrate_rk4(&che, 0.01);

    /* After one step, concentrations should change */
    int all_same = 1;
    chemoton_t fresh;
    chemoton_init(&fresh);
    for (int i = 0; i < che.n_metabolites; i++) {
        if (fabs(che.metabolites[i] - fresh.metabolites[i]) > 1e-10) {
            all_same = 0;
            break;
        }
    }
    CHECK(!all_same, "concentrations should change after integration");
    PASS();
}

static void test_chemoton_autopoiesis_check(void)
{
    TEST("chemoton_check_autopoiesis");
    chemoton_t che;
    chemoton_init(&che);

    int is_apo = chemoton_check_autopoiesis(&che);
    CHECK(is_apo == 0 || is_apo == 1, "result should be 0 or 1");
    PASS();
}

/* Hypercycle tests */
static void test_hypercycle_init(void)
{
    TEST("hypercycle_init");
    hypercycle_t hc;
    hypercycle_init(&hc, 4);
    CHECK(hc.n_species == 4, "n_species should be 4");
    CHECK(hc.total_concentration > 0.0, "total concentration > 0");
    for (int i = 0; i < hc.n_species; i++) {
        CHECK(hc.concentrations[i] > 0.0, "each concentration > 0");
    }
    PASS();
}

static void test_hypercycle_fixed_point(void)
{
    TEST("hypercycle_find_fixed_point");
    hypercycle_t hc;
    hypercycle_init(&hc, 4);

    double fixed_point[HC_MAX_SPECIES];
    int is_stable;
    int ret = hypercycle_find_fixed_point(&hc, fixed_point, &is_stable);
    CHECK(ret == 0, "fixed point computation should succeed");
    CHECK(is_stable == 1, "n=4 hypercycle should be stable");

    /* Fixed point: all equal at c/n */
    double expected = hc.total_concentration / 4.0;
    for (int i = 0; i < hc.n_species; i++) {
        CHECK(fabs(fixed_point[i] - expected) < 1e-6, "fixed point x_i = c/n");
    }
    PASS();
}

static void test_hypercycle_integrate(void)
{
    TEST("hypercycle_integrate_rk4");
    hypercycle_t hc;
    hypercycle_init(&hc, 3);

    double old_total = hc.total_concentration;
    hypercycle_integrate_rk4(&hc, 0.01);

    CHECK(hc.concentrations[0] >= 0.0, "non-negative concentration");
    /* Total should be approximately conserved (within numerical error) */
    double ratio = hc.total_concentration / old_total;
    CHECK(fabs(ratio - 1.0) < 0.2, "total concentration approximately conserved");
    PASS();
}

static void test_error_threshold(void)
{
    TEST("hypercycle_error_threshold");
    double q_min, threshold;
    hypercycle_error_threshold(10, 100.0, &q_min, &threshold);

    /* For n=10, sigma=100: q_min = (1/100)^(1/10) ≈ 0.63 */
    CHECK(q_min > 0.5 && q_min < 1.0, "q_min in reasonable range");
    CHECK(threshold > 0.0 && threshold < 0.5, "threshold non-zero");
    PASS();
}

int main(void)
{
    printf("=== Test Suite: Chemoton & Hypercycle Models ===\n\n");

    test_chemoton_init();
    test_chemoton_integrate();
    test_chemoton_autopoiesis_check();

    test_hypercycle_init();
    test_hypercycle_fixed_point();
    test_hypercycle_integrate();
    test_error_threshold();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
