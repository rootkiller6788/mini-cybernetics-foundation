#include "vna_core.h"
#include "vna_cellular.h"
#include "vna_rule.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Conway's Game of Life — Canonical Implementation
 *
 * Covers: L1 (Game of Life definition), L2 (emergent complexity),
 *         L5 (simulation algorithm), L6 (canonical patterns),
 *         L7 (artificial life application)
 *
 * Conway's Game of Life (1970) is the most famous CA.
 * It is a 2D binary CA with Moore neighborhood and B3/S23 rule.
 * Conway proved it is Turing complete (via glider logic gates).
 *
 * Key canonical patterns:
 *   Still lifes: block, beehive, loaf, boat, ship
 *   Oscillators: blinker (p2), toad (p2), beacon (p2), pulsar (p3)
 *   Spaceships: glider, LWSS, MWSS, HWSS
 *   Guns: Gosper glider gun (p30)
 *   Puffers, breeders, replicators
 * ============================================================================ */

/* --- L1: Known Pattern Definitions --- */

static const uint8_t PATTERN_BLOCK[] = {
    1, 1,
    1, 1
};

static const uint8_t PATTERN_BEEHIVE[] = {
    0, 1, 1, 0,
    1, 0, 0, 1,
    0, 1, 1, 0
};

static const uint8_t PATTERN_LOAF[] = {
    0, 1, 1, 0,
    1, 0, 0, 1,
    0, 1, 0, 1,
    0, 0, 1, 0
};

static const uint8_t PATTERN_BOAT[] = {
    1, 1, 0,
    1, 0, 1,
    0, 1, 0
};

static const uint8_t PATTERN_BLINKER[] = {
    0, 0, 0, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 0, 0, 0
};

static const uint8_t PATTERN_TOAD[] = {
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 0,
    0, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};

static const uint8_t PATTERN_GLIDER[] = {
    0, 1, 0,
    0, 0, 1,
    1, 1, 1
};

static const uint8_t PATTERN_LWSS[] = { /* Lightweight Spaceship */
    0, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    1, 0, 0, 1, 0
};

