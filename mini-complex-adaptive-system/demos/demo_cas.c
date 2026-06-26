/**
 * @file demo_cas.c
 * @brief Comprehensive CAS Demonstration
 *
 * Demonstrates: agent lifecycle, NK landscape, network dynamics,
 * self-organized criticality, genetic algorithms, emergence metrics.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/cas_agent.h"
#include "../include/cas_landscape.h"
#include "../include/cas_network.h"
#include "../include/cas_dynamics.h"
#include "../include/cas_evolution.h"
#include "../include/cas_emergence.h"

static double demo_fit_func(const uint8_t *g, uint32_t n, void *d) {
    (void)d;
    uint32_t c = 0, i;
    for (i = 0; i < n; i++)
        if ((g[i / 8] >> (i % 8)) & 1) c++;
    return (double)c;
}

int main(void) {
    printf("========================================\n");
    printf("  Complex Adaptive Systems Demo\n");
    printf("========================================\n\n");

    int i;

    /* 1. Agent Lifecycle */
    printf("[1/6] Agent OODA Cycle\n-----------------------\n");
    cas_agent_t agent;
    cas_agent_init(&agent, 1, CAS_AGENT_DELIBERATIVE, 4);
    agent.sensors[0].source_id = 0; agent.sensors[0].active = true;
    agent.sensors[0].noise_std = 0.01; agent.sensors[0].sensitivity = 1.0;
    agent.sensors[0].range_min = -10; agent.sensors[0].range_max = 10;
    agent.num_sensors = 1;
    agent.effectors[0].target_id = 0; agent.effectors[0].active = true;
    agent.effectors[0].saturation_min = -1; agent.effectors[0].saturation_max = 1;
    agent.effectors[0].energy_cost = 0.1; agent.num_effectors = 1;

    double world[4] = {1.0, 0.5, -0.2, 0.8};
    for (i = 0; i < 20; i++)
        cas_agent_step(&agent, world, 4, 0.2 + i * 0.01);
    printf("Agent %u: fitness=%.3f, energy=%.1f, age=%u, avg_reward=%.3f\n",
           agent.id, agent.fitness, agent.energy, agent.age, agent.average_reward);
    printf("Prediction error: %.4f (moving avg: %.4f)\n\n",
           agent.model.prediction_error, agent.model.moving_error);

    /* 2. NK Landscape */
    printf("[2/6] NK Landscape Navigation\n-----------------------------\n");
    cas_nk_landscape_t land;
    cas_nk_init(&land, 12, 3, 42);
    cas_landscape_point_t pt;
    cas_nk_random_genotype(&land, pt.genotype, NULL);
    cas_nk_evaluate_full(&land, &pt);
    printf("Start fitness: %.4f\n", pt.fitness);
    int steps = 0;
    while (!pt.is_local_optimum && steps < 50) {
        cas_nk_adaptive_walk_step(&land, &pt);
        steps++;
    }
    printf("Local optimum in %d steps: fitness=%.4f\n", steps, pt.fitness);
    printf("Landscape: N=%u K=%u rho(1)=%.3f\n\n",
           land.n, land.k, cas_nk_fitness_correlation(land.n, land.k, 1));

    /* 3. Network Dynamics */
    printf("[3/6] Interaction Networks\n--------------------------\n");
    cas_network_t net;
    cas_net_init(&net, 30, CAS_NET_WATTS_STROGATZ, 12345);
    cas_net_watts_strogatz(&net, 4, 0.1, NULL);
    cas_net_global_clustering(&net);
    double apl = cas_net_avg_path_length(&net);
    printf("Watts-Strogatz (n=30,k=4,beta=0.1): CC=%.3f, APL=%.2f\n",
           net.global_clustering, apl);
    cas_net_louvain_communities(&net);
    cas_net_pagerank(&net, 0.85, 50);
    printf("Top PageRank nodes: ");
    int j;
    for (j = 0; j < 5 && j < (int)net.num_nodes; j++)
        printf("n%u(%.3f) ", j, net.node_attr[j].pagerank);
    printf("\n\n");

    /* 4. Self-Organized Criticality */
    printf("[4/6] Sandpile SOC\n------------------\n");
    cas_sandpile_t sp;
    cas_sandpile_init(&sp, 32);
    cas_sandpile_run(&sp, 5000, NULL);
    double tau = cas_sandpile_powerlaw_exponent(&sp);
    printf("Grid 32x32: 5000 drops, %llu topplings, tau=%.3f\n",
           (unsigned long long)sp.total_topplings, tau);

    /* 5. Genetic Algorithm */
    printf("[5/6] Genetic Algorithm (One-Max)\n--------------------------------\n");
    cas_genetic_algorithm_t ga;
    cas_ga_init(&ga, 50, 32, demo_fit_func, NULL, 9999);
    int g;
    for (g = 0; g < 50; g++) cas_ga_generation(&ga, NULL);
    double bf; uint8_t bg[4];
    cas_ga_get_best(&ga, bg, &bf);
    printf("After 50 generations: best=%.0f/%u (avg=%.2f)\n\n",
           bf, ga.genome_bits, ga.avg_fitness);

    /* 6. Emergence Metrics */
    printf("[6/6] Emergence Detection\n------------------------\n");
    double x[200], y[200];
    for (i = 0; i < 200; i++) {
        x[i] = sin(i * 0.05) + 0.1 * ((double)rand() / RAND_MAX - 0.5);
        y[i] = cos(i * 0.05) + 0.1 * ((double)rand() / RAND_MAX - 0.5);
    }
    double mi = cas_mutual_information(x, y, 200, 20);
    double te = cas_transfer_entropy(x, y, 200, 20, 1);
    double hh = cas_hurst_exponent(x, 200);

    printf("Mutual Information: %.4f\n", mi);
    printf("Transfer Entropy: %.4f\n", te);
    printf("Hurst Exponent: %.3f\n\n", hh);

    printf("========================================\n");
    printf("  CAS Demo Complete\n");
    printf("========================================\n");

    return 0;
}
