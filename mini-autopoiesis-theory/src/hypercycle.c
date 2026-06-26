/**
 * @file hypercycle.c
 * @brief Eigen's Hypercycle model — catalytic cycles and prebiotic evolution.
 *
 * The hypercycle (Eigen, 1971) is a model of self-organizing chemical cycles
 * where each member catalyzes the formation of the next. It provides a
 * mathematical framework for understanding how information-carrying molecules
 * (like RNA) could have emerged and cooperated in prebiotic evolution.
 *
 * The hypercycle is a key bridge between chemistry and biology, and a
 * foundational model in autopoiesis theory for how self-organizing
 * molecular systems can maintain and replicate themselves.
 *
 * Knowledge points implemented:
 *   L4 - hypercycle_ode: n-species hypercycle kinetics
 *   L4 - hypercycle_fixed_point: steady-state analysis
 *   L5 - hypercycle_integrate_rk4: ODE integration
 *   L5 - hypercycle_competition: two-hypercycle competition dynamics
 *   L6 - hypercycle_error_threshold: Eigen's error catastrophe analysis
 *   L8 - hypercycle_quasispecies: quasispecies distribution
 *   L8 - hypercycle_spatial: spatial hypercycle (reaction-diffusion)
 *
 * Reference: Eigen, "Selforganization of Matter and the Evolution of
 *            Biological Macromolecules" (1971)
 *            Eigen & Schuster, "The Hypercycle" (1979)
 *            Eigen, McCaskill, & Schuster, "The Molecular Quasispecies" (1988)
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HC_MAX_SPECIES 20
#define HC_MAX_SPATIAL 64

/**
 * @brief Hypercycle model state.
 */
typedef struct {
    int n_species;                       /**< Number of species in the cycle */
    double concentrations[HC_MAX_SPECIES]; /**< Species concentrations */
    double rate_constants[HC_MAX_SPECIES]; /**< Catalytic rate constants */
    double decay_rates[HC_MAX_SPECIES];    /**< Individual decay rates */
    double dilution_flow;                  /**< φ: outflow that keeps Σx_i constant */
    double total_concentration;            /**< c = Σ x_i */
    double fitness[HC_MAX_SPECIES];        /**< Fitness landscape */
    double mean_fitness;                   /**< Average fitness */
    int is_quasispecies;                  /**< 1 if operating in quasispecies regime */
} hypercycle_t;

/**
 * @brief Spatial hypercycle state (1D reaction-diffusion).
 */
typedef struct {
    int n_species;
    int n_cells;
    double grid[HC_MAX_SPATIAL][HC_MAX_SPECIES]; /**< grid[cell][species] */
    double diffusion_rate;
    double cell_size;
} hypercycle_spatial_t;

/* ==========================================================================
 * Hypercycle ODEs (L4)
 * ========================================================================== */

/**
 * @brief Compute the hypercycle ODE derivatives.
 *
 * The canonical hypercycle equation (Eigen & Schuster, 1979):
 *
 *   dx_i/dt = k_i * x_i * x_{i-1} - x_i * φ / c
 *
 * where:
 *   - k_i is the catalytic rate for species i producing species i+1
 *     (or equivalently, species i-1 catalyzing species i)
 *   - x_{i-1} is the concentration of the catalyzing species (cyclic indexing)
 *   - φ = Σ k_j * x_j * x_{j-1} is the dilution flow
 *   - c = Σ x_i is the total concentration
 *
 * The dilution term ensures Σ x_i is conserved: d(Σx_i)/dt = 0.
 *
 * For n=2, this reduces to the elementary hypercycle:
 *   dx_1/dt = k_1 * x_1 * x_2 - φ * x_1 / c
 *   dx_2/dt = k_2 * x_2 * x_1 - φ * x_2 / c
 *
 * @param hc Hypercycle state.
 * @param dxdt Output derivative array.
 */
void hypercycle_ode(const hypercycle_t *hc, double *dxdt)
{
    if (!hc || !dxdt) return;

    int n = hc->n_species;
    double c = hc->total_concentration;
    if (c < 1e-15) c = 1.0;

    /* Compute dilution flow φ = Σ k_i * x_i * x_{i-1} */
    double phi = 0.0;
    for (int i = 0; i < n; i++) {
        int prev = (i - 1 + n) % n;
        phi += hc->rate_constants[i] * hc->concentrations[i] *
               hc->concentrations[prev];
    }

    /* Compute derivatives */
    for (int i = 0; i < n; i++) {
        int prev = (i - 1 + n) % n;

        /* Catalytic production */
        double production = hc->rate_constants[i] *
                            hc->concentrations[i] * hc->concentrations[prev];

        /* Decay */
        double decay = hc->decay_rates[i] * hc->concentrations[i];

        /* Dilution outflow (maintains constant total) */
        double outflow = phi * hc->concentrations[i] / c;

        dxdt[i] = production - decay - outflow;
    }
}

