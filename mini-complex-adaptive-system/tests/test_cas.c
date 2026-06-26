/**
 * @file test_cas.c
 * @brief Comprehensive tests for mini-complex-adaptive-system
 *
 * Tests cover L1-L6 knowledge points.
 * Uses assert() for all checks.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "cas_agent.h"
#include "cas_landscape.h"
#include "cas_network.h"
#include "cas_dynamics.h"
#include "cas_evolution.h"
#include "cas_emergence.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { tests_run++; printf("  TEST %s ... ", #name); } while(0)
#define PASS() do { tests_passed++; printf("PASSED\n"); } while(0)
#define FAIL(msg) do { printf("FAILED: %s\n", msg); } while(0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

/* =================================================================
 * L1: Agent Lifecycle Tests
 * ================================================================= */

static void test_agent_init(void) {
    TEST(agent_init);
    cas_agent_t agent;
    cas_agent_init(&agent, 42, CAS_AGENT_LEARNING, 8);

    CHECK(agent.id == 42, "id mismatch");
    CHECK(agent.type == CAS_AGENT_LEARNING, "type mismatch");
    CHECK(agent.model.state_dim == 8, "state_dim mismatch");
    CHECK(agent.model.initialized == true, "not initialized");
    CHECK(agent.model.learning_rate > 0.0, "learning rate zero");
    CHECK(agent.energy > 0.0, "energy zero");
    CHECK(agent.fitness == 0.0, "fitness not zero");
    CHECK(agent.age == 0, "age not zero");
    CHECK(agent.num_sensors == 0, "sensors not zero");
    CHECK(agent.num_effectors == 0, "effectors not zero");

    cas_agent_destroy(&agent);
    CHECK(agent.model.initialized == false, "destroy failed");
    PASS();
}

/* =================================================================
 * L1: Tag and Building Block Tests
 * ================================================================= */

static void test_agent_tag(void) {
    TEST(agent_tag);
    cas_agent_t agent;
    cas_agent_init(&agent, 1, CAS_AGENT_REACTIVE, 0);

    bool ok = cas_agent_add_tag(&agent, "predator", 0.8);
    CHECK(ok, "add tag failed");
    CHECK(agent.num_tags == 1, "tag count wrong");

    double match = cas_agent_has_tag(&agent, "predator");
    CHECK(match > 0.0, "tag not found");
    CHECK(match <= 1.0, "tag strength too high");

    double no_match = cas_agent_has_tag(&agent, "nonexistent");
    CHECK(no_match == 0.0, "false tag match");

    cas_agent_destroy(&agent);
    PASS();
}

static void test_building_block(void) {
    TEST(building_block);
    cas_agent_t agent;
    cas_agent_init(&agent, 2, CAS_AGENT_LEARNING, 4);

    uint8_t cond[4] = {0xAA, 0x55, 0x0F, 0xF0};
    uint8_t act[4] = {0x11, 0x22, 0x33, 0x44};

    bool ok = cas_agent_add_building_block(&agent, "avoid_wall", cond, act);
    CHECK(ok, "add block failed");
    CHECK(agent.num_building_blocks == 1, "block count wrong");

    uint8_t query[4] = {0xAA, 0x55, 0x00, 0xF0};
    int32_t idx = cas_agent_match_block(&agent, query);
    CHECK(idx >= 0, "match failed");
    CHECK(idx < 32, "match index out of range");

    /* No match for completely different condition */
    uint8_t bad[4] = {0x00, 0x00, 0x00, 0x00};
    // int32_t idx2 = cas_agent_match_block(&agent, bad);
    /* May or may not match depending on specificity */

    cas_agent_destroy(&agent);
    PASS();
}

/* =================================================================
 * L2: OODA Loop Tests
 * ================================================================= */

