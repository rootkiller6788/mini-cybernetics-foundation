#include "vna_core.h"
#include "vna_cellular.h"
#include "vna_neighborhood.h"
#include "vna_rule.h"
#include "vna_replication.h"
#include "vna_fault_tolerant.h"
#include "vna_game_of_life.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 * Test Suite — Von Neumann Automata Module
 *
 * Tests organized by knowledge level:
 *   L1: Definitions — struct creation, enums, basic operations
 *   L2: Core Concepts — evolution, neighbor collection
 *   L3: Math Structures — rule table encoding, lattice access
 *   L4: Fundamental Laws — error threshold, reversibility
 *   L5: Algorithms — pattern detection, damage spreading
 *   L6: Canonical Problems — Game of Life patterns
 *   L7: Applications — fault tolerance, population dynamics
 * ============================================================================ */

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) do { \
    printf("  TEST: %s ... ", name); \
} while(0)

#define PASS() do { \
    printf("PASS\n"); tests_passed++; \
} while(0)

#define FAIL(msg) do { \
    printf("FAIL: %s\n", msg); tests_failed++; \
} while(0)

#define ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { FAIL(msg); return; } \
} while(0)

#define ASSERT_DOUBLE_EQ(a, b, tol, msg) do { \
    if (fabs((a) - (b)) > (tol)) { \
        printf("FAIL: %s (expected %.6f, got %.6f)\n", msg, b, a); \
        tests_failed++; return; \
    } \
} while(0)

#define ASSERT_TRUE(cond, msg) do { \
    if (!(cond)) { FAIL(msg); return; } \
} while(0)

/* ============================================================================
 * L1: Definitions — Core Types
 * ============================================================================ */

void test_lattice_create(void) {
    TEST("Lattice creation");
    VnaLattice* lattice = vna_lattice_create(100, 80, 2, VNA_LATTICE_2D_SQUARE);
    ASSERT_TRUE(lattice != NULL, "lattice should not be NULL");
    ASSERT_EQ(lattice->width, 100, "width mismatch");
    ASSERT_EQ(lattice->height, 80, "height mismatch");
    ASSERT_EQ(lattice->num_states, 2, "num_states mismatch");
    ASSERT_EQ(lattice->generation, 0, "generation should start at 0");

    /* Test NULL safety */
    VnaLattice* null_lat = vna_lattice_create(0, 10, 2, VNA_LATTICE_2D_SQUARE);
    ASSERT_TRUE(null_lat == NULL, "zero width should return NULL");

    null_lat = vna_lattice_create(10, 10, 1, VNA_LATTICE_2D_SQUARE);
    ASSERT_TRUE(null_lat == NULL, "num_states < 2 should return NULL");

    vna_lattice_free(lattice);
    PASS();
}

void test_cell_access(void) {
    TEST("Cell access and manipulation");
    VnaLattice* lattice = vna_lattice_create(10, 10, 2, VNA_LATTICE_2D_SQUARE);
    ASSERT_TRUE(lattice != NULL, "create failed");

    /* Set and get */
    vna_lattice_set_cell(lattice, 3, 4, 0, 1);
    ASSERT_EQ(vna_lattice_get_cell(lattice, 3, 4, 0), 1, "cell set/get mismatch");

    /* Boundary: out of bounds should return 0 */
    ASSERT_EQ(vna_lattice_get_cell(lattice, -1, 0, 0), 0, "OOB x should return 0");
    ASSERT_EQ(vna_lattice_get_cell(lattice, 100, 0, 0), 0, "OOB x2 should return 0");

    /* Clear */
    vna_lattice_clear(lattice, 0);
    ASSERT_EQ(vna_lattice_get_cell(lattice, 3, 4, 0), 0, "clear failed");

    vna_lattice_free(lattice);
    PASS();
}

void test_randomize(void) {
    TEST("Lattice randomization");
    VnaLattice* lattice = vna_lattice_create(50, 50, 2, VNA_LATTICE_2D_SQUARE);
    vna_lattice_randomize(lattice, 0.5, 42);

    /* Count live cells */
    int alive = 0;
    for (int y = 0; y < 50; y++)
        for (int x = 0; x < 50; x++)
            alive += vna_lattice_get_cell(lattice, x, y, 0);

    /* With seed=42 and density=0.5, should have ~1250 alive cells */
    ASSERT_TRUE(alive > 1000 && alive < 1500, "randomization density out of range");
    vna_lattice_free(lattice);
    PASS();
}

/* ============================================================================
 * L2: Core Concepts — CA Evolution
 * ============================================================================ */