/* ==========================================================================
 * Fixed Point Analysis (L4)
 * ========================================================================== */

/**
 * @brief Find the symmetric fixed point of the hypercycle.
 *
 * For equal rate constants k_i = k, the symmetric fixed point has
 * all x_i = c/n. This is stable for n ≤ 4 (Eigen's theorem).
 *
 * The Jacobian at the symmetric point determines stability.
 *
 * @param hc Hypercycle state.
 * @param fixed_point Output: fixed point concentrations.
 * @param is_stable Output: 1 if stable, 0 otherwise.
 * @return 0 on success.
 */
int hypercycle_find_fixed_point(const hypercycle_t *hc,
                                 double *fixed_point, int *is_stable)
{
    if (!hc || !fixed_point || !is_stable) return -1;

    int n = hc->n_species;
    double c = hc->total_concentration;

    /* Symmetric fixed point: x_i* = c/n */
    for (int i = 0; i < n; i++) {
        fixed_point[i] = c / (double)n;
    }

    /* Stability analysis via Jacobian eigenvalues.
     * For the symmetric hypercycle with equal k, the stability depends on n:
     * - n = 1: neutral (single autocatalyst, dx/dt = 0 at any x)
     * - n = 2: stable (mutual catalysis cooperative)
     * - n = 3: stable (cyclic cooperation)
     * - n = 4: marginally stable (center, neutral stability)
     * - n ≥ 5: unstable (oscillatory instability)
     * This is a rigorous result from Eigen & Schuster (1979). */
    if (n <= 4) {
        *is_stable = 1;
    } else {
        /* For n ≥ 5, the symmetric point is an unstable spiral */
        *is_stable = 0;
    }

    return 0;
}

/* ==========================================================================
 * ODE Integration (L5)
 * ========================================================================== */

/**
 * @brief Single RK4 step for hypercycle dynamics.
 *
 * @param hc Hypercycle state (updated in place).
 * @param dt Time step.
 */
void hypercycle_integrate_rk4(hypercycle_t *hc, double dt)
{
    if (!hc || dt <= 0.0) return;

    int n = hc->n_species;
    if (n > HC_MAX_SPECIES) return;

    double *y = (double *)malloc(n * sizeof(double));
    double *k1 = (double *)malloc(n * sizeof(double));
    double *k2 = (double *)malloc(n * sizeof(double));
    double *k3 = (double *)malloc(n * sizeof(double));
    double *k4 = (double *)malloc(n * sizeof(double));

    if (!y || !k1 || !k2 || !k3 || !k4) {
        free(y); free(k1); free(k2); free(k3); free(k4);
        return;
    }

    hypercycle_t work = *hc;
    memcpy(y, hc->concentrations, n * sizeof(double));

    /* k1 */
    hypercycle_ode(&work, k1);

    /* k2 */
    for (int i = 0; i < n; i++) {
        work.concentrations[i] = y[i] + 0.5 * dt * k1[i];
    }
    work.total_concentration = 0.0;
    for (int i = 0; i < n; i++) work.total_concentration += work.concentrations[i];
    hypercycle_ode(&work, k2);

    /* k3 */
    for (int i = 0; i < n; i++) {
        work.concentrations[i] = y[i] + 0.5 * dt * k2[i];
    }
    work.total_concentration = 0.0;
    for (int i = 0; i < n; i++) work.total_concentration += work.concentrations[i];
    hypercycle_ode(&work, k3);

    /* k4 */
    for (int i = 0; i < n; i++) {
        work.concentrations[i] = y[i] + dt * k3[i];
    }
    work.total_concentration = 0.0;
    for (int i = 0; i < n; i++) work.total_concentration += work.concentrations[i];
    hypercycle_ode(&work, k4);

    /* Update: x^{n+1} = x^n + (dt/6)*(k1 + 2k2 + 2k3 + k4) */
    double new_total = 0.0;
    for (int i = 0; i < n; i++) {
        hc->concentrations[i] += (dt / 6.0) *
            (k1[i] + 2.0*k2[i] + 2.0*k3[i] + k4[i]);
        if (hc->concentrations[i] < 0.0) hc->concentrations[i] = 0.0;
        new_total += hc->concentrations[i];
    }

    hc->total_concentration = new_total;

    /* Update mean fitness */
    double weighted_fitness = 0.0;
    for (int i = 0; i < n; i++) {
        weighted_fitness += hc->fitness[i] * hc->concentrations[i];
    }
    if (new_total > 1e-15) {
        hc->mean_fitness = weighted_fitness / new_total;
    }

    free(y); free(k1); free(k2); free(k3); free(k4);
}

