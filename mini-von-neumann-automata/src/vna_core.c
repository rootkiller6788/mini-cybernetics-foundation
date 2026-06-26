#include "vna_core.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define PI  3.14159265358979323846
#define M_E 2.71828182845904523536

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

static uint8_t safe_get(VnaLattice* lattice, int x, int y, int z) {
    if (!lattice || !lattice->cells) return 0;
    int w = lattice->width;
    int h = lattice->height;
    if (x < 0 || x >= w || y < 0 || y >= h) {
        /* Boundary condition handling */
        if (lattice->bc_x == VNA_BC_PERIODIC) {
            x = ((x % w) + w) % w;
            y = ((y % h) + h) % h;
        } else if (lattice->bc_x == VNA_BC_REFLECTIVE) {
            if (x < 0) x = -x - 1;
            if (x >= w) x = 2 * w - x - 1;
            if (y < 0) y = -y - 1;
            if (y >= h) y = 2 * h - y - 1;
            if (x < 0) x = 0;
            if (x >= w) x = w - 1;
            if (y < 0) y = 0;
            if (y >= h) y = h - 1;
        } else if (lattice->bc_x == VNA_BC_FIXED) {
            if (x < 0 || x >= w || y < 0 || y >= h) return 0;
        } else if (lattice->bc_x == VNA_BC_ADIABATIC) {
            if (x < 0) x = 0;
            if (x >= w) x = w - 1;
            if (y < 0) y = 0;
            if (y >= h) y = h - 1;
        } else {
            return 0;
        }
    }
    return lattice->cells[y * w + x];
}

static int idx_3d(VnaLattice* lattice, int x, int y, int z) {
    return z * lattice->width * lattice->height + y * lattice->width + x;
}

static double urand_seeded(unsigned int* seed) {
    *seed = (*seed * 1103515245 + 12345) & 0x7fffffff;
    return (double)(*seed) / (double)0x7fffffff;
}

/* ============================================================================
 * Lattice Management
 * ============================================================================ */

VnaLattice* vna_lattice_create(int width, int height, int num_states,
                                VnaLatticeTopology topology) {
    if (width <= 0 || height <= 0 || num_states < 2 || num_states > 256)
        return NULL;
    VnaLattice* lattice = (VnaLattice*)calloc(1, sizeof(VnaLattice));
    if (!lattice) return NULL;
    lattice->width = width;
    lattice->height = height;
    lattice->depth = 1;
    lattice->num_states = num_states;
    lattice->topology = topology;
    lattice->bc_x = VNA_BC_PERIODIC;
    lattice->bc_y = VNA_BC_PERIODIC;
    lattice->bc_z = VNA_BC_PERIODIC;
    lattice->update_mode = VNA_UPDATE_SYNC;
    lattice->generation = 0;

    int total_cells = width * height;
    lattice->cells = (uint8_t*)calloc(total_cells, sizeof(uint8_t));
    lattice->next_cells = (uint8_t*)calloc(total_cells, sizeof(uint8_t));
    lattice->ghost_layer = (uint8_t*)calloc((width+2)*(height+2), sizeof(uint8_t));
    lattice->ghost_width = 1;

    if (!lattice->cells || !lattice->next_cells || !lattice->ghost_layer) {
        vna_lattice_free(lattice);
        return NULL;
    }
    return lattice;
}

void vna_lattice_free(VnaLattice* lattice) {
    if (!lattice) return;
    free(lattice->cells);
    free(lattice->next_cells);
    free(lattice->ghost_layer);
    free(lattice);
}

void vna_lattice_set_boundary(VnaLattice* lattice, VnaBoundaryCondition bc_x,
                               VnaBoundaryCondition bc_y, VnaBoundaryCondition bc_z) {
    if (!lattice) return;
    lattice->bc_x = bc_x;
    lattice->bc_y = bc_y;
    lattice->bc_z = bc_z;
}

void vna_lattice_set_update_mode(VnaLattice* lattice, VnaUpdateMode mode) {
    if (!lattice) return;
    lattice->update_mode = mode;
}

uint8_t vna_lattice_get_cell(VnaLattice* lattice, int x, int y, int z) {
    if (!lattice || !lattice->cells) return 0;
    if (x < 0 || x >= lattice->width || y < 0 || y >= lattice->height) return 0;
    return lattice->cells[y * lattice->width + x];
}

void vna_lattice_set_cell(VnaLattice* lattice, int x, int y, int z, uint8_t state) {
    if (!lattice || !lattice->cells) return;
    if (x < 0 || x >= lattice->width || y < 0 || y >= lattice->height) return;
    if (state >= lattice->num_states) state = state % lattice->num_states;
    lattice->cells[y * lattice->width + x] = state;
}

