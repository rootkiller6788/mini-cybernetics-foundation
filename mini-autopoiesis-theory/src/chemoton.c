/**
 * @file chemoton.c
 * @brief Gánti's Chemoton model — the minimal abstract autopoietic system.
 *
 * The Chemoton (Gánti, 1971) is the simplest abstract system that exhibits
 * autopoiesis. It consists of three stoichiometrically coupled autocatalytic
 * subsystems:
 *
 *   1. Metabolic cycle: produces chemical energy and building blocks
 *   2. Template replication: information-carrying polymer that controls reactions
 *   3. Membrane growth: boundary-forming subsystem that encloses the system
 *
 * The three subsystems are coupled such that each depends on the other two
 * for its functioning, forming a self-sustaining whole.
 *
 * Knowledge points implemented:
 *   L4 - chemoton_metabolic_ode: metabolic cycle ODEs
 *   L4 - chemoton_template_ode: template replication kinetics
 *   L4 - chemoton_membrane_ode: membrane growth dynamics
 *   L5 - chemoton_integrate_rk4: coupled 3-subsystem integration
 *   L5 - chemoton_check_autopoiesis: verify all three criteria
 *   L6 - chemoton_steady_state_analysis: find steady-state concentrations
 *   L6 - chemoton_bifurcation_analysis: parameter sensitivity
 *
 * Reference: Gánti, "The Principles of Life" (1971/2003)
 *            Gánti, "Chemoton Theory" (2003)
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================================================
 * Chemoton Model Parameters
 * ========================================================================== */

/** Maximum number of intermediates in the metabolic cycle */
#define CHE_METABOLITES 8
/** Maximum number of template monomers */
#define CHE_TEMPLATE_LENGTH 4

/**
 * @brief Complete Chemoton state.
 */
typedef struct {
    /* Metabolic subsystem */
    double metabolites[CHE_METABOLITES];     /**< Intermediate concentrations */
    int n_metabolites;
    double nutrient;                         /**< External nutrient concentration */
    double waste;                            /**< Waste product concentration */

    /* Template subsystem */
    double template_concentration;           /**< Information polymer concentration */
    double monomers[CHE_TEMPLATE_LENGTH];    /**< Monomer concentrations */
    int n_monomers;
    double template_polymerase;              /**< Replication enzyme */

    /* Membrane subsystem */
    double membrane_precursors;              /**< Membrane building blocks */
    double membrane_area;                    /**< Total membrane surface area */
    double membrane_permeability;            /**< Current permeability */
    double encapsulated_volume;              /**< Internal volume */

    /* Coupling parameters */
    double metabolic_efficiency;             /**< k_met */
    double template_replication_rate;        /**< k_rep */
    double membrane_growth_rate;             /**< k_mem */
    double decay_rate;                       /**< Generic decay */
    double nutrient_influx;                  /**< Rate of nutrient entry */

    /* Control parameters */
    double total_energy;                     /**< ATP-equivalent energy */
    double organization_measure;             /**< Autopoietic integrity */
} chemoton_t;

/* ==========================================================================
 * Metabolic Cycle ODEs (L4)
 * ========================================================================== */

/**
 * @brief Compute the metabolic cycle derivatives.
 *
 * The metabolic cycle is an autocatalytic loop of intermediates.
 * Each intermediate I_i is converted to I_{i+1} by a reaction catalyzed
 * by the previous intermediate. The cycle produces energy (ATP) and
 * building blocks for the template and membrane subsystems.
 *
 * Model:
 *   dI_i/dt = k_cat * I_{i-1} * I_i - k_dec * I_i + nutrient_input
 *   with cyclic indexing: I_{-1} ≡ I_{n-1}
 *
 * Energy production: E = Σ I_i (total cycling = energy production)
 *
 * @param che The chemoton state.
 * @param dI Output derivative array.
 */