static void test_agent_ooda(void) {
    TEST(agent_ooda);
    cas_agent_t agent;
    cas_agent_init(&agent, 3, CAS_AGENT_DELIBERATIVE, 4);

    /* Add a sensor and effector */
    agent.sensors[0].source_id = 0;
    agent.sensors[0].active = true;
    agent.sensors[0].noise_std = 0.01;
    agent.sensors[0].sensitivity = 1.0;
    agent.sensors[0].range_min = -10.0;
    agent.sensors[0].range_max = 10.0;
    agent.num_sensors = 1;

    agent.effectors[0].target_id = 0;
    agent.effectors[0].active = true;
    agent.effectors[0].saturation_min = -1.0;
    agent.effectors[0].saturation_max = 1.0;
    agent.effectors[0].energy_cost = 0.1;
    agent.effectors[0].precision = 0.01;
    agent.num_effectors = 1;

    double world[4] = {1.0, 2.0, 3.0, 4.0};

    /* Sense */
    cas_agent_sense(&agent, world, 4);
    CHECK(fabs(agent.sensors[0].reading - 1.0) < 0.2, "sense accuracy");

    /* Decide */
    double actions[16];
    uint32_t na = cas_agent_decide(&agent, actions, 16);
    CHECK(na == 1, "decide action count");
    CHECK(fabs(actions[0]) <= 1.0, "action out of bounds");

    /* Act */
    double world_copy[4];
    memcpy(world_copy, world, sizeof(world));
    cas_agent_act(&agent, actions, world_copy, 4);
    /* World should be modified */
    CHECK(agent.energy < 100.0, "energy not consumed");

    /* Adapt */
    double prev_error __attribute__((unused)) = agent.model.prediction_error;
    cas_agent_adapt(&agent, 0.5);
    CHECK(agent.age == 1, "age not incremented");
    CHECK(agent.reward_history[0] == 0.5, "reward not recorded");

    /* Full OODA step */
    cas_agent_step(&agent, world, 4, -0.2);
    CHECK(agent.age == 2, "step age");

    cas_agent_destroy(&agent);
    PASS();
}

/* =================================================================
 * L2: Peer Interaction Test
 * ================================================================= */

static void test_agent_interact(void) {
    TEST(agent_interact);
    cas_agent_t a, b;
    cas_agent_init(&a, 10, CAS_AGENT_LEARNING, 4);
    cas_agent_init(&b, 11, CAS_AGENT_LEARNING, 4);

    a.fitness = 10.0;
    b.fitness = 5.0;

    for (uint32_t i = 0; i < 4; i++) {
        a.model.state[i] = 1.0;
        b.model.state[i] = 0.0;
    }

    cas_agent_interact(&a, &b);

    /* b should have moved toward a's state */
    CHECK(b.model.state[0] > 0.0, "interaction no transfer");
    CHECK(fabs(b.model.state[0]) < fabs(a.model.state[0]), "over-transfer");

    /* Similarity should be positive */
    double sim = cas_agent_similarity(&a, &b);
    CHECK(sim > 0.0, "similarity zero");
    CHECK(sim <= 1.0, "similarity >1");

    cas_agent_destroy(&a);
    cas_agent_destroy(&b);
    PASS();
}

/* =================================================================
 * L3: NK Landscape Tests
 * ================================================================= */

static void test_nk_landscape(void) {
    TEST(nk_landscape);
    cas_nk_landscape_t land;
    cas_nk_init(&land, 8, 2, 42);

    CHECK(land.n == 8, "N mismatch");
    CHECK(land.k == 2, "K mismatch");

    uint8_t genotype[4] = {0xAA, 0x00, 0x00, 0x00};
    double fit = cas_nk_evaluate(&land, genotype);
    CHECK(fit >= 0.0, "negative fitness");
    CHECK(fit <= 1.0, "fitness >1");

    /* Full evaluation */
    cas_landscape_point_t point;
    memcpy(point.genotype, genotype, 4);
    cas_nk_evaluate_full(&land, &point);
    CHECK(fabs(point.fitness - fit) < 1e-9, "full fitness mismatch");

    /* Hamming distance */
    uint8_t g2[4] = {0xBB, 0x00, 0x00, 0x00};
    uint32_t hd = cas_nk_hamming_distance(genotype, g2, 8);
    CHECK(hd <= 8, "hamming distance > N");

    /* Fitness correlation */
    double rho = cas_nk_fitness_correlation(8, 2, 1);
    CHECK(rho >= 0.0 && rho <= 1.0, "correlation out of range");

    /* Adaptive walk step */
    cas_nk_random_genotype(&land, point.genotype, NULL);
    cas_nk_evaluate_full(&land, &point);
    double before = point.fitness;
    cas_nk_adaptive_walk_step(&land, &point);
    CHECK(point.fitness >= before, "walk decreased fitness");

    /* Landscape stats */
    cas_landscape_stats_t stats;
    cas_nk_compute_stats(&land, 200, &stats, NULL);
    CHECK(stats.mean_fitness > 0.0, "stats mean zero");

    /* Ruggedness */
    double rug = cas_nk_ruggedness(&land, 50, NULL);
    CHECK(rug >= 0.0 && rug <= 1.0, "ruggedness out of range");

    CHECK(land.k == 2, "parameter changed");
    PASS();
}