void vna_lattice_clear(VnaLattice* lattice, uint8_t fill_state) {
    if (!lattice || !lattice->cells) return;
    int total = lattice->width * lattice->height;
    memset(lattice->cells, (fill_state < lattice->num_states) ? fill_state : 0,
           total * sizeof(uint8_t));
    memset(lattice->next_cells, 0, total * sizeof(uint8_t));
    lattice->generation = 0;
}

void vna_lattice_randomize(VnaLattice* lattice, double density, unsigned int seed) {
    vna_lattice_randomize_seeded(lattice, density, seed);
}

void vna_lattice_randomize_seeded(VnaLattice* lattice, double density,
                                   unsigned int seed) {
    if (!lattice || !lattice->cells) return;
    if (density < 0.0) density = 0.0;
    if (density > 1.0) density = 1.0;
    int total = lattice->width * lattice->height;
    unsigned int local_seed = seed ? seed : (unsigned int)time(NULL);
    for (int i = 0; i < total; i++) {
        if (lattice->num_states == 2) {
            lattice->cells[i] = (urand_seeded(&local_seed) < density) ? 1 : 0;
        } else {
            double r = urand_seeded(&local_seed);
            if (r < density) {
                lattice->cells[i] = (uint8_t)(1 + (int)(urand_seeded(&local_seed) *
                                         (lattice->num_states - 1)));
            } else {
                lattice->cells[i] = 0;
            }
        }
    }
    lattice->generation = 0;
}

/* ============================================================================
 * Lattice I/O
 * ============================================================================ */

void vna_lattice_print_ascii(VnaLattice* lattice, int x0, int y0,
                              int view_w, int view_h) {
    if (!lattice || !lattice->cells) return;
    if (view_w <= 0) view_w = lattice->width;
    if (view_h <= 0) view_h = lattice->height;

    /* For binary CA, use █ and · */
    if (lattice->num_states == 2) {
        for (int dy = 0; dy < view_h; dy++) {
            for (int dx = 0; dx < view_w; dx++) {
                int x = x0 + dx;
                int y = y0 + dy;
                uint8_t s = vna_lattice_get_cell(lattice, x, y, 0);
                putchar(s ? '#' : '.');
            }
            putchar('\n');
        }
    } else {
        /* For multi-state CA, print as hex digits (up to 16 states) */
        for (int dy = 0; dy < view_h; dy++) {
            for (int dx = 0; dx < view_w; dx++) {
                int x = x0 + dx;
                int y = y0 + dy;
                uint8_t s = vna_lattice_get_cell(lattice, x, y, 0);
                if (s < 10) putchar('0' + s);
                else if (s < 16) putchar('A' + (s - 10));
                else putchar('?');
            }
            putchar('\n');
        }
    }
}

void vna_lattice_export_pgm(VnaLattice* lattice, const char* filename) {
    if (!lattice || !filename) return;
    FILE* f = fopen(filename, "wb");
    if (!f) return;
    fprintf(f, "P5\n%d %d\n%d\n", lattice->width, lattice->height,
            lattice->num_states > 1 ? 255 : 1);
    int total = lattice->width * lattice->height;
    for (int i = 0; i < total; i++) {
        uint8_t v = (uint8_t)(lattice->cells[i] * 255 / (lattice->num_states - 1));
        fputc(v, f);
    }
    fclose(f);
}

void vna_lattice_import_pgm(VnaLattice* lattice, const char* filename,
                             int* offset_x, int* offset_y) {
    if (!lattice || !filename) return;
    FILE* f = fopen(filename, "rb");
    if (!f) return;
    char magic[3] = {0};
    int w = 0, h = 0, maxval = 0;
    if (fscanf(f, "%2s %d %d %d ", magic, &w, &h, &maxval) != 4) {
        fclose(f);
        return;
    }
    if (magic[0] != 'P' || magic[1] != '5') { fclose(f); return; }
    for (int y = 0; y < h && y < lattice->height; y++) {
        for (int x = 0; x < w && x < lattice->width; x++) {
            int val = fgetc(f);
            if (val == EOF) break;
            uint8_t state = (uint8_t)(val * (lattice->num_states - 1) / maxval);
            vna_lattice_set_cell(lattice, x + (offset_x ? *offset_x : 0),
                                 y + (offset_y ? *offset_y : 0), 0, state);
        }
    }
    fclose(f);
}

/* ============================================================================
 * Neighborhood
 * ============================================================================ */

