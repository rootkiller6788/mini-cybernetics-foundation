/**
 * @file structural_coupling.c
 * @brief Structural coupling dynamics between autopoietic systems and environment.
 *
 * Implements Maturana & Varela's concept of structural coupling: the history
 * of recurrent interactions between a system and its medium that trigger
 * structural changes while preserving the autopoietic organization.
 *
 * Knowledge points implemented:
 *   L2 - sc_apply_perturbation: perturbation-response dynamics
 *   L2 - sc_structural_determinism_index: structural determinism quantification
 *   L4 - sc_evolve_coupling: continuous structural drift evolution
 *   L4 - sc_compute_structural_drift: drift magnitude
 *   L4 - sc_organization_preserved: organizational invariance verification
 *   L7 - sc_niche_construction: system modifies its environment
 *   L7 - sc_cross_coupling_measure: social autopoiesis coupling (Luhmann)
 *   L7 - sc_consensual_interaction: communication event simulation
 */

#include "structural_coupling.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================================================
 * System Setup
 * ========================================================================== */

void sc_coupling_init(sc_coupling_t *sc, int n_components, int n_boundary_params)
{
    if (!sc) return;
    memset(sc, 0, sizeof(sc_coupling_t));

    if (n_components <= 0 || n_components > SC_MAX_STATES) return;
    if (n_boundary_params < 0 || n_boundary_params > 16) return;

    sc->current_state.n_components = n_components;
    sc->current_state.n_boundary_params = n_boundary_params;
    sc->current_state.concentrations = (double *)calloc(n_components, sizeof(double));
    sc->current_state.boundary_state = (double *)calloc(n_boundary_params, sizeof(double));
    sc->current_state.organization_measure = 1.0;

    sc->reference_state.n_components = n_components;
    sc->reference_state.n_boundary_params = n_boundary_params;
    sc->reference_state.concentrations = (double *)calloc(n_components, sizeof(double));
    sc->reference_state.boundary_state = (double *)calloc(n_boundary_params, sizeof(double));
    sc->reference_state.organization_measure = 1.0;

    sc->env_temperature = 298.0;  /* Kelvin */
    sc->env_pH = 7.0;
    sc->env_pressure = 1.0;       /* atm */
    sc->env_resource_count = 0;
    memset(sc->env_resource_levels, 0, sizeof(sc->env_resource_levels));

    sc->coupling_strength = 0.0;
    sc->adaptation_rate = 0.0;
    sc->structural_drift = 0.0;
    sc->time = 0.0;
    sc->dt = 0.01;
}

void sc_coupling_set_reference(sc_coupling_t *sc, const double *concentrations,
                                const double *boundary)
{
    if (!sc) return;

    if (concentrations && sc->reference_state.concentrations && sc->current_state.concentrations) {
        memcpy(sc->reference_state.concentrations, concentrations,
               sc->reference_state.n_components * sizeof(double));
        memcpy(sc->current_state.concentrations, concentrations,
               sc->current_state.n_components * sizeof(double));
    }

    if (boundary && sc->reference_state.boundary_state && sc->current_state.boundary_state) {
        memcpy(sc->reference_state.boundary_state, boundary,
               sc->reference_state.n_boundary_params * sizeof(double));
        memcpy(sc->current_state.boundary_state, boundary,
               sc->current_state.n_boundary_params * sizeof(double));
    }
}

void sc_coupling_set_environment(sc_coupling_t *sc, double temp, double pH,
                                  const double *resources, int n_resources)
{
    if (!sc) return;
    sc->env_temperature = temp;
    sc->env_pH = pH;
    sc->env_resource_count = n_resources > 16 ? 16 : n_resources;
    if (resources) {
        memcpy(sc->env_resource_levels, resources,
               sc->env_resource_count * sizeof(double));
    }
}

void sc_coupling_destroy(sc_coupling_t *sc)
{
    if (!sc) return;
    free(sc->current_state.concentrations);
    free(sc->current_state.boundary_state);
    free(sc->reference_state.concentrations);
    free(sc->reference_state.boundary_state);
    memset(sc, 0, sizeof(sc_coupling_t));
}

/* ==========================================================================
 * Perturbation and Response — Structural Determinism
 * ========================================================================== */