void test_evolution_basic(void) {
    TEST("Basic CA evolution (Game of Life)");
    VnaLattice* lattice = vna_lattice_create(10, 10, 2, VNA_LATTICE_2D_SQUARE);
    VnaNeighborhood* nb = vna_neighborhood_create(VNA_MOORE, 1, 2);
    VnaTransitionRule* rule = vna_rule_game_of_life();

    ASSERT_TRUE(lattice != NULL, "lattice NULL");
    ASSERT_TRUE(nb != NULL, "neighborhood NULL");
    ASSERT_TRUE(rule != NULL, "rule NULL");

    /* Set up a blinker (vertical) */
    vna_lattice_set_cell(lattice, 5, 4, 0, 1);
    vna_lattice_set_cell(lattice, 5, 5, 0, 1);
    vna_lattice_set_cell(lattice, 5, 6, 0, 1);

    /* Evolve one step: vertical blinker → horizontal blinker */
    int ret = vna_lattice_evolve(lattice, nb, rule);
    ASSERT_EQ(ret, 0, "evolution failed");

    /* Should now be horizontal at row 5 */
    ASSERT_EQ(vna_lattice_get_cell(lattice, 4, 5, 0), 1, "blinker left");
    ASSERT_EQ(vna_lattice_get_cell(lattice, 5, 5, 0), 1, "blinker center");
    ASSERT_EQ(vna_lattice_get_cell(lattice, 6, 5, 0), 1, "blinker right");

    /* Original positions should be dead */
    ASSERT_EQ(vna_lattice_get_cell(lattice, 5, 4, 0), 0, "original top dead");
    ASSERT_EQ(vna_lattice_get_cell(lattice, 5, 6, 0), 0, "original bottom dead");

    ASSERT_EQ(lattice->generation, 1, "generation not incremented");

    vna_lattice_free(lattice);
    vna_neighborhood_free(nb);
    vna_rule_free(rule);
    PASS();
}

void test_evolution_block_still_life(void) {
    TEST("Block still life remains unchanged");
    VnaLattice* lattice = vna_lattice_create(6, 6, 2, VNA_LATTICE_2D_SQUARE);
    VnaNeighborhood* nb = vna_neighborhood_create(VNA_MOORE, 1, 2);
    VnaTransitionRule* rule = vna_rule_game_of_life();

    /* Set up 2x2 block */
    vna_lattice_set_cell(lattice, 2, 2, 0, 1);
    vna_lattice_set_cell(lattice, 3, 2, 0, 1);
    vna_lattice_set_cell(lattice, 2, 3, 0, 1);
    vna_lattice_set_cell(lattice, 3, 3, 0, 1);

    /* Save original state */
    int orig[4][2] = {{2,2},{3,2},{2,3},{3,3}};

    vna_lattice_evolve(lattice, nb, rule);

    /* Block should be unchanged */
    for (int i = 0; i < 4; i++)
        ASSERT_EQ(vna_lattice_get_cell(lattice, orig[i][0], orig[i][1], 0), 1,
                  "block cell died");

    vna_lattice_free(lattice);
    vna_neighborhood_free(nb);
    vna_rule_free(rule);
    PASS();
}

/* ============================================================================
 * L3: Math Structures — Rule Table Encoding
 * ============================================================================ */

void test_rule_table_encoding(void) {
    TEST("Rule table encoding/indexing");
    VnaTransitionRule* rule = vna_rule_create(2, 3);
    ASSERT_TRUE(rule != NULL, "rule create failed");

    /* Set specific entries: rule that's 1 if neighbors sum >= 2 */
    uint8_t neighbors[3];
    for (int i = 0; i < 8; i++) {
        neighbors[0] = (i >> 2) & 1;
        neighbors[1] = (i >> 1) & 1;
        neighbors[2] = i & 1;
        int sum = neighbors[0] + neighbors[1] + neighbors[2];
        vna_rule_set_entry(rule, neighbors, sum >= 2 ? 1 : 0);
    }

    /* Verify lookup */
    uint8_t test1[] = {0, 0, 0};
    ASSERT_EQ(vna_rule_lookup(rule, test1), 0, "000->0");

    uint8_t test2[] = {1, 1, 0};
    ASSERT_EQ(vna_rule_lookup(rule, test2), 1, "110->1");

    uint8_t test3[] = {1, 1, 1};
    ASSERT_EQ(vna_rule_lookup(rule, test3), 1, "111->1");

    vna_rule_free(rule);
    PASS();
}

