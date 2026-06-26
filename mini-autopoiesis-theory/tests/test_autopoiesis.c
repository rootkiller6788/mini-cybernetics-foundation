/**
 * @file test_autopoiesis.c
 * @brief Tests for core autopoiesis system functions.
 *
 * Tests L1-L4: system initialization, component management,
 * autopoiesis checking, closure computation, perturbation response.
 */
#include "autopoiesis.h"
#include <assert.h>
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
#define CHECK_EQ(a, b, msg) do { if ((a) != (b)) { FAIL(msg); return; } } while(0)
#define CHECK_DOUBLE(a, b, tol, msg) do { if (fabs((a)-(b)) > (tol)) { FAIL(msg); return; } } while(0)

/* --------------------------------------------------------------------------
 * L1: System Initialization and Component Management
 * -------------------------------------------------------------------------- */
static void test_init_empty_system(void)
{
    TEST("ap_system_init creates empty system");
    ap_system_t sys;
    ap_system_init(&sys);
    CHECK_EQ(sys.component_count, 0, "component_count should be 0");
    CHECK_EQ(sys.production_count, 0, "production_count should be 0");
    CHECK_EQ(sys.state, AP_STATE_INACTIVE, "initial state should be INACTIVE");
    CHECK_DOUBLE(sys.total_mass, 0.0, 1e-10, "total_mass should be 0");
    PASS();
}

static void test_add_component(void)
{
    TEST("ap_add_component registers components");
    ap_system_t sys;
    ap_system_init(&sys);

    int idx0 = ap_add_component(&sys, "metabolite_A", AP_COMPONENT_INTERNAL, 10.0, 0.1);
    CHECK(idx0 >= 0, "first component should succeed");

    int idx1 = ap_add_component(&sys, "membrane_lipid", AP_COMPONENT_BOUNDARY, 5.0, 0.05);
    CHECK(idx1 == 1, "second component index should be 1");

    int idx2 = ap_add_component(&sys, "enzyme_X", AP_COMPONENT_CATALYST, 2.0, 0.02);
    CHECK(idx2 == 2, "third component index should be 2");

    CHECK_EQ(sys.component_count, 3, "component_count should be 3");
    CHECK(strcmp(sys.components[0].label, "metabolite_A") == 0, "component 0 label");
    CHECK(strcmp(sys.components[1].label, "membrane_lipid") == 0, "component 1 label");
    CHECK_DOUBLE(sys.components[0].concentration, 10.0, 1e-10, "concentration check");
    CHECK_DOUBLE(sys.components[0].decay_rate, 0.1, 1e-10, "decay_rate check");

    /* Boundary component should be in boundary list */
    CHECK(sys.boundary.component_count > 0, "boundary should have component");

    PASS();
}

static void test_add_component_bounds(void)
{
    TEST("ap_add_component rejects invalid input");
    ap_system_t sys;
    ap_system_init(&sys);

    int ret = ap_add_component(NULL, "test", AP_COMPONENT_INTERNAL, 1.0, 0.1);
    CHECK_EQ(ret, -1, "NULL system should return -1");

    ret = ap_add_component(&sys, NULL, AP_COMPONENT_INTERNAL, 1.0, 0.1);
    CHECK_EQ(ret, -1, "NULL label should return -1");

    ret = ap_add_component(&sys, "test", AP_COMPONENT_INTERNAL, -1.0, 0.1);
    CHECK_EQ(ret, -1, "negative concentration should return -1");

    ret = ap_add_component(&sys, "test", AP_COMPONENT_INTERNAL, 1.0, -0.1);
    CHECK_EQ(ret, -1, "negative decay_rate should return -1");

    PASS();
}

static void test_add_production(void)
{
    TEST("ap_add_production creates production relations");
    ap_system_t sys;
    ap_system_init(&sys);

    ap_add_component(&sys, "A", AP_COMPONENT_INTERNAL, 10.0, 0.1);
    ap_add_component(&sys, "B", AP_COMPONENT_INTERNAL, 1.0, 0.1);

    int substrates[] = {0};
    int prod_idx = ap_add_production(&sys, 0, 1, 0.5, -1, substrates, 1);
    CHECK(prod_idx >= 0, "production should succeed");
    CHECK_EQ(sys.production_count, 1, "production_count should be 1");
    CHECK_EQ(sys.productions[0].producer_id, 0, "producer should be 0");
    CHECK_EQ(sys.productions[0].product_id, 1, "product should be 1");
    CHECK_DOUBLE(sys.productions[0].rate, 0.5, 1e-10, "rate should be 0.5");

    /* Product should be marked as self-produced */
    CHECK(sys.components[1].flags & AP_FLAG_SELF_PRODUCED, "product should be self-produced");

    PASS();
}