void sc_apply_perturbation(sc_coupling_t *sc, const sc_perturbation_t *pert,
                            sc_response_t *response)
{
    if (!sc || !pert || !response) return;

    memset(response, 0, sizeof(sc_response_t));
    response->time = sc->time;

    int n = sc->current_state.n_components;
    double *conc = sc->current_state.concentrations;

    /* The perturbation changes the affected component's concentration */
    if (pert->component_id >= 0 && pert->component_id < n) {
        double old_val = conc[pert->component_id];

        /* Effect depends on perturbation type */
        double effect = pert->magnitude;
        switch (pert->type) {
            case SC_PERTURB_CHEMICAL:
                effect *= exp(-pert->duration * 0.1);
                break;
            case SC_PERTURB_THERMAL:
                /* Temperature affects reaction rates; model as Arrhenius effect */
                effect *= exp(-(sc->env_temperature + pert->magnitude) / 298.0);
                break;
            case SC_PERTURB_MECHANICAL:
                effect *= (1.0 - exp(-pert->duration));
                break;
            case SC_PERTURB_INFORMATIONAL:
                /* Information perturbation: system's own structure determines response */
                effect *= sc->current_state.organization_measure;
                break;
            default:
                effect *= pert->duration;
                break;
        }

        conc[pert->component_id] += effect;
        if (conc[pert->component_id] < 0.0) conc[pert->component_id] = 0.0;

        response->response_magnitude = fabs(conc[pert->component_id] - old_val);
    }

    /* Boundary perturbation */
    if (pert->component_id == -1 && sc->current_state.boundary_state) {
        /* Perturbation affects all boundary parameters */
        for (int b = 0; b < sc->current_state.n_boundary_params; b++) {
            double old_val = sc->current_state.boundary_state[b];
            sc->current_state.boundary_state[b] += pert->magnitude * 0.1;
            if (sc->current_state.boundary_state[b] < 0.0) {
                sc->current_state.boundary_state[b] = 0.0;
            }
            response->response_magnitude +=
                fabs(sc->current_state.boundary_state[b] - old_val);
        }
    }

    /* Compute organization change */
    double ref_diff = 0.0;
    for (int i = 0; i < n; i++) {
        double d = conc[i] - sc->reference_state.concentrations[i];
        ref_diff += d * d;
    }
    ref_diff = sqrt(ref_diff);
    sc->current_state.organization_measure = exp(-ref_diff);

    /* Response characterization */
    response->recovery_time = pert->duration * 2.0; /* Estimate */
    response->structural_change = 1.0 - sc->current_state.organization_measure;

    /* Check state transition */
    int org_lost = (sc->current_state.organization_measure < 0.5);
    response->state_transition = org_lost ? 1 : 0;

    /* Update history */
    if (sc->perturbation_count < SC_MAX_HISTORY) {
        sc->perturbation_history[sc->perturbation_count] = *pert;
        sc->perturbation_count++;
    }
    if (sc->response_count < SC_MAX_HISTORY) {
        sc->response_history[sc->response_count] = *response;
        sc->response_count++;
    }

    /* Update coupling strength */
    sc->coupling_strength = 0.5 * sc->coupling_strength +
        0.5 * response->structural_change;

    /* Track adaptation */
    if (response->structural_change > 0.0 && !response->state_transition) {
        sc->adaptation_rate = 0.9 * sc->adaptation_rate +
            0.1 * (1.0 - response->structural_change);
    }

    sc->time += pert->duration;
}

double sc_structural_determinism_index(const sc_coupling_t *sc,
                                        const sc_perturbation_t *pert)
{
    if (!sc || !pert) return 0.0;

    /* Structural determinism: the system's response is determined by its
     * structure, not by the perturbation type.
     *
     * SDI = 1 - (perturbation_type_sensitivity)
     * High SDI → response dominated by system's own structure.
     * Low SDI → response dominated by perturbation characteristics.
     */

    /* For informational perturbations, SDI is maximal (the perturbation
     * is just a trigger, the response is fully structurally determined) */
    if (pert->type == SC_PERTURB_INFORMATIONAL) {
        return 0.95;
    }

    /* For other types, SDI depends on organization measure */
    double base_sdi = sc->current_state.organization_measure;

    /* Correct for perturbation magnitude: larger perturbations reduce SDI */
    double mag_factor = exp(-fabs(pert->magnitude) * 0.1);

    return base_sdi * mag_factor;
}