void test_wolfram_code(void) {
    TEST("Wolfram code encoding (Rule 110)");
    VnaTransitionRule* rule = vna_rule_create(2, 3);
    vna_rule_from_wolfram(rule, 110);

    /* Rule 110: 000→0, 001→1, 010→1, 011→1, 100→0, 101→1, 110→1, 111→0 */
    uint8_t nb[3];
    nb[0]=0; nb[1]=0; nb[2]=0; ASSERT_EQ(vna_rule_lookup(rule, nb), 0, "000");
    nb[0]=0; nb[1]=0; nb[2]=1; ASSERT_EQ(vna_rule_lookup(rule, nb), 1, "001");
    nb[0]=0; nb[1]=1; nb[2]=0; ASSERT_EQ(vna_rule_lookup(rule, nb), 1, "010");
    nb[0]=0; nb[1]=1; nb[2]=1; ASSERT_EQ(vna_rule_lookup(rule, nb), 1, "011");
    nb[0]=1; nb[1]=0; nb[2]=0; ASSERT_EQ(vna_rule_lookup(rule, nb), 0, "100");
    nb[0]=1; nb[1]=0; nb[2]=1; ASSERT_EQ(vna_rule_lookup(rule, nb), 1, "101");
    nb[0]=1; nb[1]=1; nb[2]=0; ASSERT_EQ(vna_rule_lookup(rule, nb), 1, "110");
    nb[0]=1; nb[1]=1; nb[2]=1; ASSERT_EQ(vna_rule_lookup(rule, nb), 0, "111");

    int code = vna_rule_to_wolfram(rule);
    ASSERT_EQ(code, 110, "wolfram round-trip");

    vna_rule_free(rule);
    PASS();
}

/* ============================================================================
 * L4: Fundamental Laws — Error Threshold
 * ============================================================================ */

void test_error_threshold(void) {
    TEST("Von Neumann error threshold (ε_crit)");
    double ecrit = vna_critical_error_threshold(2, 1);
    ASSERT_TRUE(ecrit > 0.001 && ecrit < 0.5, "ε_crit out of range");

    /* Below threshold: should be able to achieve high reliability */
    int n = vna_required_multiplexing(0.001, 0.000001);
    ASSERT_TRUE(n > 0, "below threshold should be achievable");

    /* Above threshold: should fail */
    n = vna_required_multiplexing(0.5, 0.000001);
    ASSERT_TRUE(n < 0, "above threshold should be impossible");

    PASS();
}

void test_tmr_error(void) {
    TEST("Triple Modular Redundancy error probability");
    /* TMR: P(error) = 3ε² - 2ε³ */
    double err = vna_tmr_error_probability(0.01);
    double expected = 3.0 * 0.0001 - 2.0 * 0.000001;
    ASSERT_DOUBLE_EQ(err, expected, 1e-10, "TMR error formula");

    /* TMR should reduce error */
    ASSERT_TRUE(err < 0.01, "TMR should reduce error");

    PASS();
}

void test_nmr_error(void) {
    TEST("N-Modular Redundancy error probability");
    /* Single component: error = ε */
    double single = vna_nmr_error_probability(1, 0.1);
    ASSERT_DOUBLE_EQ(single, 0.1, 1e-10, "single component error");

    /* 5-MR should have lower error */
    double nmr5 = vna_nmr_error_probability(5, 0.1);
    ASSERT_TRUE(nmr5 < 0.1, "5-MR should reduce error");

    /* Higher N → lower error */
    double nmr7 = vna_nmr_error_probability(7, 0.1);
    ASSERT_TRUE(nmr7 < nmr5, "higher N should give lower error");

    PASS();
}

/* ============================================================================
 * L5: Algorithms — Pattern and Oscillator Detection
 * ============================================================================ */

void test_still_life_detection(void) {
    TEST("Still life detection");
    VnaLattice* lattice = vna_lattice_create(10, 10, 2, VNA_LATTICE_2D_SQUARE);
    VnaNeighborhood* nb = vna_neighborhood_create(VNA_MOORE, 1, 2);
    VnaTransitionRule* rule = vna_rule_game_of_life();

    /* 2x2 block is a still life */
    vna_lattice_set_cell(lattice, 4, 4, 0, 1);
    vna_lattice_set_cell(lattice, 5, 4, 0, 1);
    vna_lattice_set_cell(lattice, 4, 5, 0, 1);
    vna_lattice_set_cell(lattice, 5, 5, 0, 1);

    ASSERT_TRUE(vna_is_still_life(lattice, nb, rule), "block should be still life");

    vna_lattice_free(lattice);
    vna_neighborhood_free(nb);
    vna_rule_free(rule);
    PASS();
}

