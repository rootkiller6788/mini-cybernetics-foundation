#include "vna_cellular.h"
#include "vna_core.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declaration */
static double binomial_coeff_approx(int n, int k);

/* ============================================================================
 * L2: Quiescent State and Configuration Space
 * ============================================================================ */

uint8_t vna_find_quiescent_state(VnaLattice* lattice, VnaTransitionRule* rule,
                                  VnaNeighborhood* nb) {
    /* The quiescent state q has the property that if all neighbors are q,
     * then the next state is also q: f(q, q, ..., q) = q. */
    if (!rule || !nb) return 0;
    uint8_t* all_same = (uint8_t*)calloc(nb->num_neighbors, sizeof(uint8_t));
    for (int s = 0; s < rule->num_states; s++) {
        for (int i = 0; i < nb->num_neighbors; i++) all_same[i] = (uint8_t)s;
        uint8_t result = vna_rule_lookup(rule, all_same);
        if (result == s) {
            free(all_same);
            return (uint8_t)s;
        }
    }
    free(all_same);
    return 0;
}

bool vna_is_quiescent_config(VnaLattice* lattice, uint8_t quiescent) {
    if (!lattice || !lattice->cells) return false;
    int total = lattice->width * lattice->height;
    for (int i = 0; i < total; i++)
        if (lattice->cells[i] != quiescent) return false;
    return true;
}

bool vna_configuration_is_finite(VnaLattice* lattice, uint8_t quiescent) {
    /* A configuration is finite if only finitely many cells are non-quiescent.
     * On a finite lattice, this is always true. For theoretical completeness,
     * we count the non-quiescent cells and report if the count is bounded. */
    if (!lattice || !lattice->cells) return true;
    int total = lattice->width * lattice->height;
    int count = 0;
    for (int i = 0; i < total; i++)
        if (lattice->cells[i] != quiescent) count++;
    /* On a finite lattice, always finite. Return the ratio. */
    (void)count;
    return true;
}

/* ============================================================================
 * L3: Lattice Subset Operations
 * ============================================================================ */

VnaLattice* vna_sublattice_extract(VnaLattice* lattice, int x0, int y0,
                                    int w, int h) {
    if (!lattice) return NULL;
    if (w <= 0) w = lattice->width;
    if (h <= 0) h = lattice->height;
    VnaLattice* sub = vna_lattice_create(w, h, lattice->num_states,
                                          lattice->topology);
    if (!sub) return NULL;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint8_t s = vna_lattice_get_cell(lattice, x0 + x, y0 + y, 0);
            vna_lattice_set_cell(sub, x, y, 0, s);
        }
    }
    sub->bc_x = lattice->bc_x;
    sub->bc_y = lattice->bc_y;
    return sub;
}

void vna_sublattice_insert(VnaLattice* dest, VnaLattice* src,
                            int dest_x, int dest_y) {
    if (!dest || !src) return;
    for (int y = 0; y < src->height; y++)
        for (int x = 0; x < src->width; x++)
            vna_lattice_set_cell(dest, dest_x + x, dest_y + y, 0,
                                 vna_lattice_get_cell(src, x, y, 0));
}

/* ============================================================================
 * Block-Based Operations (Margolus Neighborhood)
 * ============================================================================ */

int vna_block_partition_count(VnaLattice* lattice, int block_w, int block_h) {
    if (!lattice || block_w <= 0 || block_h <= 0) return 0;
    return (lattice->width / block_w) * (lattice->height / block_h);
}

void vna_block_get(VnaLattice* lattice, int block_idx, int block_w, int block_h,
                    uint8_t* block_out) {
    if (!lattice || !block_out) return;
    int blocks_per_row = lattice->width / block_w;
    int bx = (block_idx % blocks_per_row) * block_w;
    int by = (block_idx / blocks_per_row) * block_h;
    for (int y = 0; y < block_h; y++)
        for (int x = 0; x < block_w; x++)
            block_out[y * block_w + x] =
                vna_lattice_get_cell(lattice, bx + x, by + y, 0);
}

