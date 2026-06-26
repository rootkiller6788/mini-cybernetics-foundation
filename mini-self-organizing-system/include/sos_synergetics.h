#ifndef SOS_SYNERGETICS_H
#define SOS_SYNERGETICS_H

#include "sos_core.h"

/* ============================================================================
 * Synergetics: Haken's Theory of Self-Organization (Haken 1977, 1983)
 *
 * Synergetics studies how cooperative behavior emerges in complex systems
 * through the interplay of order parameters and enslaved subsystems.
 *
 * Core principles:
 *   1. Slaving Principle (Versklavungsprinzip): Near instability, fast-relaxing
 *      variables are "enslaved" by slow order parameters, reducing the
 *      dimensionality of the dynamics dramatically.
 *   2. Order Parameters: Macroscopic variables that describe the system's
 *      organization and obey low-dimensional nonlinear equations.
 *   3. Adiabatic Elimination: Fast variables can be set to their steady-state
 *      values as functions of the slow variables: v_fast(t) = f(xi_slow(t)).
 * ============================================================================ */

/* --- Mode Type in Linear Stability Analysis --- */
typedef enum {
    MODE_STABLE      = 0,  /* Eigenvalue Re(lambda) < 0 — relaxing mode */
    MODE_MARGINAL    = 1,  /* Re(lambda) = 0 — critical mode (order parameter candidate) */
    MODE_UNSTABLE    = 2   /* Re(lambda) > 0 — growing mode */
} ModeType;

/* --- A single eigenmode of the linearized dynamics --- */
typedef struct {
    double  eigenvalue_real;
    double  eigenvalue_imag;
    double* eigenvector;        /* Right eigenvector of length n_dims */
    ModeType type;
    double  relaxation_time;    /* tau = -1/Re(lambda) for stable modes */
    int     is_order_param;     /* 1 if this mode is a candidate order parameter */
} Eigenmode;

/* --- Linear Stability Analysis Result --- */
typedef struct {
    int       n_dims;           /* Dimensionality of the system */
    int       n_modes;          /* Number of eigenmodes */
    Eigenmode* modes;
    int       n_order_params;   /* Number of marginal/unstable modes */
    int*      order_param_idx;  /* Indices of order parameter modes */
} LinearStability;

/* --- Slaving (Adiabatic Elimination) Result ---
 * The dynamics of an N-dimensional system near instability reduce to:
 *   d(xi)/dt = f(xi, s_fast(xi))
 * where xi are the order parameters (dim = n_xi << N)
 * and s_fast(xi) are the enslaved fast variables.
 */
typedef struct {
    int       n_order_params;
    int       n_fast_vars;
    double*   order_param_values;  /* Current xi values */
    double**  slaving_coeffs;      /* s_fast[i] = sum_j c_{ij} * xi_j */
    double    adiabatic_error;     /* Error of adiabatic approximation */
} SlavingResult;

/* --- Haken's Laser Equations ---
 * The laser is the paradigmatic example of synergetics.
 * Laser rate equations (Haken 1970):
 *   dn/dt = G*N*n - 2*kappa*n + F(t)
 *   dN/dt = gamma*(N_0 - N) - G*N*n
 * where n = photon number (order parameter), N = inversion.
 * At threshold G*N_0 > 2*kappa, n grows from 0 to macroscopic values.
 */
typedef struct {
    double  n;             /* Photon number (order parameter) */
    double  N;             /* Population inversion (fast variable) */
    double  G;             /* Gain coefficient */
    double  kappa;         /* Cavity loss rate */
    double  gamma;         /* Relaxation rate of inversion */
    double  N_0;           /* Pump parameter (unsaturated inversion) */
    double  threshold;     /* G*N_0 / (2*kappa): >1 = lasing */
    double  noise;         /* Spontaneous emission noise intensity */
} LaserSystem;

/* ===== Linear Stability Analysis ===== */

/** Perform linear stability analysis on a system described by a Jacobian matrix.
 *  Computes eigenvalues, eigenvectors, and classifies each mode.
 *  @param J Jacobian matrix (n x n, row-major)
 *  @param n Dimension of the system
 *  @return Linear stability analysis result (caller must free with ls_free) */