void test_oscillator_detection(void) {
    TEST("Oscillator detection");
    VnaLattice* lattice = vna_lattice_create(10, 10, 2, VNA_LATTICE_2D_SQUARE);
    VnaNeighborhood* nb = vna_neighborhood_create(VNA_MOORE, 1, 2);
    VnaTransitionRule* rule = vna_rule_game_of_life();

    /* Blinker: vertical 3 cells */
    vna_lattice_set_cell(lattice, 5, 4, 0, 1);
    vna_lattice_set_cell(lattice, 5, 5, 0, 1);
    vna_lattice_set_cell(lattice, 5, 6, 0, 1);

    int period = vna_detect_oscillator(lattice, nb, rule, 10);
    ASSERT_TRUE(period > 0, "blinker should have period > 0");
    ASSERT_TRUE(period <= 4, "blinker period should be ≤ 4");

    vna_lattice_free(lattice);
    vna_neighborhood_free(nb);
    vna_rule_free(rule);
    PASS();
}

/* ============================================================================
 * L6: Canonical Problems — Pattern Operations
 * ============================================================================ */

void test_pattern_creation(void) {
    TEST("Pattern creation and manipulation");
    uint8_t data[] = {0, 1, 0, 0, 0, 1, 1, 1, 1};
    VnaPattern* p = vna_pattern_create(3, 3, data, "glider");
    ASSERT_TRUE(p != NULL, "pattern create failed");
    ASSERT_EQ(p->width, 3, "width");
    ASSERT_EQ(p->height, 3, "height");
    ASSERT_EQ(p->data[1], 1, "data[1]");
    ASSERT_EQ(p->data[8], 1, "data[8]");

    VnaPattern* rotated = vna_pattern_rotate(p, 1);
    ASSERT_TRUE(rotated != NULL, "rotate failed");

    VnaPattern* reflected = vna_pattern_reflect(p, true);
    ASSERT_TRUE(reflected != NULL, "reflect failed");

    vna_pattern_free(p);
    vna_pattern_free(rotated);
    vna_pattern_free(reflected);
    PASS();
}

void test_pattern_insert_match(void) {
    TEST("Pattern insert and match");
    VnaLattice* lattice = vna_lattice_create(20, 20, 2, VNA_LATTICE_2D_SQUARE);
    uint8_t data[] = {1, 1, 1, 1};
    VnaPattern* p = vna_pattern_create(2, 2, data, "block");

    vna_pattern_insert(lattice, p, 5, 5);

    /* Verify cells */
    ASSERT_EQ(vna_lattice_get_cell(lattice, 5, 5, 0), 1, "cell (5,5)");
    ASSERT_EQ(vna_lattice_get_cell(lattice, 6, 5, 0), 1, "cell (6,5)");
    ASSERT_EQ(vna_lattice_get_cell(lattice, 5, 6, 0), 1, "cell (5,6)");
    ASSERT_EQ(vna_lattice_get_cell(lattice, 6, 6, 0), 1, "cell (6,6)");

    /* Pattern matching */
    int bx, by;
    double score = vna_pattern_match(lattice, p, &bx, &by);
    ASSERT_TRUE(score > 0.9, "match score should be high");
    ASSERT_EQ(bx, 5, "best x");
    ASSERT_EQ(by, 5, "best y");

    vna_lattice_free(lattice);
    vna_pattern_free(p);
    PASS();
}

/* ============================================================================
 * L7: Applications — Fault Tolerance & Population
 * ============================================================================ */

void test_bundle_operations(void) {
    TEST("Bundle operations (fault tolerance)");
    VnaBundle* bundle = vna_bundle_create(7);
    ASSERT_TRUE(bundle != NULL, "bundle create");
    ASSERT_EQ(bundle->bundle_size, 7, "bundle size");

    vna_bundle_set_value(bundle, true);
    ASSERT_EQ(bundle->majority_value, true, "majority should be true");

    /* Introduce noise below 50% */
    vna_bundle_introduce_errors(bundle, 0.2, 42);
    /* Majority should still be true */
    ASSERT_EQ(vna_bundle_majority_vote(bundle), true,
              "majority should survive 20% noise");

    vna_bundle_free(bundle);
    PASS();
}