VnaNeighborhood* vna_neighborhood_create(VnaNeighborhoodType type, int radius,
                                          int num_states) {
    VnaNeighborhood* nb = (VnaNeighborhood*)calloc(1, sizeof(VnaNeighborhood));
    if (!nb) return NULL;
    nb->type = type;
    nb->radius = radius;
    nb->include_self = true;

    switch (type) {
        case VNA_VON_NEUMANN: {
            nb->num_neighbors = 2 * radius * (radius + 1) + 1;
            nb->offset_dx = (int*)malloc(nb->num_neighbors * sizeof(int));
            nb->offset_dy = (int*)malloc(nb->num_neighbors * sizeof(int));
            int idx = 0;
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    if (abs(dx) + abs(dy) <= radius) {
                        nb->offset_dx[idx] = dx;
                        nb->offset_dy[idx] = dy;
                        idx++;
                    }
                }
            }
            nb->num_neighbors = idx;
            break;
        }
        case VNA_MOORE: {
            int side = 2 * radius + 1;
            nb->num_neighbors = side * side;
            nb->offset_dx = (int*)malloc(nb->num_neighbors * sizeof(int));
            nb->offset_dy = (int*)malloc(nb->num_neighbors * sizeof(int));
            int idx = 0;
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    nb->offset_dx[idx] = dx;
                    nb->offset_dy[idx] = dy;
                    idx++;
                }
            }
            break;
        }
        case VNA_ONE_DIMENSIONAL: {
            nb->num_neighbors = 2 * radius + 1;
            nb->offset_dx = (int*)malloc(nb->num_neighbors * sizeof(int));
            nb->offset_dy = (int*)calloc(nb->num_neighbors, sizeof(int));
            for (int i = 0; i < nb->num_neighbors; i++) {
                nb->offset_dx[i] = i - radius;
                nb->offset_dy[i] = 0;
            }
            break;
        }
        default:
            /* For other types, allocate default Moore r=1 */
            nb->num_neighbors = 9;
            nb->offset_dx = (int*)malloc(9 * sizeof(int));
            nb->offset_dy = (int*)malloc(9 * sizeof(int));
            int idx = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    nb->offset_dx[idx] = dx;
                    nb->offset_dy[idx] = dy;
                    idx++;
                }
            break;
    }
    return nb;
}

void vna_neighborhood_free(VnaNeighborhood* nb) {
    if (!nb) return;
    free(nb->offset_dx);
    free(nb->offset_dy);
    free(nb->offset_dz);
    free(nb);
}

int vna_neighborhood_get_num_neighbors(VnaNeighborhoodType type, int radius) {
    switch (type) {
        case VNA_VON_NEUMANN: return 2 * radius * (radius + 1) + 1;
        case VNA_MOORE:      return (2 * radius + 1) * (2 * radius + 1);
        case VNA_ONE_DIMENSIONAL: return 2 * radius + 1;
        case VNA_HEXAGONAL:  return 3 * radius * (radius + 1) + 1;
        default: return 9;
    }
}

/* ============================================================================
 * Transition Rules
 * ============================================================================ */

static int64_t ipow(int base, int exp) {
    int64_t result = 1;
    for (int i = 0; i < exp; i++) result *= base;
    return result;
}

VnaTransitionRule* vna_rule_create(int num_states, int num_neighbors) {
    VnaTransitionRule* rule = (VnaTransitionRule*)calloc(1, sizeof(VnaTransitionRule));
    if (!rule) return NULL;
    rule->num_states = num_states;
    rule->num_neighbors = num_neighbors;
    int64_t table_size = ipow(num_states, num_neighbors);
    rule->rule_table = (uint8_t*)calloc(table_size, sizeof(uint8_t));
    if (!rule->rule_table) {
        free(rule);
        return NULL;
    }
    return rule;
}

void vna_rule_free(VnaTransitionRule* rule) {
    if (!rule) return;
    free(rule->rule_table);
    free(rule->name);
    free(rule->description);
    free(rule);
}

/* Map a neighbor state tuple to a flat index using mixed-radix encoding.
 * index = Σ_{i=0}^{n-1} s_i * k^i  where k = num_states. */
static int64_t tuple_to_index(const uint8_t* states, int num_states,
                               int num_neighbors) {
    int64_t index = 0;
    int64_t multiplier = 1;
    for (int i = 0; i < num_neighbors; i++) {
        index += (int64_t)states[i] * multiplier;
        multiplier *= num_states;
    }
    return index;
}

/* Reverse: map a flat index to a neighbor state tuple. */
static void index_to_tuple(int64_t index, int num_states, int num_neighbors,
                            uint8_t* states) {
    for (int i = 0; i < num_neighbors; i++) {
        states[i] = (uint8_t)(index % num_states);
        index /= num_states;
    }
}

void vna_rule_set_entry(VnaTransitionRule* rule, const uint8_t* neighbor_states,
                         uint8_t output) {
    if (!rule || !rule->rule_table) return;
    int64_t idx = tuple_to_index(neighbor_states, rule->num_states,
                                  rule->num_neighbors);
    if (idx < ipow(rule->num_states, rule->num_neighbors))
        rule->rule_table[idx] = output;
}

