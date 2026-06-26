#include "sos_cellular.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ===== Cellular Automaton Lifecycle ===== */

CellularAutomaton* ca_create(int width, int height, const char* rule_name) {
    CellularAutomaton* ca = (CellularAutomaton*)calloc(1, sizeof(CellularAutomaton));
    if (!ca) return NULL;
    ca->width = width; ca->height = height;
    ca->grid = (CellState**)malloc(height * sizeof(CellState*));
    ca->next_grid = (CellState**)malloc(height * sizeof(CellState*));
    for (int y = 0; y < height; y++) {
        ca->grid[y] = (CellState*)calloc(width, sizeof(CellState));
        ca->next_grid[y] = (CellState*)calloc(width, sizeof(CellState));
    }
    ca->boundary = CA_BOUNDARY_PERIODIC;
    ca->neighborhood = CA_NEIGHBOR_MOORE;
    ca->generation = 0;
    ca->lambda = 0.0;
    ca->rule_name = strdup(rule_name ? rule_name : "B3/S23");
    memset(ca->survive_counts, 0, sizeof(ca->survive_counts));
    memset(ca->birth_counts, 0, sizeof(ca->birth_counts));
    /* Parse Game of Life rule: B3/S23 */
    if (strcmp(ca->rule_name, "B3/S23") == 0) {
        ca->birth_counts[3] = 1;
        ca->survive_counts[2] = 1; ca->survive_counts[3] = 1;
    }
    return ca;
}

void ca_free(CellularAutomaton* ca) {
    if (!ca) return;
    for (int y = 0; y < ca->height; y++) {
        free(ca->grid[y]); free(ca->next_grid[y]);
    }
    free(ca->grid); free(ca->next_grid);
    free(ca->rule_name);
    free(ca);
}

void ca_set_boundary(CellularAutomaton* ca, CABoundaryType btype) {
    if (ca) ca->boundary = btype;
}

void ca_set_neighborhood(CellularAutomaton* ca, CANeighborhoodType ntype) {
    if (ca) ca->neighborhood = ntype;
}

void ca_set_cell(CellularAutomaton* ca, int x, int y, CellState state) {
    if (!ca || x < 0 || x >= ca->width || y < 0 || y >= ca->height) return;
    ca->grid[y][x] = state;
}

CellState ca_get_cell(const CellularAutomaton* ca, int x, int y) {
    if (!ca || x < 0 || x >= ca->width || y < 0 || y >= ca->height) return CELL_DEAD;
    return ca->grid[y][x];
}

void ca_randomize(CellularAutomaton* ca, double density) {
    if (!ca || density < 0.0 || density > 1.0) return;
    for (int y = 0; y < ca->height; y++)
        for (int x = 0; x < ca->width; x++)
            ca->grid[y][x] = ((double)rand() / RAND_MAX < density) ? CELL_ALIVE : CELL_DEAD;
}

int ca_count_neighbors(const CellularAutomaton* ca, int x, int y) {
    if (!ca) return 0;
    int count = 0;
    switch (ca->neighborhood) {
    case CA_NEIGHBOR_MOORE:
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = x + dx, ny = y + dy;
                if (ca->boundary == CA_BOUNDARY_PERIODIC) {
                    nx = (nx + ca->width) % ca->width;
                    ny = (ny + ca->height) % ca->height;
                } else {
                    if (nx < 0 || nx >= ca->width || ny < 0 || ny >= ca->height) continue;
                }
                if (ca->grid[ny][nx] == CELL_ALIVE) count++;
            }
        break;
    case CA_NEIGHBOR_VON_NEUMANN: {
        int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
        for (int d = 0; d < 4; d++) {
            int nx = x + dirs[d][0], ny = y + dirs[d][1];
            if (ca->boundary == CA_BOUNDARY_PERIODIC) {
                nx = (nx + ca->width) % ca->width;
                ny = (ny + ca->height) % ca->height;
            } else {
                if (nx < 0 || nx >= ca->width || ny < 0 || ny >= ca->height) continue;
            }
            if (ca->grid[ny][nx] == CELL_ALIVE) count++;
        }
        break;
    }
    default: break;
    }
    return count;
}