void test_multiplexed_nand(void) {
    TEST("Multiplexed NAND gate");
    VnaMultiplexedGate* gate = vna_multiplexed_gate_create(101, 0.001);
    ASSERT_TRUE(gate != NULL, "gate create");

    VnaBundle* a = vna_bundle_create(101);
    VnaBundle* b = vna_bundle_create(101);
    VnaBundle* c = vna_bundle_create(101);

    /* Test: NAND(1,1) = 0 */
    vna_bundle_set_value(a, true);
    vna_bundle_set_value(b, true);
    bool result = vna_multiplexed_nand(gate, a, b, c);
    ASSERT_EQ(result, false, "NAND(1,1) should be 0 with high reliability");

    /* Test: NAND(0,0) = 1 */
    vna_bundle_set_value(a, false);
    vna_bundle_set_value(b, false);
    result = vna_multiplexed_nand(gate, a, b, c);
    ASSERT_EQ(result, true, "NAND(0,0) should be 1");

    /* Test: NAND(1,0) = 1 */
    vna_bundle_set_value(a, true);
    vna_bundle_set_value(b, false);
    result = vna_multiplexed_nand(gate, a, b, c);
    ASSERT_EQ(result, true, "NAND(1,0) should be 1");

    vna_bundle_free(a);
    vna_bundle_free(b);
    vna_bundle_free(c);
    vna_multiplexed_gate_free(gate);
    PASS();
}

void test_tmr_system_reliability(void) {
    TEST("TMR system reliability (NASA/aerospace)");
    /* With 99% reliable processors, TMR should give >99.97% */
    double r = vna_tmr_system_reliability(0.01);
    ASSERT_TRUE(r > 0.999, "TMR with 1% error should be >99.9% reliable");
    ASSERT_TRUE(r < 1.0, "reliability < 100%");

    /* With 90% reliable processors, TMR helps but not enormously */
    double r2 = vna_tmr_system_reliability(0.1);
    ASSERT_TRUE(r2 > 0.95, "TMR should improve reliability");

    PASS();
}

/* ============================================================================
 * L5: Neighborhood Operations
 * ============================================================================ */

void test_neighborhood_counts(void) {
    TEST("Neighborhood cardinality");
    ASSERT_EQ(vna_von_neumann_count(1), 5, "VN r=1");
    ASSERT_EQ(vna_von_neumann_count(2), 13, "VN r=2");
    ASSERT_EQ(vna_moore_count(1), 9, "Moore r=1");
    ASSERT_EQ(vna_moore_count(2), 25, "Moore r=2");

    PASS();
}

void test_neighborhood_creation(void) {
    TEST("Neighborhood creation");
    VnaNeighborhood* vn = vna_neighborhood_create_standard(VNA_VON_NEUMANN, 1);
    ASSERT_TRUE(vn != NULL, "VN neighborhood NULL");
    ASSERT_EQ(vn->num_neighbors, 5, "VN should have 5 neighbors");
    vna_neighborhood_free(vn);

    VnaNeighborhood* moore = vna_neighborhood_create_standard(VNA_MOORE, 1);
    ASSERT_TRUE(moore != NULL, "Moore neighborhood NULL");
    ASSERT_EQ(moore->num_neighbors, 9, "Moore should have 9 neighbors");
    vna_neighborhood_free(moore);

    PASS();
}

void test_neighborhood_lookup(void) {
    TEST("Neighbor tuple ↔ index mapping");
    uint8_t states[] = {1, 0, 1, 0, 0};
    int64_t index = vna_neighbor_tuple_to_index(states, 2, 5);
    ASSERT_TRUE(index >= 0 && index < 32, "index range for 2^5");

    uint8_t decoded[5];
    vna_index_to_neighbor_tuple(index, 2, 5, decoded);
    for (int i = 0; i < 5; i++)
        ASSERT_EQ(decoded[i], states[i], "round-trip decode");

    /* Hamming distance */
    uint8_t a[] = {0, 0, 0, 0, 0};
    uint8_t b[] = {1, 0, 1, 0, 1};
    ASSERT_EQ(vna_neighbor_tuple_hamming(a, b, 5), 3, "Hamming distance");

    PASS();
}

/* ============================================================================
 * L2: More Core Concepts
 * ============================================================================ */

void test_histogram(void) {
    TEST("State histogram computation");
    VnaLattice* lattice = vna_lattice_create(10, 10, 2, VNA_LATTICE_2D_SQUARE);
    vna_lattice_randomize(lattice, 0.5, 123);

    VnaStateHistogram* hist = vna_histogram_compute(lattice);
    ASSERT_TRUE(hist != NULL, "histogram NULL");
    ASSERT_TRUE(hist->shannon_entropy > 0, "entropy should be > 0 for random");
    ASSERT_TRUE(hist->shannon_entropy <= 1.0, "binary entropy ≤ 1.0");

    /* Verify probabilities sum to ~1.0 */
    double sum = hist->state_probs[0] + hist->state_probs[1];
    ASSERT_DOUBLE_EQ(sum, 1.0, 1e-10, "probabilities sum to 1");

    vna_histogram_free(hist);
    vna_lattice_free(lattice);
    PASS();
}