uint8_t vna_rule_lookup(VnaTransitionRule* rule, const uint8_t* neighbor_states) {
    if (!rule || !rule->rule_table) return 0;
    int64_t idx = tuple_to_index(neighbor_states, rule->num_states,
                                  rule->num_neighbors);
    return rule->rule_table[idx];
}

void vna_rule_from_wolfram(VnaTransitionRule* rule, int wolfram_code) {
    if (!rule || rule->num_states != 2) return;
    rule->wolfram_code = wolfram_code;

    /* Wolfram code convention:
     * For 1D binary CA with radius r:
     *   Bit index = Σ_{j=0}^{2r} n_j * 2^{2r-j}
     * where (n_0, ..., n_{2r}) is the neighborhood tuple (left to right).
     *
     * For r=1: index = left*4 + center*2 + right*1.
     * The internal tuple_to_index uses: index = left*1 + center*k + right*k^2.
     * We need to translate between these two conventions. */

    int num_nb = rule->num_neighbors;
    uint8_t nb[8]; /* Max neighbors for reasonable CA */

    int64_t table_size = 1;
    for (int i = 0; i < num_nb; i++) table_size *= 2;

    for (int64_t i = 0; i < table_size && i < 256; i++) {
        /* Decode i as Wolfram index: i = Σ n_j * 2^{num_nb-1-j} */
        int tmp = (int)i;
        for (int j = num_nb - 1; j >= 0; j--) {
            nb[j] = (uint8_t)(tmp & 1);
            tmp >>= 1;
        }
        /* Map to internal index */
        int64_t internal_idx = tuple_to_index(nb, 2, num_nb);
        int bit = (wolfram_code >> (int)i) & 1;
        rule->rule_table[internal_idx] = (uint8_t)bit;
    }
}

int vna_rule_to_wolfram(VnaTransitionRule* rule) {
    if (!rule || rule->num_states != 2) return -1;
    int num_nb = rule->num_neighbors;

    int code = 0;
    int64_t table_size = 1;
    for (int i = 0; i < num_nb; i++) table_size *= 2;

    uint8_t nb[8];
    for (int i = 0; i < (int)table_size && i < 256; i++) {
        /* Build Wolfram convention neighbor tuple */
        int tmp = i;
        for (int j = num_nb - 1; j >= 0; j--) {
            nb[j] = (uint8_t)(tmp & 1);
            tmp >>= 1;
        }
        int64_t internal_idx = tuple_to_index(nb, 2, num_nb);
        if (rule->rule_table[internal_idx]) code |= (1 << i);
    }
    return code;
}

void vna_rule_set_totalistic(VnaTransitionRule* rule,
                              const uint8_t* sum_to_state, int max_sum) {
    if (!rule || !rule->rule_table) return;
    rule->is_totalistic = true;
    int64_t table_size = ipow(rule->num_states, rule->num_neighbors);
    for (int64_t i = 0; i < table_size; i++) {
        uint8_t* states = (uint8_t*)malloc(rule->num_neighbors);
        index_to_tuple(i, rule->num_states, rule->num_neighbors, states);
        int sum = 0;
        for (int j = 0; j < rule->num_neighbors; j++) sum += states[j];
        if (sum > max_sum) sum = max_sum;
        rule->rule_table[i] = sum_to_state[sum];
        free(states);
    }
}

VnaTransitionRule* vna_rule_compose(VnaTransitionRule* r1,
                                     VnaTransitionRule* r2) {
    if (!r1 || !r2) return NULL;
    if (r1->num_states != r2->num_states) return NULL;
    if (r1->num_neighbors != r2->num_neighbors) return NULL;

    VnaTransitionRule* result = vna_rule_create(r1->num_states,
                                                  r1->num_neighbors);
    if (!result) return NULL;

    int64_t table_size = ipow(r1->num_states, r1->num_neighbors);
    /* Composition: apply r1, then r2 to the same neighborhood.
     * Note: this is an approximation; true composition requires
     * computing the global map. */
    uint8_t* intermediate = (uint8_t*)malloc(r1->num_neighbors);
    for (int64_t i = 0; i < table_size; i++) {
        uint8_t* orig = (uint8_t*)malloc(r1->num_neighbors);
        index_to_tuple(i, r1->num_states, r1->num_neighbors, orig);
        /* Apply r1 to get intermediate neighbor states */
        for (int j = 0; j < r1->num_neighbors; j++)
            intermediate[j] = r1->rule_table[tuple_to_index(
                /* This is simplified — real composition needs spatial awareness */
                orig, r1->num_states, r1->num_neighbors)];
        result->rule_table[i] = r2->rule_table[tuple_to_index(
            intermediate, r2->num_states, r2->num_neighbors)];
        free(orig);
    }
    free(intermediate);
    return result;
}

/* ============================================================================
 * CA Evolution — The core simulation engine
 * ============================================================================ */