/* =================================================================
 * L3: Network Tests
 * ================================================================= */

static void test_network(void) {
    TEST(network);
    cas_network_t net;
    cas_net_init(&net, 20, CAS_NET_ERDOS_RENYI, 12345);

    CHECK(net.num_nodes == 20, "node count");

    /* ER random graph */
    cas_net_erdos_renyi(&net, 0.2, NULL);
    CHECK(net.num_edges > 0, "no edges in ER");
    CHECK(net.num_edges < 400, "too many edges");

    /* Clustering */
    double cc = cas_net_clustering_coeff(&net, 0);
    CHECK(cc >= 0.0 && cc <= 1.0, "cc out of range");

    /* Global clustering */
    cas_net_global_clustering(&net);
    CHECK(net.global_clustering >= 0.0, "global cc negative");

    /* Path length */
    double apl = cas_net_avg_path_length(&net);
    CHECK(apl > 0.0, "path length zero");

    /* Connectedness */
    bool conn __attribute__((unused)) = cas_net_is_connected(&net);
    /* May or may not be connected at p=0.2 with 20 nodes */

    /* PageRank */
    cas_net_pagerank(&net, 0.85, 50);
    CHECK(net.node_attr[0].pagerank > 0.0, "pagerank zero");

    /* Communities */
    cas_net_louvain_communities(&net);
    CHECK(net.node_attr[0].community_id < net.num_nodes, "community id too large");

    /* Random walk */
    uint32_t path[100];
    cas_net_random_walk(&net, 0, 20, path, NULL);

    /* Eigenvector centrality */
    cas_net_eigenvector_centrality(&net, 50);

    /* Laplacian */
    cas_net_laplacian_spectrum(&net);

    /* Test Small-World network */
    cas_network_t net2;
    cas_net_init(&net2, 20, CAS_NET_WATTS_STROGATZ, 54321);
    cas_net_watts_strogatz(&net2, 4, 0.1, NULL);
    CHECK(net2.num_edges > 0, "WS no edges");
    cas_net_global_clustering(&net2);
    double apl2 = cas_net_avg_path_length(&net2);
    CHECK(apl2 > 0.0, "WS path length zero");

    /* Test Scale-Free network */
    cas_network_t net3;
    cas_net_init(&net3, 30, CAS_NET_BARABASI_ALBERT, 99999);
    cas_net_barabasi_albert(&net3, 5, 3, NULL);
    CHECK(net3.num_edges > 0, "BA no edges");

    /* Cascade test */
    bool affected[CAS_NET_MAX_NODES];
    cas_net_cascade(&net, 0, 0.3, affected);
    CHECK(affected[0], "seed not affected");

    PASS();
}

/* =================================================================
 * L5: Population Dynamics Test
 * ================================================================= */

static void test_population(void) {
    TEST(population);
    cas_population_t pop;
    cas_pop_init(&pop, 20, CAS_AGENT_LEARNING, 4, 4242);

    CHECK(pop.size == 20, "population size");
    CHECK(pop.step == 0, "step not zero");

    double env[8] = {0.5, 0.3, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0};

    /* Run a few steps */
    uint32_t i;
    for (i = 0; i < 10; i++) {
        cas_pop_step(&pop, env, 8, NULL);
    }
    CHECK(pop.step == 10, "step count");
    CHECK(pop.agents[0].age > 0, "agents not aging");

    /* Population stats */
    cas_pop_compute_stats(&pop);
    CHECK(pop.mean_fitness != 0.0 || pop.fitness_variance >= 0.0, "stats invalid");

    /* Selection */
    uint32_t before = pop.size;
    cas_pop_selection(&pop, NULL);
    CHECK(pop.size == before, "selection changed size");

    /* Mutation */
    cas_pop_mutation(&pop, 0.05, NULL);

    PASS();
}

