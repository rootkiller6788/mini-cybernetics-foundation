/**
 * @file bench_cas.c
 * @brief Performance benchmarks for CAS operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../include/cas_agent.h"
#include "../include/cas_landscape.h"
#include "../include/cas_network.h"
#include "../include/cas_dynamics.h"
#include "../include/cas_evolution.h"

static double get_time_ms(clock_t start, clock_t end) {
    return 1000.0 * (double)(end - start) / CLOCKS_PER_SEC;
}

int main(void) {
    printf("=== CAS Performance Benchmarks ===\n\n");
    clock_t t0, t1;

    /* Agent step benchmark */
    cas_agent_t agent;
    cas_agent_init(&agent, 0, CAS_AGENT_DELIBERATIVE, 8);
    double world[8] = {1,2,3,4,5,6,7,8};
    t0 = clock();
    int i;
    for (i = 0; i < 100000; i++)
        cas_agent_step(&agent, world, 8, 0.1);
    t1 = clock();
    printf("Agent step x100k: %.2f ms\n", get_time_ms(t0, t1));

    /* NK evaluation benchmark */
    cas_nk_landscape_t land;
    cas_nk_init(&land, 20, 4, 42);
    uint8_t g[4] = {0xAA, 0x55, 0x33, 0x0F};
    t0 = clock();
    for (i = 0; i < 100000; i++)
        cas_nk_evaluate(&land, g);
    t1 = clock();
    printf("NK eval x100k (N=20,K=4): %.2f ms\n", get_time_ms(t0, t1));

    /* Network operations */
    cas_network_t net;
    cas_net_init(&net, 100, CAS_NET_ERDOS_RENYI, 12345);
    cas_net_erdos_renyi(&net, 0.1, NULL);
    t0 = clock();
    cas_net_global_clustering(&net);
    t1 = clock();
    printf("Global clustering (n=100): %.2f ms\n", get_time_ms(t0, t1));

    t0 = clock();
    double apl = cas_net_avg_path_length(&net);
    t1 = clock();
    printf("Avg path length (n=100): %.2f ms (APL=%.2f)\n", get_time_ms(t0, t1), apl);

    /* GA generation benchmark */
    static double fit(const uint8_t *genes, uint32_t n, void *d) {
        (void)d; uint32_t c = 0, j;
        for (j = 0; j < n; j++) if ((genes[j/8] >> (j%8)) & 1) c++;
        return (double)c;
    }
    cas_genetic_algorithm_t ga;
    cas_ga_init(&ga, 100, 64, fit, NULL, 555);
    t0 = clock();
    for (i = 0; i < 100; i++)
        cas_ga_generation(&ga, NULL);
    t1 = clock();
    printf("GA 100 gen (pop=100,64bit): %.2f ms\n", get_time_ms(t0, t1));

    /* Population step benchmark */
    cas_population_t pop;
    cas_pop_init(&pop, 50, CAS_AGENT_LEARNING, 4, 999);
    double env[8] = {0.5, 0, 0, 0, 0, 0, 0, 0};
    t0 = clock();
    for (i = 0; i < 1000; i++)
        cas_pop_step(&pop, env, 8, NULL);
    t1 = clock();
    printf("Population step x1k (pop=50): %.2f ms\n", get_time_ms(t0, t1));

    printf("\n=== Benchmarks Complete ===\n");
    return 0;
}