int vna_lattice_evolve(VnaLattice* lattice, VnaNeighborhood* nb,
                        VnaTransitionRule* rule) {
    if (!lattice || !nb || !rule) return -1;
    if (!lattice->cells || !lattice->next_cells) return -1;

    int w = lattice->width;
    int h = lattice->height;
    uint8_t* neighbor_buf = (uint8_t*)malloc(nb->num_neighbors);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            /* Collect neighbor states */
            for (int ni = 0; ni < nb->num_neighbors; ni++) {
                int nx = x + nb->offset_dx[ni];
                int ny = y + nb->offset_dy[ni];
                neighbor_buf[ni] = safe_get(lattice, nx, ny, 0);
            }
            /* Look up transition rule */
            int64_t idx = tuple_to_index(neighbor_buf, rule->num_states,
                                          nb->num_neighbors);
            lattice->next_cells[y * w + x] = rule->rule_table[idx];
        }
    }

    /* Swap buffers */
    int total = w * h;
    memcpy(lattice->cells, lattice->next_cells, total * sizeof(uint8_t));
    lattice->generation++;
    free(neighbor_buf);
    return 0;
}

int vna_lattice_evolve_n_steps(VnaLattice* lattice, VnaNeighborhood* nb,
                                VnaTransitionRule* rule, int n) {
    for (int i = 0; i < n; i++) {
        int ret = vna_lattice_evolve(lattice, nb, rule);
        if (ret != 0) return ret;
    }
    return 0;
}

void vna_lattice_evolve_async(VnaLattice* lattice, VnaNeighborhood* nb,
                               VnaTransitionRule* rule, double update_prob) {
    if (!lattice || !nb || !rule) return;
    int total = lattice->width * lattice->height;
    uint8_t* neighbor_buf = (uint8_t*)malloc(nb->num_neighbors);
    unsigned int seed = (unsigned int)(lattice->generation + 1);

    for (int cell_idx = 0; cell_idx < total; cell_idx++) {
        if (urand_seeded(&seed) > update_prob) continue;
        int x = (int)(urand_seeded(&seed) * lattice->width);
        int y = (int)(urand_seeded(&seed) * lattice->height);
        if (x >= lattice->width) x = lattice->width - 1;
        if (y >= lattice->height) y = lattice->height - 1;

        for (int ni = 0; ni < nb->num_neighbors; ni++) {
            int nx = x + nb->offset_dx[ni];
            int ny = y + nb->offset_dy[ni];
            neighbor_buf[ni] = safe_get(lattice, nx, ny, 0);
        }
        int64_t idx = tuple_to_index(neighbor_buf, rule->num_states,
                                      nb->num_neighbors);
        lattice->cells[y * lattice->width + x] = rule->rule_table[idx];
    }
    lattice->generation++;
    free(neighbor_buf);
}

/* ============================================================================
 * Pattern Operations
 * ============================================================================ */

VnaPattern* vna_pattern_create(int width, int height, const uint8_t* data,
                                const char* name) {
    if (width <= 0 || height <= 0) return NULL;
    VnaPattern* pattern = (VnaPattern*)calloc(1, sizeof(VnaPattern));
    if (!pattern) return NULL;
    pattern->width = width;
    pattern->height = height;
    pattern->data = (uint8_t*)malloc(width * height);
    if (!pattern->data) { free(pattern); return NULL; }
    if (data) memcpy(pattern->data, data, width * height);
    else memset(pattern->data, 0, width * height);
    pattern->name = name ? strdup(name) : strdup("unnamed");
    return pattern;
}

void vna_pattern_free(VnaPattern* pattern) {
    if (!pattern) return;
    free(pattern->data);
    free(pattern->name);
    free(pattern->origin);
    free(pattern);
}

void vna_pattern_insert(VnaLattice* lattice, VnaPattern* pattern,
                         int x0, int y0) {
    if (!lattice || !pattern || !pattern->data) return;
    for (int y = 0; y < pattern->height; y++) {
        for (int x = 0; x < pattern->width; x++) {
            uint8_t s = pattern->data[y * pattern->width + x];
            if (s > 0 || pattern->name) { /* Insert all cells including 0s */
                vna_lattice_set_cell(lattice, x0 + x, y0 + y, 0, s);
            }
        }
    }
}

VnaPattern* vna_pattern_extract(VnaLattice* lattice, int x0, int y0,
                                 int w, int h) {
    if (!lattice) return NULL;
    VnaPattern* pattern = vna_pattern_create(w, h, NULL, "extracted");
    if (!pattern) return NULL;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            pattern->data[y * w + x] = vna_lattice_get_cell(lattice, x0 + x, y0 + y, 0);
    return pattern;
}

