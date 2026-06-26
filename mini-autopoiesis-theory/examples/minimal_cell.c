/**
 * @file minimal_cell.c
 * @brief Example: Construct and analyze a minimal autopoietic cell.
 *
 * This example builds a minimal autopoietic system consisting of:
 *   - A metabolic network that converts nutrients into building blocks
 *   - A membrane that encloses the system
 *   - A template that controls reactions
 *
 * It demonstrates:
 *   L6 - Canonical problem: minimal autopoietic cell assembly
 *   L4 - Autopoiesis verification via Maturana-Varela criteria
 *   L2 - Structural coupling with environment
 *
 * Reference: Maturana & Varela (1980), Gánti (1971)
 */
#include "autopoiesis.h"
#include "reaction_network.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void)
{
    printf("============================================================\n");
    printf("  Minimal Autopoietic Cell — Construction and Analysis\n");
    printf("  Based on Maturana & Varela's autopoiesis theory\n");
    printf("============================================================\n\n");

    /* ==================================================================
     * Step 1: Define the chemical components
     * ================================================================== */
    printf("[1] Defining cell components...\n");

    ap_system_t cell;
    ap_system_init(&cell);

    /* Internal metabolic components */
    int nutrient   = ap_add_component(&cell, "Nutrient",    AP_COMPONENT_SUBSTRATE, 100.0, 0.01);
    int atp        = ap_add_component(&cell, "ATP",         AP_COMPONENT_INTERNAL,   10.0, 0.05);
    int amino_acid = ap_add_component(&cell, "Amino_Acid",  AP_COMPONENT_SUBSTRATE,  50.0, 0.02);
    int enzyme_met = ap_add_component(&cell, "Enzyme_Met",  AP_COMPONENT_CATALYST,    5.0, 0.01);
    int enzyme_syn = ap_add_component(&cell, "Enzyme_Syn",  AP_COMPONENT_CATALYST,    3.0, 0.01);

    /* Membrane components */
    int lipid      = ap_add_component(&cell, "Membrane_Lipid", AP_COMPONENT_BOUNDARY, 20.0, 0.005);
    int transport  = ap_add_component(&cell, "Transporter",    AP_COMPONENT_BOUNDARY,  5.0, 0.01);

    /* Genetic template */
    int rna        = ap_add_component(&cell, "RNA_Template", AP_COMPONENT_INTERNAL,   2.0, 0.001);

    printf("  Defined %d components: %s\n", cell.component_count,
           "metabolites, enzymes, membrane, template");

    /* ==================================================================
     * Step 2: Define the production network
     * ================================================================== */
    printf("\n[2] Building production network...\n");

    /* Nutrient → ATP (catalyzed by metabolic enzyme) */
    int subs_nut[] = {nutrient};
    ap_add_production(&cell, nutrient, atp, 0.1, enzyme_met, subs_nut, 1);
    printf("  Nutrient -[Enzyme_Met]-> ATP\n");

    /* ATP → Amino Acid (catalyzed by metabolic enzyme) */
    int subs_atp[] = {atp};
    ap_add_production(&cell, atp, amino_acid, 0.08, enzyme_met, subs_atp, 1);
    printf("  ATP -[Enzyme_Met]-> Amino_Acid\n");

    /* Amino Acid → Enzyme_Met (the enzyme reproduces itself) */
    int subs_aa[] = {amino_acid};
    ap_add_production(&cell, amino_acid, enzyme_met, 0.05, enzyme_met, subs_aa, 1);
    printf("  Amino_Acid -[Enzyme_Met]-> Enzyme_Met (autocatalytic)\n");

    /* Amino Acid → Enzyme_Syn */
    ap_add_production(&cell, amino_acid, enzyme_syn, 0.04, rna, subs_aa, 1);
    printf("  Amino_Acid -[RNA]-> Enzyme_Syn\n");

    /* Amino Acid → Membrane Lipid */
    ap_add_production(&cell, amino_acid, lipid, 0.03, enzyme_syn, subs_aa, 1);
    printf("  Amino_Acid -[Enzyme_Syn]-> Membrane_Lipid\n");

    /* Membrane Lipid → Transporter (membrane self-production) */
    int subs_lip[] = {lipid};
    ap_add_production(&cell, lipid, transport, 0.02, enzyme_syn, subs_lip, 1);
    printf("  Membrane_Lipid -[Enzyme_Syn]-> Transporter\n");

    /* Amino Acid → RNA (information replication) */
    ap_add_production(&cell, amino_acid, rna, 0.02, rna, subs_aa, 1);
    printf("  Amino_Acid -[RNA]-> RNA_Template (autocatalytic)\n");

    /* ==================================================================
     * Step 3: Verify autopoiesis
     * ================================================================== */
    printf("\n[3] Verifying autopoiesis criteria...\n");

    int is_autopoietic = ap_check_autopoiesis(&cell);
    double closure = ap_compute_organizational_closure(&cell);
    int boundary_ok = ap_verify_self_produced_boundary(&cell);

    printf("  Organizational closure:    %.2f\n", closure);
    printf("  Self-produced boundary:     %s\n", boundary_ok ? "YES" : "NO");
    printf("  Autopoietic (M&V criteria): %s\n", is_autopoietic ? "YES" : "NO");

    /* ==================================================================
     * Step 4: Simulate system evolution
     * ================================================================== */
    printf("\n[4] Simulating time evolution...\n");

    for (int step = 0; step < 20; step++) {
        ap_evolve_step(&cell, 0.5);

        if (step % 5 == 0) {
            printf("  t=%.1f  mass=%.2f  org=%.3f  state=",
                   cell.time, cell.total_mass, cell.organization_measure);
            switch (cell.state) {
                case AP_STATE_SELF_PRODUCING:  printf("SELF-PRODUCING\n"); break;
                case AP_STATE_MAINTAINING:     printf("MAINTAINING\n"); break;
                case AP_STATE_DEGRADING:       printf("DEGRADING\n"); break;
                case AP_STATE_ADAPTING:        printf("ADAPTING\n"); break;
                default:                       printf("OTHER\n"); break;
            }
        }
    }

    /* ==================================================================
     * Step 5: Environmental perturbation (structural coupling)
     * ================================================================== */
    printf("\n[5] Testing structural coupling — nutrient shock...\n");

    ap_apply_perturbation(&cell, nutrient, -50.0);
    printf("  Applied nutrient depletion (-50.0)\n");
    printf("  State after perturbation: ");
    switch (cell.state) {
        case AP_STATE_DEGRADING:      printf("DEGRADING\n"); break;
        case AP_STATE_ADAPTING:       printf("ADAPTING\n"); break;
        default:                      printf("OTHER\n"); break;
    }

    /* Let system recover */
    for (int step = 0; step < 10; step++) {
        ap_evolve_step(&cell, 0.5);
    }
    printf("  After recovery: mass=%.2f  org=%.3f\n",
           cell.total_mass, cell.organization_measure);

    /* ==================================================================
     * Step 6: Summary
     * ================================================================== */
    printf("\n[6] Final system state:\n");
    ap_system_print_summary(&cell);

    printf("\n============================================================\n");
    printf("  Conclusion: This minimal cell demonstrates the three\n");
    printf("  defining properties of autopoietic systems:\n");
    printf("    1. Operational closure (all components internally produced)\n");
    printf("    2. Self-produced boundary (membrane synthesized internally)\n");
    printf("    3. Organizational invariance (structure adapts, org persists)\n");
    printf("============================================================\n");

    ap_system_destroy(&cell);
    return 0;
}