/* --------------------------------------------------------------------------
 * L2: Autopoiesis Checking and Evolution
 * -------------------------------------------------------------------------- */
static void test_check_autopoiesis_empty(void)
{
    TEST("ap_check_autopoiesis returns 0 for empty system");
    ap_system_t sys;
    ap_system_init(&sys);
    CHECK_EQ(ap_check_autopoiesis(&sys), 0, "empty system is not autopoietic");
    CHECK_EQ(ap_check_autopoiesis(NULL), 0, "NULL system returns 0");
    PASS();
}

static void test_check_autopoiesis_minimal(void)
{
    TEST("ap_check_autopoiesis with minimal autopoietic system");
    ap_system_t sys;
    ap_system_init(&sys);

    /* Create a simple autopoietic system: components that produce each other */
    ap_add_component(&sys, "internal_A", AP_COMPONENT_INTERNAL, 5.0, 0.1);
    ap_add_component(&sys, "internal_B", AP_COMPONENT_INTERNAL, 5.0, 0.1);
    ap_add_component(&sys, "boundary_C", AP_COMPONENT_BOUNDARY, 2.0, 0.05);

    /* Mutual production: A produces B, B produces A, A produces boundary_C */
    int subs0[] = {0};
    ap_add_production(&sys, 0, 1, 0.5, -1, subs0, 1);  /* A → B */
    int subs1[] = {1};
    ap_add_production(&sys, 1, 0, 0.5, -1, subs1, 1);  /* B → A */
    int subs2[] = {0};
    ap_add_production(&sys, 0, 2, 0.3, -1, subs2, 1);  /* A → boundary_C */

    int result = ap_check_autopoiesis(&sys);
    /* Should be autopoietic: all components are produced internally,
     * boundary is self-produced */
    CHECK(result == 1, "minimal system should be autopoietic");

    PASS();
}

static void test_organizational_closure_measure(void)
{
    TEST("ap_compute_organizational_closure");
    ap_system_t sys;
    ap_system_init(&sys);

    ap_add_component(&sys, "A", AP_COMPONENT_INTERNAL, 5.0, 0.1);
    ap_add_component(&sys, "B", AP_COMPONENT_INTERNAL, 3.0, 0.1);

    double closure = ap_compute_organizational_closure(&sys);
    CHECK_DOUBLE(closure, 0.0, 1e-10, "no productions → 0 closure");

    int subs[] = {0};
    ap_add_production(&sys, 0, 1, 0.5, -1, subs, 1);

    closure = ap_compute_organizational_closure(&sys);
    CHECK_DOUBLE(closure, 0.5, 1e-10, "one self-produced out of two → 0.5");

    PASS();
}

static void test_evolve_step(void)
{
    TEST("ap_evolve_step updates concentrations");
    ap_system_t sys;
    ap_system_init(&sys);

    ap_add_component(&sys, "A", AP_COMPONENT_INTERNAL, 10.0, 0.01);
    ap_add_component(&sys, "B", AP_COMPONENT_INTERNAL, 1.0, 0.01);

    int subs[] = {0};
    ap_add_production(&sys, 0, 1, 0.1, -1, subs, 1);

    double old_time = sys.time;
    double old_B = sys.components[1].concentration;

    ap_evolve_step(&sys, 0.1);

    CHECK(sys.time > old_time, "time should advance");
    CHECK(sys.components[1].concentration > old_B, "B should increase due to production");
    CHECK(sys.state != AP_STATE_INACTIVE, "system should not be inactive");

    PASS();
}

static void test_apply_perturbation(void)
{
    TEST("ap_apply_perturbation modifies component and updates state");
    ap_system_t sys;
    ap_system_init(&sys);

    ap_add_component(&sys, "A", AP_COMPONENT_INTERNAL, 10.0, 0.01);
    ap_add_component(&sys, "B", AP_COMPONENT_INTERNAL, 5.0, 0.01);

    int subs[] = {0};
    ap_add_production(&sys, 0, 1, 0.1, -1, subs, 1);

    double old_A = sys.components[0].concentration;
    ap_system_state_t new_state = ap_apply_perturbation(&sys, 0, -3.0);

    CHECK_DOUBLE(sys.components[0].concentration, old_A - 3.0, 1e-10, "A should decrease by 3");
    CHECK(sys.perturbation_count == 1, "perturbation_count increment");
    CHECK(new_state != AP_STATE_INACTIVE, "state should update");

    PASS();
}