static void chemoton_metabolic_ode(const chemoton_t *che, double *dI)
{
    int n = che->n_metabolites;
    double k_cat = che->metabolic_efficiency;
    double k_dec = che->decay_rate;

    for (int i = 0; i < n; i++) {
        int prev = (i - 1 + n) % n; /* Cyclic: I_{-1} = I_{n-1} */

        /* Catalytic conversion: I_{prev} catalyzes production of I_i */
        double production = k_cat * che->metabolites[prev] * che->metabolites[i];

        /* Decay */
        double decay = k_dec * che->metabolites[i];

        /* Nutrient input only for first metabolite */
        double input = (i == 0) ? che->nutrient_influx * che->nutrient : 0.0;

        /* Waste removal from last metabolite */
        double waste_removal = (i == n - 1) ?
            k_cat * che->metabolites[i] * 0.1 : 0.0;

        dI[i] = production - decay + input - waste_removal;

        /* Non-negativity enforcement */
        if (che->metabolites[i] < 0.0 && dI[i] < 0.0) dI[i] = 0.0;
    }

    /* Update total energy */
    double energy = 0.0;
    for (int i = 0; i < n; i++) {
        energy += che->metabolites[i];
    }
    ((chemoton_t *)che)->total_energy = energy; /* Cast away const */
}

/**
 * @brief Compute the template replication derivatives.
 *
 * The template subsystem models an information-carrying polymer (like RNA)
 * that replicates autocatalytically. The template catalyzes its own
 * replication using monomers produced by the metabolic cycle.
 *
 * Model:
 *   dT/dt = k_rep * T * (Π m_i) / (K_m + Π m_i) - k_dec * T
 *   dm_i/dt = α_i * E * T - k_rep * T * m_i / (K_m + m_i) - k_dec * m_i
 *
 * where T=template concentration, m_i=monomer i, E=metabolic energy.
 *
 * @param che The chemoton state.
 * @param dT Output: template derivative.
 * @param dm Output: monomer derivatives array.
 */
static void chemoton_template_ode(const chemoton_t *che, double *dT, double *dm)
{
    double T = che->template_concentration;
    double k_rep = che->template_replication_rate;
    double k_dec = che->decay_rate;
    double E = che->total_energy;
    int nm = che->n_monomers;

    /* Monomer product for Michaelis-Menten: Π m_i / (K + Π m_i) */
    double monomer_product = 1.0;
    for (int i = 0; i < nm; i++) {
        double m = che->monomers[i];
        if (m < 1e-10) m = 1e-10;
        monomer_product *= m;
    }

    double Km = 0.5; /* Michaelis constant */
    double saturation = monomer_product / (Km + monomer_product);

    /* Template replication: autocatalytic + saturation */
    *dT = k_rep * T * saturation - k_dec * T;

    /* Monomer dynamics: produced from metabolic energy, consumed by replication */
    for (int i = 0; i < nm; i++) {
        /* Production proportional to metabolic energy */
        double production = 0.1 * E * che->metabolic_efficiency;

        /* Consumption by template replication */
        double m = che->monomers[i];
        if (m < 1e-10) m = 1e-10;
        double consumption = k_rep * T * m / (0.5 + m);

        dm[i] = production - consumption - k_dec * m;

        if (che->monomers[i] < 0.0 && dm[i] < 0.0) dm[i] = 0.0;
    }
}

/**
 * @brief Compute the membrane growth derivatives.
 *
 * The membrane subsystem forms the boundary of the chemoton. Membrane
 * precursors are produced by the metabolic cycle and assembled into
 * membrane material. The membrane grows proportionally to precursor
 * concentration and surface area.
 *
 * Model:
 *   dP/dt = β * E - γ * P * A - k_dec * P
 *   dA/dt = γ * P * A / (1 + A/A_max) - k_dec * A
 *
 * where P=precursors, A=membrane area, E=metabolic energy.
 *
 * @param che The chemoton state.
 * @param dP Output: precursor derivative.
 * @param dA Output: membrane area derivative.
 */
static void chemoton_membrane_ode(const chemoton_t *che, double *dP, double *dA)
{
    double P = che->membrane_precursors;
    double A = che->membrane_area;
    double k_mem = che->membrane_growth_rate;
    double k_dec = che->decay_rate;
    double E = che->total_energy;

    /* Precursor production from metabolic energy */
    double beta = 0.05; /* Production coefficient */
    *dP = beta * E * che->metabolic_efficiency - k_mem * P * A - k_dec * P;

    /* Membrane growth: autocatalytic (more area → more growth sites) */
    double A_max = 100.0; /* Maximum membrane area */
    double growth_saturation = 1.0 / (1.0 + A / A_max);
    *dA = k_mem * P * A * growth_saturation - k_dec * A;

    if (che->membrane_precursors < 0.0 && *dP < 0.0) *dP = 0.0;
    if (che->membrane_area < 0.0 && *dA < 0.0) *dA = 0.0;

    /* Update encapsulated volume (∝ area^(3/2)) */
    ((chemoton_t *)che)->encapsulated_volume = pow(A, 1.5) * 0.1;

    /* Update permeability (decreases with area growth) */
    ((chemoton_t *)che)->membrane_permeability = 1.0 / (1.0 + A * 0.1);
}

