#include "sos_cellular.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    srand((unsigned)time(NULL));
    printf("=== Conway's Game of Life — Self-Organization Example ===\n\n");
    printf("Conway's Game of Life (1970) demonstrates how simple local rules\n");
    printf("can produce complex self-organizing patterns. The universe is a\n");
    printf("grid of cells where each cell's fate depends only on its 8 neighbors.\n\n");
    printf("Rule B3/S23:\n");
    printf("  - A dead cell with exactly 3 live neighbors becomes alive (birth)\n");
    printf("  - A live cell with 2 or 3 live neighbors stays alive (survival)\n");
    printf("  - Otherwise the cell dies or stays dead\n\n");

    int width = 30, height = 20;
    CellularAutomaton* ca = ca_create(width, height, "B3/S23");

    /* Initialize with random pattern */
    ca_randomize(ca, 0.3);

    printf("Initial random configuration (%d x %d):\n", width, height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++)
            printf("%c", ca_get_cell(ca, x, y) == CELL_ALIVE ? 'O' : '.');
        printf("\n");
    }

    /* Evolve and find still lifes / oscillators */
    int live_initial = ca_live_count(ca);
    printf("\nInitial live cells: %d\n", live_initial);

    int max_generations = 100;
    int stable_at = -1;
    int prev_live = live_initial;

    for (int g = 1; g <= max_generations; g++) {
        ca_evolve(ca);
        int live = ca_live_count(ca);
        double density = ca_density(ca);

        if (g <= 10 || g % 10 == 0)
            printf("Generation %3d: %4d live cells (density=%.3f)\n",
                   g, live, density);

        if (live == prev_live && stable_at < 0)
            stable_at = g;
        prev_live = live;
    }

    if (stable_at > 0)
        printf("\nSystem stabilized at generation %d (still life or oscillator)\n", stable_at);
    else
        printf("\nSystem continues to evolve (no stabilization in %d gens)\n", max_generations);

    /* Final configuration */
    printf("\nFinal configuration:\n");
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++)
            printf("%c", ca_get_cell(ca, x, y) == CELL_ALIVE ? 'O' : '.');
        printf("\n");
    }

    int still_lives = ca_detect_still_life(ca);
    printf("\nStill-life cells (unchanged in 1 generation): %d\n", still_lives);

    double lambda = ca_lambda_parameter(ca);
    printf("Langton's lambda parameter: %.3f\n", lambda);

    CAClass wc = ca_classify_wolfram(ca, 50);
    const char* class_names[] = {"I (fixed)", "II (periodic)", "III (chaotic)", "IV (complex)"};
    printf("Wolfram classification: Class %s\n", class_names[wc]);

    printf("\nKey insight: Local rules produce global order.\n");
    printf("The Game of Life illustrates emergence: complex macro patterns\n");
    printf("(gliders, blinkers, glider guns) arise from simple micro rules.\n");

    ca_free(ca);
    return 0;
}