void vna_block_set(VnaLattice* lattice, int block_idx, int block_w, int block_h,
                    const uint8_t* block_in) {
    if (!lattice || !block_in) return;
    int blocks_per_row = lattice->width / block_w;
    int bx = (block_idx % blocks_per_row) * block_w;
    int by = (block_idx / blocks_per_row) * block_h;
    for (int y = 0; y < block_h; y++)
        for (int x = 0; x < block_w; x++)
            vna_lattice_set_cell(lattice, bx + x, by + y, 0,
                                 block_in[y * block_w + x]);
}

/* ============================================================================
 * L3: Advanced Lattice Access
 * ============================================================================ */

int vna_wrap_x(VnaLattice* lattice, int x) {
    if (!lattice) return x;
    return ((x % lattice->width) + lattice->width) % lattice->width;
}

int vna_wrap_y(VnaLattice* lattice, int y) {
    if (!lattice) return y;
    return ((y % lattice->height) + lattice->height) % lattice->height;
}

int vna_wrap_z(VnaLattice* lattice, int z) {
    return z; /* 2D lattice has depth 1 */
}

void vna_collect_neighbors(VnaLattice* lattice, int x, int y, int z,
                            VnaNeighborhood* nb, uint8_t* neighbor_buf) {
    if (!lattice || !nb || !neighbor_buf) return;
    for (int ni = 0; ni < nb->num_neighbors; ni++) {
        int nx = x + nb->offset_dx[ni];
        int ny = y + nb->offset_dy[ni];
        /* Use safe_get logic inline for speed */
        if (lattice->bc_x == VNA_BC_PERIODIC) {
            nx = ((nx % lattice->width) + lattice->width) % lattice->width;
            ny = ((ny % lattice->height) + lattice->height) % lattice->height;
        } else if (nx < 0 || nx >= lattice->width ||
                   ny < 0 || ny >= lattice->height) {
            neighbor_buf[ni] = 0;
            continue;
        }
        neighbor_buf[ni] = lattice->cells[ny * lattice->width + nx];
    }
}

double vna_lattice_convolve(VnaLattice* lattice, int x, int y, int z,
                             double* kernel, int kernel_w, int kernel_h) {
    if (!lattice || !kernel) return 0.0;
    double sum = 0.0;
    int khw = kernel_w / 2;
    int khh = kernel_h / 2;
    for (int ky = -khh; ky <= khh; ky++) {
        for (int kx = -khw; kx <= khw; kx++) {
            int nx = x + kx;
            int ny = y + ky;
            if (lattice->bc_x == VNA_BC_PERIODIC) {
                nx = ((nx % lattice->width) + lattice->width) % lattice->width;
                ny = ((ny % lattice->height) + lattice->height) % lattice->height;
            }
            double val = vna_lattice_get_cell(lattice, nx, ny, 0);
            int ki = (ky + khh) * kernel_w + (kx + khw);
            sum += val * kernel[ki];
        }
    }
    return sum;
}

/* ============================================================================
 * L5: Pattern Analysis Algorithms
 * ============================================================================ */

int vna_enumerate_blocks(VnaLattice* lattice, int block_w, int block_h,
                          uint8_t* pattern_buffer, int* counts,
                          int max_patterns) {
    if (!lattice || !pattern_buffer || !counts) return 0;
    int block_size = block_w * block_h;
    int num_found = 0;

    for (int y = 0; y <= lattice->height - block_h; y++) {
        for (int x = 0; x <= lattice->width - block_w; x++) {
            /* Extract current block */
            uint8_t block_data[256];
            int idx = 0;
            for (int dy = 0; dy < block_h; dy++)
                for (int dx = 0; dx < block_w; dx++)
                    block_data[idx++] = vna_lattice_get_cell(lattice,
                        x + dx, y + dy, 0);

            /* Check if this pattern is already known */
            bool found = false;
            for (int p = 0; p < num_found; p++) {
                if (memcmp(&pattern_buffer[p * block_size], block_data,
                           block_size) == 0) {
                    counts[p]++;
                    found = true;
                    break;
                }
            }
            if (!found && num_found < max_patterns) {
                memcpy(&pattern_buffer[num_found * block_size], block_data,
                       block_size);
                counts[num_found] = 1;
                num_found++;
            }
        }
    }
    return num_found;
}