/* ==========================================================================
 * Structural Drift
 * ========================================================================== */

void sc_evolve_coupling(sc_coupling_t *sc, double dt)
{
    if (!sc || dt <= 0.0) return;

    /* Structural drift: the system's structure changes continuously
     * due to recurrent perturbations and internal dynamics.
     *
     * The drift is modeled as diffusion in concentration space plus
     * a restoring force toward the reference state. */

    int n = sc->current_state.n_components;
    double *conc = sc->current_state.concentrations;
    double *ref = sc->reference_state.concentrations;

    /* Drift coefficient: proportional to coupling strength */
    double drift_coeff = sc->coupling_strength * 0.1;

    /* Temperature-dependent noise */
    double noise_amplitude = sqrt(2.0 * drift_coeff * sc->env_temperature / 298.0) * sqrt(dt);

    for (int i = 0; i < n; i++) {
        /* Restoring force toward reference */
        double restoring = -0.5 * (conc[i] - ref[i]);

        /* Random drift (simplified: pseudo-random from time) */
        double noise = noise_amplitude * sin(sc->time * (double)(i + 1) * 3.14159 +
                                               (double)(i * 7));

        conc[i] += (restoring + noise) * dt;

        /* Environmental resource coupling */
        if (i < sc->env_resource_count && sc->env_resource_levels[i] > 0.0) {
            conc[i] += 0.01 * sc->env_resource_levels[i] * dt;
        }

        if (conc[i] < 0.0) conc[i] = 0.0;
    }

    /* Update organization measure */
    double ref_diff = 0.0;
    for (int i = 0; i < n; i++) {
        double d = conc[i] - ref[i];
        ref_diff += d * d;
    }
    ref_diff = sqrt(ref_diff);
    sc->current_state.organization_measure = exp(-ref_diff * 0.5);

    /* Accumulate structural drift */
    sc->structural_drift += ref_diff * dt;

    sc->time += dt;
}

double sc_compute_structural_drift(const sc_coupling_t *sc)
{
    if (!sc) return 0.0;

    /* Compute accumulated structural drift */
    int n = sc->current_state.n_components;
    double *conc = sc->current_state.concentrations;
    double *ref = sc->reference_state.concentrations;

    double drift = 0.0;
    for (int i = 0; i < n; i++) {
        double d = conc[i] - ref[i];
        drift += d * d;
    }
    return sqrt(drift);
}

int sc_organization_preserved(const sc_coupling_t *sc, double tolerance)
{
    if (!sc) return 0;
    return (sc->current_state.organization_measure >= (1.0 - tolerance)) ? 1 : 0;
}

/* ==========================================================================
 * Niche Construction (L7 — Application)
 * ========================================================================== */

void sc_niche_construction(sc_coupling_t *sc, int resource_id,
                             double modification_amount)
{
    if (!sc || resource_id < 0 || resource_id >= sc->env_resource_count) return;

    /* Niche construction: the system modifies its environment.
     * This creates a feedback loop: system changes environment →
     * environment changes system → system adapts → etc.
     *
     * Example: organisms modify soil chemistry, beavers build dams,
     * social systems create institutions. */

    double old_level = sc->env_resource_levels[resource_id];
    sc->env_resource_levels[resource_id] += modification_amount;

    /* Resource cannot go negative */
    if (sc->env_resource_levels[resource_id] < 0.0) {
        sc->env_resource_levels[resource_id] = 0.0;
    }

    /* The modification affects coupling strength */
    double actual_change = sc->env_resource_levels[resource_id] - old_level;
    sc->coupling_strength += 0.01 * fabs(actual_change);
    if (sc->coupling_strength > 1.0) sc->coupling_strength = 1.0;
}

double sc_niche_construction_index(const sc_coupling_t *sc)
{
    if (!sc || sc->env_resource_count == 0) return 0.0;

    /* Measure: how much has the resource profile changed from default?
     * Default: all resources at 1.0 (normalized). */
    double deviation = 0.0;
    for (int i = 0; i < sc->env_resource_count; i++) {
        double d = sc->env_resource_levels[i] - 1.0;
        deviation += d * d;
    }
    deviation = sqrt(deviation) / sqrt((double)sc->env_resource_count);

    /* Normalize to [0, 1] */
    return 1.0 - exp(-deviation);
}