/* ==========================================================================
 * Coupled Integration (L5)
 * ========================================================================== */

/**
 * @brief Single RK4 integration step for the full coupled chemoton.
 *
 * @param che Chemoton state (updated in place).
 * @param dt Time step.
 */
void chemoton_integrate_rk4(chemoton_t *che, double dt)
{
    if (!che || dt <= 0.0) return;

    int nm = che->n_metabolites;
    int nmono = che->n_monomers;

    /* Save current state */
    chemoton_t orig = *che;

    /* Allocate work arrays */
    double *k1_I = (double *)malloc(nm * sizeof(double));
    double *k2_I = (double *)malloc(nm * sizeof(double));
    double *k3_I = (double *)malloc(nm * sizeof(double));
    double *k4_I = (double *)malloc(nm * sizeof(double));

    double k1_T, k2_T, k3_T, k4_T;
    double *k1_m = (double *)malloc(nmono * sizeof(double));
    double *k2_m = (double *)malloc(nmono * sizeof(double));
    double *k3_m = (double *)malloc(nmono * sizeof(double));
    double *k4_m = (double *)malloc(nmono * sizeof(double));

    double k1_P, k2_P, k3_P, k4_P;
    double k1_A, k2_A, k3_A, k4_A;

    if (!k1_I || !k1_m) {
        free(k1_I); free(k2_I); free(k3_I); free(k4_I);
        free(k1_m); free(k2_m); free(k3_m); free(k4_m);
        return;
    }

    /* k1: derivatives at current state */
    chemoton_metabolic_ode(che, k1_I);
    chemoton_template_ode(che, &k1_T, k1_m);
    chemoton_membrane_ode(che, &k1_P, &k1_A);

    /* k2: at t + dt/2 */
    *che = orig;
    for (int i = 0; i < nm; i++) che->metabolites[i] += 0.5 * dt * k1_I[i];
    che->template_concentration += 0.5 * dt * k1_T;
    for (int i = 0; i < nmono; i++) che->monomers[i] += 0.5 * dt * k1_m[i];
    che->membrane_precursors += 0.5 * dt * k1_P;
    che->membrane_area += 0.5 * dt * k1_A;

    chemoton_metabolic_ode(che, k2_I);
    chemoton_template_ode(che, &k2_T, k2_m);
    chemoton_membrane_ode(che, &k2_P, &k2_A);

    /* k3: at t + dt/2 */
    *che = orig;
    for (int i = 0; i < nm; i++) che->metabolites[i] += 0.5 * dt * k2_I[i];
    che->template_concentration += 0.5 * dt * k2_T;
    for (int i = 0; i < nmono; i++) che->monomers[i] += 0.5 * dt * k2_m[i];
    che->membrane_precursors += 0.5 * dt * k2_P;
    che->membrane_area += 0.5 * dt * k2_A;

    chemoton_metabolic_ode(che, k3_I);
    chemoton_template_ode(che, &k3_T, k3_m);
    chemoton_membrane_ode(che, &k3_P, &k3_A);

    /* k4: at t + dt */
    *che = orig;
    for (int i = 0; i < nm; i++) che->metabolites[i] += dt * k3_I[i];
    che->template_concentration += dt * k3_T;
    for (int i = 0; i < nmono; i++) che->monomers[i] += dt * k3_m[i];
    che->membrane_precursors += dt * k3_P;
    che->membrane_area += dt * k3_A;

    chemoton_metabolic_ode(che, k4_I);
    chemoton_template_ode(che, &k4_T, k4_m);
    chemoton_membrane_ode(che, &k4_P, &k4_A);

    /* Final update: x^{n+1} = x^n + (dt/6)*(k1 + 2k2 + 2k3 + k4) */
    *che = orig;
    for (int i = 0; i < nm; i++) {
        che->metabolites[i] += (dt / 6.0) *
            (k1_I[i] + 2.0*k2_I[i] + 2.0*k3_I[i] + k4_I[i]);
        if (che->metabolites[i] < 0.0) che->metabolites[i] = 0.0;
    }

    che->template_concentration += (dt / 6.0) *
        (k1_T + 2.0*k2_T + 2.0*k3_T + k4_T);
    if (che->template_concentration < 0.0) che->template_concentration = 0.0;

    for (int i = 0; i < nmono; i++) {
        che->monomers[i] += (dt / 6.0) *
            (k1_m[i] + 2.0*k2_m[i] + 2.0*k3_m[i] + k4_m[i]);
        if (che->monomers[i] < 0.0) che->monomers[i] = 0.0;
    }

    che->membrane_precursors += (dt / 6.0) *
        (k1_P + 2.0*k2_P + 2.0*k3_P + k4_P);
    if (che->membrane_precursors < 0.0) che->membrane_precursors = 0.0;

    che->membrane_area += (dt / 6.0) *
        (k1_A + 2.0*k2_A + 2.0*k3_A + k4_A);
    if (che->membrane_area < 0.0) che->membrane_area = 0.0;

    /* Recompute energy and organization measure */
    double energy = 0.0;
    for (int i = 0; i < nm; i++) energy += che->metabolites[i];
    che->total_energy = energy;

    /* Organization measure: all three subsystems must be active */
    double met_active = (energy > 0.1) ? 1.0 : 0.0;
    double tmp_active = (che->template_concentration > 0.01) ? 1.0 : 0.0;
    double mem_active = (che->membrane_area > 0.1) ? 1.0 : 0.0;
    che->organization_measure = (met_active + tmp_active + mem_active) / 3.0;

    free(k1_I); free(k2_I); free(k3_I); free(k4_I);
    free(k1_m); free(k2_m); free(k3_m); free(k4_m);
}