double vna_pattern_match(VnaLattice* lattice, VnaPattern* pattern,
                          int* best_x, int* best_y) {
    if (!lattice || !pattern) return -1.0;
    int max_x = lattice->width - pattern->width;
    int max_y = lattice->height - pattern->height;
    if (max_x < 0 || max_y < 0) return -1.0;

    double best_score = -1.0;
    int bx = 0, by = 0;
    int psize = pattern->width * pattern->height;

    for (int y = 0; y <= max_y; y++) {
        for (int x = 0; x <= max_x; x++) {
            int matches = 0;
            for (int py = 0; py < pattern->height; py++) {
                for (int px = 0; px < pattern->width; px++) {
                    uint8_t ps = pattern->data[py * pattern->width + px];
                    uint8_t ls = vna_lattice_get_cell(lattice, x + px, y + py, 0);
                    if (ps == ls) matches++;
                }
            }
            double score = (double)matches / psize;
            if (score > best_score) {
                best_score = score;
                bx = x; by = y;
            }
        }
    }
    if (best_x) *best_x = bx;
    if (best_y) *best_y = by;
    return best_score;
}

VnaPattern* vna_pattern_rotate(VnaPattern* pattern, int quarters) {
    if (!pattern) return NULL;
    quarters = ((quarters % 4) + 4) % 4;
    if (quarters == 0) {
        VnaPattern* copy = vna_pattern_create(pattern->width, pattern->height,
                                               pattern->data, pattern->name);
        return copy;
    }
    int new_w = (quarters % 2 == 0) ? pattern->width : pattern->height;
    int new_h = (quarters % 2 == 0) ? pattern->height : pattern->width;
    VnaPattern* rotated = vna_pattern_create(new_w, new_h, NULL, pattern->name);
    if (!rotated) return NULL;

    for (int y = 0; y < pattern->height; y++) {
        for (int x = 0; x < pattern->width; x++) {
            uint8_t s = pattern->data[y * pattern->width + x];
            int nx, ny;
            switch (quarters) {
                case 1: nx = pattern->height - 1 - y; ny = x; break;
                case 2: nx = pattern->width - 1 - x;
                        ny = pattern->height - 1 - y; break;
                case 3: nx = y; ny = pattern->width - 1 - x; break;
                default: nx = x; ny = y; break;
            }
            rotated->data[ny * new_w + nx] = s;
        }
    }
    return rotated;
}

VnaPattern* vna_pattern_reflect(VnaPattern* pattern, bool horizontal) {
    if (!pattern) return NULL;
    VnaPattern* reflected = vna_pattern_create(pattern->width, pattern->height,
                                                NULL, pattern->name);
    if (!reflected) return NULL;
    for (int y = 0; y < pattern->height; y++) {
        for (int x = 0; x < pattern->width; x++) {
            int sx = horizontal ? (pattern->width - 1 - x) : x;
            int sy = horizontal ? y : (pattern->height - 1 - y);
            reflected->data[y * pattern->width + x] =
                pattern->data[sy * pattern->width + sx];
        }
    }
    return reflected;
}

/* ============================================================================
 * Histogram and Statistics
 * ============================================================================ */

VnaStateHistogram* vna_histogram_compute(VnaLattice* lattice) {
    if (!lattice) return NULL;
    VnaStateHistogram* hist = (VnaStateHistogram*)calloc(1, sizeof(VnaStateHistogram));
    if (!hist) return NULL;
    hist->num_states = lattice->num_states;
    hist->state_counts = (double*)calloc(lattice->num_states, sizeof(double));
    hist->state_probs = (double*)calloc(lattice->num_states, sizeof(double));
    if (!hist->state_counts || !hist->state_probs) {
        vna_histogram_free(hist);
        return NULL;
    }
    int total = lattice->width * lattice->height;
    for (int i = 0; i < total; i++) {
        uint8_t s = lattice->cells[i];
        if (s < lattice->num_states) hist->state_counts[s] += 1.0;
    }
    /* Compute probabilities and entropy */
    for (int s = 0; s < lattice->num_states; s++) {
        hist->state_probs[s] = hist->state_counts[s] / total;
        if (hist->state_probs[s] > 1e-15)
            hist->shannon_entropy -= hist->state_probs[s] *
                                     log2(hist->state_probs[s]);
    }
    /* Find dominant state */
    for (int s = 0; s < lattice->num_states; s++) {
        if (hist->state_counts[s] > hist->state_counts[hist->dominant_state])
            hist->dominant_state = s;
    }
    hist->dominant_fraction = hist->state_probs[hist->dominant_state];
    return hist;
}

void vna_histogram_free(VnaStateHistogram* hist) {
    if (!hist) return;
    free(hist->state_counts);
    free(hist->state_probs);
    free(hist);
}

