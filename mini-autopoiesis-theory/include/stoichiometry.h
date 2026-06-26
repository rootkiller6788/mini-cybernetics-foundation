/**
 * @file stoichiometry.h
 * @brief Stoichiometric matrix operations and ODE integration.
 *
 * Implements the mathematical backbone of autopoietic dynamics: the
 * stoichiometric matrix S and the dynamical system dx/dt = S·v(x).
 *
 * Knowledge coverage:
 *   L3 - Mathematical structures: stoichiometric matrix, flux space
 *   L4 - Fundamental laws: mass conservation, steady-state conditions
 *   L5 - Numerical methods: Runge-Kutta 4, steady-state detection
 *
 * Reference: Feinberg, "Foundations of Chemical Reaction Network Theory" (2019)
 *            Heinrich & Schuster, "The Regulation of Cellular Systems" (1996)
 */

#ifndef STOICHIOMETRY_H
#define STOICHIOMETRY_H

#include <stddef.h>

#define STOICH_MAX_SPECIES  256
#define STOICH_MAX_REACTIONS 512

/**
 * @brief Sparse stoichiometric matrix representation.
 *
 * For large networks, the stoichiometric matrix is sparse. We use
 * compressed sparse row (CSR) format for efficient operations.
 */
typedef struct {
    int rows;            /**< Number of species */
    int cols;            /**< Number of reactions */
    int nnz;             /**< Number of non-zero entries */
    double *values;      /**< Non-zero values */
    int *col_indices;    /**< Column index for each value */
    int *row_pointers;   /**< Start index for each row in values/col_indices */
} stoich_matrix_t;

/**
 * @brief Dense matrix representation for smaller systems.
 */
typedef struct {
    int rows;
    int cols;
    double *data;  /**< Row-major: data[i*cols + j] */
} stoich_dense_t;

/**
 * @brief Flux vector: reaction rates.
 */
typedef struct {
    int length;
    double *rates;
} stoich_flux_t;

/**
 * @brief Concentration vector: species amounts.
 */
typedef struct {
    int length;
    double *concentrations;
} stoich_concentration_t;

/**
 * @brief Stoichiometric analysis workspace.
 */
typedef struct {
    stoich_dense_t *S;          /**< Stoichiometric matrix (dense, for small nets) */
    stoich_matrix_t *S_sparse;  /**< Stoichiometric matrix (sparse, for large nets) */
    stoich_flux_t v;            /**< Current reaction rates */
    stoich_concentration_t x;   /**< Current species concentrations */
    stoich_concentration_t dxdt;/**< Current derivatives */
    int use_sparse;             /**< 1 if using sparse representation */
} stoich_system_t;

/* ==========================================================================
 * Matrix Construction and Operations
 * ========================================================================== */

/** Initialize an empty stoichiometric system. */
void stoich_system_init(stoich_system_t *sys, int n_species, int n_reactions);

/** Set a stoichiometric coefficient S[species][reaction] = value. */
void stoich_set_coefficient(stoich_system_t *sys, int species, int reaction,
                             double value);

/** Get a stoichiometric coefficient. */
double stoich_get_coefficient(const stoich_system_t *sys, int species, int reaction);

/** Build the sparse representation from the dense matrix. */
void stoich_build_sparse(stoich_system_t *sys);

/* ==========================================================================
 * Linear Algebra Operations
 * ========================================================================== */

/**
 * @brief Compute dx/dt = S * v.
 *
 * The fundamental dynamical equation: species concentrations change
 * according to the stoichiometric matrix multiplied by reaction rates.
 *
 * @param sys Stoichiometric system.
 * @param rates Reaction rate vector (if NULL, uses sys->v.rates).
 * @param derivatives Output derivative vector.
 */
void stoich_compute_derivatives(const stoich_system_t *sys,
                                 const double *rates, double *derivatives);

/**
 * @brief Compute the right nullspace of S (fluxes that conserve mass).
 *
 * S * v_null = 0: these are steady-state fluxes.
 *
 * @param sys Stoichiometric system.
 * @param nullspace Output matrix (caller-allocated, cols x rank_deficiency).
 * @param dim Output: dimension of the nullspace.
 * @return 0 on success, -1 on error.
 */
int stoich_nullspace(const stoich_system_t *sys, double *nullspace, int *dim);