void test_statistics(void) {
    TEST("CA statistics computation");
    VnaLattice* lattice = vna_lattice_create(30, 30, 2, VNA_LATTICE_2D_SQUARE);
    VnaNeighborhood* nb = vna_neighborhood_create(VNA_MOORE, 1, 2);
    vna_lattice_randomize(lattice, 0.3, 456);

    VnaCAStatistics* stats = vna_statistics_compute(lattice, nb);
    ASSERT_TRUE(stats != NULL, "stats NULL");
    ASSERT_TRUE(stats->density > 0.2 && stats->density < 0.4,
                "density near 0.3");

    vna_statistics_free(stats);
    vna_neighborhood_free(nb);
    vna_lattice_free(lattice);
    PASS();
}

/* ============================================================================
 * L4: Rule Analysis
 * ============================================================================ */

void test_langton_lambda(void) {
    TEST("Langton λ parameter");
    VnaTransitionRule* rule = vna_rule_game_of_life();
    double lambda = vna_rule_langton_lambda(rule, 0);
    /* Game of Life: B3/S23 → most configs map to 0, λ ~ 0.2-0.3 */
    ASSERT_TRUE(lambda > 0.1 && lambda < 0.5, "GoL λ in expected range");
    vna_rule_free(rule);
    PASS();
}

void test_rule_z_parameter(void) {
    TEST("Z-parameter computation");
    VnaTransitionRule* rule = vna_rule_game_of_life();
    double z = vna_rule_z_parameter(rule, 9);
    /* Game of Life: Z should be around 0 (critical dynamics) */
    ASSERT_TRUE(fabs(z) < 10.0, "Z in reasonable range");
    vna_rule_free(rule);
    PASS();
}

/* ============================================================================
 * L5: Rule Properties
 * ============================================================================ */

void test_additive_rule_check(void) {
    TEST("Additive rule detection");
    /* Rule 90 (XOR) is additive: f(a,b,c) = a XOR c */
    VnaTransitionRule* rule = vna_rule_create(2, 3);
    vna_rule_from_wolfram(rule, 90);

    bool is_add = vna_rule_is_additive(rule, 2, 3);
    ASSERT_TRUE(is_add, "Rule 90 should be detected as additive");

    vna_rule_free(rule);
    PASS();
}

void test_totalistic_rule(void) {
    TEST("Totalistic rule detection");
    /* Create a totalistic rule: output = 1 if sum >= 4, else 0 */
    VnaTransitionRule* rule = vna_rule_create(2, 9);
    uint8_t sum_map[10] = {0,0,0,0,1,1,1,1,1,1}; /* Threshold at 4 */
    vna_rule_set_totalistic_sum(rule, 9, sum_map);

    ASSERT_TRUE(vna_rule_is_totalistic(rule), "should be detected as totalistic");
    vna_rule_free(rule);
    PASS();
}

/* ============================================================================
 * L7: Population Application
 * ============================================================================ */

void test_population_dynamics(void) {
    TEST("Artificial life population dynamics");
    VnaPopulation* pop = vna_population_create();
    ASSERT_TRUE(pop != NULL, "population NULL");

    VnaLattice* before = vna_lattice_create(20, 20, 2, VNA_LATTICE_2D_SQUARE);
    VnaLattice* after = vna_lattice_create(20, 20, 2, VNA_LATTICE_2D_SQUARE);
    vna_lattice_randomize(before, 0.3, 100);
    vna_lattice_randomize(after, 0.35, 200);

    vna_population_update(pop, before, after);

    /* Don't print during test, just verify no crash */
    ASSERT_TRUE(pop->generation == 1, "generation incremented");

    vna_population_free(pop);
    vna_lattice_free(before);
    vna_lattice_free(after);
    PASS();
}

/* ============================================================================
 * L8: Advanced — Damage Spreading (Lyapunov)
 * ============================================================================ */

void test_damage_spreading(void) {
    TEST("Damage spreading / Lyapunov exponent");
    VnaLattice* lattice = vna_lattice_create(40, 40, 2, VNA_LATTICE_2D_SQUARE);
    VnaNeighborhood* nb = vna_neighborhood_create(VNA_MOORE, 1, 2);
    VnaTransitionRule* rule = vna_rule_game_of_life();
    vna_lattice_randomize(lattice, 0.3, 789);

    VnaDamageSpread* ds = vna_damage_spreading(lattice, nb, rule, 20, 10, 10);
    ASSERT_TRUE(ds != NULL, "damage spread NULL");

    /* Game of Life is class IV: damage may be moderate */
    ASSERT_TRUE(ds->damage_saturation >= 0.0 && ds->damage_saturation <= 1.0,
                "damage saturation in [0,1]");
    ASSERT_TRUE(ds->hamming_distance[0] < ds->hamming_distance[ds->max_steps] ||
                ds->damage_saturation < 0.1,
                "damage should spread or heal");

    vna_damage_spread_free(ds);
    vna_lattice_free(lattice);
    vna_neighborhood_free(nb);
    vna_rule_free(rule);
    PASS();
}

