#include "sos_autocatalytic.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ===== Catalytic Network Functions ===== */

CatalyticNetwork* cn_create(void) {
    CatalyticNetwork* cn = (CatalyticNetwork*)calloc(1, sizeof(CatalyticNetwork));
    return cn;
}

void cn_free(CatalyticNetwork* cn) {
    if (!cn) return;
    for (int i = 0; i < cn->n_molecules; i++)
        free(cn->molecules[i].name);
    free(cn->molecules);
    free(cn->reactions);
    if (cn->catalysis_matrix) {
        for (int i = 0; i < cn->n_molecules; i++)
            free(cn->catalysis_matrix[i]);
        free(cn->catalysis_matrix);
    }
    if (cn->reaction_products) {
        for (int i = 0; i < cn->n_reactions; i++)
            free(cn->reaction_products[i]);
        free(cn->reaction_products);
    }
    free(cn->food_set);
    free(cn->raf_set);
    free(cn);
}

int cn_add_molecule(CatalyticNetwork* cn, const char* name, MoleculeType type) {
    if (!cn || !name) return -1;
    int idx = cn->n_molecules++;
    cn->molecules = (Molecule*)realloc(cn->molecules,
                                        cn->n_molecules * sizeof(Molecule));
    cn->molecules[idx].id = idx;
    cn->molecules[idx].name = strdup(name);
    cn->molecules[idx].type = type;
    cn->molecules[idx].concentration = 1.0;
    cn->molecules[idx].formation_rate = 0.0;
    cn->molecules[idx].decay_rate = 0.01;
    /* Add row to catalysis matrix */
    cn->catalysis_matrix = (int**)realloc(cn->catalysis_matrix,
                                            cn->n_molecules * sizeof(int*));
    cn->catalysis_matrix[idx] = (int*)calloc(cn->n_reactions + 1, sizeof(int));
    return idx;
}

int cn_add_reaction(CatalyticNetwork* cn, int sub1, int sub2,
                    int product, int catalyst, double k_cat, double k_uncat) {
    if (!cn) return -1;
    int idx = cn->n_reactions++;
    cn->reactions = (CatalyzedReaction*)realloc(cn->reactions,
                                                  cn->n_reactions * sizeof(CatalyzedReaction));
    cn->reactions[idx].id = idx;
    cn->reactions[idx].n_substrates = (sub1 >= 0 ? 1 : 0) + (sub2 >= 0 ? 1 : 0);
    cn->reactions[idx].substrate_ids[0] = sub1;
    cn->reactions[idx].substrate_ids[1] = sub2;
    cn->reactions[idx].product_id = product;
    cn->reactions[idx].catalyst_id = catalyst;
    cn->reactions[idx].rate_constant = k_cat;
    cn->reactions[idx].catalytic_power = (k_uncat > 1e-15) ? k_cat / k_uncat : 1e6;

    /* Expand all existing catalysis matrix rows to include new reaction column */
    for (int i = 0; i < cn->n_molecules; i++) {
        cn->catalysis_matrix[i] = (int*)realloc(cn->catalysis_matrix[i],
                                                  cn->n_reactions * sizeof(int));
        cn->catalysis_matrix[i][idx] = 0;
    }

    /* Mark catalyst for this reaction */
    if (catalyst >= 0 && catalyst < cn->n_molecules)
        cn->catalysis_matrix[catalyst][idx] = 1;

    /* Expand reaction products */
    cn->reaction_products = (int**)realloc(cn->reaction_products,
                                             cn->n_reactions * sizeof(int*));
    cn->reaction_products[idx] = (int*)calloc(cn->n_molecules, sizeof(int));
    if (product >= 0 && product < cn->n_molecules)
        cn->reaction_products[idx][product] = 1;
    return idx;
}

void cn_set_food(CatalyticNetwork* cn, const int* food_ids, int n_food) {
    if (!cn) return;
    free(cn->food_set);
    cn->n_food = n_food;
    cn->food_set = (int*)malloc(n_food * sizeof(int));
    memcpy(cn->food_set, food_ids, n_food * sizeof(int));
}