/* ==========================================================================
 * Chemoton Initialization and Analysis
 * ========================================================================== */

/**
 * @brief Initialize a default chemoton.
 */
void chemoton_init(chemoton_t *che)
{
    if (!che) return;
    memset(che, 0, sizeof(chemoton_t));

    /* Default parameters */
    che->n_metabolites = 4;
    che->n_monomers = 2;
    che->nutrient = 10.0;
    che->metabolic_efficiency = 1.0;
    che->template_replication_rate = 0.5;
    che->membrane_growth_rate = 0.3;
    che->decay_rate = 0.01;
    che->nutrient_influx = 0.1;

    /* Initial concentrations (small but non-zero to avoid trivial steady state) */
    for (int i = 0; i < che->n_metabolites; i++) {
        che->metabolites[i] = 0.5 + 0.1 * i;
    }
    che->template_concentration = 0.1;
    for (int i = 0; i < che->n_monomers; i++) {
        che->monomers[i] = 0.2;
    }
    che->membrane_precursors = 1.0;
    che->membrane_area = 1.0;
    che->membrane_permeability = 0.5;
    che->encapsulated_volume = 1.0;
}

/**
 * @brief Check if the chemoton satisfies autopoiesis criteria.
 *
 * Three criteria from Gánti:
 *   1. Metabolic cycle is autocatalytic (sustains itself)
 *   2. Template is replicating (information is preserved)
 *   3. Membrane is growing (boundary is self-produced)
 *
 * @param che The chemoton state.
 * @return 1 if autopoietic, 0 otherwise.
 */
int chemoton_check_autopoiesis(const chemoton_t *che)
{
    if (!che) return 0;

    /* Criterion 1: Metabolic cycle active */
    double met_total = 0.0;
    for (int i = 0; i < che->n_metabolites; i++) {
        met_total += che->metabolites[i];
    }
    if (met_total < 0.2 * che->n_metabolites) return 0;

    /* Criterion 2: Template replication active */
    if (che->template_concentration < 0.05) return 0;

    /* Criterion 3: Membrane present */
    if (che->membrane_area < 0.5) return 0;

    /* Check coupling: each subsystem depends on others */
    /* Metabolic energy required for template and membrane */
    if (che->total_energy < 0.5) return 0;

    /* Template required for metabolic regulation (simplified check) */
    /* Membrane required to contain the system */

    return (che->organization_measure > 0.66) ? 1 : 0;
}

/**
 * @brief Find steady-state concentrations by long-time integration.
 *
 * @param che Chemoton state (updated to steady state).
 * @param max_time Maximum integration time.
 * @param tolerance Convergence tolerance.
 * @return 1 if steady state found, 0 otherwise.
 */