VnaCAStatistics* vna_statistics_compute(VnaLattice* lattice,
                                         VnaNeighborhood* nb) {
    if (!lattice) return NULL;
    VnaCAStatistics* stats = (VnaCAStatistics*)calloc(1, sizeof(VnaCAStatistics));
    if (!stats) return NULL;

    int total = lattice->width * lattice->height;

    /* Density */
    double sum = 0;
    for (int i = 0; i < total; i++) sum += lattice->cells[i];
    stats->density = sum / total;

    /* Spatial entropy via histogram */
    VnaStateHistogram* hist = vna_histogram_compute(lattice);
    if (hist) {
        stats->spatial_entropy = hist->shannon_entropy;
        vna_histogram_free(hist);
    }

    /* Correlation length: autocorrelation decay */
    double** autocorr = (double**)calloc(lattice->height, sizeof(double*));
    for (int i = 0; i < lattice->height; i++)
        autocorr[i] = (double*)calloc(lattice->width, sizeof(double));

    double mean_val = stats->density;
    double variance = 0.0;
    for (int i = 0; i < total; i++) {
        double diff = lattice->cells[i] - mean_val;
        variance += diff * diff;
    }
    variance /= total;
    if (variance > 1e-15) {
        /* Compute spatial autocorrelation */
        for (int dy = 0; dy < lattice->height; dy++) {
            for (int dx = 0; dx < lattice->width; dx++) {
                double corr = 0.0;
                int count = 0;
                for (int y = 0; y < lattice->height; y++) {
                    for (int x = 0; x < lattice->width; x++) {
                        int nx = (x + dx) % lattice->width;
                        int ny = (y + dy) % lattice->height;
                        double d1 = lattice->cells[y * lattice->width + x]
                                    - mean_val;
                        double d2 = lattice->cells[ny * lattice->width + nx]
                                    - mean_val;
                        corr += d1 * d2;
                        count++;
                    }
                }
                autocorr[dy][dx] = corr / (count * variance);
            }
        }
        /* Estimate correlation length: first distance where |C(r)| < 1/e */
        for (int d = 1; d < lattice->width / 2; d++) {
            double avg_corr = 0.0;
            int cnt = 0;
            for (int dy = -d; dy <= d; dy++) {
                int dx = d - abs(dy);
                if (dx >= 0 && dx < lattice->width &&
                    (dy + lattice->height) % lattice->height < lattice->height) {
                    int ay = (dy + lattice->height) % lattice->height;
                    avg_corr += fabs(autocorr[ay][dx]);
                    cnt++;
                }
            }
            if (cnt > 0) avg_corr /= cnt;
            if (avg_corr < 1.0 / M_E) {
                stats->correlation_length = d;
                break;
            }
        }
    }

    for (int i = 0; i < lattice->height; i++) free(autocorr[i]);
    free(autocorr);

    return stats;
}

void vna_statistics_free(VnaCAStatistics* stats) {
    free(stats);
}

void vna_statistics_print(VnaCAStatistics* stats) {
    if (!stats) return;
    printf("CA Statistics:\n");
    printf("  Density:             %.6f\n", stats->density);
    printf("  Activity:            %.6f\n", stats->activity);
    printf("  Spatial Entropy:     %.6f bits\n", stats->spatial_entropy);
    printf("  Mutual Information:  %.6f bits\n", stats->mutual_information);
    printf("  Lyapunov Approx:     %.6f\n", stats->lyapunov_approx);
    printf("  Active Clusters:     %lld\n", (long long)stats->active_clusters);
    printf("  Correlation Length:  %.3f\n", stats->correlation_length);
}

/* ============================================================================
 * Universal Constructor
 * ============================================================================ */

VnaUniversalConstructor* vna_constructor_create(void) {
    VnaUniversalConstructor* uc =
        (VnaUniversalConstructor*)calloc(1, sizeof(VnaUniversalConstructor));
    if (!uc) return NULL;
    uc->arm_state = VNA_ARM_IDLE;
    uc->arm_direction = 0; /* North */
    uc->tape.capacity = 256;
    uc->tape.instructions = (VnaTapeInstruction*)calloc(256,
        sizeof(VnaTapeInstruction));
    if (!uc->tape.instructions) {
        free(uc);
        return NULL;
    }
    uc->construction_phase = 0;
    return uc;
}

void vna_constructor_free(VnaUniversalConstructor* uc) {
    if (!uc) return;
    free(uc->tape.instructions);
    free(uc);
}

void vna_constructor_add_instruction(VnaUniversalConstructor* uc,
                                      uint8_t op, uint8_t arg, int repeat) {
    if (!uc || !uc->tape.instructions) return;
    if (uc->tape.length >= uc->tape.capacity) {
        uc->tape.capacity *= 2;
        uc->tape.instructions = (VnaTapeInstruction*)realloc(
            uc->tape.instructions, uc->tape.capacity * sizeof(VnaTapeInstruction));
        if (!uc->tape.instructions) return;
    }
    uc->tape.instructions[uc->tape.length].operation = op;
    uc->tape.instructions[uc->tape.length].argument = arg;
    uc->tape.instructions[uc->tape.length].repeat_count = repeat;
    uc->tape.length++;
}

