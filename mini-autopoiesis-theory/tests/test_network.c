/**
 * @file test_network.c
 * @brief Tests for reaction network construction and analysis.
 */
#include "reaction_network.h"
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

static void test_network_init(void)
{
    TEST("rn_network_init");
    rn_network_t net;
    rn_network_init(&net);
    CHECK(net.species_count == 0, "species_count should be 0");
    CHECK(net.reaction_count == 0, "reaction_count should be 0");
    PASS();
}

static void test_add_species(void)
{
    TEST("rn_add_species");
    rn_network_t net;
    rn_network_init(&net);

    int idx = rn_add_species(&net, "H2O", 10.0, 18.0);
    CHECK(idx == 0, "first species index 0");
    CHECK(net.species_count == 1, "species_count 1");

    int idx2 = rn_add_species(&net, "ATP", 1.0, 507.0);
    CHECK(idx2 == 1, "second species index 1");

    CHECK(strcmp(net.species[0].name, "H2O") == 0, "name check");
    CHECK(fabs(net.species[0].molecular_weight - 18.0) < 1e-10, "mw check");
    PASS();
}

static void test_add_reaction(void)
{
    TEST("rn_add_reaction");
    rn_network_t net;
    rn_network_init(&net);

    rn_add_species(&net, "A", 10.0, 100.0);
    rn_add_species(&net, "B", 1.0, 200.0);

    int reactants[] = {0};
    int r_stoich[] = {1};
    int products[] = {1};
    int p_stoich[] = {1};

    int idx = rn_add_reaction(&net, reactants, r_stoich, 1,
                               products, p_stoich, 1, NULL, 0, 0.5);
    CHECK(idx == 0, "reaction index 0");
    CHECK(net.reaction_count == 1, "reaction_count 1");
    CHECK(fabs(net.reactions[0].rate_constant - 0.5) < 1e-10, "rate check");
    PASS();
}

static void test_build_matrices(void)
{
    TEST("rn_build_matrices");
    rn_network_t net;
    rn_network_init(&net);

    rn_add_species(&net, "A", 10.0, 100.0);
    rn_add_species(&net, "B", 1.0, 200.0);

    int reactants[] = {0};
    int r_stoich[] = {1};
    int products[] = {1};
    int p_stoich[] = {1};

    rn_add_reaction(&net, reactants, r_stoich, 1,
                     products, p_stoich, 1, NULL, 0, 0.5);

    rn_build_matrices(&net);

    /* A is consumed: S[0][0] should be negative */
    CHECK(net.stoichiometry[0][0] < 0.0, "A should be consumed (negative stoich)");
    /* B is produced: S[1][0] should be positive */
    CHECK(net.stoichiometry[1][0] > 0.0, "B should be produced (positive stoich)");
    PASS();
}

static void test_compute_rates(void)
{
    TEST("rn_compute_reaction_rates");
    rn_network_t net;
    rn_network_init(&net);

    rn_add_species(&net, "A", 2.0, 100.0);
    rn_add_species(&net, "B", 1.0, 200.0);

    int reactants[] = {0};
    int r_stoich[] = {1};
    int products[] = {1};
    int p_stoich[] = {1};

    rn_add_reaction(&net, reactants, r_stoich, 1,
                     products, p_stoich, 1, NULL, 0, 2.0);
    rn_build_matrices(&net);

    double rates[RN_MAX_REACTIONS];
    rn_compute_reaction_rates(&net, rates);

    /* Mass-action: rate = k * [A] = 2.0 * 2.0 = 4.0 */
    CHECK(fabs(rates[0] - 4.0) < 0.1, "rate should be k*[A] = 4.0");
    PASS();
}

static void test_autocatalytic_detection(void)
{
    TEST("rn_is_autocatalytic_species");
    rn_network_t net;
    rn_network_init(&net);

    rn_add_species(&net, "X", 1.0, 50.0);  /* Species 0: autocatalyst */
    rn_add_species(&net, "Y", 1.0, 50.0);  /* Species 1: substrate */

    /* X catalyzes its own production: X + Y → 2X */
    int reactants[] = {0, 1};
    int r_stoich[] = {1, 1};
    int products[] = {0};
    int p_stoich[] = {2};
    int catalysts[] = {0};  /* X is the catalyst */

    rn_add_reaction(&net, reactants, r_stoich, 2,
                     products, p_stoich, 1, catalysts, 1, 1.0);

    int result = rn_is_autocatalytic_species(&net, 0);
    CHECK(result == 1, "species 0 should be autocatalytic");

    result = rn_is_autocatalytic_species(&net, 1);
    CHECK(result == 0, "species 1 should NOT be autocatalytic");
    PASS();
}

static void test_connectivity(void)
{
    TEST("rn_network_connectivity");
    rn_network_t net;
    rn_network_init(&net);

    rn_add_species(&net, "A", 1.0, 100.0);
    rn_add_species(&net, "B", 1.0, 200.0);

    int r[] = {0}, rs[] = {1}, p[] = {1}, ps[] = {1};
    rn_add_reaction(&net, r, rs, 1, p, ps, 1, NULL, 0, 1.0);
    rn_build_matrices(&net);

    double conn = rn_network_connectivity(&net);
    CHECK(conn > 0.0, "connected network should have connectivity > 0");
    PASS();
}

int main(void)
{
    printf("=== Test Suite: Reaction Network ===\n\n");
    test_network_init();
    test_add_species();
    test_add_reaction();
    test_build_matrices();
    test_compute_rates();
    test_autocatalytic_detection();
    test_connectivity();
    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