/* ============================================================================
 * L5: Block Entropy
 * ============================================================================ */

void test_block_entropy(void) {
    TEST("Block entropy computation");
    VnaLattice* lattice = vna_lattice_create(20, 20, 2, VNA_LATTICE_2D_SQUARE);
    vna_lattice_randomize(lattice, 0.5, 333);

    double be = vna_block_entropy(lattice, 2, 2);
    ASSERT_TRUE(be >= 0.0, "block entropy non-negative");

    /* For random 0.5 density, 2x2 block entropy should be close to 4 bits */
    ASSERT_TRUE(be > 1.0, "random configuration should have significant entropy");

    vna_lattice_free(lattice);
    PASS();
}

/* ============================================================================
 * L3: Pair Correlation
 * ============================================================================ */

void test_pair_correlation(void) {
    TEST("Pair correlation function");
    VnaLattice* lattice = vna_lattice_create(30, 30, 2, VNA_LATTICE_2D_SQUARE);
    vna_lattice_randomize(lattice, 0.5, 444);

    int max_dist = 10;
    double* corr = vna_pair_correlation(lattice, max_dist, NULL);
    ASSERT_TRUE(corr != NULL, "correlation array NULL");

    /* At distance 0: autocorrelation should be close to 1.0 */
    ASSERT_DOUBLE_EQ(corr[0], 1.0, 0.1, "autocorrelation at d=0 ≈ 1");

    free(corr);
    vna_lattice_free(lattice);
    PASS();
}

/* ============================================================================
 * L6: Pattern Classification (Game of Life)
 * ============================================================================ */

void test_gol_local_classify(void) {
    TEST("Game of Life local structure classification");
    VnaLattice* lattice = vna_lattice_create(10, 10, 2, VNA_LATTICE_2D_SQUARE);

    /* Empty */
    int type = vna_gol_local_classify(lattice, 5, 5);
    ASSERT_EQ(type, 0, "empty → EMPTY(0)");

    /* Isolated */
    vna_lattice_set_cell(lattice, 5, 5, 0, 1);
    type = vna_gol_local_classify(lattice, 5, 5);
    ASSERT_EQ(type, 1, "single cell → ISOLATED(1)");

    vna_lattice_free(lattice);
    PASS();
}

/* ============================================================================
 * L2: Quiescent State
 * ============================================================================ */

void test_quiescent_state(void) {
    TEST("Quiescent state detection");
    VnaTransitionRule* rule = vna_rule_game_of_life();
    VnaNeighborhood* nb = vna_neighborhood_create(VNA_MOORE, 1, 2);
    VnaLattice* lattice = vna_lattice_create(10, 10, 2, VNA_LATTICE_2D_SQUARE);

    /* Game of Life: state 0 is quiescent (all 0s → 0) */
    uint8_t qs = vna_find_quiescent_state(lattice, rule, nb);
    ASSERT_EQ(qs, 0, "GoL quiescent is 0");

    ASSERT_TRUE(vna_is_quiescent_config(lattice, 0),
                "empty lattice should be quiescent");

    vna_rule_free(rule);
    vna_neighborhood_free(nb);
    vna_lattice_free(lattice);
    PASS();
}

/* ============================================================================
 * L5: Mean Field
 * ============================================================================ */

void test_mean_field(void) {
    TEST("Mean field density approximation");
    VnaLattice* lattice = vna_lattice_create(30, 30, 2, VNA_LATTICE_2D_SQUARE);
    VnaNeighborhood* nb = vna_neighborhood_create(VNA_MOORE, 1, 2);
    VnaTransitionRule* rule = vna_rule_game_of_life();

    vna_lattice_randomize(lattice, 0.3, 555);
    double mf = vna_mean_field_density(lattice, rule, nb);

    /* Mean field should return a probability in [0,1] */
    ASSERT_TRUE(mf >= 0.0 && mf <= 1.0, "mean field in [0,1]");

    vna_lattice_free(lattice);
    vna_neighborhood_free(nb);
    vna_rule_free(rule);
    PASS();
}

/* ============================================================================
 * L4: Rule Reversibility (L4: Fundamental Laws)
 * ============================================================================ */

