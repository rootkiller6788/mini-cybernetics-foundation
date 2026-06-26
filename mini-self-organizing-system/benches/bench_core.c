#include "sos_core.h"
#include "sos_cellular.h"
#include "sos_turing.h"
#include <stdio.h>
#include <time.h>

static double time_diff_sec(clock_t start, clock_t end) {
    return (double)(end - start) / CLOCKS_PER_SEC;
}

int main(void) {
    printf("=== Self-Organizing Systems Benchmarks ===\n\n");

    /* Benchmark: CA evolution (100x100 grid, 1000 generations) */
    {
        CellularAutomaton* ca = ca_create(100, 100, "B3/S23");
        ca_randomize(ca, 0.3);
        clock_t start = clock();
        for (int i = 0; i < 1000; i++) ca_evolve(ca);
        clock_t end = clock();
        printf("CA 100x100 x1000 gens: %.3f s\n", time_diff_sec(start, end));
        ca_free(ca);
    }

    /* Benchmark: Sandpile (50x50, 10000 grains) */
    {
        SandpileModel* sp = sp_create(50, 50, 4);
        clock_t start = clock();
        sp_add_random_grains(sp, 10000);
        clock_t end = clock();
        printf("Sandpile 50x50 x10000 grains: %.3f s\n", time_diff_sec(start, end));
        sp_free(sp);
    }

    /* Benchmark: Turing pattern evolution (64x64, 500 steps) */
    {
        TuringSystem* ts = turing_create(TURING_SCHNAKENBERG, 64, 64, 10.0, 10.0);
        double params[2] = {0.3, 0.5};
        turing_set_params(ts, params, 2);
        turing_set_diffusion(ts, 0.1, 20.0);
        turing_random_init(ts, 1.3, 0.5, 0.01);
        clock_t start = clock();
        turing_evolve(ts, 500);
        clock_t end = clock();
        printf("Turing 64x64 x500 steps: %.3f s\n", time_diff_sec(start, end));
        turing_free(ts);
    }

    /* Benchmark: Floyd-Warshall (100 nodes) */
    {
        SOSystem* sys = sos_system_create("bench", 100, 0);
        sos_network_init(sys, 100);
        for (int i = 0; i < 100; i++)
            sos_network_add_edge(sys, i, (i+1)%100, 1.0);
        clock_t start = clock();
        double L = sos_average_path_length(sys);
        clock_t end = clock();
        printf("Floyd-Warshall 100 nodes: %.4f s (L=%.2f)\n",
               time_diff_sec(start, end), L);
        sos_system_free(sys);
    }

    printf("\n=== Benchmarks Complete ===\n");
    return 0;
}
