#include "variety.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void) {
    printf("=== Ashby's Law of Requisite Variety ===\n\n");
    printf("'Only variety can destroy variety.' - W. Ross Ashby\n");
    printf("This example demonstrates Ashby's Law applied to\n");
    printf("regulation problems at different scales.\n\n");

    srand(42);

    /* ----- Example 1: Basic Variety Channel ----- */
    printf("--- Example 1: Simple Regulation Problem ---\n");
    VarietySpace* vd = variety_space_create(20, 0.01);
    variety_space_set_uniform(vd, 16); /* 16 disturbance states -> V(D)=4 bits */
    variety_space_compute(vd);
    printf("Disturbance variety V(D) = %.2f bits\n", vd->total_variety);

    VarietySpace* vr = variety_space_create(20, 0.01);
    variety_space_set_uniform(vr, 8);  /* 8 regulation states -> V(R)=3 bits */
    variety_space_compute(vr);
    printf("Regulator variety V(R) = %.2f bits\n", vr->total_variety);

    VarietySpace* ve = variety_space_create(20, 0.01);
    variety_space_set_uniform(ve, 2);  /* 2 residual states -> V(E)=1 bit */
    variety_space_compute(ve);
    printf("Essential variable variety V(E) = %.2f bits\n\n", ve->total_variety);

    VarietyChannel* vc = variety_channel_create();
    variety_channel_set_disturbance(vc, vd);
    variety_channel_set_regulation(vc, vr);
    variety_channel_set_essential(vc, ve);
    variety_channel_evaluate(vc);

    printf("Channel evaluation:\n");
    printf("  Variety blocked: %.2f bits\n", vc->variety_blocked);
    printf("  Regulatory efficiency: %.1f%%\n", vc->regulatory_efficiency * 100.0);
    printf("  Requisite variety satisfied: %s\n",
           vc->satisfies_requisite ? "YES" : "NO - need more regulator variety!");

    double req = variety_requisite_minimum(vd->total_variety, ve->total_variety);
    printf("  Minimum V(R) required: %.2f bits (have: %.2f bits)\n",
           req, vr->total_variety);

    /* ----- Example 2: Variety estimation from data ----- */
    printf("\n--- Example 2: Estimating Variety from Data ---\n");
    double data[100];
    for (int i = 0; i < 100; i++)
        data[i] = 5.0 + 2.0 * ((double)rand() / RAND_MAX - 0.5);
    double V_est = variety_estimate_from_samples(data, 100, 0.2);
    printf("100 random samples, bin_width=0.2: V = %.4f bits\n", V_est);

    double V_adapt = variety_estimate_adaptive(data, 100, 20);
    printf("Adaptive binning (max 20 bins): V = %.4f bits\n", V_adapt);

    /* ----- Example 3: Variety engineering strategies ----- */
    printf("\n--- Example 3: Variety Engineering Strategies ---\n");
    double V_input = 8.0; /* 8 bits of disturbance variety */
    printf("Input variety: %.1f bits\n", V_input);
    printf("  After filtering (selectivity=0.8):  %.2f bits\n",
           variety_attenuation_by_filtering(V_input, 0.8));
    printf("  After buffering (capacity=10):       %.2f bits\n",
           variety_attenuation_by_buffering(V_input, 10.0));
    printf("  After prediction (accuracy=0.9):    %.2f bits\n",
           variety_attenuation_by_prediction(V_input, 0.9));
    printf("  Amplification needed (target=12):   %.2fx\n",
           variety_amplification_required(12.0, V_input));

    /* ----- Example 4: Variety pipeline ----- */
    printf("\n--- Example 4: Multi-Stage Variety Pipeline ---\n");
    VarietyPipeline* vp = variety_pipeline_create(3);
    /* Stage 1: High variety reduction */
    VarietyChannel* s1 = variety_channel_create();
    VarietySpace* vd1 = variety_space_create(10, 0.01);
    variety_space_set_uniform(vd1, 16); variety_space_compute(vd1);
    VarietySpace* vr1 = variety_space_create(10, 0.01);
    variety_space_set_uniform(vr1, 8); variety_space_compute(vr1);
    VarietySpace* ve1 = variety_space_create(10, 0.01);
    variety_space_set_uniform(ve1, 4); variety_space_compute(ve1);
    variety_channel_set_disturbance(s1, vd1);
    variety_channel_set_regulation(s1, vr1);
    variety_channel_set_essential(s1, ve1);
    variety_pipeline_set_stage(vp, 0, s1);
    /* Stage 2 & 3 similar but with decreasing variety */
    for (int stage = 1; stage < 3; stage++) {
        VarietyChannel* sc = variety_channel_create();
        VarietySpace* vdd = variety_space_create(10, 0.01);
        variety_space_set_uniform(vdd, 8 >> stage); variety_space_compute(vdd);
        VarietySpace* vrr = variety_space_create(10, 0.01);
        variety_space_set_uniform(vrr, 4 >> (stage-1)); variety_space_compute(vrr);
        VarietySpace* vee = variety_space_create(10, 0.01);
        variety_space_set_uniform(vee, 2 >> (stage-1)); variety_space_compute(vee);
        variety_channel_set_disturbance(sc, vdd);
        variety_channel_set_regulation(sc, vrr);
        variety_channel_set_essential(sc, vee);
        variety_pipeline_set_stage(vp, stage, sc);
    }
    variety_pipeline_analyze(vp);
    printf("Pipeline: %d stages, feasible=%s, bottleneck at stage %d\n",
           vp->n_stages, variety_pipeline_feasible(vp) ? "YES" : "NO",
           vp->bottleneck_stage);
    printf("Total attenuation: %.2f bits, Residual: %.2f bits\n",
           vp->total_attenuation, variety_pipeline_residual(vp));

    /* Cleanup */
    variety_channel_free(vc);
    variety_space_free(vd); variety_space_free(vr); variety_space_free(ve);
    variety_space_free(vd1); variety_space_free(vr1); variety_space_free(ve1);
    variety_pipeline_free(vp);

    printf("\nDemonstration complete. Ashby's Law holds: V(R) >= V(D) is required for perfect regulation.\n");
    return 0;
}