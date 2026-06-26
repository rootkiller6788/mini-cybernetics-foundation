#include "vna_fault_tolerant.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * L4: Von Neumann's Error Threshold Theorem
 *
 * Von Neumann (1956) proved that reliable computation is possible from
 * unreliable components iff the component error probability ε < ε_crit.
 * For NAND gates with multiplexing, ε_crit ≈ 0.0107.
 *
 * The key formula:
 *   ε_out ≈ ε_crit * (ε / ε_crit)^{2^d}
 * where d is the number of multiplexing stages and ε is the component error.
 *
 * This is the foundation of fault-tolerant computing.
 * ============================================================================ */

double vna_critical_error_threshold(int fan_in, int fan_out) {
    /* Von Neumann's calculated thresholds for different gate types.
     * For standard NAND gates (fan_in=2): ε_crit = (1 - 1/√e)/2 ≈ 0.088
     * but von Neumann's more conservative bound gives ≈ 0.0107.
     *
     * The threshold depends on the gate's ability to restore signals.
     * More inputs → higher threshold (more redundancy in the gate itself). */
    double base_threshold = 0.0107; /* von Neumann's NAND threshold */

    /* Adjust for fan-in: more inputs provide implicit redundancy */
    if (fan_in > 2) base_threshold *= sqrt((double)fan_in / 2.0);

    /* Adjust for fan-out: high fan-out increases error probability */
    if (fan_out > 1) base_threshold /= sqrt((double)fan_out);

    /* Clamp to physically meaningful range */
    if (base_threshold > 0.5) base_threshold = 0.5;
    if (base_threshold < 0.001) base_threshold = 0.001;

    return base_threshold;
}

int vna_required_multiplexing(double component_error, double target_error) {
    /* Solve von Neumann's formula for N:
     *   target_error ≈ ε_crit * (component_error / ε_crit)^{2^d}
     * where d ≈ log2(N).
     *
     * Rearranging:
     *   log(target_error/ε_crit) = 2^d * log(ε/ε_crit)
     *   2^d = log(target_error/ε_crit) / log(ε/ε_crit)
     *   d = log2(log(target/ε_crit) / log(ε/ε_crit))
     *   N ≈ 2^d */

    double eps_crit = vna_critical_error_threshold(2, 1);

    if (component_error <= 0.0) return 1;
    if (component_error >= eps_crit) return -1; /* Cannot achieve reliability */
    if (target_error >= component_error) return 1;

    double log_ratio_target = log(target_error / eps_crit);
    double log_ratio_comp = log(component_error / eps_crit);

    if (log_ratio_comp >= 0.0) return -1;
    if (log_ratio_target <= 0.0) return 1;

    double exponent = log_ratio_target / log_ratio_comp;
    if (exponent <= 0.0) return -1;
    if (exponent >= 1.0) return -1; /* Would need negative depth */

    double stages_d = log2(exponent);
    if (stages_d < 0.0) stages_d = 0.0;

    /* Map stages to multiplexing degree:
     * Each stage approximately squares the effective error reduction.
     * N = 2 * 2^d (odd multiplexing) */
    int n = (int)(2.0 * pow(2.0, stages_d));
    if (n < 3) n = 3;
    if (n % 2 == 0) n++; /* Ensure odd */

    /* Verify: compute actual error probability for this N */
    double actual_error = 1.0 - vna_multiplexing_reliability(n, component_error);
    /* If not good enough, increase N */
    while (actual_error > target_error && n < 10001) {
        n += 2;
        actual_error = 1.0 - vna_multiplexing_reliability(n, component_error);
    }

    return (n > 10000) ? -1 : n;
}

/* ============================================================================
 * L3: Bundle Operations
 * ============================================================================ */

VnaBundle* vna_bundle_create(int bundle_size) {
    if (bundle_size < 1) bundle_size = 1;
    VnaBundle* bundle = (VnaBundle*)calloc(1, sizeof(VnaBundle));
    if (!bundle) return NULL;
    bundle->bundle_size = bundle_size;
    bundle->lines = (uint8_t*)calloc(bundle_size, sizeof(uint8_t));
    if (!bundle->lines) {
        free(bundle);
        return NULL;
    }
    return bundle;
}

void vna_bundle_free(VnaBundle* bundle) {
    if (!bundle) return;
    free(bundle->lines);
    free(bundle);
}

void vna_bundle_set_value(VnaBundle* bundle, bool value) {
    if (!bundle || !bundle->lines) return;
    memset(bundle->lines, value ? 1 : 0, bundle->bundle_size);
    bundle->ones_count = value ? bundle->bundle_size : 0;
    bundle->majority_value = value;
}

