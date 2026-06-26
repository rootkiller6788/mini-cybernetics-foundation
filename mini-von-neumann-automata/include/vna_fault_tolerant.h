#ifndef VNA_FAULT_TOLERANT_H
#define VNA_FAULT_TOLERANT_H

#include "vna_core.h"

/* ============================================================================
 * Fault-Tolerant Computation — von Neumann's Probabilistic Logic
 *
 * Covers: L1 (definitions), L2 (concepts), L3 (mathematical structures),
 *         L4 (von Neumann's error threshold theorem),
 *         L5 (multiplexing algorithms), L7 (applications)
 *
 * In "Probabilistic Logics and the Synthesis of Reliable Organisms from
 * Unreliable Components" (1956), von Neumann showed that arbitrary
 * reliable computation is possible from unreliable components, provided
 * the component error probability is below a critical threshold.
 *
 * This anticipates both fault-tolerant quantum computing and neural
 * network robustness. The key technique is multiplexing with majority
 * voting.
 * ============================================================================ */

/* --- L1: Component Reliability Types --- */

/* A probabilistic NAND gate: with probability (1-ε), outputs the correct
 * NAND of its inputs. With probability ε, outputs the wrong value.
 * Von Neumann's key insight: even with ε > 0, arbitrarily reliable
 * computation is possible via multiplexing. */
typedef struct {
    double error_probability;    /* ε — probability of incorrect output */
    double correct_probability;  /* 1-ε */
    int    fan_in;               /* Number of inputs (typically 2) */
    int    fan_out;              /* Number of outputs this drives */
} VnaProbabilisticGate;

/* A bundle — von Neumann's multiplexing unit.
 * Instead of a single wire carrying one bit, we use N wires (a bundle)
 * carrying the same logical value. The bundle value is determined by
 * majority vote. This redundancy allows error correction. */
typedef struct {
    int     bundle_size;         /* N — number of lines in the bundle */
    uint8_t* lines;              /* Individual line states (0 or 1) */
    int     ones_count;          /* Number of lines currently at state 1 */
    bool    majority_value;      /* Majority vote result */
    double  bundle_error_prob;   /* Probability the bundle value is wrong */
} VnaBundle;

/* A multiplexed NAND gate — von Neumann's restorative organ.
 * Takes two input bundles of size N, produces an output bundle of size N.
 * Internally: N copies of the NAND gate, each taking one random line
 * from each input bundle. This decorrelates errors. */
typedef struct {
    int                multiplexing_degree; /* N */
    VnaProbabilisticGate component;         /* The unreliable gate */
    double             output_error_prob;   /* Computed output error prob */
} VnaMultiplexedGate;

/* A multiplexed computation network — multiple stages of multiplexed
 * gates, with majority-vote restoration between stages.
 * Von Neumann showed that with sufficient multiplexing, the final
 * error probability can be made arbitrarily small. */
typedef struct {
    int    num_stages;              /* Depth of the computation */
    int    multiplexing_degree;     /* N for each stage */
    double component_error;         /* ε of individual components */
    double* stage_error_prob;       /* Error prob after each stage */
    double final_error_prob;        /* Error prob at network output */
    bool   below_threshold;         /* Whether ε < ε_critical */
} VnaMultiplexedNetwork;

/* --- L4: Error Threshold Theorem --- */

/* Von Neumann's error threshold theorem:
 * There exists ε_crit > 0 such that for any component error ε < ε_crit,
 * and any desired output error δ > 0, there exists a multiplexing
 * degree N(ε,δ) such that the multiplexed computation has error < δ.
 *
 * For NAND gates, ε_crit ≈ 0.0107 (von Neumann's calculation).
 * Modern analysis gives ε_crit = (1/2) * (1 - 1/√2) ≈ 0.088 (for
 * majority voting with 3 inputs). */

/* Compute the critical error threshold for a given gate type.
 * For NAND multiplexing: ε_crit ≈ 0.0107. */
double vna_critical_error_threshold(int fan_in, int fan_out);

/* Compute the required multiplexing degree to achieve target_error
 * given component_error. Uses von Neumann's formula:
 * N ≥ log(δ/ε_eff) / log(ε_eff/ε_crit). */
int vna_required_multiplexing(double component_error, double target_error);

/* --- L3: Bundle Operations --- */

