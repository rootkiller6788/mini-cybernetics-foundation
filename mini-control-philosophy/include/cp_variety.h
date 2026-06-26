#ifndef CP_VARIETY_H
#define CP_VARIETY_H

#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * Control Philosophy ? Variety, Entropy, and Information Theory
 *
 * Ashby's Law of Requisite Variety:
 *   "Only variety can destroy variety."
 *   V(E) >= V(D) - K, where
 *     V(E) = variety of the regulator's responses
 *     V(D) = variety of the disturbances
 *     K    = channel capacity of the regulator
 *
 * References:
 *   Ashby, W.R. (1956). Introduction to Cybernetics.
 *   Shannon, C.E. (1948). A Mathematical Theory of Communication.
 *   Conant, R.C. & Ashby, W.R. (1970). Every Good Regulator...
 * ============================================================================ */

/* --- Variety and State Space --- */

/** A discrete set of distinguishable states from the regulator's perspective.
 *  Variety V = log2(|S|) ? the information needed to specify a state. */
typedef struct {
    int     cardinality;       /* |S|: number of distinguishable states */
    double  variety_bits;      /* log2(cardinality) */
    double  max_entropy;       /* ln(cardinality) in nats */
    double  reduction_factor;  /* how much variety has been reduced by regulation */
    bool    is_finite;         /* true iff state space is enumerable */
} VarietyMeasure;

/** A control channel: the informational path from sensor to actuator.
 *  Modeled as a discrete memoryless channel (DMC) with transition matrix. */
typedef struct {
    int     input_alphabet_size;
    int     output_alphabet_size;
    double* transition_matrix;  /* P(output | input), flattened row-major */
    double  capacity;           /* max_{P(X)} I(X;Y)  in bits */
    double  error_probability;
    double  equivocation;       /* H(X|Y): uncertainty remaining after observation */
} ControlChannel;

/** An ensemble of possible disturbances the regulator must cope with. */
typedef struct {
    int     n_disturbances;
    double* probabilities;      /* probability distribution over disturbance types */
    double  entropy;            /* H(D): uncertainty about which disturbance will occur */
    double  variety;            /* log2(effective_cardinality) = H(D) in bits */
} DisturbanceEnsemble;

/** Ashby's regulator: maps disturbances to actions via a strategy table.
 *  This is the core of the Law of Requisite Variety. */
typedef struct {
    int      n_disturbance_types;
    int      n_response_types;
    int      n_outcome_types;
    int*     strategy_table;     /* R(d) = a: for each disturbance d, pick action a */
    double   variety_disturbance;
    double   variety_response;
    double   variety_outcome;
    double   requisite_ratio;    /* V(R) / V(D): must be >= 1 for perfect regulation */
    bool     satisfies_ashby_law;
} AshbyRegulator;

/* --- API: Variety & Entropy --- */

/**
 * Compute variety V = log2(N) for a system with N distinguishable states.
 * [Source: Ashby, Introduction to Cybernetics (1956), ?7.5]
 * Complexity: O(1)
 */
double cp_variety(int num_states);

/**
 * Compute the effective variety from an entropy measure H.
 * Effective variety = exp(H) in natural units, or 2^H in bits.
 * Complexity: O(1)
 */
double cp_effective_variety(double entropy, bool in_bits);

/**
 * Compute Shannon entropy H(p) = -? p_i log2(p_i) over a probability distribution.
 * [Source: Shannon (1948)]
 * Complexity: O(n)
 */
double cp_entropy(const double* probabilities, int n, bool in_bits);

/**
 * Compute conditional entropy H(Y|X) given a joint distribution P(X,Y).
 * Complexity: O(nx * ny)
 */
double cp_conditional_entropy(const double* joint_pmf, int nx, int ny, bool in_bits);

/**
 * Compute mutual information I(X;Y) = H(X) + H(Y) - H(X,Y).
 * Complexity: O(nx * ny)
 */
double cp_mutual_information(const double* joint_pmf, int nx, int ny, bool in_bits);

/**
 * Compute the Ashby index A = V(R) - V(D) + K.
 * If A >= 0, the regulator has sufficient variety to handle disturbances.
 * [Source: Ashby (1956), ?11 ? Requisite Variety]
 * Complexity: O(1)
 */