void vna_bundle_randomize(VnaBundle* bundle, double noise_level,
                            unsigned int seed) {
    if (!bundle || !bundle->lines) return;
    unsigned int local_seed = seed ? seed : 42;

    /* Set lines randomly but biased toward the current majority value */
    bool target = bundle->majority_value;
    int ones = 0;
    for (int i = 0; i < bundle->bundle_size; i++) {
        double r = (double)((local_seed = local_seed * 1103515245 + 12345) &
                   0x7fffffff) / 0x7fffffff;
        if (r < noise_level) {
            bundle->lines[i] = target ? 0 : 1;
        } else {
            bundle->lines[i] = target ? 1 : 0;
        }
        if (bundle->lines[i]) ones++;
    }
    bundle->ones_count = ones;
    bundle->majority_value = (ones > bundle->bundle_size / 2);
}

bool vna_bundle_majority_vote(VnaBundle* bundle) {
    if (!bundle || !bundle->lines) return false;
    int ones = 0;
    for (int i = 0; i < bundle->bundle_size; i++)
        if (bundle->lines[i]) ones++;
    bundle->ones_count = ones;
    bundle->majority_value = (ones > bundle->bundle_size / 2);
    return bundle->majority_value;
}

void vna_bundle_introduce_errors(VnaBundle* bundle, double error_prob,
                                   unsigned int seed) {
    if (!bundle || !bundle->lines) return;
    unsigned int local_seed = seed ? seed : 12345;
    for (int i = 0; i < bundle->bundle_size; i++) {
        double r = (double)((local_seed = local_seed * 1103515245 + 12345) &
                   0x7fffffff) / 0x7fffffff;
        if (r < error_prob) {
            bundle->lines[i] = bundle->lines[i] ? 0 : 1;
        }
    }
    vna_bundle_majority_vote(bundle);
}

double vna_bundle_error_probability(int bundle_size, double line_error) {
    /* P(bundle wrong) = P(ones ≤ N/2)
     * = Σ_{k=0}^{N/2} C(N,k) p_line^k (1-p_line)^{N-k}
     * for a bundle meant to represent 1. */
    if (bundle_size < 1) return 1.0;
    int majority_threshold = bundle_size / 2;
    double prob_error = 0.0;

    for (int k = 0; k <= majority_threshold; k++) {
        double binom = 1.0;
        for (int i = 1; i <= k; i++) {
            binom *= (bundle_size - k + i);
            binom /= i;
        }
        prob_error += binom * pow(line_error, k) *
                       pow(1.0 - line_error, bundle_size - k);
    }
    return prob_error;
}

/* ============================================================================
 * L3: Multiplexed Gate Operations
 * ============================================================================ */

VnaMultiplexedGate* vna_multiplexed_gate_create(int multiplexing_degree,
                                                  double component_error) {
    VnaMultiplexedGate* gate =
        (VnaMultiplexedGate*)calloc(1, sizeof(VnaMultiplexedGate));
    if (!gate) return NULL;
    if (multiplexing_degree < 3) multiplexing_degree = 3;
    if (multiplexing_degree % 2 == 0) multiplexing_degree++;
    gate->multiplexing_degree = multiplexing_degree;
    gate->component.error_probability = component_error;
    gate->component.correct_probability = 1.0 - component_error;
    gate->component.fan_in = 2;
    gate->component.fan_out = 1;

    /* Compute output error probability */
    gate->output_error_prob = vna_bundle_error_probability(multiplexing_degree,
                                                            component_error);
    return gate;
}

void vna_multiplexed_gate_free(VnaMultiplexedGate* gate) {
    free(gate);
}

bool vna_multiplexed_nand(VnaMultiplexedGate* gate, VnaBundle* a,
                           VnaBundle* b, VnaBundle* c) {
    if (!gate || !a || !b || !c) return false;
    int n = gate->multiplexing_degree;
    double eps = gate->component.error_probability;
    static unsigned int seed = 54321;

    for (int i = 0; i < n; i++) {
        /* Random permutation: pick one line from each input bundle.
         * Von Neumann showed that random permutation decorrelates errors,
         * making the majority vote effective. */
        int ia = (int)(((seed = seed * 1103515245 + 12345) & 0x7fffffff) %
                       a->bundle_size);
        int ib = (int)(((seed = seed * 1103515245 + 12345) & 0x7fffffff) %
                       b->bundle_size);

        int input_a = a->lines[ia];
        int input_b = b->lines[ib];

        /* Compute NAND */
        int correct_output = !(input_a && input_b);

        /* Apply component error */
        double r = (double)((seed = seed * 1103515245 + 12345) & 0x7fffffff) /
                   0x7fffffff;
        if (r < eps) {
            correct_output = !correct_output; /* Flip */
        }

        c->lines[i] = (uint8_t)correct_output;
    }

    return vna_bundle_majority_vote(c);
}