/* Create and manage bundles. */
VnaBundle* vna_bundle_create(int bundle_size);
void vna_bundle_free(VnaBundle* bundle);
void vna_bundle_set_value(VnaBundle* bundle, bool value);
void vna_bundle_randomize(VnaBundle* bundle, double noise_level,
                            unsigned int seed);
bool vna_bundle_majority_vote(VnaBundle* bundle);
void vna_bundle_introduce_errors(VnaBundle* bundle, double error_prob,
                                   unsigned int seed);

/* Compute the probability that a bundle of size N has the wrong
 * majority value, given per-line error probability p.
 * Uses the binomial CDF: P(error) = Σ_{k > N/2}^{N} C(N,k) p^k (1-p)^{N-k}. */
double vna_bundle_error_probability(int bundle_size, double line_error);

/* --- L3: Multiplexed Gate Operations --- */

/* Create a multiplexed NAND gate with given degree. */
VnaMultiplexedGate* vna_multiplexed_gate_create(int multiplexing_degree,
                                                  double component_error);
void vna_multiplexed_gate_free(VnaMultiplexedGate* gate);

/* Execute one multiplexed NAND operation.
 * Input: two bundles (A, B). Output: bundle C = NOT(A AND B).
 * Each of the N lines uses a random permutation to select which
 * specific A-line and B-line to feed into each component NAND.
 * This decorrelation is essential for the error theorem to hold. */
bool vna_multiplexed_nand(VnaMultiplexedGate* gate, VnaBundle* a,
                           VnaBundle* b, VnaBundle* c);

/* --- L5: Multiplexed Network Simulation --- */

/* Create and simulate a multiplexed computation network with
 * the specified number of stages. Each stage consists of a
 * multiplexed NAND followed by a majority-vote restoration. */
VnaMultiplexedNetwork* vna_multiplexed_network_create(int num_stages,
                                                       int multiplexing_degree,
                                                       double component_error);
void vna_multiplexed_network_free(VnaMultiplexedNetwork* net);

/* Simulate one forward pass through the multiplexed network.
 * Returns the fraction of trials where the output was correct. */
double vna_multiplexed_network_simulate(VnaMultiplexedNetwork* net,
                                          bool input_a, bool input_b,
                                          int num_trials, unsigned int seed);

/* --- L5: Error Correction via Redundancy --- */

/* Triple Modular Redundancy (TMR): three copies of the computation,
 * majority vote for the result. This is the simplest form of
 * von Neumann multiplexing (N=3). */
bool vna_triple_modular_redundancy(bool result1, bool result2, bool result3);
double vna_tmr_error_probability(double component_error);

/* N-Modular Redundancy: N copies with majority vote.
 * Generalizes TMR to arbitrary odd N. */
double vna_nmr_error_probability(int n, double component_error);

/* --- L7: Application — Reliable Memory from Unreliable Cells --- */

/* Simulate a reliable 1-bit memory cell using unreliable components.
 * Uses von Neumann multiplexing to achieve error probability < 10^{-9}
 * from components with error probability up to 10^{-3}.
 *
 * Reference: von Neumann (1956), "Probabilistic Logics..." */
typedef struct {
    int     multiplexing_degree;
    double  component_error;
    double  achieved_reliability;
    int64_t error_count;
    int64_t total_accesses;
} VnaReliableMemory;

VnaReliableMemory* vna_reliable_memory_create(int multiplexing_degree,
                                                double component_error);
void vna_reliable_memory_free(VnaReliableMemory* mem);
bool vna_reliable_memory_read(VnaReliableMemory* mem);
void vna_reliable_memory_write(VnaReliableMemory* mem, bool value, unsigned int seed);
double vna_reliable_memory_measured_reliability(VnaReliableMemory* mem);

/* --- L7: Application — Reliable Computation in NASA/Aerospace --- */

/* Compute the reliability of a TMR-based flight computer.
 * Model: 3 independent processors with error prob p each, majority vote.
 * The system survives if ≥2 processors produce correct output.
 * Reference: NASA fault-tolerant computing standards (N-STD-1002). */
double vna_tmr_system_reliability(double processor_error_prob);

/* Compute the MTTF (Mean Time To Failure) improvement factor
 * when using N-modular redundancy vs a single component.
 * MTTF_NMR / MTTF_single ≈ (n+1) / (2 * λ * T_repair)
 * (simplified model for repairable systems). */
double vna_nmr_mttf_improvement(int n, double repair_rate, double failure_rate);

#endif /* VNA_FAULT_TOLERANT_H */