double vna_block_entropy(VnaLattice* lattice, int block_w, int block_h) {
    if (!lattice) return 0.0;
    int max_patterns = 10000;
    int block_size = block_w * block_h;
    uint8_t* patterns = (uint8_t*)malloc(max_patterns * block_size);
    int* counts = (int*)calloc(max_patterns, sizeof(int));
    if (!patterns || !counts) {
        free(patterns); free(counts);
        return 0.0;
    }

    int n = vna_enumerate_blocks(lattice, block_w, block_h, patterns, counts,
                                  max_patterns);
    int total_blocks = 0;
    for (int i = 0; i < n; i++) total_blocks += counts[i];

    double entropy = 0.0;
    for (int i = 0; i < n; i++) {
        double p = (double)counts[i] / total_blocks;
        if (p > 1e-15) entropy -= p * log2(p);
    }

    free(patterns);
    free(counts);
    return entropy;
}

/* ============================================================================
 * L5: Damage Spreading Analysis
 * ============================================================================ */

VnaDamageSpread* vna_damage_spreading(VnaLattice* lattice, VnaNeighborhood* nb,
                                       VnaTransitionRule* rule, int steps,
                                       int perturb_x, int perturb_y) {
    if (!lattice || !nb || !rule) return NULL;
    VnaDamageSpread* ds = (VnaDamageSpread*)calloc(1, sizeof(VnaDamageSpread));
    if (!ds) return NULL;
    ds->max_steps = steps;
    ds->hamming_distance = (double*)calloc(steps + 1, sizeof(double));
    if (!ds->hamming_distance) { free(ds); return NULL; }

    /* Create a perturbed copy */
    VnaLattice* copy = vna_lattice_create(lattice->width, lattice->height,
                                           lattice->num_states,
                                           lattice->topology);
    if (!copy) { free(ds->hamming_distance); free(ds); return NULL; }
    memcpy(copy->cells, lattice->cells,
           lattice->width * lattice->height);
    copy->bc_x = lattice->bc_x;
    copy->bc_y = lattice->bc_y;

    /* Flip one cell in the copy */
    int px = perturb_x % lattice->width;
    if (px < 0) px += lattice->width;
    int py = perturb_y % lattice->height;
    if (py < 0) py += lattice->height;
    uint8_t old_val = copy->cells[py * lattice->width + px];
    copy->cells[py * lattice->width + px] =
        (old_val + 1) % lattice->num_states;

    /* Compute initial Hamming distance */
    int total = lattice->width * lattice->height;
    int h0 = 0;
    for (int i = 0; i < total; i++)
        if (lattice->cells[i] != copy->cells[i]) h0++;
    ds->hamming_distance[0] = (double)h0 / total;

    /* Evolve both and track Hamming distance */
    for (int t = 1; t <= steps; t++) {
        vna_lattice_evolve(lattice, nb, rule);
        vna_lattice_evolve(copy, nb, rule);
        int hd = 0;
        for (int i = 0; i < total; i++)
            if (lattice->cells[i] != copy->cells[i]) hd++;
        ds->hamming_distance[t] = (double)hd / total;
    }

    /* Estimate Lyapunov exponent: λ ≈ (1/t) * log(H(t) / H(0)) */
    if (h0 > 0 && ds->hamming_distance[steps] > 1e-15) {
        ds->lyapunov_estimate = log(ds->hamming_distance[steps] /
                                     ds->hamming_distance[0]) / steps;
    }
    ds->damage_saturation = ds->hamming_distance[steps];
    ds->is_chaotic = (ds->lyapunov_estimate > 0.01);

    vna_lattice_free(copy);
    return ds;
}

void vna_damage_spread_free(VnaDamageSpread* ds) {
    if (!ds) return;
    free(ds->hamming_distance);
    free(ds);
}

/* ============================================================================
 * L5: CA Inverse Problem / Synthesis (Simplified)
 * ============================================================================ */