int cn_find_raf(CatalyticNetwork* cn) {
    /* Hordijk-Steel (2004) RAF algorithm:
     * Iteratively remove reactions that are not catalyzed or have
     * substrates not in the closure of the food set.
     * The remaining set is the maximal RAF. */
    if (!cn || cn->n_molecules == 0) return 0;
    free(cn->raf_set);
    cn->raf_set = (int*)calloc(cn->n_molecules, sizeof(int));

    /* Start with food set */
    for (int i = 0; i < cn->n_food; i++)
        cn->raf_set[cn->food_set[i]] = 1;

    int changed = 1;
    while (changed) {
        changed = 0;
        for (int r = 0; r < cn->n_reactions; r++) {
            if (cn->reactions[r].catalyst_id < 0) continue;
            if (!cn->raf_set[cn->reactions[r].catalyst_id]) continue;
            int all_subs = 1;
            for (int s = 0; s < cn->reactions[r].n_substrates; s++) {
                int sub = cn->reactions[r].substrate_ids[s];
                if (sub >= 0 && !cn->raf_set[sub]) { all_subs = 0; break; }
            }
            if (all_subs) {
                int prod = cn->reactions[r].product_id;
                if (prod >= 0 && !cn->raf_set[prod]) {
                    cn->raf_set[prod] = 1;
                    changed = 1;
                }
            }
        }
    }
    cn->n_raf = 0;
    for (int i = 0; i < cn->n_molecules; i++)
        if (cn->raf_set[i]) cn->n_raf++;
    cn->has_raf = (cn->n_raf > cn->n_food) ? 1 : 0;
    return cn->n_raf;
}

int cn_is_autocatalytic(const CatalyticNetwork* cn) {
    if (!cn) return 0;
    return cn->has_raf;
}

int cn_catalytic_closure(CatalyticNetwork* cn, int* closure_set) {
    if (!cn || !closure_set) return 0;
    /* Initialize with food set */
    for (int i = 0; i < cn->n_molecules; i++) closure_set[i] = 0;
    for (int i = 0; i < cn->n_food; i++)
        closure_set[cn->food_set[i]] = 1;

    int changed = 1;
    while (changed) {
        changed = 0;
        for (int r = 0; r < cn->n_reactions; r++) {
            int all_subs = 1;
            for (int s = 0; s < cn->reactions[r].n_substrates; s++) {
                int sub = cn->reactions[r].substrate_ids[s];
                if (sub >= 0 && !closure_set[sub]) { all_subs = 0; break; }
            }
            if (all_subs) {
                int prod = cn->reactions[r].product_id;
                if (prod >= 0 && !closure_set[prod]) {
                    closure_set[prod] = 1; changed = 1;
                }
            }
        }
    }
    int count = 0;
    for (int i = 0; i < cn->n_molecules; i++)
        if (closure_set[i]) count++;
    return count;
}

void cn_simulate_dynamics(CatalyticNetwork* cn, double dt, int steps) {
    if (!cn || steps <= 0) return;
    double* dx = (double*)malloc(cn->n_molecules * sizeof(double));
    for (int s = 0; s < steps; s++) {
        for (int i = 0; i < cn->n_molecules; i++) dx[i] = 0.0;
        for (int r = 0; r < cn->n_reactions; r++) {
            double rate = cn->reactions[r].rate_constant;
            double conc_prod = 1.0;
            for (int si = 0; si < cn->reactions[r].n_substrates; si++) {
                int sub = cn->reactions[r].substrate_ids[si];
                if (sub >= 0)
                    conc_prod *= cn->molecules[sub].concentration;
            }
            if (cn->reactions[r].catalyst_id >= 0)
                conc_prod *= cn->molecules[cn->reactions[r].catalyst_id].concentration;
            double flux = rate * conc_prod;
            for (int si = 0; si < cn->reactions[r].n_substrates; si++) {
                int sub = cn->reactions[r].substrate_ids[si];
                if (sub >= 0) dx[sub] -= flux;
            }
            int prod = cn->reactions[r].product_id;
            if (prod >= 0) dx[prod] += flux;
        }
        for (int i = 0; i < cn->n_molecules; i++) {
            dx[i] += cn->molecules[i].formation_rate;
            dx[i] -= cn->molecules[i].decay_rate * cn->molecules[i].concentration;
            cn->molecules[i].concentration += dx[i] * dt;
            if (cn->molecules[i].concentration < 0.0)
                cn->molecules[i].concentration = 0.0;
        }
    }
    free(dx);
}

double cn_get_concentration(const CatalyticNetwork* cn, int mol_id) {
    if (!cn || mol_id < 0 || mol_id >= cn->n_molecules) return 0.0;
    return cn->molecules[mol_id].concentration;
}