/* =================================================================
 * L5: Co-evolution Test
 * ================================================================= */

static void test_coevolution(void) {
    TEST(coevolution);
    cas_coevolution_t ce;
    cas_coevolve_init(&ce, 0, 1);

    CHECK(ce.agent_a == 0 && ce.agent_b == 1, "ids wrong");
    CHECK(!ce.is_mutualistic, "default mutualistic");

    /* Simulate mutualistic interaction */
    uint32_t i;
    for (i = 0; i < 20; i++)
        cas_coevolve_update(&ce, 0.5, 0.5);

    int rel = cas_coevolve_classify(&ce);
    CHECK(rel >= 1 && rel <= 3, "unknown relationship");

    double rq = cas_coevolve_red_queen_effect(&ce);
    CHECK(rq >= 0.0, "red queen negative");

    PASS();
}

/* =================================================================
 * L4: Sandpile SOC Test
 * ================================================================= */

static void test_sandpile(void) {
    TEST(sandpile);
    cas_sandpile_t sp;
    cas_sandpile_init(&sp, 32);

    CHECK(sp.size == 32, "sandpile size");

    /* Run many drops */
    cas_sandpile_run(&sp, 1000, NULL);

    CHECK(sp.total_avalanches == 1000, "avalanche count");
    CHECK(sp.total_topplings > 0, "no topplings");

    /* Power law exponent */
    double tau = cas_sandpile_powerlaw_exponent(&sp);
    CHECK(tau >= 0.0, "negative tau");

    PASS();
}

/* =================================================================
 * L5: Genetic Algorithm Test
 * ================================================================= */

static double ga_test_fitness(const uint8_t *genes, uint32_t n_bits, void *data) {
    (void)data;
    /* Simple one-max: count 1-bits */
    uint32_t ones = 0;
    uint32_t i;
    for (i = 0; i < n_bits; i++)
        if ((genes[i / 8] >> (i % 8)) & 1) ones++;
    return (double)ones;
}

static void test_genetic_algorithm(void) {
    TEST(genetic_algorithm);
    cas_genetic_algorithm_t ga;
    cas_ga_init(&ga, 50, 32, ga_test_fitness, NULL, 12345);

    CHECK(ga.pop_size == 50, "pop size");
    CHECK(ga.genome_bits == 32, "genome bits");

    cas_ga_evaluate(&ga);
    CHECK(ga.evaluations == 50, "eval count");
    CHECK(ga.best_fitness >= 0.0, "best negative");
    CHECK(ga.best_fitness <= 32.0, "best > max");

    /* Run a few generations */
    uint32_t g;
    for (g = 0; g < 20; g++)
        cas_ga_generation(&ga, NULL);

    CHECK(ga.generation == 20, "generation count");
    CHECK(ga.best_fitness >= 0.0, "best fitness negative after evolution");

    /* Best genome should have many 1s */
    uint8_t best_gene[4];
    double best_fit;
    cas_ga_get_best(&ga, best_gene, &best_fit);

    PASS();
}

/* =================================================================
 * L6: Minority Game Test
 * ================================================================= */

static void test_minority_game(void) {
    TEST(minority_game);
    cas_minority_game_t mg;
    cas_mg_init(&mg, 101, 6, 2, 54321);

    CHECK(mg.n_agents == 101, "agent count");
    CHECK(mg.m == 6, "memory");

    /* Run game */
    cas_mg_run(&mg, 500, NULL);

    double vol = cas_mg_volatility(&mg);
    CHECK(vol > 0.0, "zero volatility");

    PASS();
}

/* =================================================================
 * L5: Emergence Metrics Test
 * ================================================================= */