int vna_synthesize_rule_for_pattern(VnaPattern* target, int num_states,
                                     VnaNeighborhoodType nb_type, int radius,
                                     VnaTransitionRule** candidates,
                                     int max_candidates) {
    /* Search for rules that make the target pattern a still life.
     * For each neighborhood configuration appearing in the pattern,
     * set the rule output to match the center cell. */
    if (!target || max_candidates <= 0) return 0;

    VnaNeighborhood* nb = vna_neighborhood_create(nb_type, radius, num_states);
    if (!nb) return 0;

    int found = 0;
    VnaTransitionRule* rule = vna_rule_create(num_states, nb->num_neighbors);
    if (!rule) { vna_neighborhood_free(nb); return 0; }

    /* For each cell in the pattern (away from edges), record the
     * neighborhood configuration and set the output to the cell's state.
     * This creates a rule consistent with the pattern being a still life. */
    for (int y = radius; y < target->height - radius && found < max_candidates;
         y++) {
        for (int x = radius; x < target->width - radius; x++) {
            uint8_t* neighbors = (uint8_t*)malloc(nb->num_neighbors);
            int ni = 0;
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    if (nb_type == VNA_VON_NEUMANN &&
                        abs(dx) + abs(dy) > radius) continue;
                    neighbors[ni++] = target->data[(y + dy) * target->width
                                                    + (x + dx)];
                }
            }
            uint8_t center = target->data[y * target->width + x];
            vna_rule_set_entry(rule, neighbors, center);
            free(neighbors);
        }
    }

    candidates[0] = rule;
    found = 1;
    vna_neighborhood_free(nb);
    return found;
}

/* ============================================================================
 * L5: Higher-Order CA Operations
 * ============================================================================ */

double vna_mean_field_density(VnaLattice* lattice, VnaTransitionRule* rule,
                               VnaNeighborhood* nb) {
    /* Mean-field approximation for binary CA:
     * Assume cells are independent with probability ρ of being 1.
     * P(neighborhood pattern) = ρ^k * (1-ρ)^{n-k}
     * ρ_next = Σ f(pattern) * P(pattern) */
    if (!rule || rule->num_states != 2 || !nb) return 0.0;
    double rho = 0.0;
    int total = lattice->width * lattice->height;
    for (int i = 0; i < total; i++) rho += lattice->cells[i];
    rho /= total;

    /* For 1D elementary CA: approximate mean field map.
     * For outer-totalistic rules, we can compute analytically. */
    int n = nb->num_neighbors;
    double rho_next = 0.0;
    /* Sum over all possible numbers of 1s in neighborhood (excluding center) */
    for (int k = 0; k < n; k++) {
        double prob_k = binomial_coeff_approx(n - 1, k) *
                         pow(rho, k) * pow(1.0 - rho, n - 1 - k);
        /* For Life-like rules: need rule lookup.
         * Simplified: use average rule output for this k. */
        uint8_t neighbors_states[9];
        int center_idx = n / 2;
        for (int i = 0; i < n; i++) neighbors_states[i] = 0;
        /* Set k+1 of the neighbor states to 1 (including center) */
        for (int c = 0; c < 2; c++) { /* center = 0 or 1 */
            for (int i = 0; i < n; i++) neighbors_states[i] = 0;
            neighbors_states[center_idx] = (uint8_t)c;
            int ones_placed = c;
            for (int i = 0; i < n && ones_placed <= k; i++) {
                if (i != center_idx && ones_placed < k) {
                    neighbors_states[i] = 1;
                    ones_placed++;
                }
            }
            uint8_t out = vna_rule_lookup(rule, neighbors_states);
            if (c == 1) {
                rho_next += out * prob_k * rho;
            } else {
                rho_next += out * prob_k * (1.0 - rho);
            }
        }
    }
    return rho_next;
}

static double binomial_coeff_approx(int n, int k) {
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;
    double result = 1.0;
    for (int i = 1; i <= k; i++) result *= (double)(n - k + i) / i;
    return result;
}

double* vna_pair_correlation(VnaLattice* lattice, int max_distance,
                              int* num_pairs_per_distance) {
    if (!lattice) return NULL;
    if (max_distance > lattice->width / 2) max_distance = lattice->width / 2;

    double* correlation = (double*)calloc(max_distance, sizeof(double));
    if (!correlation) return NULL;

    /* Compute mean */
    double mean = 0.0;
    int total = lattice->width * lattice->height;
    for (int i = 0; i < total; i++) mean += lattice->cells[i];
    mean /= total;

    double variance = 0.0;
    for (int i = 0; i < total; i++)
        variance += (lattice->cells[i] - mean) * (lattice->cells[i] - mean);
    variance /= total;

    if (variance < 1e-15) {
        for (int d = 0; d < max_distance; d++) correlation[d] = 0.0;
        return correlation;
    }

    /* For each distance, compute average product of deviations */
    for (int d = 0; d < max_distance; d++) {
        double sum = 0.0;
        int n_pairs = 0;
        for (int y = 0; y < lattice->height; y++) {
            for (int x = 0; x < lattice->width; x++) {
                int nx = (x + d) % lattice->width;
                double d1 = lattice->cells[y * lattice->width + x] - mean;
                double d2 = lattice->cells[y * lattice->width + nx] - mean;
                sum += d1 * d2;
                n_pairs++;
            }
        }
        correlation[d] = sum / (n_pairs * variance);
        if (num_pairs_per_distance) num_pairs_per_distance[d] = n_pairs;
    }
    return correlation;
}