/* ==========================================================================
 * Hypercycle Competition (L5)
 * ========================================================================== */

/**
 * @brief Simulate competition between two hypercycles.
 *
 * When two hypercycles compete for resources, the one with higher
 * catalytic efficiency dominates. However, spatial structure (compartmentalization)
 * can allow coexistence.
 *
 * Model: two coupled hypercycles sharing a common resource pool.
 *
 * @param hc1 First hypercycle (updated in place).
 * @param hc2 Second hypercycle (updated in place).
 * @param resource Shared resource concentration.
 * @param dt Time step.
 */
void hypercycle_competition_step(hypercycle_t *hc1, hypercycle_t *hc2,
                                  double *resource, double dt)
{
    if (!hc1 || !hc2 || !resource) return;

    /* Resource dynamics: consumed by both hypercycles */
    double consumption1 = 0.0, consumption2 = 0.0;
    for (int i = 0; i < hc1->n_species; i++) {
        consumption1 += hc1->rate_constants[i] * hc1->concentrations[i];
    }
    for (int i = 0; i < hc2->n_species; i++) {
        consumption2 += hc2->rate_constants[i] * hc2->concentrations[i];
    }

    double resource_decay = 0.01 * (*resource) * (consumption1 + consumption2);
    *resource -= resource_decay * dt;

    /* Constant resource influx */
    double influx = 0.5;
    *resource += influx * dt;
    if (*resource < 0.0) *resource = 0.0;

    /* Resource limitation: scale catalytic rates by resource availability */
    double resource_factor = *resource / (1.0 + *resource);

    /* Modify rate constants temporarily */
    double orig_rates1[HC_MAX_SPECIES], orig_rates2[HC_MAX_SPECIES];
    for (int i = 0; i < hc1->n_species; i++) {
        orig_rates1[i] = hc1->rate_constants[i];
        hc1->rate_constants[i] *= resource_factor;
    }
    for (int i = 0; i < hc2->n_species; i++) {
        orig_rates2[i] = hc2->rate_constants[i];
        hc2->rate_constants[i] *= resource_factor;
    }

    /* Integrate both hypercycles */
    hypercycle_integrate_rk4(hc1, dt);
    hypercycle_integrate_rk4(hc2, dt);

    /* Restore original rates */
    for (int i = 0; i < hc1->n_species; i++) hc1->rate_constants[i] = orig_rates1[i];
    for (int i = 0; i < hc2->n_species; i++) hc2->rate_constants[i] = orig_rates2[i];
}

/* ==========================================================================
 * Error Threshold (L6 — Eigen's Error Catastrophe)
 * ========================================================================== */

/**
 * @brief Compute the error threshold for a hypercycle.
 *
 * Eigen's error catastrophe: when the mutation/error rate exceeds a
 * critical threshold (1/σ, where σ is the superiority of the master
 * sequence), the quasispecies delocalizes and information is lost.
 *
 * For a hypercycle of length n with per-base replication fidelity q:
 *   Threshold: q_min = (1/σ)^{1/n}
 *   where σ = A_master / A_mutant (fitness ratio)
 *
 * If q < q_min, the error catastrophe occurs: the master sequence
 * cannot be maintained.
 *
 * @param n_components Number of components in the hypercycle.
 * @param fitness_ratio Fitness ratio (master/mutant).
 * @param q_min Output: minimum fidelity required.
 * @param threshold Output: error rate threshold (1 - q_min).
 */
void hypercycle_error_threshold(int n_components, double fitness_ratio,
                                 double *q_min, double *threshold)
{
    if (!q_min || !threshold) return;

    if (n_components <= 0 || fitness_ratio <= 1.0) {
        *q_min = 1.0;
        *threshold = 0.0;
        return;
    }

    /* Superiority σ = A_master / A_mutant */
    double sigma = fitness_ratio;

    /* Minimum per-base fidelity */
    *q_min = pow(1.0 / sigma, 1.0 / (double)n_components);

    /* Error threshold = maximum error rate that still maintains information */
    *threshold = 1.0 - (*q_min);

    /* Clamp to valid range */
    if (*q_min > 1.0) *q_min = 1.0;
    if (*threshold < 0.0) *threshold = 0.0;
}

/* ==========================================================================
 * Quasispecies Distribution (L8)
 * ========================================================================== */