LinearStability* ls_analyze(const double* J, int n);
void ls_free(LinearStability* ls);

/** Identify order parameter candidates: modes with Re(lambda) >= -epsilon.
 *  These are the slow modes that will dominate the dynamics near instability. */
int ls_identify_order_params(LinearStability* ls, double epsilon);

/** Project the full state vector onto the order parameter subspace.
 *  Uses the left eigenvectors (bi-orthogonal projection). */
void ls_project_order_params(const LinearStability* ls,
                             const double* state,
                             double* order_param_amplitudes);

/** Reconstruct the approximate full state from order parameters.
 *  Using the slaving principle: x ~ sum_i xi_i * v_i + s_fast(xi). */
void ls_reconstruct_state(const LinearStability* ls,
                          const double* order_params,
                          SlavingResult* slave,
                          double* reconstructed_state);

/* ===== Slaving Principle (Adiabatic Elimination) ===== */

/** Compute the slaving coefficients c_{ij} such that:
 *  s_fast[i] = sum_j c_{ij} * xi_j + O(xi^2)
 *  Uses the linearized dynamics: set ds_fast/dt = 0, solve for s_fast. */
SlavingResult* slaving_compute(const LinearStability* ls);
void slaving_free(SlavingResult* sr);

/** Evolve the order parameters using the reduced (adiabatic) dynamics.
 *  d(xi_i)/dt = lambda_i * xi_i + N_i(xi) where N_i includes
 *  nonlinear self-interactions (cubic saturation) and coupling to
 *  fast variables via the slaving coefficients. */
void slaving_evolve_order_params(SlavingResult* sr,
                                 const double* linear_rates,
                                 const double* cubic_coeffs,
                                 double dt);

/** Evaluate the adiabatic approximation error:
 *  error = ||ds_fast/dt||_2 / ||s_fast||_2
 *  Small error validates the slaving principle. */
double slaving_error_estimate(const SlavingResult* sr,
                              const double* order_param_values);

/* ===== Laser System (Haken's Paradigm) ===== */

LaserSystem* laser_create(double G, double kappa, double gamma, double N_0);
void laser_free(LaserSystem* laser);

/** Evolve the laser equations one step using stochastic Runge-Kutta.
 *  Below threshold (pump < 1): n fluctuates around 0 (thermal light)
 *  Above threshold (pump > 1): n grows to macroscopic value (coherent light)
 *  At threshold (pump = 1): critical fluctuations, n ~ N^{1/2} */
void laser_step(LaserSystem* laser, double dt);

/** Compute the steady-state photon number above threshold:
 *  n_ss = (gamma/G) * (pump - 1) where pump = G*N_0/(2*kappa). */
double laser_steady_state_photons(const LaserSystem* laser);

/** Check if the laser is above threshold (lasing). */
int laser_is_lasing(const LaserSystem* laser);

/* ===== Order Parameter Dynamics (Landau Theory) ===== */

/** Compute the Landau free energy for a single order parameter:
 *  F(xi) = F_0 + (a/2)*xi^2 + (b/4)*xi^4
 *  For a > 0: minimum at xi = 0 (disordered phase)
 *  For a < 0: minima at xi = +/- sqrt(-a/b) (ordered phase)
 *  At a = 0: critical point (second-order phase transition) */
double landau_free_energy(double xi, double a, double b);

/** Find the equilibrium order parameter by minimizing Landau free energy:
 *  xi_eq = 0 if a >= 0
 *  xi_eq = +/- sqrt(-a/b) if a < 0 */
double landau_equilibrium(double a, double b);

/** Compute the susceptibility chi = d(xi)/d(h) at h=0:
 *  chi = 1/a for a > 0 (diverges as 1/|T-T_c| near critical point)
 *  This is the Curie-Weiss law: chi ~ |T - T_c|^{-gamma} with gamma = 1. */
double landau_susceptibility(double a);

#endif /* SOS_SYNERGETICS_H */
