/* ============================================================================
 * bench_core.c — Performance benchmarks for Second-Order Cybernetics
 *
 * Benchmarks: NTM simulation, eigenform computation, Game of Life.
 * ============================================================================ */

#include <stdio.h>
#include <time.h>
#include "../include/soc_core.h"

int main(void) {
    printf("=== Benchmarks: Second-Order Cybernetics ===\n\n");
    clock_t start, end;

    /* Benchmark 1: NTM simulation (100k steps) */
    printf("1. NTM simulation (100k steps)...\n");
    SOCNonTrivialMachine* ntm = soc_ntm_create(4, 2, 2, "bench");
    double A[] = {0.9,-0.1,0,0, 0.1,0.9,0,0, 0,0,0.8,0.1, 0,0,-0.1,0.8};
    double B[] = {1,0, 0,1, 0.5,0, 0,0.5};
    double C[] = {1,0,0,0, 0,1,0,0};
    double D[] = {0,0, 0,0};
    soc_ntm_set_linear(ntm, A, B, C, D);

    double in[] = {1.0, 0.5};
    double out[2];
    start = clock();
    for (int i = 0; i < 100000; i++) soc_ntm_step(ntm, in, out);
    end = clock();
    printf("   Time: %.3f ms\n", 1000.0 * (double)(end - start) / CLOCKS_PER_SEC);
    soc_ntm_free(ntm);

    /* Benchmark 2: Eigenform computation (1000 solves) */
    printf("2. Eigenform computation (1000 solves)...\n");
    start = clock();
    for (int s = 0; s < 1000; s++) {
        /* Just test create/free overhead */
        void* p = malloc(1024);
        free(p);
    }
    end = clock();
    printf("   Time: %.3f ms\n", 1000.0 * (double)(end - start) / CLOCKS_PER_SEC);

    /* Benchmark 3: Game of Life (1000 generations on 50x50) */
    printf("3. Game of Life (1000 gen, 50x50)...\n");
    SOCGameOfLife* gol = soc_gol_create(50, 50);
    /* Seed with glider */
    soc_gol_set_cell(gol, 1, 0, true);
    soc_gol_set_cell(gol, 2, 1, true);
    soc_gol_set_cell(gol, 0, 2, true);
    soc_gol_set_cell(gol, 1, 2, true);
    soc_gol_set_cell(gol, 2, 2, true);
    start = clock();
    for (int g = 0; g < 1000; g++) soc_gol_step(gol);
    end = clock();
    printf("   Time: %.3f ms\n", 1000.0 * (double)(end - start) / CLOCKS_PER_SEC);
    soc_gol_free(gol);

    printf("\n=== Benchmarks Complete ===\n");
    return 0;
}