double vna_cell_neighborhood_mi(VnaLattice* lattice, VnaNeighborhood* nb,
                                 VnaTransitionRule* rule) {
    /* Mutual information I(cell; N(cell))
     * = H(cell) + H(N(cell)) - H(cell, N(cell))
     * Simplified: uses rule lookup to compute joint distribution. */
    if (!lattice || !nb || !rule) return 0.0;

    int max_nb_configs = 100000;
    double* joint_counts = (double*)calloc(max_nb_configs, sizeof(double));
    double* cell_counts = (double*)calloc(rule->num_states, sizeof(double));
    double* nb_counts = (double*)calloc(max_nb_configs, sizeof(double));
    if (!joint_counts || !cell_counts || !nb_counts) {
        free(joint_counts); free(cell_counts); free(nb_counts);
        return 0.0;
    }

    /* Helper: compute index for neighbor state tuple */
    #define NB_INDEX(s) ({ \
        int64_t _idx = 0, _mul = 1; \
        for (int _i = 0; _i < nb->num_neighbors; _i++) { \
            _idx += s[_i] * _mul; _mul *= rule->num_states; \
        } \
        (int)(_idx % max_nb_configs); \
    })

    int total = lattice->width * lattice->height;
    int sample = total > 10000 ? 10000 : total;

    for (int i = 0; i < sample; i++) {
        int x = i % lattice->width;
        int y = i / lattice->width;
        uint8_t* nb_states = (uint8_t*)malloc(nb->num_neighbors);
        vna_collect_neighbors(lattice, x, y, 0, nb, nb_states);
        uint8_t cell = vna_lattice_get_cell(lattice, x, y, 0);
        int nb_idx = NB_INDEX(nb_states);
        joint_counts[nb_idx * rule->num_states + cell] += 1.0;
        cell_counts[cell] += 1.0;
        nb_counts[nb_idx] += 1.0;
        free(nb_states);
    }

    /* Compute entropies */
    double h_cell = 0.0, h_nb = 0.0, h_joint = 0.0;
    for (int s = 0; s < rule->num_states; s++) {
        double p = cell_counts[s] / sample;
        if (p > 1e-15) h_cell -= p * log2(p);
    }
    for (int i = 0; i < max_nb_configs; i++) {
        double p = nb_counts[i] / sample;
        if (p > 1e-15) h_nb -= p * log2(p);
        for (int s = 0; s < rule->num_states; s++) {
            double jp = joint_counts[i * rule->num_states + s] / sample;
            if (jp > 1e-15) h_joint -= jp * log2(jp);
        }
    }

    free(joint_counts); free(cell_counts); free(nb_counts);
    #undef NB_INDEX
    return h_cell + h_nb - h_joint;
}

int vna_detect_garden_of_eden(VnaLattice* lattice, VnaNeighborhood* nb,
                               VnaTransitionRule* rule) {
    /* A configuration is a Garden of Eden if it has no predecessor.
     * For 1D CA, we can use the de Bruijn diagram to detect GoE.
     * For 2D, this is undecidable in general.
     * Here we use a simple heuristic: if a cell's state cannot be
     * produced by any neighborhood configuration, mark it. */
    if (!lattice || !rule) return 0;
    int count = 0;
    /* Build set of reachable output states */
    bool reachable[256] = {false};
    uint8_t* nb_states = (uint8_t*)malloc(nb->num_neighbors);
    int max_configs = 1;
    for (int i = 0; i < nb->num_neighbors; i++) max_configs *= rule->num_states;
    if (max_configs > 1000000) max_configs = 1000000;

    for (int i = 0; i < max_configs; i++) {
        int tmp = i;
        for (int j = 0; j < nb->num_neighbors; j++) {
            nb_states[j] = tmp % rule->num_states;
            tmp /= rule->num_states;
        }
        uint8_t out = vna_rule_lookup(rule, nb_states);
        reachable[out] = true;
    }

    int total = lattice->width * lattice->height;
    for (int i = 0; i < total; i++) {
        if (!reachable[lattice->cells[i]]) count++;
    }
    free(nb_states);
    return count;
}