double cp_ashby_index(double variety_regulator, double variety_disturbance, double channel_capacity);

/* --- API: Channel --- */

/**
 * Create a control channel with given input/output alphabet sizes.
 * Complexity: O(nx * ny)
 */
ControlChannel* cp_channel_create(int input_size, int output_size);

/**
 * Free a control channel.
 * Complexity: O(1)
 */
void cp_channel_free(ControlChannel* ch);

/**
 * Set the transition probability P(output | input).
 * Complexity: O(1)
 */
void cp_channel_set_transition(ControlChannel* ch, int input, int output, double prob);

/**
 * Compute the channel capacity via the Blahut-Arimoto algorithm
 * (simplified: binary search over input distribution).
 * [Source: Blahut, "Computation of Channel Capacity" (1972)]
 * Complexity: O(max_iter * nx * ny)
 */
double cp_channel_compute_capacity(ControlChannel* ch, int max_iter, double tol);

/**
 * Compute equivocation H(X|Y): how much uncertainty about input remains
 * given the channel output.
 * Complexity: O(nx * ny)
 */
double cp_channel_equivocation(const ControlChannel* ch);

/**
 * Compute the error probability of the channel (sum over off-diagonal transitions).
 * Complexity: O(nx * ny)
 */
double cp_channel_error_probability(const ControlChannel* ch);

/* --- API: Disturbance Ensemble --- */

DisturbanceEnsemble* cp_disturbance_ensemble_create(int n);
void cp_disturbance_ensemble_free(DisturbanceEnsemble* de);
void cp_disturbance_ensemble_set_probability(DisturbanceEnsemble* de, int idx, double p);
void cp_disturbance_ensemble_normalize(DisturbanceEnsemble* de);
double cp_disturbance_ensemble_variety(const DisturbanceEnsemble* de);

/* --- API: Ashby Regulator --- */

/**
 * Create an Ashby-style regulator with given disturbance/response/outcome spaces.
 * Complexity: O(nd * nr)
 */
AshbyRegulator* cp_ashby_regulator_create(int n_disturbances, int n_responses, int n_outcomes);

/**
 * Free an Ashby regulator.
 * Complexity: O(1)
 */
void cp_ashby_regulator_free(AshbyRegulator* reg);

/**
 * Set the strategy: for disturbance d, choose response r.
 * Complexity: O(1)
 */
void cp_ashby_regulator_set_strategy(AshbyRegulator* reg, int disturbance, int response);

/**
 * Evaluate the regulator against a disturbance ensemble.
 * Updates requisite_ratio and satisfies_ashby_law fields.
 * [Source: Ashby (1956), ?11.5 ? The law of requisite variety]
 * Complexity: O(nd * nr)
 */
void cp_ashby_regulator_evaluate(AshbyRegulator* reg, const DisturbanceEnsemble* de);

/**
 * Compute outcome variety ? how many distinct outcomes the regulator produces.
 * Complexity: O(nd * nr)
 */
double cp_ashby_regulator_outcome_variety(const AshbyRegulator* reg, const DisturbanceEnsemble* de);

/**
 * Print the strategy table and variety analysis.
 * Complexity: O(nd * nr)
 */
void cp_ashby_regulator_print(const AshbyRegulator* reg);

/* --- API: Transfer Entropy (Control-Specific) --- */

/**
 * Compute transfer entropy from disturbance to output:
 * TE(D ? Y) = I(Y_future; D_past | Y_past)
 * This quantifies how much disturbance information is "leaking" through
 * the controller ? lower is better.
 * [Source: Schreiber, "Measuring Information Transfer" (2000)]
 * Complexity: O(n * bins?)
 */
double cp_transfer_entropy_discrete(const int* source, const int* target,
                                     int n, int lag, int alphabet_size);

/**
 * Estimate the information bottleneck T = I(X;U) - ??I(U;Y)
 * for controller state U mediating between sensor X and actuator Y.
 * [Source: Tishby et al., "The Information Bottleneck Method" (1999)]
 * Complexity: O(n)
 */
double cp_information_bottleneck(double i_xu, double i_uy, double beta);

#endif /* CP_VARIETY_H */