static const uint8_t PATTERN_GOSPER_GLIDER_GUN[] = {
    /* Row 1 */  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
    /* Row 2 */  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,
    /* Row 3 */  0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    /* Row 4 */  0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    /* Row 5 */  1,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* Row 6 */  1,1,0,0,0,0,0,0,0,0,1,0,0,0,1,0,1,1,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,
    /* Row 7 */  0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
    /* Row 8 */  0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* Row 9 */  0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

#define GOSPER_W 36
#define GOSPER_H 9

/* --- L5: Named Pattern Creation --- */

VnaPattern* vna_gol_pattern_block(void) {
    return vna_pattern_create(2, 2, PATTERN_BLOCK, "Block");
}

VnaPattern* vna_gol_pattern_beehive(void) {
    return vna_pattern_create(4, 3, PATTERN_BEEHIVE, "Beehive");
}

VnaPattern* vna_gol_pattern_loaf(void) {
    return vna_pattern_create(4, 4, PATTERN_LOAF, "Loaf");
}

VnaPattern* vna_gol_pattern_boat(void) {
    return vna_pattern_create(3, 3, PATTERN_BOAT, "Boat");
}

VnaPattern* vna_gol_pattern_blinker(void) {
    return vna_pattern_create(5, 5, PATTERN_BLINKER, "Blinker (p2)");
}

VnaPattern* vna_gol_pattern_toad(void) {
    return vna_pattern_create(6, 6, PATTERN_TOAD, "Toad (p2)");
}

VnaPattern* vna_gol_pattern_glider(void) {
    VnaPattern* p = vna_pattern_create(3, 3, PATTERN_GLIDER, "Glider");
    if (p) { p->is_glider = true; p->period = 4; p->dx = 1; p->dy = -1; }
    return p;
}

VnaPattern* vna_gol_pattern_lwss(void) {
    VnaPattern* p = vna_pattern_create(5, 4, PATTERN_LWSS,
                                        "Lightweight Spaceship");
    if (p) { p->is_glider = true; p->period = 4; p->dx = 2; p->dy = 0; }
    return p;
}

VnaPattern* vna_gol_pattern_gosper_gun(void) {
    VnaPattern* p = vna_pattern_create(GOSPER_W, GOSPER_H,
                                        PATTERN_GOSPER_GLIDER_GUN,
                                        "Gosper Glider Gun (p30)");
    if (p) { p->period = 30; p->is_glider = false; }
    return p;
}

/* --- L5: Game of Life Specific Operations --- */

/* Initialize a lattice with the Game of Life rule */
VnaTransitionRule* vna_gol_create_rule(void) {
    return vna_rule_game_of_life();
}

VnaNeighborhood* vna_gol_create_neighborhood(void) {
    return vna_neighborhood_create(VNA_MOORE, 1, 2);
}

VnaLattice* vna_gol_create_lattice(int width, int height) {
    return vna_lattice_create(width, height, 2, VNA_LATTICE_2D_SQUARE);
}

/* --- L5: Pattern Insertion Helpers --- */

int vna_gol_insert_block(VnaLattice* lattice, int x, int y) {
    VnaPattern* p = vna_gol_pattern_block();
    if (!p) return -1;
    vna_pattern_insert(lattice, p, x, y);
    vna_pattern_free(p);
    return 0;
}

int vna_gol_insert_glider(VnaLattice* lattice, int x, int y) {
    VnaPattern* p = vna_gol_pattern_glider();
    if (!p) return -1;
    vna_pattern_insert(lattice, p, x, y);
    vna_pattern_free(p);
    return 0;
}

int vna_gol_insert_blinker(VnaLattice* lattice, int x, int y) {
    VnaPattern* p = vna_gol_pattern_blinker();
    if (!p) return -1;
    vna_pattern_insert(lattice, p, x, y);
    vna_pattern_free(p);
    return 0;
}

int vna_gol_insert_gosper_gun(VnaLattice* lattice, int x, int y) {
    VnaPattern* p = vna_gol_pattern_gosper_gun();
    if (!p) return -1;
    vna_pattern_insert(lattice, p, x, y);
    vna_pattern_free(p);
    return 0;
}

int vna_gol_insert_beehive(VnaLattice* lattice, int x, int y) {
    VnaPattern* p = vna_gol_pattern_beehive();
    if (!p) return -1;
    vna_pattern_insert(lattice, p, x, y);
    vna_pattern_free(p);
    return 0;
}

int vna_gol_insert_toad(VnaLattice* lattice, int x, int y) {
    VnaPattern* p = vna_gol_pattern_toad();
    if (!p) return -1;
    vna_pattern_insert(lattice, p, x, y);
    vna_pattern_free(p);
    return 0;
}

int vna_gol_insert_lwss(VnaLattice* lattice, int x, int y) {
    VnaPattern* p = vna_gol_pattern_lwss();
    if (!p) return -1;
    vna_pattern_insert(lattice, p, x, y);
    vna_pattern_free(p);
    return 0;
}

/* --- L5: Population Count --- */

int64_t vna_gol_population(VnaLattice* lattice) {
    if (!lattice) return 0;
    int64_t count = 0;
    int total = lattice->width * lattice->height;
    for (int i = 0; i < total; i++)
        if (lattice->cells[i]) count++;
    return count;
}

/* --- L5: Glider Detection --- */

int vna_gol_count_gliders(VnaLattice* lattice, VnaNeighborhood* nb,
                           VnaTransitionRule* rule) {
    /* Count gliders by comparing successive generations.
     * A glider moves by (1,1) every 4 generations. */
    if (!lattice || !rule) return 0;

    VnaLattice* copy = vna_lattice_create(lattice->width, lattice->height,
                                           2, VNA_LATTICE_2D_SQUARE);
    if (!copy) return 0;
    memcpy(copy->cells, lattice->cells, lattice->width * lattice->height);

    /* Detect moving patterns: compute cross-correlation */
    vna_lattice_evolve_n_steps(copy, nb, rule, 4);

    /* Look for 3x3 patterns that shifted by (1,-1) */
    int glider_count = 0;
    for (int y = 1; y < lattice->height - 2; y++) {
        for (int x = 1; x < lattice->width - 2; x++) {
            /* Check if a 3x3 region at (x,y) in original matches
             * a 3x3 region at (x+1, y-1) in evolved copy */
            int matches = 0;
            for (int dy = 0; dy < 3; dy++) {
                for (int dx = 0; dx < 3; dx++) {
                    uint8_t orig = vna_lattice_get_cell(lattice, x + dx, y + dy, 0);
                    uint8_t moved = vna_lattice_get_cell(copy, x + dx + 1,
                                                          y + dy - 1, 0);
                    if (orig == moved) matches++;
                }
            }
            if (matches >= 8) glider_count++; /* Allow 1 cell difference */
        }
    }

    vna_lattice_free(copy);
    return glider_count;
}

/* --- L6: Classification of Local Structures --- */

/* Determine the type of local structure at a given position.
 * Used for pattern recognition and analysis. */
typedef enum {
    GOL_EMPTY      = 0,  /* No live cells nearby */
    GOL_ISOLATED   = 1,  /* Single cell */
    GOL_BLOCK_LIKE = 2,  /* 2x2 block or part thereof */
    GOL_LINE       = 3,  /* Straight or diagonal line */
    GOL_CORNER     = 4,  /* L-shaped configuration */
    GOL_MOVING     = 5,  /* Likely part of a moving pattern */
    GOL_COMPLEX    = 6   /* Complex unclassifiable structure */
} GolLocalType;

GolLocalType vna_gol_local_classify(VnaLattice* lattice, int x, int y) {
    if (!lattice) return GOL_EMPTY;

    /* Examine 5x5 window centered at (x,y) */
    int live_count = 0;
    int neighbors[25];
    int idx = 0;
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            neighbors[idx] = vna_lattice_get_cell(lattice, x + dx, y + dy, 0);
            if (neighbors[idx]) live_count++;
            idx++;
        }
    }
    if (live_count == 0) return GOL_EMPTY;
    if (live_count == 1) return GOL_ISOLATED;

    /* Check for 2x2 block pattern */
    bool is_block = false;
    if (live_count == 4) {
        for (int dy = 0; dy < 1; dy++) {
            for (int dx = 0; dx < 1; dx++) {
                int c = (int)vna_lattice_get_cell(lattice, x+dx, y+dy, 0) +
                        (int)vna_lattice_get_cell(lattice, x+dx+1, y+dy, 0) +
                        (int)vna_lattice_get_cell(lattice, x+dx, y+dy+1, 0) +
                        (int)vna_lattice_get_cell(lattice, x+dx+1, y+dy+1, 0);
                if (c == 4) is_block = true;
            }
        }
    }
    if (is_block) return GOL_BLOCK_LIKE;

    /* Check for line-like structure (horizontal, vertical, diagonal) */
    int h_line = 0, v_line = 0;
    for (int dx = -2; dx <= 2; dx++)
        if (vna_lattice_get_cell(lattice, x + dx, y, 0)) h_line++;
    for (int dy = -2; dy <= 2; dy++)
        if (vna_lattice_get_cell(lattice, x, y + dy, 0)) v_line++;
    if (h_line >= 3 || v_line >= 3) return GOL_LINE;

    if (live_count > 8) return GOL_COMPLEX;
    return GOL_CORNER;
}