int chemoton_find_steady_state(chemoton_t *che, double max_time, double tolerance)
{
    if (!che) return 0;

    double t = 0.0;
    double dt = 0.01;
    int max_steps = (int)(max_time / dt);
    if (max_steps > 100000) max_steps = 100000;

    chemoton_t prev_state;
    memcpy(&prev_state, che, sizeof(chemoton_t));

    int steady_steps = 0;
    const int required_steady = 100;

    for (int step = 0; step < max_steps; step++) {
        chemoton_integrate_rk4(che, dt);
        t += dt;

        /* Check convergence: max relative change in any concentration */
        double max_change = 0.0;
        for (int i = 0; i < che->n_metabolites; i++) {
            double old_val = prev_state.metabolites[i];
            double new_val = che->metabolites[i];
            double change = fabs(new_val - old_val) / (fabs(new_val) + 1e-10);
            if (change > max_change) max_change = change;
        }

        double T_change = fabs(che->template_concentration - prev_state.template_concentration) /
            (fabs(che->template_concentration) + 1e-10);
        if (T_change > max_change) max_change = T_change;

        double A_change = fabs(che->membrane_area - prev_state.membrane_area) /
            (fabs(che->membrane_area) + 1e-10);
        if (A_change > max_change) max_change = A_change;

        if (max_change < tolerance) {
            steady_steps++;
            if (steady_steps >= required_steady) return 1;
        } else {
            steady_steps = 0;
        }

        memcpy(&prev_state, che, sizeof(chemoton_t));
    }

    return 0;
}

/**
 * @brief Bifurcation analysis: scan a parameter and record steady states.
 *
 * @param che Base chemoton state.
 * @param param_name Which parameter to vary.
 * @param param_start Start value.
 * @param param_end End value.
 * @param n_points Number of scan points.
 * @param results Output: organization measure for each point.
 */
void chemoton_bifurcation_scan(chemoton_t *che,
                                const char *param_name,
                                double param_start, double param_end,
                                int n_points, double *results)
{
    if (!che || !results || n_points <= 0) return;

    for (int i = 0; i < n_points; i++) {
        double t = (double)i / (double)(n_points - 1);
        double param_value = param_start + t * (param_end - param_start);

        /* Reset chemoton */
        chemoton_t working;
        memcpy(&working, che, sizeof(chemoton_t));
        chemoton_init(&working);

        /* Set the parameter */
        if (strcmp(param_name, "nutrient_influx") == 0) {
            working.nutrient_influx = param_value;
        } else if (strcmp(param_name, "metabolic_efficiency") == 0) {
            working.metabolic_efficiency = param_value;
        } else if (strcmp(param_name, "template_rate") == 0) {
            working.template_replication_rate = param_value;
        } else if (strcmp(param_name, "membrane_rate") == 0) {
            working.membrane_growth_rate = param_value;
        } else {
            working.nutrient_influx = param_value; /* Default */
        }

        /* Integrate to steady state */
        chemoton_find_steady_state(&working, 50.0, 1e-6);

        /* Record organization measure */
        results[i] = working.organization_measure;
    }
}

/**
 * @brief Print chemoton state report.
 */
void chemoton_print_state(const chemoton_t *che)
{
    if (!che) { printf("Chemoton: NULL\n"); return; }

    printf("=== Gánti Chemoton State ===\n");
    printf("Metabolic Cycle (%d intermediates): ", che->n_metabolites);
    for (int i = 0; i < che->n_metabolites; i++) {
        printf("%.3f ", che->metabolites[i]);
    }
    printf("\n  Nutrient: %.3f  Waste: %.3f  Energy: %.3f\n",
           che->nutrient, che->waste, che->total_energy);

    printf("Template: %.4f  Monomers: ", che->template_concentration);
    for (int i = 0; i < che->n_monomers; i++) {
        printf("%.3f ", che->monomers[i]);
    }
    printf("\n");

    printf("Membrane: area=%.3f  precursors=%.3f  perm=%.3f  volume=%.3f\n",
           che->membrane_area, che->membrane_precursors,
           che->membrane_permeability, che->encapsulated_volume);

    printf("Organization: %.3f  |  Autopoietic: %s\n",
           che->organization_measure,
           chemoton_check_autopoiesis(che) ? "YES" : "NO");
    printf("============================\n");
}