bool vna_constructor_step(VnaUniversalConstructor* uc, VnaLattice* target) {
    if (!uc || !target) return false;

    /* Simple state machine for construction */
    switch (uc->arm_state) {
        case VNA_ARM_IDLE:
            if (uc->tape.position < uc->tape.length) {
                uc->arm_state = VNA_ARM_EXTENDING;
            }
            break;
        case VNA_ARM_EXTENDING:
            if (uc->arm_length < 10) {
                uc->arm_length++;
                switch (uc->arm_direction) {
                    case 0: uc->arm_y--; break;
                    case 1: uc->arm_x++; break;
                    case 2: uc->arm_y++; break;
                    case 3: uc->arm_x--; break;
                }
            } else {
                uc->arm_state = VNA_ARM_INJECTING;
            }
            break;
        case VNA_ARM_INJECTING:
            if (uc->tape.position < uc->tape.length) {
                VnaTapeInstruction* instr =
                    &uc->tape.instructions[uc->tape.position];
                vna_lattice_set_cell(target, uc->arm_x, uc->arm_y, 0,
                                     instr->argument);
                uc->tape.position++;
                uc->arm_state = VNA_ARM_RETRACTING;
            } else {
                uc->arm_state = VNA_ARM_IDLE;
                return false; /* Done */
            }
            break;
        case VNA_ARM_RETRACTING:
            if (uc->arm_length > 0) {
                uc->arm_length--;
                switch (uc->arm_direction) {
                    case 0: uc->arm_y++; break;
                    case 1: uc->arm_x--; break;
                    case 2: uc->arm_y--; break;
                    case 3: uc->arm_x++; break;
                }
            } else {
                uc->arm_state = VNA_ARM_TURNING;
            }
            break;
        case VNA_ARM_TURNING:
            uc->arm_direction = (uc->arm_direction + 1) % 4;
            uc->arm_state = VNA_ARM_EXTENDING;
            break;
    }
    return true;
}

VnaReplicationPhase vna_constructor_get_phase(VnaUniversalConstructor* uc) {
    if (!uc) return VNA_REPL_IDLE;
    if (uc->tape.position == 0) return VNA_REPL_IDLE;
    if (uc->tape.position < uc->tape.length / 4) return VNA_REPL_READ_TAPE;
    if (uc->tape.position < uc->tape.length / 2) return VNA_REPL_BUILD_ARM;
    if (uc->tape.position < 3 * uc->tape.length / 4) return VNA_REPL_BUILD_BODY;
    if (uc->tape.position < uc->tape.length) return VNA_REPL_COPY_TAPE;
    return VNA_REPL_DONE;
}

/* ============================================================================
 * Fault Tolerance Utilities
 * ============================================================================ */

static double binomial_coeff(int n, int k) {
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;
    if (k > n / 2) k = n - k;
    double result = 1.0;
    for (int i = 1; i <= k; i++) {
        result *= (n - k + i);
        result /= i;
    }
    return result;
}

/* P(majority error in bundle of size N with line error p)
 * = Σ_{k > N/2}^{N} C(N,k) p^k (1-p)^{N-k} */
double vna_error_probability_network(int depth, double gate_error_prob) {
    /* Simplified von Neumann network model.
     * After depth d multiplexing stages, the error is approximately
     * ε(d) ≈ ε_crit * (ε/ε_crit)^{2^d}
     * This is the key formula from von Neumann (1956). */
    double eps_crit = 0.0107; /* von Neumann's threshold for NAND */
    if (gate_error_prob >= eps_crit) return 1.0;
    double ratio = gate_error_prob / eps_crit;
    return eps_crit * pow(ratio, pow(2.0, depth));
}

double vna_multiplexing_reliability(int multiplexing_degree,
                                     double component_error) {
    /* Probability that a multiplexed bundle gives the correct majority.
     * Uses the cumulative binomial distribution. */
    int n = multiplexing_degree;
    if (n % 2 == 0) n++; /* Ensure odd */
    double p_error = 0.0;
    for (int k = n / 2 + 1; k <= n; k++) {
        p_error += binomial_coeff(n, k) * pow(component_error, k) *
                   pow(1.0 - component_error, n - k);
    }
    return 1.0 - p_error;
}

int vna_optimal_multiplexing_degree(double component_error,
                                     double target_error) {
    /* Binary search for minimum N achieving target error. */
    if (component_error >= 0.5) return -1;
    if (component_error < target_error) return 1;

    int lo = 1, hi = 10001;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (mid % 2 == 0) mid++;
        double reliability = vna_multiplexing_reliability(mid, component_error);
        if (1.0 - reliability <= target_error) {
            hi = mid;
        } else {
            lo = mid + 2;
        }
    }
    return (lo > 10000) ? -1 : lo;
}
