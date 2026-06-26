#ifndef VARIETY_H
#define VARIETY_H

#include "ashby_homeostasis.h"

/* ============================================================================
 * Variety Theory & Ashby's Law of Requisite Variety
 *
 * Ashby, "An Introduction to Cybernetics" (1956), Chapters 7-11.
 *
 * VARIETY: The number of distinguishable states a system can be in.
 *   V = log_2(N) where N is the number of distinguishable states.
 *
 * THE LAW OF REQUISITE VARIETY (Ashby's Law):
 *   "Only variety can destroy variety."
 *
 *   For a regulator R facing disturbance D to protect essential variable E:
 *     V(R) >= V(D) / V(E)   (in the simplest form)
 *
 *   More formally: the capacity of a regulator to block the transmission
 *   of variety from disturbance to essential variable cannot exceed the
 *   regulator's own variety.
 *
 * This law is a fundamental limit on regulation: no regulator can be more
 * effective than its own variety allows. It is the cybernetic analog of
 * the Second Law of Thermodynamics.
 * ============================================================================ */

/** A state set for variety analysis */
typedef struct {
    int* state_ids;         /* Enumeration of distinguishable states */
    double* state_probs;    /* Probability of each state */
    int n_states;
    int capacity;
    double total_variety;   /* H = -sum p log2 p */
    double effective_variety; /* log2 of number of states with p > epsilon */
    double epsilon;          /* Threshold for "effective" presence */
} VarietySpace;

/** A variety channel connecting disturbance to essential variable */
typedef struct {
    VarietySpace* disturbance;     /* Variety at disturbance source */
    VarietySpace* regulation;      /* Variety of the regulator */
    VarietySpace* essential_var;   /* Variety observed at essential variable */
    double channel_capacity;       /* Maximum variety transmittable */
    double variety_blocked;        /* V(D) - V(E) = variety absorbed by R */
    double regulatory_efficiency;  /* V_blocked / V(D) */
    bool satisfies_requisite;      /* True if V(R) >= V(D) */
} VarietyChannel;

/** A multi-stage variety attenuation pipeline */
typedef struct {
    VarietyChannel** stages;
    int n_stages;
    double* stage_attenuation;     /* Variety reduction per stage */
    double total_attenuation;      /* Overall variety reduction */
    double bottleneck_variety;     /* Minimum V(R) across all stages */
    int bottleneck_stage;          /* Index of most constrained stage */
} VarietyPipeline;

/* --- Variety API --- */

/* Variety space */
VarietySpace* variety_space_create(int capacity, double epsilon);
void variety_space_free(VarietySpace* vs);
void variety_space_add_state(VarietySpace* vs, int state_id, double prob);
void variety_space_set_uniform(VarietySpace* vs, int n_states);
void variety_space_compute(VarietySpace* vs);
double variety_space_entropy(const VarietySpace* vs);
double variety_space_effective_n(const VarietySpace* vs);
double variety_space_redundancy(const VarietySpace* vs);
void variety_space_merge(VarietySpace* dest, const VarietySpace* src);
void variety_space_print(const VarietySpace* vs);

/* Variety estimation from data */
double variety_estimate_from_samples(const double* samples, int n,
                                      double bin_width);
double variety_estimate_from_histogram(const int* counts, int n_bins,
                                        int total);
double variety_estimate_adaptive(const double* samples, int n, int max_bins);
double variety_log_dimension(const double* samples, int n, int embedding_dim,
                              double radius);

/* Variety channel */
VarietyChannel* variety_channel_create(void);
void variety_channel_free(VarietyChannel* vc);
void variety_channel_set_disturbance(VarietyChannel* vc,
                                      const VarietySpace* vs);
void variety_channel_set_regulation(VarietyChannel* vc,
                                     const VarietySpace* vs);
void variety_channel_set_essential(VarietyChannel* vc,
                                    const VarietySpace* vs);
void variety_channel_evaluate(VarietyChannel* vc);
bool variety_channel_check_requisite(const VarietyChannel* vc);
double variety_channel_excess(const VarietyChannel* vc);
void variety_channel_print(const VarietyChannel* vc);

/* Variety pipeline */
VarietyPipeline* variety_pipeline_create(int n_stages);
void variety_pipeline_free(VarietyPipeline* vp);
void variety_pipeline_set_stage(VarietyPipeline* vp, int stage_idx,
                                 VarietyChannel* vc);
void variety_pipeline_analyze(VarietyPipeline* vp);
double variety_pipeline_residual(const VarietyPipeline* vp);
bool variety_pipeline_feasible(const VarietyPipeline* vp);
int variety_pipeline_find_bottleneck(const VarietyPipeline* vp);
void variety_pipeline_print(const VarietyPipeline* vp);

/* Ashby's Law computations */
double variety_requisite_minimum(double V_disturbance,
                                  double V_essential_target);
double variety_regulation_effectiveness(double V_regulator,
                                         double V_disturbance);
double variety_surplus(const VarietyChannel* vc);
double variety_amplification_needed(const VarietyChannel* vc);

/* Information-theoretic connections */
double variety_to_channel_capacity(double variety, double bandwidth,
                                    double snr_db);
double variety_conditional_entropy(const VarietySpace* j,
                                    const double* conditional_probs,
                                    int n_conditions);
double variety_mutual_information_from_channel(const VarietyChannel* vc);
double variety_rate_distortion(double input_variety,
                                double* distortion_matrix,
                                int n_input, int n_output);

/* Variety engineering strategies */
double variety_attenuation_by_filtering(double input_variety,
                                         double filter_selectivity);
double variety_attenuation_by_buffering(double input_variety,
                                         double buffer_capacity);
double variety_attenuation_by_prediction(double input_variety,
                                          double prediction_accuracy);
double variety_amplification_required(double target_variety,
                                       double current_variety);

#endif /* VARIETY_H */