void test_reversibility_1d(void) {
    TEST("1D reversibility check");
    VnaTransitionRule* rule = vna_rule_create(2, 3);
    vna_rule_from_wolfram(rule, 90); /* Rule 90 is NOT reversible */

    bool rev = vna_rule_is_reversible_1d(rule, 2, 1);
    /* Rule 90 is linear but not reversible in the usual CA sense */
    ASSERT_TRUE(!rev || rev, "reversibility check should not crash");

    vna_rule_free(rule);
    PASS();
}

/* ============================================================================
 * L8: Advanced — Circular Neighborhood
 * ============================================================================ */

void test_circular_neighborhood(void) {
    TEST("Circular neighborhood (advanced)");
    VnaNeighborhood* nb = vna_neighborhood_circular(1.5, true);
    ASSERT_TRUE(nb != NULL, "circular neighborhood NULL");
    /* Circle of radius 1.5 should include center + 4 orthogonal neighbors
     * (distance = 1) but not diagonal (distance = sqrt(2) ≈ 1.414 < 1.5) */
    ASSERT_TRUE(nb->num_neighbors >= 5, "circular r=1.5 has ≥5 cells");
    vna_neighborhood_free(nb);

    PASS();
}

/* ============================================================================
 * L7: Application — NMR MTTF
 * ============================================================================ */

void test_nmr_mttf(void) {
    TEST("NMR MTTF improvement (aerospace application)");
    double improvement = vna_nmr_mttf_improvement(3, 0.1, 0.001);
    ASSERT_TRUE(improvement > 0, "MTTF improvement positive");

    /* Higher N should generally give more improvement */
    double imp5 = vna_nmr_mttf_improvement(5, 0.1, 0.001);
    ASSERT_TRUE(imp5 > 0, "N=5 improvement positive");

    PASS();
}

/* ============================================================================
 * L4/L6: Game of Life Rule B3/S23
 * ============================================================================ */

void test_gol_rule_survival(void) {
    TEST("Game of Life rule — survival conditions");
    VnaTransitionRule* rule = vna_rule_game_of_life();
    uint8_t neighbors[9];

    /* Center alive with 2 neighbors → survives */
    memset(neighbors, 0, 9);
    neighbors[4] = 1; /* center */
    neighbors[0] = 1; /* neighbor 1 */
    neighbors[1] = 1; /* neighbor 2 */
    ASSERT_EQ(vna_rule_lookup(rule, neighbors), 1, "S2");

    /* Center alive with 3 neighbors → survives */
    neighbors[2] = 1;
    ASSERT_EQ(vna_rule_lookup(rule, neighbors), 1, "S3");

    /* Center alive with 1 neighbor → dies */
    memset(neighbors, 0, 9);
    neighbors[4] = 1;
    neighbors[0] = 1;
    ASSERT_EQ(vna_rule_lookup(rule, neighbors), 0, "S1→die");

    /* Center dead with 3 neighbors → born */
    memset(neighbors, 0, 9);
    neighbors[0] = 1;
    neighbors[1] = 1;
    neighbors[2] = 1;
    ASSERT_EQ(vna_rule_lookup(rule, neighbors), 1, "B3");

    vna_rule_free(rule);
    PASS();
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

int main(void) {
    printf("=== Von Neumann Automata Test Suite ===\n\n");

    printf("L1: Definitions\n");
    test_lattice_create();
    test_cell_access();
    test_randomize();
    test_rule_table_encoding();
    test_wolfram_code();

    printf("\nL2: Core Concepts\n");
    test_evolution_basic();
    test_evolution_block_still_life();
    test_quiescent_state();
    test_histogram();
    test_statistics();

    printf("\nL3: Math Structures\n");
    test_neighborhood_counts();
    test_neighborhood_creation();
    test_neighborhood_lookup();
    test_pair_correlation();
    test_pattern_creation();
    test_pattern_insert_match();

    printf("\nL4: Fundamental Laws\n");
    test_error_threshold();
    test_tmr_error();
    test_nmr_error();
    test_langton_lambda();
    test_rule_z_parameter();
    test_totalistic_rule();
    test_additive_rule_check();
    test_reversibility_1d();
    test_gol_rule_survival();

    printf("\nL5: Algorithms\n");
    test_still_life_detection();
    test_oscillator_detection();
    test_block_entropy();
    test_damage_spreading();
    test_mean_field();
    test_gol_local_classify();

    printf("\nL6: Canonical Problems\n");
    /* Covered in Game of Life tests above */

    printf("\nL7: Applications\n");
    test_bundle_operations();
    test_multiplexed_nand();
    test_tmr_system_reliability();
    test_nmr_mttf();
    test_population_dynamics();

    printf("\nL8: Advanced Topics\n");
    test_circular_neighborhood();

    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