/* ===== Hypercycle Functions ===== */

Hypercycle* hc_create(int n_species) {
    Hypercycle* hc = (Hypercycle*)calloc(1, sizeof(Hypercycle));
    if (!hc) return NULL;
    hc->n_species = n_species;
    hc->concentrations = (double*)calloc(n_species, sizeof(double));
    hc->fitness = (double*)malloc(n_species * sizeof(double));
    hc->quality = (double*)malloc(n_species * sizeof(double));
    hc->coupling = (double**)malloc(n_species * sizeof(double*));
    for (int i = 0; i < n_species; i++) {
        hc->coupling[i] = (double*)calloc(n_species, sizeof(double));
        hc->fitness[i] = 1.0;
        hc->quality[i] = 1.0;
        hc->concentrations[i] = 1.0 / n_species;
    }
    hc->total_concentration = 1.0;
    hc->mutation_rate = 0.0;
    hc->mean_fitness = 1.0;
    hc->coexist = 1;
    return hc;
}

void hc_free(Hypercycle* hc) {
    if (!hc) return;
    for (int i = 0; i < hc->n_species; i++) free(hc->coupling[i]);
    free(hc->coupling); free(hc->concentrations);
    free(hc->fitness); free(hc->quality);
    free(hc);
}

void hc_set_fitness(Hypercycle* hc, const double* fitness) {
    if (!hc || !fitness) return;
    memcpy(hc->fitness, fitness, hc->n_species * sizeof(double));
}

void hc_set_quality(Hypercycle* hc, const double* quality) {
    if (!hc || !quality) return;
    memcpy(hc->quality, quality, hc->n_species * sizeof(double));
}

void hc_set_coupling(Hypercycle* hc, double strength) {
    if (!hc) return;
    for (int i = 0; i < hc->n_species; i++)
        for (int j = 0; j < hc->n_species; j++)
            hc->coupling[i][j] = (i != j) ? strength : 0.0;
    /* Hypercycle: i catalyzes (i+1) mod n */
    for (int i = 0; i < hc->n_species; i++)
        hc->coupling[i][(i+1) % hc->n_species] = strength;
}

void hc_set_mutation_rate(Hypercycle* hc, double mu) {
    if (hc) hc->mutation_rate = mu;
}

void hc_evolve(Hypercycle* hc, double dt) {
    if (!hc) return;
    double* dx = (double*)malloc(hc->n_species * sizeof(double));
    double phi = 0.0; /* Mean fitness: outflow term */
    for (int i = 0; i < hc->n_species; i++)
        phi += hc->fitness[i] * hc->concentrations[i];

    for (int i = 0; i < hc->n_species; i++) {
        double self = hc->concentrations[i] * (hc->fitness[i]*hc->quality[i] - phi);
        double coupling_term = 0.0;
        for (int j = 0; j < hc->n_species; j++)
            if (i != j)
                coupling_term += hc->coupling[j][i] * hc->concentrations[j];
        dx[i] = self + coupling_term;
    }
    for (int i = 0; i < hc->n_species; i++) {
        hc->concentrations[i] += dx[i] * dt;
        if (hc->concentrations[i] < 1e-15) hc->concentrations[i] = 0.0;
    }
    hc->mean_fitness = phi;
    hc->coexist = hc_check_coexistence(hc, 1e-6);
    free(dx);
}

int hc_check_coexistence(const Hypercycle* hc, double threshold) {
    if (!hc) return 0;
    for (int i = 0; i < hc->n_species; i++)
        if (hc->concentrations[i] < threshold) return 0;
    return 1;
}

double hc_error_threshold(const Hypercycle* hc) {
    /* Eigen (1971): Q_min = 1 / sigma_m
     * where sigma_m = A_master / A_mutant_max */
    if (!hc || hc->n_species < 2) return 0.0;
    double max_fit = hc->fitness[0];
    for (int i = 1; i < hc->n_species; i++)
        if (hc->fitness[i] > max_fit) max_fit = hc->fitness[i];
    double second_max = 0.0;
    for (int i = 0; i < hc->n_species; i++)
        if (hc->fitness[i] < max_fit && hc->fitness[i] > second_max)
            second_max = hc->fitness[i];
    if (second_max < 1e-15) return 1.0;
    double sigma = max_fit / second_max;
    return 1.0 / sigma;
}

/* ===== NK Fitness Landscape Functions ===== */