/* ==========================================================================
 * Social Coupling (L7 — Luhmann's Social Autopoiesis)
 * ========================================================================== */

double sc_cross_coupling_measure(const sc_coupling_t *sc1,
                                  const sc_coupling_t *sc2)
{
    if (!sc1 || !sc2) return 0.0;

    /* Cross-coupling between two autopoietic systems measures the degree
     * to which they trigger structural changes in each other. */

    /* Component profile similarity */
    int n1 = sc1->current_state.n_components;
    int n2 = sc2->current_state.n_components;
    int n_min = (n1 < n2) ? n1 : n2;

    double conc_similarity = 0.0;
    for (int i = 0; i < n_min; i++) {
        double c1 = sc1->current_state.concentrations[i];
        double c2 = sc2->current_state.concentrations[i];
        double avg = (c1 + c2) / 2.0;
        if (avg > 1e-10) {
            conc_similarity += 1.0 - fabs(c1 - c2) / (avg * 2.0 + 1e-10);
        } else {
            conc_similarity += 1.0;
        }
    }
    if (n_min > 0) conc_similarity /= (double)n_min;

    /* Organization similarity */
    double org_similarity = 1.0 - fabs(sc1->current_state.organization_measure -
                                        sc2->current_state.organization_measure);

    /* Interaction history */
    double interaction_history = (sc1->perturbation_count + sc2->perturbation_count) /
        (double)(sc1->perturbation_count + sc2->perturbation_count + 10);

    /* Combined measure */
    return 0.35 * conc_similarity + 0.35 * org_similarity + 0.30 * interaction_history;
}

sc_response_t sc_consensual_interaction(sc_coupling_t *sc1,
                                         sc_coupling_t *sc2,
                                         double signal_magnitude)
{
    sc_response_t response;
    memset(&response, 0, sizeof(sc_response_t));

    if (!sc1 || !sc2) return response;

    /* Consensual interaction: sc1 perturbs sc2 with an informational signal.
     * In Luhmann's social autopoiesis, this corresponds to a communication
     * event. The response of sc2 depends on sc2's own structure (structural
     * determinism), not merely on the signal content. */

    sc_perturbation_t signal;
    signal.time = sc1->time;
    signal.component_id = 0; /* Affect the first component as a proxy */
    signal.magnitude = signal_magnitude;
    signal.duration = 0.1;
    signal.type = SC_PERTURB_INFORMATIONAL;

    sc_apply_perturbation(sc2, &signal, &response);

    /* Also record that sc1 was affected by sending the signal */
    sc1->coupling_strength += 0.01 * fabs(signal_magnitude);
    if (sc1->coupling_strength > 1.0) sc1->coupling_strength = 1.0;

    return response;
}

void sc_coupling_print_report(const sc_coupling_t *sc)
{
    if (!sc) { printf("Coupling System: NULL\n"); return; }

    printf("=== Structural Coupling Report ===\n");
    printf("Time: %.3f  |  dt: %.4f\n", sc->time, sc->dt);
    printf("Components: %d  |  Boundary params: %d\n",
           sc->current_state.n_components, sc->current_state.n_boundary_params);
    printf("Organization: %.4f  |  Drift: %.4f\n",
           sc->current_state.organization_measure, sc_compute_structural_drift(sc));
    printf("Coupling Strength: %.4f  |  Adaptation Rate: %.4f\n",
           sc->coupling_strength, sc->adaptation_rate);
    printf("Perturbations: %d  |  Responses: %d\n",
           sc->perturbation_count, sc->response_count);

    printf("Environment: T=%.1fK  pH=%.1f  P=%.2f atm\n",
           sc->env_temperature, sc->env_pH, sc->env_pressure);
    printf("Resources: ");
    for (int i = 0; i < sc->env_resource_count; i++) {
        printf("%.2f ", sc->env_resource_levels[i]);
    }
    printf("\n");

    printf("Niche Construction Index: %.4f\n", sc_niche_construction_index(sc));

    int org_ok = sc_organization_preserved(sc, 0.2);
    printf("Organization Preserved: %s\n", org_ok ? "YES" : "NO");
    printf("====================================\n");
}