/* ============================================================================
 * L5: Multiplexed Network Simulation
 * ============================================================================ */

VnaMultiplexedNetwork* vna_multiplexed_network_create(int num_stages,
                                                       int multiplexing_degree,
                                                       double component_error) {
    if (num_stages < 1) num_stages = 1;
    VnaMultiplexedNetwork* net =
        (VnaMultiplexedNetwork*)calloc(1, sizeof(VnaMultiplexedNetwork));
    if (!net) return NULL;
    net->num_stages = num_stages;
    net->multiplexing_degree = multiplexing_degree;
    net->component_error = component_error;
    net->stage_error_prob = (double*)calloc(num_stages, sizeof(double));
    if (!net->stage_error_prob) {
        free(net);
        return NULL;
    }

    /* Compute theoretical error probabilities for each stage */
    double eps = component_error;
    double eps_crit = vna_critical_error_threshold(2, 1);
    net->below_threshold = (eps < eps_crit);

    for (int s = 0; s < num_stages; s++) {
        net->stage_error_prob[s] = eps;
        /* After one stage of multiplexing:
         * ε_{s+1} ≈ ε_crit * (ε_s / ε_crit)^2 */
        if (eps < eps_crit) {
            eps = eps_crit * pow(eps / eps_crit, 2.0);
        }
    }
    net->final_error_prob = (num_stages > 0) ?
        net->stage_error_prob[num_stages - 1] : eps;

    return net;
}

void vna_multiplexed_network_free(VnaMultiplexedNetwork* net) {
    if (!net) return;
    free(net->stage_error_prob);
    free(net);
}

double vna_multiplexed_network_simulate(VnaMultiplexedNetwork* net,
                                          bool input_a, bool input_b,
                                          int num_trials, unsigned int seed) {
    if (!net) return 0.0;

    VnaMultiplexedGate* gate = vna_multiplexed_gate_create(
        net->multiplexing_degree, net->component_error);
    if (!gate) return 0.0;

    int correct_count = 0;
    bool expected_output = !(input_a && input_b); /* NAND */

    for (int trial = 0; trial < num_trials; trial++) {
        VnaBundle* a = vna_bundle_create(net->multiplexing_degree);
        VnaBundle* b = vna_bundle_create(net->multiplexing_degree);
        VnaBundle* c = vna_bundle_create(net->multiplexing_degree);
        if (!a || !b || !c) {
            vna_bundle_free(a); vna_bundle_free(b); vna_bundle_free(c);
            continue;
        }

        vna_bundle_set_value(a, input_a);
        vna_bundle_set_value(b, input_b);

        /* Multi-stage computation */
        for (int stage = 0; stage < net->num_stages; stage++) {
            vna_multiplexed_nand(gate, a, b, c);

            /* Swap: c becomes a for next stage, create fresh b, c */
            VnaBundle* temp = a;
            a = c;
            bool prev_result = vna_bundle_majority_vote(a);
            vna_bundle_set_value(b, prev_result ? input_b : !input_b);
            c = vna_bundle_create(net->multiplexing_degree);
            if (!c) { vna_bundle_free(temp); c = a; break; }
            if (stage > 0) vna_bundle_free(temp); /* free old a */
        }

        bool result = vna_bundle_majority_vote(a);
        if (result == expected_output) correct_count++;

        vna_bundle_free(a);
        vna_bundle_free(b);
        vna_bundle_free(c);
        /* Refresh seed */
        seed = seed * 1103515245 + 12345;
    }

    vna_multiplexed_gate_free(gate);
    return (double)correct_count / num_trials;
}

/* ============================================================================
 * L5: Error Correction via Redundancy
 * ============================================================================ */

bool vna_triple_modular_redundancy(bool r1, bool r2, bool r3) {
    int votes = (r1 ? 1 : 0) + (r2 ? 1 : 0) + (r3 ? 1 : 0);
    return votes >= 2;
}

double vna_tmr_error_probability(double component_error) {
    /* TMR fails if ≥2 of 3 components fail.
     * P(fail) = C(3,2) * ε^2 * (1-ε) + C(3,3) * ε^3
     *          = 3ε^2(1-ε) + ε^3 = 3ε^2 - 2ε^3 */
    double eps = component_error;
    return 3.0 * eps * eps - 2.0 * eps * eps * eps;
}