void ca_evolve(CellularAutomaton* ca) {
    if (!ca) return;
    for (int y = 0; y < ca->height; y++)
        for (int x = 0; x < ca->width; x++) {
            int n = ca_count_neighbors(ca, x, y);
            if (ca->grid[y][x] == CELL_ALIVE)
                ca->next_grid[y][x] = ca->survive_counts[n] ? CELL_ALIVE : CELL_DEAD;
            else
                ca->next_grid[y][x] = ca->birth_counts[n] ? CELL_ALIVE : CELL_DEAD;
        }
    for (int y = 0; y < ca->height; y++)
        for (int x = 0; x < ca->width; x++)
            ca->grid[y][x] = ca->next_grid[y][x];
    ca->generation++;
}

int ca_live_count(const CellularAutomaton* ca) {
    if (!ca) return 0;
    int count = 0;
    for (int y = 0; y < ca->height; y++)
        for (int x = 0; x < ca->width; x++)
            if (ca->grid[y][x] == CELL_ALIVE) count++;
    return count;
}

double ca_density(const CellularAutomaton* ca) {
    if (!ca || ca->width <= 0 || ca->height <= 0) return 0.0;
    return (double)ca_live_count(ca) / (ca->width * ca->height);
}

int ca_detect_still_life(const CellularAutomaton* ca) {
    if (!ca) return 0;
    /* A still life is a pattern unchanged after 1 generation.
     * We compare current grid with one evolved step. */
    CellularAutomaton* copy = ca_create(ca->width, ca->height, ca->rule_name);
    for (int y = 0; y < ca->height; y++)
        for (int x = 0; x < ca->width; x++)
            copy->grid[y][x] = ca->grid[y][x];
    copy->boundary = ca->boundary;
    copy->neighborhood = ca->neighborhood;
    memcpy(copy->survive_counts, ca->survive_counts, sizeof(ca->survive_counts));
    memcpy(copy->birth_counts, ca->birth_counts, sizeof(ca->birth_counts));
    ca_evolve(copy);
    int count = 0;
    for (int y = 0; y < ca->height; y++)
        for (int x = 0; x < ca->width; x++)
            if (ca->grid[y][x] == copy->grid[y][x]) count++;
    ca_free(copy);
    return count;
}

int ca_hamming_distance(const CellularAutomaton* ca1, const CellularAutomaton* ca2) {
    if (!ca1 || !ca2 || ca1->width != ca2->width || ca1->height != ca2->height)
        return -1;
    int dist = 0;
    for (int y = 0; y < ca1->height; y++)
        for (int x = 0; x < ca1->width; x++)
            if (ca1->grid[y][x] != ca2->grid[y][x]) dist++;
    return dist;
}

double ca_damage_spreading(CellularAutomaton* ca, int steps) {
    /* Bagnoli et al. (1992): Damage spreading Lyapunov exponent.
     * Create a slightly perturbed copy, evolve both, track Hamming distance. */
    if (!ca || steps <= 0) return 0.0;
    CellularAutomaton* ca2 = ca_create(ca->width, ca->height, ca->rule_name);
    for (int y = 0; y < ca->height; y++)
        for (int x = 0; x < ca->width; x++)
            ca2->grid[y][x] = ca->grid[y][x];
    /* Flip the center cell */
    ca2->grid[ca->height/2][ca->width/2] =
        (ca2->grid[ca->height/2][ca->width/2] == CELL_ALIVE) ? CELL_DEAD : CELL_ALIVE;
    ca2->boundary = ca->boundary; ca2->neighborhood = ca->neighborhood;
    memcpy(ca2->birth_counts, ca->birth_counts, sizeof(ca->birth_counts));
    memcpy(ca2->survive_counts, ca->survive_counts, sizeof(ca->survive_counts));

    double sum_log = 0.0;
    for (int s = 0; s < steps; s++) {
        ca_evolve(ca); ca_evolve(ca2);
        int d = ca_hamming_distance(ca, ca2);
        if (d > 0) sum_log += log((double)d);
    }
    ca_free(ca2);
    return (steps > 0) ? sum_log / steps : 0.0;
}

double ca_lambda_parameter(const CellularAutomaton* ca) {
    /* Langton (1990): lambda = fraction of neighborhood configurations
     * that map to a non-quiescent (live) state.
     * For Game of Life: 2 states, 9 possible neighbor counts.
     * Live states: birth[3]=1, survive[2]=survive[3]=1 -> 3/9 = 0.333 */
    if (!ca) return 0.0;
    int total_rules = 18; /* 9 dead + 9 alive */
    int non_quiescent = 0;
    for (int n = 0; n <= 8; n++) {
        if (ca->birth_counts[n]) non_quiescent++;
        if (ca->survive_counts[n]) non_quiescent++;
    }
    return (double)non_quiescent / total_rules;
}