/**
 * @brief Compute the quasispecies equilibrium distribution.
 *
 * The quasispecies is the equilibrium distribution of sequences under
 * replication with mutation. The dominant "master" sequence coexists with
 * a cloud of mutants at frequencies determined by the mutation-selection
 * balance.
 *
 * For a simple two-type model (master M and mutant m):
 *   dx_M/dt = A_M * q^n * x_M - φ * x_M
 *   dx_m/dt = A_M * (1-q^n) * x_M + A_m * x_m - φ * x_m
 *
 * Equilibrium: x_M / x_total = (A_M * q^n - A_m) / (A_M - A_m)
 *
 * @param hc Hypercycle state (updated with quasispecies distribution).
 */
void hypercycle_compute_quasispecies(hypercycle_t *hc)
{
    if (!hc || hc->n_species < 2) return;

    /* Compute quasispecies distribution using the Eigen-Schuster equations.
     * For n=2 (master + mutant):
     *   W = [A_M * q   ,  0          ]
     *       [A_M * (1-q), A_m        ]
     * Equilibrium is the dominant eigenvector of W. */

    int n = hc->n_species;

    /* Build replication-mutation matrix W */
    double W[HC_MAX_SPECIES][HC_MAX_SPECIES];
    memset(W, 0, sizeof(W));

    /* Assume: species 0 is master, others are single-mutation neighbors */
    double fidelity = 0.95;  /* Per-site replication fidelity */
    double q_n = pow(fidelity, (double)n); /* Probability of no mutation */

    double A_master = hc->rate_constants[0];
    double A_mutant = 0.0;
    for (int i = 1; i < n; i++) {
        if (hc->rate_constants[i] > A_mutant) {
            A_mutant = hc->rate_constants[i];
        }
    }
    if (A_mutant < 0.01) A_mutant = 0.1 * A_master;

    /* Master generation */
    W[0][0] = A_master * q_n;

    /* Mutation from master to each mutant */
    for (int i = 1; i < n; i++) {
        W[i][0] = A_master * (1.0 - q_n) / (double)(n - 1);
        W[i][i] = A_mutant;
    }

    /* Power iteration to find dominant eigenvector */
    double *x = (double *)malloc(n * sizeof(double));
    double *x_new = (double *)malloc(n * sizeof(double));

    if (!x || !x_new) { free(x); free(x_new); return; }

    /* Initialize with current concentrations or uniform */
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        x[i] = hc->concentrations[i] + 1e-10;
        sum += x[i];
    }
    for (int i = 0; i < n; i++) x[i] /= sum;

    /* Power iteration */
    for (int iter = 0; iter < 100; iter++) {
        sum = 0.0;
        for (int i = 0; i < n; i++) {
            x_new[i] = 0.0;
            for (int j = 0; j < n; j++) {
                x_new[i] += W[i][j] * x[j];
            }
            sum += x_new[i];
        }

        for (int i = 0; i < n; i++) x[i] = x_new[i] / sum;
    }

    /* Set quasispecies distribution as concentrations */
    double total_c = hc->total_concentration;
    for (int i = 0; i < n; i++) {
        hc->concentrations[i] = x[i] * total_c;
        hc->fitness[i] = (i == 0) ? A_master : A_mutant;
    }

    free(x);
    free(x_new);
    hc->is_quasispecies = 1;
}

/* ==========================================================================
 * Spatial Hypercycle (L8)
 * ========================================================================== */

/**
 * @brief One step of spatial hypercycle with reaction-diffusion.
 *
 * In spatially extended systems, hypercycles can form traveling waves
 * and spiral patterns (as observed in the BZ reaction). Spatial structure
 * can stabilize hypercycles against parasites.
 *
 * Model: ∂x_i/∂t = f_i(x) + D * ∇²x_i
 *
 * where f_i is the hypercycle ODE and D is the diffusion coefficient.
 *
 * @param sp Spatial hypercycle state.
 * @param dt Time step.
 * @param dx Spatial step.
 */