double vna_nmr_error_probability(int n, double component_error) {
    /* N-modular redundancy with odd N:
     * P(fail) = Σ_{k ≥ (N+1)/2}^{N} C(N,k) ε^k (1-ε)^{N-k}
     * For even N, ≥ N/2 + 1 */
    if (n <= 0) return 1.0;
    double prob = 0.0;
    int threshold = n / 2 + 1;

    for (int k = threshold; k <= n; k++) {
        double binom = 1.0;
        for (int i = 1; i <= k; i++) {
            binom *= (n - k + i);
            binom /= i;
        }
        prob += binom * pow(component_error, k) *
                pow(1.0 - component_error, n - k);
    }
    return prob;
}

/* ============================================================================
 * L7: Reliable Memory from Unreliable Cells
 * ============================================================================ */

VnaReliableMemory* vna_reliable_memory_create(int multiplexing_degree,
                                                double component_error) {
    VnaReliableMemory* mem =
        (VnaReliableMemory*)calloc(1, sizeof(VnaReliableMemory));
    if (!mem) return NULL;
    mem->multiplexing_degree = multiplexing_degree;
    mem->component_error = component_error;

    /* Theoretical reliability of the multiplexed memory cell */
    mem->achieved_reliability = vna_multiplexing_reliability(
        multiplexing_degree, component_error);

    return mem;
}

void vna_reliable_memory_free(VnaReliableMemory* mem) {
    free(mem);
}

bool vna_reliable_memory_read(VnaReliableMemory* mem) {
    if (!mem) return false;
    mem->total_accesses++;
    /* In a real implementation, this would read and majority-vote
     * the N copies of the stored bit. */
    return true; /* Simplified: assume read is reliable */
}

void vna_reliable_memory_write(VnaReliableMemory* mem, bool value,
                                 unsigned int seed) {
    if (!mem) return;
    /* Write the value to N copies of the memory cell.
     * Each write has probability ε of storing the wrong value.
     * After writing, the majority vote determines the stored value. */
    int n = mem->multiplexing_degree;
    int correct_copies = 0;
    unsigned int local_seed = seed ? seed : 99999;

    for (int i = 0; i < n; i++) {
        double r = (double)((local_seed = local_seed * 1103515245 + 12345) &
                   0x7fffffff) / 0x7fffffff;
        bool written_correctly = (r >= mem->component_error);
        if (written_correctly == value) correct_copies++;
    }

    /* The stored value is the majority */
    bool stored_value = (correct_copies > n / 2);
    if (stored_value != value) mem->error_count++;

    mem->total_accesses++;
}

double vna_reliable_memory_measured_reliability(VnaReliableMemory* mem) {
    if (!mem || mem->total_accesses == 0) return 1.0;
    return 1.0 - (double)mem->error_count / mem->total_accesses;
}

/* ============================================================================
 * L7: NASA/Aerospace Reliability Calculation
 * ============================================================================ */

double vna_tmr_system_reliability(double processor_error_prob) {
    /* TMR system reliability: system survives if ≥2 of 3 processors work.
     * R_sys = C(3,2) * p_w^2 * p_f + C(3,3) * p_w^3
     *       = 3 * p_w^2 * (1-p_w) + p_w^3
     *       = 3p_w^2 - 2p_w^3
     * where p_w = 1 - processor_error_prob */
    double p_working = 1.0 - processor_error_prob;
    return 3.0 * p_working * p_working - 2.0 * p_working * p_working * p_working;
}

double vna_nmr_mttf_improvement(int n, double repair_rate, double failure_rate) {
    /* MTTF improvement for NMR vs single component.
     * MTTF_single = 1/λ (λ = failure rate)
     * MTTF_NMR ≈ (n+1) / (2λ * n * T_repair * λ)
     * Simplified Markov model for repairable NMR system. */
    if (n < 3 || failure_rate <= 0.0) return 1.0;

    /* Probability that a second failure occurs before repair of first */
    /* Probability of second failure before first repair:
     * p_2 = (n-1)*λ / (μ + (n-1)*λ) */
    double p_second_before_repair = (n - 1) * failure_rate /
                                     (repair_rate + (n - 1) * failure_rate);

    /* MTTF improvement: roughly (μ/λ) / (n * (n-1)) */
    double improvement = (repair_rate / failure_rate) / (n * (n - 1));
    (void)p_second_before_repair;
    if (improvement < 1.0) improvement = 1.0;

    return improvement;
}
