#include "vna_neighborhood.h"
#include "vna_core.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Neighborhood Creation
 * ============================================================================ */

VnaNeighborhood* vna_neighborhood_create_standard(VnaNeighborhoodType type,
                                                   int radius) {
    return vna_neighborhood_create(type, radius, 2);
}

VnaNeighborhood* vna_neighborhood_create_custom(int capacity) {
    VnaNeighborhood* nb = (VnaNeighborhood*)calloc(1, sizeof(VnaNeighborhood));
    if (!nb) return NULL;
    nb->type = VNA_CUSTOM;
    nb->radius = 0;
    nb->num_neighbors = 0;
    nb->include_self = true;
    nb->offset_dx = (int*)malloc(capacity * sizeof(int));
    nb->offset_dy = (int*)malloc(capacity * sizeof(int));
    nb->offset_dz = NULL;
    if (!nb->offset_dx || !nb->offset_dy) {
        vna_neighborhood_free(nb);
        return NULL;
    }
    return nb;
}

void vna_neighborhood_add_offset(VnaNeighborhood* nb, int dx, int dy, int dz) {
    if (!nb) return;
    /* Resize if needed (simple doubling strategy) */
    int cur_cap = nb->num_neighbors > 0 ? nb->num_neighbors : 8;
    if (nb->num_neighbors >= cur_cap) {
        /* Conservative estimate: we don't track capacity separately */
    }
    int n = nb->num_neighbors;
    nb->offset_dx = (int*)realloc(nb->offset_dx, (n + 1) * sizeof(int));
    nb->offset_dy = (int*)realloc(nb->offset_dy, (n + 1) * sizeof(int));
    nb->offset_dx[n] = dx;
    nb->offset_dy[n] = dy;
    nb->num_neighbors = n + 1;
}

void vna_neighborhood_finalize(VnaNeighborhood* nb) {
    /* No-op: compaction could be done here if we tracked capacity */
    (void)nb;
}

/* ============================================================================
 * L2: Neighborhood Properties
 * ============================================================================ */

int vna_von_neumann_count(int radius) {
    /* Number of integer lattice points with |dx| + |dy| ≤ r */
    return 2 * radius * (radius + 1) + 1;
}

int vna_moore_count(int radius) {
    /* Number of integer lattice points with max(|dx|,|dy|) ≤ r */
    int side = 2 * radius + 1;
    return side * side;
}

int64_t vna_rule_space_size(int num_states, int num_neighbors) {
    /* The number of possible CA rules = |S|^(|S|^|N|)
     * This grows hyper-exponentially: for binary CA with 9 neighbors,
     * there are 2^512 ≈ 10^154 possible rules. */
    int64_t num_configs = 1;
    for (int i = 0; i < num_neighbors; i++) {
        num_configs *= num_states;
        if (num_configs > 1LL << 60) return -1; /* Overflow */
    }
    /* The full rule space size is num_states^num_configs, which is
     * astronomically large. Return just the number of configurations. */
    return num_configs;
}

/* ============================================================================
 * L3: Geometric and Topological Operations
 * ============================================================================ */

void vna_neighborhood_bbox(VnaNeighborhood* nb, int* min_dx, int* max_dx,
                            int* min_dy, int* max_dy, int* min_dz, int* max_dz) {
    if (!nb || nb->num_neighbors == 0) return;
    *min_dx = *max_dx = nb->offset_dx[0];
    *min_dy = *max_dy = nb->offset_dy[0];
    if (min_dz && max_dz && nb->offset_dz) {
        *min_dz = *max_dz = nb->offset_dz[0];
    }
    for (int i = 1; i < nb->num_neighbors; i++) {
        if (nb->offset_dx[i] < *min_dx) *min_dx = nb->offset_dx[i];
        if (nb->offset_dx[i] > *max_dx) *max_dx = nb->offset_dx[i];
        if (nb->offset_dy[i] < *min_dy) *min_dy = nb->offset_dy[i];
        if (nb->offset_dy[i] > *max_dy) *max_dy = nb->offset_dy[i];
        if (min_dz && max_dz && nb->offset_dz) {
            if (nb->offset_dz[i] < *min_dz) *min_dz = nb->offset_dz[i];
            if (nb->offset_dz[i] > *max_dz) *max_dz = nb->offset_dz[i];
        }
    }
}

