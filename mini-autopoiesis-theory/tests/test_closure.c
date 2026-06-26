/**
 * @file test_closure.c
 * @brief Tests for organizational closure and autocatalytic set algorithms.
 */
#include "organizational_closure.h"
#include "autocatalytic_set.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

/* Organizational closure tests */
static void test_production_closure(void)
{
    TEST("oc_production_closure");
    oc_system_t sys;
    oc_system_init(&sys, 4, 2);

    /* Reaction 0: A + B → C */
    int subs0[] = {0, 1};
    int prods0[] = {2};
    oc_add_reaction(&sys, 0, subs0, 2, prods0, 1);

    /* Reaction 1: C → D */
    int subs1[] = {2};
    int prods1[] = {3};
    oc_add_reaction(&sys, 1, subs1, 1, prods1, 1);

    int food[] = {0, 1};
    int closure[OC_MAX_COMPONENTS];
    int closure_size;

    oc_production_closure(&sys, food, 2, closure, &closure_size);

    /* Food + C + D = 4 components */
    CHECK(closure_size == 4, "closure should contain all 4 components");
    int found[4] = {0, 0, 0, 0};
    for (int i = 0; i < closure_size; i++) found[closure[i]] = 1;
    CHECK(found[0] && found[1] && found[2] && found[3], "all 4 components should be present");

    oc_system_destroy(&sys);
    PASS();
}

static void test_closure_index(void)
{
    TEST("oc_closure_index");
    oc_system_t sys;
    oc_system_init(&sys, 3, 1);

    /* No reactions → nothing is "produced" */
    int prods[] = {1};
    oc_add_reaction(&sys, 0, NULL, 0, prods, 1);

    double ci = oc_closure_index(&sys);
    CHECK(ci >= 0.0 && ci <= 1.0, "closure index in [0, 1]");
    /* With 1 reaction producing component 1, and component 1 is the only
     * producible one, closure should be 1.0 if it's reachable from food */
    CHECK(ci >= 0.0, "closure index valid");

    oc_system_destroy(&sys);
    PASS();
}

static void test_dependency_depths(void)
{
    TEST("oc_dependency_depths");
    oc_system_t sys;
    oc_system_init(&sys, 4, 2);

    int subs0[] = {0};
    int prods0[] = {1};
    oc_add_reaction(&sys, 0, subs0, 1, prods0, 1);

    int subs1[] = {1};
    int prods1[] = {2};
    oc_add_reaction(&sys, 1, subs1, 1, prods1, 1);

    int food[] = {0};
    int depths[OC_MAX_COMPONENTS];
    oc_dependency_depths(&sys, food, 1, depths);

    CHECK(depths[0] == 0, "food component depth = 0");
    CHECK(depths[1] == 1, "B depth = 1 (one step from A)");
    CHECK(depths[2] == 2, "C depth = 2");

    oc_system_destroy(&sys);
    PASS();
}

/* Autocatalytic set (RAF) tests */
static void test_raf_empty(void)
{
    TEST("acs_find_maximal_raf empty");
    acs_system_t acs;
    acs_system_init(&acs);

    int result = acs_find_maximal_raf(&acs);
    CHECK(result == 0, "empty system should have no RAF");

    acs_system_destroy(&acs);
    PASS();
}

static void test_raf_simple(void)
{
    TEST("acs_find_maximal_raf simple autocatalytic");
    acs_system_t acs;
    acs_system_init(&acs);

    /* Food: A, B */
    acs_add_molecule(&acs, "A", 1);  /* 0: food */
    acs_add_molecule(&acs, "B", 1);  /* 1: food */
    acs_add_molecule(&acs, "C", 0);  /* 2: product */

    /* Reaction 0: A + B → C, catalyzed by C (autocatalytic) */
    int r[] = {0, 1};
    int p[] = {2};
    int c[] = {2};
    acs_add_reaction(&acs, r, 2, p, 1, c, 1, 1.0);

    int result = acs_find_maximal_raf(&acs);
    /* C is not in the food set and is needed as catalyst,
     * so this shouldn't immediately form a RAF without C being produced first.
     * Actually with A+B→C, C is produced, then can catalyze itself.
     * Let's check: A, B in food. Reaction A+B→C produces C. C catalyzes this reaction.
     * This IS a RAF. */
    CHECK(result == 1, "autocatalytic system should have a RAF");
    CHECK(acs.max_raf_size >= 1, "RAF should contain at least 1 reaction");

    acs_system_destroy(&acs);
    PASS();
}

static void test_raf_property_check(void)
{
    TEST("acs_is_raf verification");
    acs_system_t acs;
    acs_system_init(&acs);

    acs_add_molecule(&acs, "X", 1);  /* food */
    acs_add_molecule(&acs, "Y", 0);  /* non-food */
    int r0[] = {0}, p0[] = {1};
    /* Uncatalyzed: X → Y */
    acs_add_reaction(&acs, r0, 1, p0, 1, NULL, 0, 1.0);

    int raf_result = acs_find_maximal_raf(&acs);
    /* Uncatalyzed reaction with food reactants should form a RAF */
    CHECK(raf_result == 1, "food-sourced uncatalyzed reaction is RAF");

    /* Verify RAF property */
    int raf_set[] = {0};
    int is_raf = acs_is_raf(&acs, raf_set, 1);
    CHECK(is_raf == 1, "single reaction should be RAF");

    acs_system_destroy(&acs);
    PASS();
}

int main(void)
{
    printf("=== Test Suite: Organizational Closure & RAF ===\n\n");

    test_production_closure();
    test_closure_index();
    test_dependency_depths();
    test_raf_empty();
    test_raf_simple();
    test_raf_property_check();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    fflush(stdout);
    return tests_failed > 0 ? 1 : 0;
}