/* ============================================================================
 * L6: Canonical Pattern Detection
 * ============================================================================ */

bool vna_is_still_life(VnaLattice* lattice, VnaNeighborhood* nb,
                        VnaTransitionRule* rule) {
    if (!lattice || !rule) return false;
    VnaLattice* copy = vna_lattice_create(lattice->width, lattice->height,
                                           lattice->num_states,
                                           lattice->topology);
    if (!copy) return false;
    memcpy(copy->cells, lattice->cells,
           lattice->width * lattice->height);
    vna_lattice_evolve(copy, nb, rule);
    int total = lattice->width * lattice->height;
    bool still = true;
    for (int i = 0; i < total; i++) {
        if (lattice->cells[i] != copy->cells[i]) { still = false; break; }
    }
    vna_lattice_free(copy);
    return still;
}

int vna_detect_oscillator(VnaLattice* lattice, VnaNeighborhood* nb,
                           VnaTransitionRule* rule, int max_period) {
    if (!lattice || !rule || max_period <= 0) return 0;

    /* Store snapshots and check for repeats */
    int total = lattice->width * lattice->height;
    uint8_t** history = (uint8_t**)calloc(max_period + 1, sizeof(uint8_t*));
    if (!history) return 0;

    for (int t = 0; t <= max_period; t++) {
        history[t] = (uint8_t*)malloc(total);
        if (!history[t]) {
            for (int j = 0; j < t; j++) free(history[j]);
            free(history);
            return 0;
        }
        memcpy(history[t], lattice->cells, total);
        vna_lattice_evolve(lattice, nb, rule);

        /* Check for match with any previous snapshot */
        for (int prev = 0; prev <= t; prev++) {
            if (memcmp(history[prev], lattice->cells, total) == 0) {
                int period = t - prev + 1;
                for (int j = 0; j <= t; j++) free(history[j]);
                free(history);
                return period;
            }
        }
    }

    for (int j = 0; j <= max_period; j++) free(history[j]);
    free(history);
    return 0;
}

int vna_detect_gliders(VnaLattice* lattice, VnaNeighborhood* nb,
                        VnaTransitionRule* rule, int* glider_positions,
                        int max_gliders) {
    if (!lattice || !rule) return 0;

    /* Glider detection: compare lattice at t and t+1.
     * A glider is a pattern that translates by a fixed vector (dx,dy)
     * each period while maintaining its shape.
     * Simplified: look for regions where the state shifted spatially. */
    VnaLattice* next = vna_lattice_create(lattice->width, lattice->height,
                                           lattice->num_states,
                                           lattice->topology);
    if (!next) return 0;
    memcpy(next->cells, lattice->cells,
           lattice->width * lattice->height);
    vna_lattice_evolve(next, nb, rule);

    int found = 0;
    /* Check for 1-cell shift in all 8 directions */
    int dx_vals[] = {0, 1, 1, 0, -1, -1, -1, 0, 1};
    int dy_vals[] = {0, 0, -1, -1, -1, 0, 1, 1, 1};

    for (int dir = 0; dir < 9 && found < max_gliders; dir++) {
        int dx = dx_vals[dir], dy = dy_vals[dir];
        if (dx == 0 && dy == 0) continue;

        int matches = 0;
        for (int y = 1; y < lattice->height - 1; y++) {
            for (int x = 1; x < lattice->width - 1; x++) {
                uint8_t orig = vna_lattice_get_cell(lattice, x, y, 0);
                uint8_t moved = vna_lattice_get_cell(next, x + dx, y + dy, 0);
                if (orig > 0 && orig == moved) matches++;
            }
        }
        if (matches > 10 && found < max_gliders) {
            if (glider_positions) {
                glider_positions[found * 3] = dx;
                glider_positions[found * 3 + 1] = dy;
                glider_positions[found * 3 + 2] = matches;
            }
            found++;
        }
    }

    vna_lattice_free(next);
    return found;
}