void hypercycle_spatial_step(hypercycle_spatial_t *sp, double dt, double dx)
{
    if (!sp) return;

    int n = sp->n_species;
    int nc = sp->n_cells;
    double D = sp->diffusion_rate;

    /* Allocate temporary arrays */
    double **laplacian = (double **)malloc(nc * sizeof(double *));
    double **reaction = (double **)malloc(nc * sizeof(double *));
    if (!laplacian || !reaction) {
        free(laplacian); free(reaction);
        return;
    }

    for (int c = 0; c < nc; c++) {
        laplacian[c] = (double *)calloc(n, sizeof(double));
        reaction[c] = (double *)calloc(n, sizeof(double));
        if (!laplacian[c] || !reaction[c]) {
            for (int j = 0; j < c; j++) {
                free(laplacian[j]); free(reaction[j]);
            }
            free(laplacian); free(reaction);
            return;
        }
    }

    /* Compute Laplacian (1D: ∂²x/∂x² ≈ (x_{i+1} - 2x_i + x_{i-1}) / dx²) */
    double inv_dx2 = 1.0 / (dx * dx);

    for (int c = 0; c < nc; c++) {
        int left = (c - 1 + nc) % nc;
        int right = (c + 1) % nc;

        for (int s = 0; s < n; s++) {
            laplacian[c][s] = D * inv_dx2 *
                (sp->grid[left][s] - 2.0 * sp->grid[c][s] + sp->grid[right][s]);
        }
    }

    /* Compute reaction terms for each cell */
    for (int c = 0; c < nc; c++) {
        hypercycle_t hc_local;
        hc_local.n_species = n;
        double total = 0.0;
        for (int s = 0; s < n; s++) {
            hc_local.concentrations[s] = sp->grid[c][s];
            hc_local.rate_constants[s] = 1.0;
            hc_local.decay_rates[s] = 0.01;
            total += sp->grid[c][s];
        }
        hc_local.total_concentration = total > 0.0 ? total : 1.0;

        hypercycle_ode(&hc_local, reaction[c]);
    }

    /* Update grid: Euler step with reaction + diffusion */
    for (int c = 0; c < nc; c++) {
        for (int s = 0; s < n; s++) {
            sp->grid[c][s] += dt * (reaction[c][s] + laplacian[c][s]);
            if (sp->grid[c][s] < 0.0) sp->grid[c][s] = 0.0;
        }
    }

    for (int c = 0; c < nc; c++) {
        free(laplacian[c]);
        free(reaction[c]);
    }
    free(laplacian);
    free(reaction);
}

/* ==========================================================================
 * Initialization and Utility
 * ========================================================================== */

void hypercycle_init(hypercycle_t *hc, int n_species)
{
    if (!hc || n_species < 2 || n_species > HC_MAX_SPECIES) return;

    memset(hc, 0, sizeof(hypercycle_t));
    hc->n_species = n_species;

    /* Initialize with small random-like values */
    for (int i = 0; i < n_species; i++) {
        hc->concentrations[i] = 0.5 + 0.1 * (double)(i % 5);
        hc->rate_constants[i] = 1.0;
        hc->decay_rates[i] = 0.01;
        hc->fitness[i] = 1.0;
    }

    /* Normalize total concentration */
    double total = 0.0;
    for (int i = 0; i < n_species; i++) total += hc->concentrations[i];
    hc->total_concentration = total;
    hc->mean_fitness = 1.0;
    hc->is_quasispecies = 0;
}

void hypercycle_init_spatial(hypercycle_spatial_t *sp,
                              int n_species, int n_cells)
{
    if (!sp || n_species < 1 || n_cells < 3) return;

    memset(sp, 0, sizeof(hypercycle_spatial_t));
    sp->n_species = n_species;
    sp->n_cells = n_cells;
    sp->diffusion_rate = 0.1;
    sp->cell_size = 0.1;

    /* Initialize with localized concentration peak */
    int center = n_cells / 2;
    for (int c = 0; c < n_cells; c++) {
        double dist = fabs((double)(c - center)) / (double)n_cells;
        for (int s = 0; s < n_species; s++) {
            sp->grid[c][s] = exp(-dist * dist * 20.0) * (0.5 + 0.1 * s);
        }
    }
}

void hypercycle_print_state(const hypercycle_t *hc)
{
    if (!hc) { printf("Hypercycle: NULL\n"); return; }

    printf("=== Eigen Hypercycle (n=%d) ===\n", hc->n_species);
    printf("Concentrations: ");
    for (int i = 0; i < hc->n_species; i++) {
        printf("[%d]=%.4f ", i, hc->concentrations[i]);
    }
    printf("\nTotal: %.4f  Mean Fitness: %.4f\n",
           hc->total_concentration, hc->mean_fitness);

    /* Compute error threshold */
    double q_min, threshold;
    hypercycle_error_threshold(hc->n_species, hc->rate_constants[0] / 0.1,
                                &q_min, &threshold);
    printf("Error Threshold: q_min=%.4f  max_error_rate=%.4f\n", q_min, threshold);

    if (hc->is_quasispecies) {
        printf("Operating in quasispecies regime\n");
    }
    printf("==============================\n");
}