/* --- L7: Application — Artificial Life Metrics --- */

typedef struct {
    int64_t generation;
    int64_t population;
    double  spatial_entropy;
    int64_t still_lifes;
    int64_t oscillators;
    int64_t gliders;
    double  growth_rate;
} GolEcosystemMetrics;

GolEcosystemMetrics* vna_gol_ecosystem_analyze(VnaLattice* lattice,
                                                VnaNeighborhood* nb,
                                                VnaTransitionRule* rule) {
    GolEcosystemMetrics* m =
        (GolEcosystemMetrics*)calloc(1, sizeof(GolEcosystemMetrics));
    if (!m) return NULL;

    m->population = vna_gol_population(lattice);

    VnaStateHistogram* hist = vna_histogram_compute(lattice);
    if (hist) {
        m->spatial_entropy = hist->shannon_entropy;
        vna_histogram_free(hist);
    }

    m->still_lifes = vna_is_still_life(lattice, nb, rule) ? 1 : 0;
    m->gliders = vna_gol_count_gliders(lattice, nb, rule);

    return m;
}

void vna_gol_ecosystem_free(GolEcosystemMetrics* m) {
    free(m);
}

/* --- L5: Run Length Encoding for Sparse Lattices --- */

/* HashLife-like coordinate compression for large sparse lattices.
 * Stores only non-quiescent cells for efficiency.
 * This enables simulation of unbounded patterns. */
typedef struct {
    int     x, y;
    uint8_t state;
} GolLiveCell;

typedef struct {
    GolLiveCell* cells;
    int     count;
    int     capacity;
    int64_t generation;
} GolSparseLattice;

GolSparseLattice* vna_gol_sparse_create(void) {
    GolSparseLattice* sl =
        (GolSparseLattice*)calloc(1, sizeof(GolSparseLattice));
    if (!sl) return NULL;
    sl->capacity = 1024;
    sl->cells = (GolLiveCell*)malloc(sl->capacity * sizeof(GolLiveCell));
    if (!sl->cells) { free(sl); return NULL; }
    return sl;
}

