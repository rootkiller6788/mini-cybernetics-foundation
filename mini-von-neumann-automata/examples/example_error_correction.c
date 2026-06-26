#include "vna_fault_tolerant.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * Example: Von Neumann's Fault-Tolerant Computation
 *
 * Demonstrates:
 *   - Von Neumann's error threshold theorem
 *   - Multiplexed NAND gates
 *   - Triple Modular Redundancy (TMR)
 *   - N-Modular Redundancy reliability analysis
 *   - NASA/aerospace reliability calculations
 *   - Reliable memory from unreliable components
 * ============================================================================ */

int main(void) {
    printf("=== Von Neumann's Fault-Tolerant Computation ===\n\n");

    /* --- Error Threshold Theorem --- */
    printf("1. Error Threshold Theorem (von Neumann, 1956)\n");
    double ecrit = vna_critical_error_threshold(2, 1);
    printf("   Critical error threshold (NAND gate): ε_crit = %.6f\n", ecrit);

    /* Demonstrate the threshold effect */
    printf("\n   Multiplexing requirements:\n");
    double errors[] = {0.0001, 0.001, 0.005, 0.01, 0.05, 0.1};
    double target = 1e-9;
    for (int i = 0; i < 6; i++) {
        int n = vna_required_multiplexing(errors[i], target);
        printf("   ε = %.4f → ", errors[i]);
        if (n > 0)
            printf("N = %d required for δ < 1e-9\n", n);
        else if (errors[i] >= ecrit)
            printf("ABOVE THRESHOLD — cannot achieve reliability\n");
        else
            printf("N too large (exceeds bounds)\n");
    }

    /* --- Triple Modular Redundancy --- */
    printf("\n2. Triple Modular Redundancy (TMR)\n");
    printf("   TMR: three independent computations, majority vote\n");

    /* Show TMR error reduction */
    printf("\n   Component error → TMR system error:\n");
    for (int i = 1; i <= 5; i++) {
        double eps = i * 0.01;
        double tmr_err = vna_tmr_error_probability(eps);
        double single_reliability = 1.0 - eps;
        double tmr_reliability = vna_tmr_system_reliability(eps);

        printf("   ε = %.2f:  single R = %.4f  →  TMR R = %.6f  "
               "(error reduced by factor %.0fx)\n",
               eps, single_reliability, tmr_reliability,
               eps / tmr_err);
    }

    /* --- N-Modular Redundancy Comparison --- */
    printf("\n3. N-Modular Redundancy Comparison\n");
    printf("   Component error ε = 0.01:\n");
    printf("   %-4s  %-16s  %-16s\n", "N", "P(error)", "Reliability");
    for (int n = 1; n <= 9; n += 2) {
        double err = vna_nmr_error_probability(n, 0.01);
        printf("   %d    %.10f  %.10f\n", n, err, 1.0 - err);
    }

    /* --- Multiplexed NAND Demonstration --- */
    printf("\n4. Multiplexed NAND Gate Demonstration\n");
    printf("   Component error ε = 0.005, N = 101\n");

    VnaMultiplexedGate* gate = vna_multiplexed_gate_create(101, 0.005);
    printf("   Theoretical output error: %.10f\n", gate->output_error_prob);

    /* Test NAND truth table with 1000 trials per case */
    printf("   NAND truth table verification (100 trials each):\n");
    bool inputs[4][2] = {{0,0}, {0,1}, {1,0}, {1,1}};

    for (int c = 0; c < 4; c++) {
        VnaBundle* a = vna_bundle_create(101);
        VnaBundle* b = vna_bundle_create(101);
        VnaBundle* out = vna_bundle_create(101);
        vna_bundle_set_value(a, inputs[c][0]);
        vna_bundle_set_value(b, inputs[c][1]);

        int correct = 0;
        for (int trial = 0; trial < 100; trial++) {
            bool result = vna_multiplexed_nand(gate, a, b, out);
            bool expected = !(inputs[c][0] && inputs[c][1]);
            if (result == expected) correct++;
            /* Re-randomize bundle lines to simulate fresh trial */
            vna_bundle_introduce_errors(a, 0.005, (unsigned int)(1000 + trial));
            vna_bundle_introduce_errors(b, 0.005, (unsigned int)(2000 + trial));
        }
        printf("   NAND(%d,%d) = %d: %d/100 correct (%.1f%%)\n",
               inputs[c][0], inputs[c][1], !(inputs[c][0] && inputs[c][1]),
               correct, correct * 1.0);

        vna_bundle_free(a);
        vna_bundle_free(b);
        vna_bundle_free(out);
    }
    vna_multiplexed_gate_free(gate);

    /* --- NASA/Aerospace Application --- */
    printf("\n5. NASA Flight Computer Reliability (TMR Architecture)\n");
    printf("   Model: 3 independent processors, majority vote\n");

    double processor_errors[] = {1e-9, 1e-6, 1e-3, 0.01};
    printf("   %-16s  %-16s\n", "Processor P(err)", "System Reliability");
    for (int i = 0; i < 4; i++) {
        double sys_r = vna_tmr_system_reliability(processor_errors[i]);
        printf("   %-16.0e  %.12f\n", processor_errors[i], sys_r);
    }

    /* MTTF comparison */
    printf("\n   MTTF Improvement with NMR (λ=0.001/hr, μ=0.1/hr):\n");
    printf("   %-4s  %-16s\n", "N", "MTTF factor");
    for (int n = 3; n <= 9; n += 2) {
        double factor = vna_nmr_mttf_improvement(n, 0.1, 0.001);
        printf("   %d    %.2fx\n", n, factor);
    }

    /* --- Reliable Memory --- */
    printf("\n6. Reliable Memory Cell\n");
    for (int n = 3; n <= 31; n *= 2) {
        VnaReliableMemory* mem = vna_reliable_memory_create(n + 1, 0.001);
        if (mem) {
            printf("   N = %d, ε = 0.001 → achieved reliability ≈ %.10f\n",
                   mem->multiplexing_degree, mem->achieved_reliability);
            vna_reliable_memory_free(mem);
        }
    }

    /* --- Bundle Error Probability --- */
    printf("\n7. Bundle Error Probability Analysis\n");
    printf("   Line error p = 0.1:\n");
    for (int n = 1; n <= 15; n += 2) {
        double bundle_err = vna_bundle_error_probability(n, 0.1);
        printf("   N = %2d:  P(bundle error) = %.10f\n", n, bundle_err);
    }

    printf("\n[Fault-tolerant computation demonstration complete]\n");
    return 0;
}