CAClass ca_classify_wolfram(const CellularAutomaton* ca, int max_steps) {
    /* Simplified Wolfram classification based on density evolution.
     * Class I: density -> 0 (all dead)
     * Class II: density -> constant non-zero (periodic)
     * Class III: density fluctuates chaotically
     * Class IV: density shows complex transients */
    if (!ca || max_steps < 10) return CA_CLASS_I;
    double prev_density = ca_density(ca);
    double min_density = prev_density, max_density = prev_density;
    double sum_change = 0.0;
    CellularAutomaton* test = ca_create(ca->width, ca->height, ca->rule_name);
    for (int y = 0; y < ca->height; y++)
        for (int x = 0; x < ca->width; x++)
            test->grid[y][x] = ca->grid[y][x];
    test->boundary = ca->boundary; test->neighborhood = ca->neighborhood;
    memcpy(test->birth_counts, ca->birth_counts, sizeof(ca->birth_counts));
    memcpy(test->survive_counts, ca->survive_counts, sizeof(ca->survive_counts));

    for (int s = 0; s < max_steps; s++) {
        ca_evolve(test);
        double d = ca_density(test);
        sum_change += fabs(d - prev_density);
        if (d < min_density) min_density = d;
        if (d > max_density) max_density = d;
        prev_density = d;
    }
    ca_free(test);
    if (min_density < 1e-15) return CA_CLASS_I;
    double fluctuation = max_density - min_density;
    if (fluctuation < 0.05) return CA_CLASS_II;
    if (fluctuation > 0.3) return CA_CLASS_III;
    return CA_CLASS_IV;
}

/* ===== Elementary 1D CA ===== */

ElementaryCA* eca_create(int width, int rule_number) {
    if (width <= 0) return NULL;
    ElementaryCA* eca = (ElementaryCA*)calloc(1, sizeof(ElementaryCA));
    if (!eca) return NULL;
    eca->width = width;
    eca->cells = (int*)calloc(width, sizeof(int));
    eca->next_cells = (int*)calloc(width, sizeof(int));
    eca->rule_number = rule_number;
    eca->generation = 0;
    eca->boundary = CA_BOUNDARY_PERIODIC;
    return eca;
}

void eca_free(ElementaryCA* eca) {
    if (!eca) return;
    free(eca->cells); free(eca->next_cells);
    free(eca);
}

void eca_set_cell(ElementaryCA* eca, int x, int value) {
    if (!eca || x < 0 || x >= eca->width) return;
    eca->cells[x] = (value != 0) ? 1 : 0;
}

int eca_get_cell(const ElementaryCA* eca, int x) {
    if (!eca || x < 0 || x >= eca->width) return 0;
    return eca->cells[x];
}

void eca_randomize(ElementaryCA* eca, double density) {
    if (!eca) return;
    for (int x = 0; x < eca->width; x++)
        eca->cells[x] = ((double)rand() / RAND_MAX < density) ? 1 : 0;
}

int eca_compute_rule(const ElementaryCA* eca, int x) {
    /* Wolfram encoding: neighborhood of 3 bits (left, center, right)
     * encoded as binary number 0-7, maps to rule bit. */
    if (!eca) return 0;
    int left = eca_get_cell(eca, x-1);
    int center = eca_get_cell(eca, x);
    int right = eca_get_cell(eca, x+1);
    int idx = (left << 2) | (center << 1) | right;
    return (eca->rule_number >> idx) & 1;
}

void eca_evolve(ElementaryCA* eca) {
    if (!eca) return;
    for (int x = 0; x < eca->width; x++)
        eca->next_cells[x] = eca_compute_rule(eca, x);
    for (int x = 0; x < eca->width; x++)
        eca->cells[x] = eca->next_cells[x];
    eca->generation++;
}

double eca_spatial_entropy(const ElementaryCA* eca) {
    if (!eca || eca->width <= 0) return 0.0;
    int ones = 0;
    for (int x = 0; x < eca->width; x++)
        if (eca->cells[x]) ones++;
    double p1 = (double)ones / eca->width;
    double p0 = 1.0 - p1;
    double H = 0.0;
    if (p0 > 1e-15) H -= p0 * log2(p0);
    if (p1 > 1e-15) H -= p1 * log2(p1);
    return H;
}

/* ===== Sandpile Model ===== */