void vna_gol_sparse_free(GolSparseLattice* sl) {
    if (!sl) return;
    free(sl->cells);
    free(sl);
}

void vna_gol_sparse_add_cell(GolSparseLattice* sl, int x, int y, uint8_t s) {
    if (!sl || !sl->cells) return;
    if (sl->count >= sl->capacity) {
        sl->capacity *= 2;
        sl->cells = (GolLiveCell*)realloc(sl->cells,
                                           sl->capacity * sizeof(GolLiveCell));
        if (!sl->cells) return;
    }
    sl->cells[sl->count].x = x;
    sl->cells[sl->count].y = y;
    sl->cells[sl->count].state = s;
    sl->count++;
}

void vna_gol_sparse_to_lattice(GolSparseLattice* sl, VnaLattice* lattice) {
    if (!sl || !lattice) return;
    vna_lattice_clear(lattice, 0);
    int half_w = lattice->width / 2;
    int half_h = lattice->height / 2;
    for (int i = 0; i < sl->count; i++) {
        int lx = sl->cells[i].x + half_w;
        int ly = sl->cells[i].y + half_h;
        if (lx >= 0 && lx < lattice->width && ly >= 0 && ly < lattice->height)
            vna_lattice_set_cell(lattice, lx, ly, 0, sl->cells[i].state);
    }
}

/* --- L7: Application — Pattern Database / Lexicon --- */

/* A simple pattern name lookup for known Game of Life configurations. */
typedef struct {
    const char* name;
    int width, height;
    const uint8_t* data;
    int period;
    const char* category; /* "still_life", "oscillator", "spaceship", "gun" */
} GolPatternEntry;

static const GolPatternEntry gol_lexicon[] = {
    {"Block",      2, 2, PATTERN_BLOCK,   1, "still_life"},
    {"Beehive",    4, 3, PATTERN_BEEHIVE, 1, "still_life"},
    {"Loaf",       4, 4, PATTERN_LOAF,    1, "still_life"},
    {"Boat",       3, 3, PATTERN_BOAT,    1, "still_life"},
    {"Blinker",    5, 5, PATTERN_BLINKER, 2, "oscillator"},
    {"Toad",       6, 6, PATTERN_TOAD,    2, "oscillator"},
    {"Glider",     3, 3, PATTERN_GLIDER,  4, "spaceship"},
    {"LWSS",       5, 4, PATTERN_LWSS,    4, "spaceship"},
    {"Gosper Gun", 36,9, PATTERN_GOSPER_GLIDER_GUN, 30, "gun"},
    {NULL, 0, 0, NULL, 0, NULL}
};

int vna_gol_lexicon_size(void) {
    int n = 0;
    while (gol_lexicon[n].name) n++;
    return n;
}

const char* vna_gol_lexicon_name(int idx) {
    if (idx < 0 || idx >= vna_gol_lexicon_size()) return NULL;
    return gol_lexicon[idx].name;
}

VnaPattern* vna_gol_lexicon_pattern(int idx) {
    if (idx < 0 || idx >= vna_gol_lexicon_size()) return NULL;
    const GolPatternEntry* e = &gol_lexicon[idx];
    VnaPattern* p = vna_pattern_create(e->width, e->height, e->data, e->name);
    if (p) {
        p->period = e->period;
        if (e->period > 1 && strcmp(e->category, "spaceship") == 0)
            p->is_glider = true;
    }
    return p;
}

const char* vna_gol_lexicon_category(int idx) {
    if (idx < 0 || idx >= vna_gol_lexicon_size()) return NULL;
    return gol_lexicon[idx].category;
}

/* --- L5: Boundary Condition Experiment --- */

void vna_gol_run_experiment(VnaLattice* lattice, VnaNeighborhood* nb,
                             VnaTransitionRule* rule, int steps,
                             int64_t* population_history,
                             double* entropy_history) {
    if (!lattice || !population_history || !entropy_history) return;

    for (int t = 0; t < steps; t++) {
        population_history[t] = vna_gol_population(lattice);

        VnaStateHistogram* hist = vna_histogram_compute(lattice);
        entropy_history[t] = hist ? hist->shannon_entropy : 0.0;
        vna_histogram_free(hist);

        vna_lattice_evolve(lattice, nb, rule);
    }
}