/**
 * @brief Compute the left nullspace of S (conserved quantities).
 *
 * w^T * S = 0: w represents conserved moieties.
 *
 * @param sys Stoichiometric system.
 * @param conserved Output matrix.
 * @param dim Output: number of conserved quantities.
 * @return 0 on success, -1 on error.
 */
int stoich_conserved_quantities(const stoich_system_t *sys,
                                 double *conserved, int *dim);

/* ==========================================================================
 * ODE Integration
 * ========================================================================== */

/**
 * @brief Single step of classical Runge-Kutta 4th order integration.
 *
 * Integrates dx/dt = f(x) from t to t + dt.
 *
 * @param sys System to integrate.
 * @param dt Time step.
 * @param rate_func Function computing f(x) → reaction rates.
 * @param ctx Context for rate function.
 */
void stoich_rk4_step(stoich_system_t *sys, double dt,
                     void (*rate_func)(const double *x, double *v, void *ctx),
                     void *ctx);

/**
 * @brief Adaptive Runge-Kutta 4(5) integration (Dormand-Prince).
 *
 * Uses error estimation to adapt step size for accuracy.
 *
 * @param sys System to integrate.
 * @param t_start Start time.
 * @param t_end End time.
 * @param dt_initial Initial step size.
 * @param tolerance Error tolerance.
 * @param rate_func Rate function.
 * @param ctx Context.
 * @return Number of steps taken.
 */
int stoich_integrate_adaptive(stoich_system_t *sys,
                               double t_start, double t_end,
                               double dt_initial, double tolerance,
                               void (*rate_func)(const double *x, double *v, void *ctx),
                               void *ctx);

/**
 * @brief Integrate using backward Euler (implicit, stable for stiff ODEs).
 *
 * Suitable for stiff autopoietic systems with widely varying timescales.
 *
 * @param sys System to integrate.
 * @param dt Time step.
 * @param rate_func Rate function.
 * @param ctx Context.
 * @param max_newton_iter Maximum Newton iterations per step.
 * @return 0 on convergence, -1 if Newton did not converge.
 */
int stoich_backward_euler_step(stoich_system_t *sys, double dt,
                                void (*rate_func)(const double *x, double *v, void *ctx),
                                void *ctx, int max_newton_iter);

/* ==========================================================================
 * Steady-State Analysis
 * ========================================================================== */

/**
 * @brief Find a steady state by integrating until convergence.
 *
 * @param sys System.
 * @param rate_func Rate function.
 * @param ctx Context.
 * @param max_time Maximum integration time.
 * @param tolerance Convergence tolerance.
 * @return 1 if steady state found, 0 if max_time exceeded.
 */
int stoich_find_steady_state(stoich_system_t *sys,
                              void (*rate_func)(const double *x, double *v, void *ctx),
                              void *ctx, double max_time, double tolerance);

/**
 * @brief Check if current state is near steady state.
 *
 * @param sys System.
 * @param tolerance Maximum allowed derivative magnitude.
 * @return 1 if steady state, 0 otherwise.
 */
int stoich_is_steady_state(const stoich_system_t *sys, double tolerance);

/**
 * @brief Compute the Jacobian matrix at the current state.
 *
 * J[species][reaction] = ∂(dx_i/dt) / ∂x_j
 *
 * @param sys System.
 * @param rate_func Rate function.
 * @param ctx Context.
 * @param jacobian Output matrix (caller-allocated, n×n).
 * @param epsilon Finite difference perturbation.
 */
void stoich_compute_jacobian(const stoich_system_t *sys,
                              void (*rate_func)(const double *x, double *v, void *ctx),
                              void *ctx, double *jacobian, double epsilon);

/**
 * @brief Compute eigenvalues of the Jacobian (for stability analysis).
 *
 * Uses QR algorithm for real matrices.
 *
 * @param jacobian Input Jacobian matrix (n×n, row-major).
 * @param n Dimension.
 * @param eigenvalues_real Output: real parts.
 * @param eigenvalues_imag Output: imaginary parts.
 * @param max_iter Maximum QR iterations.
 * @return 0 on success.
 */
int stoich_eigenvalues(const double *jacobian, int n,
                        double *eigenvalues_real, double *eigenvalues_imag,
                        int max_iter);

/* ==========================================================================
 * Cleanup
 * ========================================================================== */

void stoich_system_destroy(stoich_system_t *sys);

#endif /* STOICHIOMETRY_H */