NKLandscape* nk_create(int N, int K) {
    NKLandscape* nk = (NKLandscape*)calloc(1, sizeof(NKLandscape));
    if (!nk) return NULL;
    nk->N = N; nk->K = K;
    nk->genome_len = N;
    /* fitness_table[N][2^(K+1)] */
    int contexts = 1 << (K + 1);
    nk->fitness_table = (double***)malloc(N * sizeof(double**));
    for (int i = 0; i < N; i++) {
        nk->fitness_table[i] = (double**)malloc(contexts * sizeof(double*));
        for (int c = 0; c < contexts; c++) {
            /* Each context has one random fitness value */
            nk->fitness_table[i][c] = (double*)malloc(sizeof(double));
            nk->fitness_table[i][c][0] = (double)rand() / RAND_MAX;
        }
    }
    return nk;
}

void nk_free(NKLandscape* nk) {
    if (!nk) return;
    int contexts = 1 << (nk->K + 1);
    for (int i = 0; i < nk->N; i++) {
        for (int c = 0; c < contexts; c++)
            free(nk->fitness_table[i][c]);
        free(nk->fitness_table[i]);
    }
    free(nk->fitness_table);
    free(nk);
}

double nk_fitness(const NKLandscape* nk, const int* genome) {
    if (!nk || !genome) return 0.0;
    double total = 0.0;
    for (int i = 0; i < nk->N; i++) {
        /* Build context from gene i and K neighbors to the right */
        int context = 0;
        for (int k = 0; k <= nk->K; k++) {
            int neighbor = (i + k) % nk->N;
            context = (context << 1) | (genome[neighbor] ? 1 : 0);
        }
        total += nk->fitness_table[i][context][0];
    }
    return total / nk->N;
}

void nk_random_genome(const NKLandscape* nk, int* genome) {
    if (!nk || !genome) return;
    for (int i = 0; i < nk->N; i++)
        genome[i] = (rand() % 2);
}

int nk_local_optimum(const NKLandscape* nk, int* genome, int max_steps) {
    if (!nk || !genome) return 0;
    int steps = 0;
    while (steps < max_steps) {
        double current = nk_fitness(nk, genome);
        int best_flip = -1;
        double best_fit = current;
        for (int i = 0; i < nk->N; i++) {
            genome[i] = !genome[i];
            double new_fit = nk_fitness(nk, genome);
            if (new_fit > best_fit) { best_fit = new_fit; best_flip = i; }
            genome[i] = !genome[i];
        }
        if (best_flip < 0) break;
        genome[best_flip] = !genome[best_flip];
        steps++;
    }
    return steps;
}

int nk_count_local_optima(const NKLandscape* nk, int n_samples) {
    if (!nk) return 0;
    int* genome = (int*)malloc(nk->N * sizeof(int));
    int count = 0;
    for (int s = 0; s < n_samples; s++) {
        nk_random_genome(nk, genome);
        nk_local_optimum(nk, genome, nk->N * 10);
        /* Check if it's a true local optimum (no 1-bit improvement) */
        double current = nk_fitness(nk, genome);
        int is_opt = 1;
        for (int i = 0; i < nk->N; i++) {
            genome[i] = !genome[i];
            if (nk_fitness(nk, genome) > current) { is_opt = 0; genome[i] = !genome[i]; break; }
            genome[i] = !genome[i];
        }
        if (is_opt) count++;
    }
    free(genome);
    return count;
}

double nk_fitness_autocorrelation(const NKLandscape* nk, int walk_length) {
    if (!nk || walk_length <= 0) return 0.0;
    int* g = (int*)malloc(nk->N * sizeof(int));
    double* fits = (double*)malloc(walk_length * sizeof(double));
    nk_random_genome(nk, g);
    for (int w = 0; w < walk_length; w++) {
        fits[w] = nk_fitness(nk, g);
        /* Random 1-bit flip */
        int bit = rand() % nk->N;
        g[bit] = !g[bit];
    }
    double mean = 0.0;
    for (int w = 0; w < walk_length; w++) mean += fits[w];
    mean /= walk_length;
    double num = 0.0, den = 0.0;
    for (int w = 0; w < walk_length - 1; w++) {
        num += (fits[w] - mean) * (fits[w+1] - mean);
        den += (fits[w] - mean) * (fits[w] - mean);
    }
    free(g); free(fits);
    return (fabs(den) > 1e-15) ? num / den : 0.0;
}