bool vna_neighborhood_is_isotropic(VnaNeighborhood* nb) {
    /* A neighborhood is isotropic under 90° rotations if for every
     * offset (dx,dy), the rotated version (-dy,dx) is also present. */
    if (!nb || nb->num_neighbors == 0) return true;
    for (int i = 0; i < nb->num_neighbors; i++) {
        int dx = nb->offset_dx[i];
        int dy = nb->offset_dy[i];
        int rx = -dy;
        int ry = dx;
        bool found = false;
        for (int j = 0; j < nb->num_neighbors; j++) {
            if (nb->offset_dx[j] == rx && nb->offset_dy[j] == ry) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

VnaNeighborhoodGraph* vna_neighborhood_graph(VnaNeighborhood* nb) {
    if (!nb) return NULL;
    VnaNeighborhoodGraph* ng =
        (VnaNeighborhoodGraph*)calloc(1, sizeof(VnaNeighborhoodGraph));
    if (!ng) return NULL;
    ng->num_vertices = nb->num_neighbors;
    int v = ng->num_vertices;
    ng->adjacency_matrix = (int*)calloc(v * v, sizeof(int));
    if (!ng->adjacency_matrix) { free(ng); return NULL; }

    /* Two neighbor offsets are adjacent if they share an edge
     * (Manhattan distance = 1). */
    for (int i = 0; i < v; i++) {
        for (int j = i + 1; j < v; j++) {
            int mdist = abs(nb->offset_dx[i] - nb->offset_dx[j]) +
                        abs(nb->offset_dy[i] - nb->offset_dy[j]);
            if (mdist == 1) {
                ng->adjacency_matrix[i * v + j] = 1;
                ng->adjacency_matrix[j * v + i] = 1;
                ng->num_edges++;
            }
        }
    }
    return ng;
}

void vna_neighborhood_graph_free(VnaNeighborhoodGraph* ng) {
    if (!ng) return;
    free(ng->adjacency_matrix);
    free(ng);
}

/* ============================================================================
 * L2: Neighborhood Transformations
 * ============================================================================ */

VnaNeighborhood* vna_neighborhood_reflect(VnaNeighborhood* nb, bool horizontal) {
    if (!nb) return NULL;
    VnaNeighborhood* reflected = vna_neighborhood_create_custom(nb->num_neighbors);
    if (!reflected) return NULL;
    for (int i = 0; i < nb->num_neighbors; i++) {
        int dx = horizontal ? -nb->offset_dx[i] : nb->offset_dx[i];
        int dy = horizontal ? nb->offset_dy[i] : -nb->offset_dy[i];
        vna_neighborhood_add_offset(reflected, dx, dy, 0);
    }
    reflected->type = nb->type;
    reflected->radius = nb->radius;
    reflected->include_self = nb->include_self;
    return reflected;
}

VnaNeighborhood* vna_neighborhood_rotate(VnaNeighborhood* nb, int quarter_turns) {
    if (!nb) return NULL;
    quarter_turns = ((quarter_turns % 4) + 4) % 4;
    if (quarter_turns == 0) {
        VnaNeighborhood* copy = vna_neighborhood_create_custom(nb->num_neighbors);
        if (!copy) return NULL;
        for (int i = 0; i < nb->num_neighbors; i++)
            vna_neighborhood_add_offset(copy, nb->offset_dx[i],
                                        nb->offset_dy[i], 0);
        copy->type = nb->type;
        copy->radius = nb->radius;
        return copy;
    }
    VnaNeighborhood* rotated = vna_neighborhood_create_custom(nb->num_neighbors);
    if (!rotated) return NULL;
    for (int i = 0; i < nb->num_neighbors; i++) {
        int dx = nb->offset_dx[i];
        int dy = nb->offset_dy[i];
        int nx, ny;
        switch (quarter_turns) {
            case 1: nx = -dy; ny = dx; break;
            case 2: nx = -dx; ny = -dy; break;
            case 3: nx = dy; ny = -dx; break;
            default: nx = dx; ny = dy; break;
        }
        vna_neighborhood_add_offset(rotated, nx, ny, 0);
    }
    rotated->type = nb->type;
    rotated->radius = nb->radius;
    rotated->include_self = nb->include_self;
    return rotated;
}

void vna_neighborhood_symmetrize(VnaNeighborhood* nb) {
    if (!nb) return;
    int orig_count = nb->num_neighbors;
    /* For each existing offset, add its 90°, 180°, 270° rotations
     * and its horizontal and vertical reflections. */
    for (int i = 0; i < orig_count; i++) {
        int dx = nb->offset_dx[i];
        int dy = nb->offset_dy[i];

        /* 90° rotations */
        int sym_dx[] = {-dy, -dx, dy, -dx, dx};
        int sym_dy[] = {dx, -dy, -dx, dy, -dy};

        for (int s = 0; s < 5; s++) {
            int sx = sym_dx[s], sy = sym_dy[s];
            bool exists = false;
            for (int j = 0; j < nb->num_neighbors; j++) {
                if (nb->offset_dx[j] == sx && nb->offset_dy[j] == sy) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                vna_neighborhood_add_offset(nb, sx, sy, 0);
            }
        }
    }
}

/* ============================================================================
 * L2: Neighborhood Comparison
 * ============================================================================ */

bool vna_neighborhood_equivalent(VnaNeighborhood* a, VnaNeighborhood* b) {
    if (!a || !b) return false;
    if (a->num_neighbors != b->num_neighbors) return false;

    /* Check if a and b are equivalent up to rotation/reflection.
     * Try all 8 symmetries of the square (D4 group). */
    int sym_dx[] = {1, 0, -1, 0, 1, 0, -1, 0};
    int sym_dy[] = {0, 1, 0, -1, 0, -1, 0, 1};
    int sym_mirror[] = {0, 0, 0, 0, 1, 1, 1, 1}; /* 0=identity, 1=reflect */

    for (int sym = 0; sym < 8; sym++) {
        bool match = true;
        for (int i = 0; i < a->num_neighbors && match; i++) {
            int ax = a->offset_dx[i], ay = a->offset_dy[i];
            /* Apply symmetry transformation to ax, ay */
            int tx = ax, ty = ay;
            if (sym_mirror[sym]) { tx = -tx; } /* Horizontal reflection */
            int qx = tx * sym_dx[sym] - ty * sym_dy[sym];
            int qy = tx * sym_dy[sym] + ty * sym_dx[sym];

            bool found = false;
            for (int j = 0; j < b->num_neighbors; j++) {
                if (b->offset_dx[j] == qx && b->offset_dy[j] == qy) {
                    found = true;
                    break;
                }
            }
            if (!found) match = false;
        }
        if (match) return true;
    }
    return false;
}

double vna_neighborhood_overlap(VnaNeighborhood* a, VnaNeighborhood* b) {
    if (!a || !b) return 0.0;
    int overlap = 0;
    for (int i = 0; i < a->num_neighbors; i++) {
        for (int j = 0; j < b->num_neighbors; j++) {
            if (a->offset_dx[i] == b->offset_dx[j] &&
                a->offset_dy[i] == b->offset_dy[j]) {
                overlap++;
                break;
            }
        }
    }
    int union_size = a->num_neighbors + b->num_neighbors - overlap;
    return (union_size > 0) ? (double)overlap / union_size : 0.0;
}

/* ============================================================================
 * L5: Neighborhood Optimization
 * ============================================================================ */

int* vna_neighborhood_lookup_table(VnaNeighborhood* nb) {
    if (!nb) return NULL;
    int min_dx, max_dx, min_dy, max_dy;
    vna_neighborhood_bbox(nb, &min_dx, &max_dx, &min_dy, &max_dy, NULL, NULL);
    int table_w = max_dx - min_dx + 1;
    int table_h = max_dy - min_dy + 1;
    int table_size = table_w * table_h;
    int* lookup = (int*)malloc(table_size * sizeof(int));
    if (!lookup) return NULL;
    for (int i = 0; i < table_size; i++) lookup[i] = -1;

    for (int i = 0; i < nb->num_neighbors; i++) {
        int col = nb->offset_dx[i] - min_dx;
        int row = nb->offset_dy[i] - min_dy;
        lookup[row * table_w + col] = i;
    }
    return lookup;
}

int64_t vna_neighbor_tuple_to_index(const uint8_t* states, int num_states,
                                     int num_neighbors) {
    int64_t index = 0;
    int64_t multiplier = 1;
    for (int i = 0; i < num_neighbors; i++) {
        index += (int64_t)states[i] * multiplier;
        multiplier *= num_states;
    }
    return index;
}

void vna_index_to_neighbor_tuple(int64_t index, int num_states,
                                  int num_neighbors, uint8_t* states_out) {
    for (int i = 0; i < num_neighbors; i++) {
        states_out[i] = (uint8_t)(index % num_states);
        index /= num_states;
    }
}

int vna_neighbor_tuple_hamming(const uint8_t* a, const uint8_t* b,
                                int num_neighbors) {
    int dist = 0;
    for (int i = 0; i < num_neighbors; i++)
        if (a[i] != b[i]) dist++;
    return dist;
}

/* ============================================================================
 * L8: Advanced Neighborhood Types
 * ============================================================================ */

VnaNeighborhood* vna_neighborhood_circular(double radius, bool include_self) {
    if (radius < 0) radius = 1.0;
    int ir = (int)ceil(radius);
    /* Count cells within Euclidean distance */
    int count = 0;
    for (int dy = -ir; dy <= ir; dy++)
        for (int dx = -ir; dx <= ir; dx++)
            if (sqrt(dx*dx + dy*dy) <= radius + 1e-9)
                if (dx != 0 || dy != 0 || include_self)
                    count++;

    VnaNeighborhood* nb = vna_neighborhood_create_custom(count);
    if (!nb) return NULL;
    nb->radius = ir;
    nb->include_self = include_self;
    for (int dy = -ir; dy <= ir; dy++)
        for (int dx = -ir; dx <= ir; dx++)
            if (sqrt(dx*dx + dy*dy) <= radius + 1e-9)
                if (dx != 0 || dy != 0 || include_self)
                    vna_neighborhood_add_offset(nb, dx, dy, 0);
    nb->type = VNA_CUSTOM;
    return nb;
}

VnaNeighborhood* vna_neighborhood_gaussian(double sigma, int truncate_radius) {
    if (sigma <= 0) sigma = 1.0;
    if (truncate_radius <= 0) truncate_radius = (int)ceil(3.0 * sigma);
    int count = 0;
    int r = truncate_radius;
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++)
            if (exp(-(dx*dx + dy*dy) / (2.0 * sigma * sigma)) > 0.01)
                count++;

    VnaNeighborhood* nb = vna_neighborhood_create_custom(count);
    if (!nb) return NULL;
    nb->radius = r;
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++) {
            double w = exp(-(dx*dx + dy*dy) / (2.0 * sigma * sigma));
            if (w > 0.01)
                vna_neighborhood_add_offset(nb, dx, dy, 0);
        }
    nb->type = VNA_CUSTOM;
    return nb;
}