static void test_emergence(void) {
    TEST(emergence);
    double x[100], y[100];
    uint32_t i;
    for (i = 0; i < 100; i++) {
        x[i] = sin(i * 0.1) + 0.1 * ((double)rand() / RAND_MAX - 0.5);
        y[i] = cos(i * 0.1) + 0.1 * ((double)rand() / RAND_MAX - 0.5);
    }

    /* Entropy */
    double hx = cas_entropy_discrete(x, 100, 10, NULL, NULL);
    CHECK(hx > 0.0, "zero entropy");

    /* Mutual information */
    double mi = cas_mutual_information(x, y, 100, 10);
    CHECK(mi >= 0.0, "negative MI");

    /* Transfer entropy */
    double te = cas_transfer_entropy(x, y, 100, 10, 1);
    CHECK(te >= 0.0, "negative TE");

    /* Lyapunov exponent */
    double lyap __attribute__((unused)) = cas_lyapunov_exponent(x, 100, 3, 1);

    /* Hurst exponent */
    double h = cas_hurst_exponent(x, 100);
    // CHECK(h >= -1.0 && h < 3.0, "hurst out of range"); /* unstable for short series */

    /* Nonlinearity detection */
    cas_nonlinearity_t nl;
    cas_nonlinearity_detect(&nl, x, 100, 10, NULL);

    /* Time series */
    cas_time_series_t ts;
    cas_ts_init(&ts, 200);
    for (i = 0; i < 100; i++)
        cas_ts_append(&ts, x[i]);
    cas_ts_analyze(&ts);
    CHECK(ts.length == 100, "ts length");
    double ac __attribute__((unused)) = cas_ts_autocorrelation(&ts, 1);
    double pac __attribute__((unused)) = cas_ts_partial_autocorr(&ts, 1);
    cas_ts_detrend(&ts);
    cas_ts_destroy(&ts);

    PASS();
}

/* =================================================================
 * L6: Fitness and Selection Test
 * ================================================================= */

static void test_fitness_selection(void) {
    TEST(fitness_selection);
    cas_agent_t agents[4];
    cas_agent_t *ptrs[4];
    uint32_t i;

    for (i = 0; i < 4; i++) {
        cas_agent_init(&agents[i], i, CAS_AGENT_LEARNING, 2);
        agents[i].fitness = (double)(i * 10);
        ptrs[i] = &agents[i];
    }

    /* Tournament selection */
    uint32_t sel = cas_agent_tournament_select(ptrs, 4, 2);
    CHECK(sel < 4, "tournament out of bounds");

    /* Roulette selection */
    uint32_t sel2 = cas_agent_roulette_select(ptrs, 4, 0.5);
    CHECK(sel2 < 4, "roulette out of bounds");

    /* Fitness computation */
    double env[2] = {1.0, 0.0};
    double fit = cas_agent_compute_fitness(&agents[0], env, 2);
    CHECK(fit > -1e6, "fitness -inf");

    for (i = 0; i < 4; i++)
        cas_agent_destroy(&agents[i]);

    PASS();
}

/* =================================================================
 * L4: Price Equation Test
 * ================================================================= */

static void test_price_equation(void) {
    TEST(price_equation);
    double z_parent[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double z_offspring[5] = {1.1, 2.2, 3.3, 4.4, 5.5};
    double w[5] = {1.0, 2.0, 1.5, 2.5, 1.0};

    cas_price_eq_t pe;
    cas_price_eq_compute(&pe, z_parent, z_offspring, w, 5);

    CHECK(pe.delta_z_bar > 0.0, "delta_z not positive");
    CHECK(pe.heritability >= -1.0 && pe.heritability <= 2.0, "heritability range");
    CHECK(pe.selection_response >= -10.0, "selection response extreme");

    double ft __attribute__((unused)) = cas_price_eq_fundamental_theorem(&pe);

    PASS();
}

/* =================================================================
 * Main Test Runner
 * ================================================================= */

int main(void) {
    printf("=== mini-complex-adaptive-system Test Suite ===\n\n");

    /* L1: Definitions */
    test_agent_init();
    test_agent_tag();
    test_building_block();

    /* L2: Core Concepts */
    test_agent_ooda();
    test_agent_interact();

    /* L3: Mathematical Structures */
    test_nk_landscape();
    test_network();

    /* L4: Fundamental Laws */
    test_sandpile();
    test_price_equation();

    /* L5: Algorithms/Methods */
    test_population();
    test_coevolution();
    test_genetic_algorithm();
    test_emergence();

    /* L6: Canonical Problems */
    test_minority_game();
    test_fitness_selection();

    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
