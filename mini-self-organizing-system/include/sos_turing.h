#ifndef SOS_TURING_H
#define SOS_TURING_H

#include "sos_core.h"

/* ============================================================================
 * Turing Pattern Formation (Turing 1952) - Reaction-Diffusion Systems
 *
 * Alan Turing's 1952 paper "The Chemical Basis of Morphogenesis" showed that
 * coupled reaction-diffusion equations can spontaneously produce spatial
 * patterns (spots, stripes, labyrinths) from a homogeneous steady state.
 *
 * The key insight: "local activation with long-range inhibition" (Gierer-Meinhardt 1972)
 * where an activator catalyzes its own production and that of an inhibitor which
 * diffuses faster, creating a Turing instability (diffusion-driven instability).
 * ============================================================================ */

/* --- Activator-Inhibitor System Type --- */
typedef enum {
    TURING_SCHNAKENBERG      = 0,  /* Simplest Turing-capable system */
    TURING_GIERER_MEINHARDT  = 1,  /* Activator-inhibitor morphogenesis */
    TURING_GRAY_SCOTT        = 2,  /* Rich pattern diversity */
    TURING_BRUSSELATOR       = 3,  /* Prigogine-Lefever chemical oscillator */
    TURING_FITZHUGH_NAGUMO   = 4,  /* Excitable media (neural) */
    TURING_CUSTOM            = 5   /* User-defined reaction kinetics */
} TuringSystemType;

/* --- Spatial Discretization --- */
typedef struct {
    int     nx;           /* Grid points in x */
    int     ny;           /* Grid points in y */
    double  Lx;           /* Domain length in x */
    double  Ly;           /* Domain length in y */
    double  dx;           /* Grid spacing in x */
    double  dy;           /* Grid spacing in y */
} SpatialGrid;

/* --- Turing System State --- */
typedef struct {
    TuringSystemType  type;
    SpatialGrid       grid;

    /* Species concentrations (nx * ny arrays) */
    double*           u;         /* Activator concentration */
    double*           v;         /* Inhibitor concentration */
    double*           u_next;    /* Next timestep buffer */
    double*           v_next;

    /* Diffusion coefficients */
    double            Du;        /* Activator diffusion */
    double            Dv;        /* Inhibitor diffusion (must be > Du for Turing) */

    /* Reaction parameters (system-dependent) */
    double            params[8]; /* a, b, c, d, rho_A, rho_H, mu_A, mu_H etc. */
    int               n_params;

    double            dt;        /* Time step */
    double            t;         /* Current time */
    int               step;      /* Current step number */

    /* Analysis */
    double            turing_condition;  /* value of Turing instability criterion */
    double            dominant_wavelength; /* Fastest-growing mode wavelength */
    int               is_pattern; /* 1 if pattern has formed */
} TuringSystem;

/* ===== Lifecycle ===== */

TuringSystem* turing_create(TuringSystemType type, int nx, int ny,
                             double Lx, double Ly);
void turing_free(TuringSystem* ts);

/** Set reaction parameters (interpretation depends on system type).
 *  Schnakenberg: params = {a, b}
 *  Gierer-Meinhardt: params = {rho_A, mu_A, rho_H, mu_H}
 *  Gray-Scott: params = {F, k}
 */
void turing_set_params(TuringSystem* ts, const double* params, int n);

/** Set diffusion coefficients. Dv > Du is necessary (not sufficient)
 *  for Turing instability. */
void turing_set_diffusion(TuringSystem* ts, double Du, double Dv);

/** Initialize concentrations with small random perturbations around
 *  the homogeneous steady state. This is essential — without noise,
 *  the system stays at the (unstable) homogeneous state forever. */
void turing_random_init(TuringSystem* ts, double u0, double v0, double noise);

/** Set a specific initial pattern (e.g., spot in center). */
void turing_set_initial(TuringSystem* ts, const double* u_init, const double* v_init);

/* ===== Evolution ===== */

/** Evaluate reaction kinetics RHS at a grid point.
 *  Returns {du/dt, dv/dt} (reaction only, no diffusion). */
void turing_reaction_rhs(TuringSystem* ts, double u, double v,
                         double* du_dt, double* dv_dt);

/** Compute the full reaction-diffusion RHS for the entire grid. */
void turing_rhs_full(TuringSystem* ts, double* du_dt, double* dv_dt);

/** Evolve one timestep using forward Euler (fine for demonstration).
 *  For production use, consider implicit-explicit (IMEX) schemes. */
void turing_step_euler(TuringSystem* ts);

/** Evolve one timestep using 2nd-order Runge-Kutta. */
void turing_step_rk2(TuringSystem* ts);

/** Evolve for n_steps, optionally reporting progress. */
void turing_evolve(TuringSystem* ts, int n_steps);

/* ===== Turing Instability Analysis ===== */

/** Compute the homogeneous steady state (u*, v*) for the current parameters.
 *  For Schnakenberg: u* = a + b, v* = b/(a+b)^2. */
void turing_homogeneous_steady_state(const TuringSystem* ts,
                                     double* u_star, double* v_star);

/** Compute the Jacobian of the reaction terms at the steady state.
 *  Returns the 2x2 Jacobian matrix [[f_u, f_v], [g_u, g_v]]. */
void turing_reaction_jacobian(const TuringSystem* ts,
                              double u_star, double v_star,
                              double* J);

/** Compute the Turing instability condition.
 *  The Turing condition requires:
 *    1. f_u + g_v < 0 (stable to homogeneous perturbations)
 *    2. f_u*g_v - f_v*g_u > 0 (determinant positive)
 *    3. Dv*f_u + Du*g_v > 2*sqrt(Du*Dv*(f_u*g_v - f_v*g_u)) (dispersion relation)
 *  Returns: 1 if Turing unstable, 0 if stable. */
int turing_check_instability(const TuringSystem* ts,
                             double u_star, double v_star);

/** Compute the dispersion relation lambda(k) for wavenumber k.
 *  lambda(k) = eigenvalue of [J - k^2*D].
 *  Returns the maximum real part of lambda. */
double turing_dispersion_relation(const TuringSystem* ts,
                                  double u_star, double v_star, double k);

/** Find the fastest-growing wavenumber (maximum of dispersion relation).
 *  This determines the characteristic wavelength of the pattern. */
double turing_dominant_wavenumber(const TuringSystem* ts,
                                  double u_star, double v_star);

/* ===== Pattern Analysis ===== */

/** Compute the 2D Fourier transform of the concentration field.
 *  Used to identify dominant spatial frequencies. */
void turing_fourier_2d(const double* field, int nx, int ny,
                       double Lx, double Ly,
                       double* power_spectrum, int* kx_peak, int* ky_peak);

/** Detect whether a pattern has formed by comparing spatial variance
 *  to the initial noise level. Pattern formation = variance growth. */
int turing_pattern_formed(const TuringSystem* ts, double threshold);

/** Compute pattern wavelength by autocorrelation analysis. */
double turing_pattern_wavelength(const TuringSystem* ts);

/** Classify pattern type: 0=none, 1=spots, 2=stripes, 3=labyrinth, 4=traveling_wave. */
int turing_classify_pattern(const TuringSystem* ts);

#endif /* SOS_TURING_H */