/* --------------------------------------------------------------------------
 * L3: Mathematical Structure
 * -------------------------------------------------------------------------- */
static void test_production_adjacency(void)
{
    TEST("ap_build_production_adjacency");
    ap_system_t sys;
    ap_system_init(&sys);

    ap_add_component(&sys, "A", AP_COMPONENT_INTERNAL, 5.0, 0.1);
    ap_add_component(&sys, "B", AP_COMPONENT_INTERNAL, 3.0, 0.1);
    ap_add_component(&sys, "C", AP_COMPONENT_INTERNAL, 2.0, 0.1);

    int subs0[] = {0};
    ap_add_production(&sys, 0, 1, 0.5, -1, subs0, 1);
    int subs1[] = {1};
    ap_add_production(&sys, 1, 2, 0.5, -1, subs1, 1);

    int n = sys.component_count;
    int *adj = (int *)calloc(n * n, sizeof(int));
    ap_build_production_adjacency(&sys, adj, n);

    CHECK(adj[0 * n + 1] == 1, "A → B should be 1");
    CHECK(adj[1 * n + 2] == 1, "B → C should be 1");
    CHECK(adj[0 * n + 2] == 0, "A ↛ C directly should be 0");

    free(adj);
    PASS();
}

static void test_transitive_closure(void)
{
    TEST("ap_production_transitive_closure");
    ap_system_t sys;
    ap_system_init(&sys);

    ap_add_component(&sys, "A", AP_COMPONENT_INTERNAL, 5.0, 0.1);
    ap_add_component(&sys, "B", AP_COMPONENT_INTERNAL, 3.0, 0.1);
    ap_add_component(&sys, "C", AP_COMPONENT_INTERNAL, 2.0, 0.1);
    ap_add_component(&sys, "D", AP_COMPONENT_INTERNAL, 1.0, 0.1);

    ap_add_production(&sys, 0, 1, 0.5, -1, NULL, 0);
    int subs1[] = {1};
    ap_add_production(&sys, 1, 2, 0.5, -1, subs1, 1);
    /* D has no producer → should NOT be in closure from A */

    int initial[] = {0};
    int closure[AP_MAX_COMPONENTS];
    int closure_size;

    ap_production_transitive_closure(&sys, initial, 1, closure, &closure_size);

    /* A (initial), B (from A), C (from B) should be in closure */
    CHECK(closure_size >= 3, "closure should have at least 3 components");

    int found_A = 0, found_B = 0, found_C = 0, found_D = 0;
    for (int i = 0; i < closure_size; i++) {
        if (closure[i] == 0) found_A = 1;
        if (closure[i] == 1) found_B = 1;
        if (closure[i] == 2) found_C = 1;
        if (closure[i] == 3) found_D = 1;
    }
    CHECK(found_A, "A should be in closure");
    CHECK(found_B, "B should be in closure");
    CHECK(found_C, "C should be in closure");
    CHECK(!found_D, "D should NOT be in closure (unproducible)");

    PASS();
}

/* --------------------------------------------------------------------------
 * L4: Boundary Self-Production
 * -------------------------------------------------------------------------- */
static void test_self_produced_boundary(void)
{
    TEST("ap_verify_self_produced_boundary");
    ap_system_t sys;
    ap_system_init(&sys);

    ap_add_component(&sys, "internal", AP_COMPONENT_INTERNAL, 5.0, 0.1);
    ap_add_component(&sys, "boundary_lipid", AP_COMPONENT_BOUNDARY, 2.0, 0.05);

    /* Boundary lipid produced from internal component */
    int subs[] = {0};
    ap_add_production(&sys, 0, 1, 0.3, -1, subs, 1);

    int result = ap_verify_self_produced_boundary(&sys);
    CHECK(result == 1, "boundary should be self-produced");

    PASS();
}

/* ==========================================================================
 * Main
 * ========================================================================== */
int main(void)
{
    printf("=== Test Suite: Autopoiesis Core ===\n\n");

    /* L1 Tests */
    test_init_empty_system();
    test_add_component();
    test_add_component_bounds();
    test_add_production();

    /* L2 Tests */
    test_check_autopoiesis_empty();
    test_check_autopoiesis_minimal();
    test_organizational_closure_measure();
    test_evolve_step();
    test_apply_perturbation();

    /* L3 Tests */
    test_production_adjacency();
    test_transitive_closure();

    /* L4 Tests */
    test_self_produced_boundary();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
