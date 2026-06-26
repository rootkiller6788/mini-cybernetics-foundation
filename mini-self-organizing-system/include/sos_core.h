#ifndef SOS_CORE_H
#define SOS_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/*
 * Self-Organizing Systems (SOS) - Core Definitions and Types
 *
 * Foundational works:
 *   W. Ross Ashby     - Principles of the Self-Organizing System (1947)
 *   Heinz von Foerster - On Self-Organizing Systems... (1960)
 *   Ilya Prigogine    - Thermodynamics of Irreversible Processes (1967)
 *   Hermann Haken     - Synergetics: An Introduction (1977)
 *   Stuart Kauffman   - The Origins of Order (1993)
 *   Per Bak           - How Nature Works: Self-Organized Criticality (1996)
 */

/* Organization Level (Ashby 1947, Boulding 1956) */
typedef enum {
    SOS_STATIC_STRUCTURE   = 0,
    SOS_CLOCKWORK          = 1,
    SOS_CYBERNETIC         = 2,
    SOS_SELF_MAINTAINING   = 3,
    SOS_SELF_ORGANIZING    = 4,
    SOS_SELF_REPRODUCING   = 5,
    SOS_SELF_AWARE         = 6
} OrganizationLevel;

/* Thermodynamic Regime (Prigogine 1967) */
typedef enum {
    SOS_REGIME_EQUILIBRIUM       = 0,
    SOS_REGIME_NEAR_EQUILIBRIUM  = 1,
    SOS_REGIME_FAR_EQUILIBRIUM   = 2,
    SOS_REGIME_CRITICAL          = 3,
    SOS_REGIME_TURBULENT         = 4
} ThermodynamicRegime;

/* Emergence Type (Broad 1925, Bedau 1997) */
typedef enum {
    SOS_EMERGENCE_NONE        = 0,
    SOS_EMERGENCE_WEAK        = 1,
    SOS_EMERGENCE_STRONG      = 2,
    SOS_EMERGENCE_NOMOLOGICAL = 3
} EmergenceType;

/* Criticality State (Bak, Tang, Wiesenfeld 1987) */
typedef enum {
    SOS_SUBCRITICAL    = 0,
    SOS_CRITICAL       = 1,
    SOS_SUPERCRITICAL  = 2,
    SOS_METASTABLE     = 3
} CriticalityState;

/* Numeric Types */
typedef struct {
    double value;
    double real;
    double imag;
    bool   is_complex;
} SOSComplex;

typedef struct {
    double* data;
    int     rows;
    int     cols;
} SOSMatrix;

typedef struct {
    double* components;
    int     dimension;
} SOSStateVector;

/*
 * Order Parameter (Haken 1977)
 * Landau-Ginzburg: d_xi/dt = alpha*xi - beta*xi^3 + F(t)
 * Fast variables enslaved by order parameters near criticality.
 */
typedef struct {
    double  value;
    double  critical_value;
    double  alpha;
    double  beta;
    double  noise_intensity;
    int     mode_index;
    char*   name;
} OrderParameter;

/*
 * Organization Information Metrics
 * Shalizi (2001): causal states + epsilon-machine
 * Tononi (2004): integrated information Phi
 * Lopez-Ruiz et al. (1995): LMC complexity C = H * D
 */
typedef struct {
    double shannon_entropy;
    double joint_entropy;
    double mutual_information;
    double transfer_entropy;
    double integrated_info;
    double lmc_complexity;
    double effective_complexity;
    double excess_entropy;
    double statistical_complexity;
} OrgInfoMetrics;

/* Main Self-Organizing System Object */
typedef struct SOSystem {
    char*             name;
    OrganizationLevel level;
    ThermodynamicRegime regime;
    CriticalityState  criticality;

    int               n_components;
    int               n_states;
    double*           macro_state;
    double*           micro_diversity;

    double entropy_total;
    double entropy_internal;
    double entropy_external;
    double free_energy;
    double energy_throughput;

    int               n_order_params;
    OrderParameter*   order_params;

    OrgInfoMetrics    info;

    EmergenceType     emergence;
    double            emergence_strength;

    int               n_nodes;
    int               n_edges;
    double**          adjacency;
    double*           node_degrees;
    double            clustering_coeff;
    double            avg_path_length;
    double            modularity;

    int               history_length;
    int               history_capacity;
    double*           history;
    double            current_time;

    double*           control_params;
    int               n_control_params;
} SOSystem;