SandpileModel* sp_create(int width, int height, int threshold) {
    SandpileModel* sp = (SandpileModel*)calloc(1, sizeof(SandpileModel));
    if (!sp) return NULL;
    sp->width = width; sp->height = height;
    sp->threshold = threshold > 0 ? threshold : 4;
    sp->pile = (int**)malloc(height * sizeof(int*));
    for (int y = 0; y < height; y++)
        sp->pile[y] = (int*)calloc(width, sizeof(int));
    sp->total_grains = 0; sp->total_avalanches = 0;
    sp->aval_history_cap = 10000;
    sp->aval_history_len = 0;
    sp->avalanche_sizes = (long long*)malloc(sp->aval_history_cap * sizeof(long long));
    return sp;
}

void sp_free(SandpileModel* sp) {
    if (!sp) return;
    for (int y = 0; y < sp->height; y++) free(sp->pile[y]);
    free(sp->pile); free(sp->avalanche_sizes);
    free(sp->aval_size_dist); free(sp->aval_time_dist);
    free(sp);
}

int sp_add_grain(SandpileModel* sp, int x, int y) {
    if (!sp || x < 0 || x >= sp->width || y < 0 || y >= sp->height) return 0;
    sp->pile[y][x]++; sp->total_grains++;
    if (sp->pile[y][x] >= sp->threshold)
        return sp_relax(sp);
    return 0;
}

int sp_add_random_grains(SandpileModel* sp, int n_grains) {
    if (!sp) return 0;
    int total = 0;
    for (int i = 0; i < n_grains; i++)
        total += sp_add_grain(sp, rand() % sp->width, rand() % sp->height);
    return total;
}

int sp_relax(SandpileModel* sp) {
    if (!sp) return 0;
    int toppled = 0;
    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int y = 0; y < sp->height; y++)
            for (int x = 0; x < sp->width; x++) {
                if (sp->pile[y][x] >= sp->threshold) {
                    sp->pile[y][x] -= sp->threshold;
                    for (int d = 0; d < 4; d++) {
                        int nx = x + dirs[d][0], ny = y + dirs[d][1];
                        if (nx >= 0 && nx < sp->width && ny >= 0 && ny < sp->height)
                            sp->pile[ny][nx]++;
                    }
                    toppled++; changed = 1;
                }
            }
    }
    if (toppled > 0) {
        if (sp->aval_history_len >= sp->aval_history_cap) {
            sp->aval_history_cap *= 2;
            sp->avalanche_sizes = (long long*)realloc(sp->avalanche_sizes,
                                         sp->aval_history_cap * sizeof(long long));
        }
        sp->avalanche_sizes[sp->aval_history_len++] = toppled;
        sp->total_avalanches++;
    }
    return toppled;
}

long long sp_total_grains(const SandpileModel* sp) {
    return sp ? sp->total_grains : 0;
}

double sp_average_slope(const SandpileModel* sp) {
    if (!sp) return 0.0;
    /* Average slope = mean gradient magnitude across the pile.
     * Approximate by mean pile height normalized by system size. */
    double sum = 0.0;
    for (int y = 0; y < sp->height; y++)
        for (int x = 0; x < sp->width; x++)
            sum += sp->pile[y][x];
    return sum / (sp->width * sp->height);
}

void sp_compute_size_distribution(SandpileModel* sp, int n_samples, int n_bins) {
    (void)n_samples;
    if (!sp || n_bins <= 0) return;
    free(sp->aval_size_dist);
    sp->aval_size_dist = (double*)calloc(n_bins, sizeof(double));
    if (sp->aval_history_len == 0) return;
    /* Find max avalanche size */
    long long max_s = 0;
    for (int i = 0; i < sp->aval_history_len; i++)
        if (sp->avalanche_sizes[i] > max_s)
            max_s = sp->avalanche_sizes[i];
    if (max_s == 0) return;
    /* Log-binned histogram */
    double log_min = 0.0;
    double log_max = log((double)max_s);
    double dlog = (log_max - log_min) / n_bins;
    for (int i = 0; i < sp->aval_history_len; i++) {
        double ls = log((double)(sp->avalanche_sizes[i] + 1));
        int bin = (int)((ls - log_min) / dlog);
        if (bin >= 0 && bin < n_bins) sp->aval_size_dist[bin]++;
    }
    /* Normalize */
    for (int b = 0; b < n_bins; b++)
        sp->aval_size_dist[b] /= sp->aval_history_len;
}