/* === Core Lifecycle === */
SOSystem* sos_system_create(const char* name, int n_components, int n_states);
void sos_system_free(SOSystem* sys);
void sos_system_set_regime(SOSystem* sys, ThermodynamicRegime regime);
void sos_system_set_level(SOSystem* sys, OrganizationLevel level);
void sos_add_state(SOSystem* sys, const char* name, double initial_val);
double sos_get_state(const SOSystem* sys, int idx);
void sos_set_state(SOSystem* sys, int idx, double value);
void sos_add_order_param(SOSystem* sys, const char* name,
                         double initial_value, double critical_value,
                         double alpha, double beta);
void sos_record_history(SOSystem* sys);
void sos_advance_time(SOSystem* sys, double dt);

/* === Entropy and Thermodynamics === */
double sos_shannon_entropy(const double* probabilities, int n);
double sos_entropy_production(const SOSystem* sys,
                              const double* reaction_rates,
                              const double* affinities, int n_reactions);
double sos_negentropy_balance(const SOSystem* sys,
                              double internal_production,
                              double external_exchange);
double sos_glansdorff_prigogine(const double* flux_variations,
                                const double* force_variations, int n_pairs);
double sos_free_energy(double internal_energy, double temperature, double entropy);

/* === Emergence Detection === */
double sos_detect_emergence(double macro_entropy_rate, double micro_entropy_rate);
double sos_lmc_complexity(const double* probabilities, int n);
double sos_effective_complexity(double total_info, double random_info);
double sos_mutual_information(const double* joint_prob, const double* prob_x,
                              const double* prob_y, int n_x, int n_y);
double sos_transfer_entropy(const double* y_future, const double* y_past,
                            const double* x_past, int n);

/* === Network Topology === */
void sos_network_init(SOSystem* sys, int n_nodes);
void sos_network_add_edge(SOSystem* sys, int src, int dst, double weight);
double sos_clustering_coefficient(const SOSystem* sys);
double sos_average_path_length(SOSystem* sys);
double sos_modularity(SOSystem* sys, const int* community_assignments);

/* === Correlation and Fluctuation === */
double sos_autocorrelation(const double* series, int n, int lag);
void sos_power_spectrum(const double* series, int n, double* spectrum);
double sos_powerlaw_exponent(const double* values, int n, double x_min);
double sos_box_counting_dimension(const double* points_x,
                                  const double* points_y,
                                  int n_points, double min_box, double max_box,
                                  int n_scales);
double sos_jensen_shannon_divergence(const double* p, const double* q, int n);
double sos_ks_entropy(const double* lyapunov_exponents, int n);
double sos_correlation_dimension(const double** embedded_points,
                                 int n_points, int embedding_dim, double epsilon);

/* === Bifurcation Detection === */
int sos_detect_bifurcation(const double* eigenvalues_real,
                           const double* eigenvalues_imag, int n);
int sos_classify_bifurcation(double lambda_real_before,
                             double lambda_imag_before,
                             double lambda_real_after,
                             double lambda_imag_after);

/* === Math Utilities === */
double sos_sigmoid(double x);
double sos_logistic_map(double r, double x);
void sos_eigenvalues_2x2(double a, double b, double c, double d,
                         double* l1r, double* l1i,
                         double* l2r, double* l2i);
void sos_rk4_step(void (*f)(double t, const double* x, double* dx, void* p),
                  double t, double* x, int n, double h, void* p);
double sos_max_lyapunov(void (*f)(const double* x, double* dx, void* p),
                        double* x0, int n, double delta0, double dt,
                        int steps, int transient, void* p);
void sos_print_system(const SOSystem* sys, FILE* stream);

#endif /* SOS_CORE_H */
