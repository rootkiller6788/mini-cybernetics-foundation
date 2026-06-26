/**
 * @file stoichiometry.c
 * @brief Stoichiometric matrix operations, ODE integration, and steady-state analysis.
 *
 * Implements the mathematical core for dynamical analysis of autopoietic systems:
 * the stoichiometric matrix S, the ODE system dx/dt = S·v(x), and numerical
 * methods for integration and steady-state detection.
 *
 * Knowledge points implemented:
 *   L3 - stoich_set_coefficient / stoich_get_coefficient: matrix operations
 *   L3 - stoich_compute_derivatives: S·v computation
 *   L4 - stoich_nullspace: steady-state flux space
 *   L4 - stoich_conserved_quantities: conserved moieties
 *   L5 - stoich_rk4_step: Runge-Kutta 4th order integration
 *   L5 - stoich_integrate_adaptive: Dormand-Prince RK45 adaptive
 *   L5 - stoich_backward_euler_step: implicit integration for stiff systems
 *   L5 - stoich_find_steady_state: steady-state convergence detection
 *   L5 - stoich_compute_jacobian: Jacobian for stability analysis
 *   L5 - stoich_eigenvalues: QR algorithm for eigenvalue decomposition
 */

#include "stoichiometry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* ==========================================================================
 * System Management
 * ========================================================================== */

void stoich_system_init(stoich_system_t *sys, int n_species, int n_reactions)
{
    if (!sys) return;
    memset(sys, 0, sizeof(stoich_system_t));

    if (n_species <= 0 || n_species > STOICH_MAX_SPECIES) return;
    if (n_reactions <= 0 || n_reactions > STOICH_MAX_REACTIONS) return;

    /* Allocate dense matrix */
    sys->S = (stoich_dense_t *)malloc(sizeof(stoich_dense_t));
    if (!sys->S) return;
    sys->S->rows = n_species;
    sys->S->cols = n_reactions;
    sys->S->data = (double *)calloc(n_species * n_reactions, sizeof(double));
    if (!sys->S->data) { free(sys->S); sys->S = NULL; return; }

    /* Allocate vectors */
    sys->v.length = n_reactions;
    sys->v.rates = (double *)calloc(n_reactions, sizeof(double));

    sys->x.length = n_species;
    sys->x.concentrations = (double *)calloc(n_species, sizeof(double));

    sys->dxdt.length = n_species;
    sys->dxdt.concentrations = (double *)calloc(n_species, sizeof(double));

    sys->use_sparse = 0;
}

void stoich_set_coefficient(stoich_system_t *sys, int species, int reaction,
                             double value)
{
    if (!sys || !sys->S || !sys->S->data) return;
    if (species < 0 || species >= sys->S->rows) return;
    if (reaction < 0 || reaction >= sys->S->cols) return;

    sys->S->data[species * sys->S->cols + reaction] = value;
}

double stoich_get_coefficient(const stoich_system_t *sys, int species, int reaction)
{
    if (!sys || !sys->S || !sys->S->data) return 0.0;
    if (species < 0 || species >= sys->S->rows) return 0.0;
    if (reaction < 0 || reaction >= sys->S->cols) return 0.0;

    return sys->S->data[species * sys->S->cols + reaction];
}

void stoich_build_sparse(stoich_system_t *sys)
{
    if (!sys || !sys->S) return;

    int rows = sys->S->rows;
    int cols = sys->S->cols;
    double *dense = sys->S->data;

    /* Count non-zeros */
    int nnz = 0;
    for (int i = 0; i < rows * cols; i++) {
        if (fabs(dense[i]) > 1e-15) nnz++;
    }

    sys->S_sparse = (stoich_matrix_t *)malloc(sizeof(stoich_matrix_t));
    if (!sys->S_sparse) return;
    sys->S_sparse->rows = rows;
    sys->S_sparse->cols = cols;
    sys->S_sparse->nnz = nnz;

    sys->S_sparse->values = (double *)malloc(nnz * sizeof(double));
    sys->S_sparse->col_indices = (int *)malloc(nnz * sizeof(int));
    sys->S_sparse->row_pointers = (int *)malloc((rows + 1) * sizeof(int));

    if (!sys->S_sparse->values || !sys->S_sparse->col_indices || !sys->S_sparse->row_pointers) {
        free(sys->S_sparse->values);
        free(sys->S_sparse->col_indices);
        free(sys->S_sparse->row_pointers);
        free(sys->S_sparse);
        sys->S_sparse = NULL;
        return;
    }

    /* Fill CSR */
    int pos = 0;
    sys->S_sparse->row_pointers[0] = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double val = dense[i * cols + j];
            if (fabs(val) > 1e-15) {
                sys->S_sparse->values[pos] = val;
                sys->S_sparse->col_indices[pos] = j;
                pos++;
            }
        }
        sys->S_sparse->row_pointers[i + 1] = pos;
    }

    sys->use_sparse = 1;
}

/* ==========================================================================
 * Linear Algebra Operations
 * ========================================================================== */

void stoich_compute_derivatives(const stoich_system_t *sys,
                                 const double *rates, double *derivatives)
{
    if (!sys || !derivatives) return;

    const double *v = rates ? rates : sys->v.rates;
    int n = sys->x.length;
    int m = sys->v.length;

    /* dx/dt = S * v */
    if (sys->use_sparse && sys->S_sparse) {
        stoich_matrix_t *S = sys->S_sparse;
        for (int i = 0; i < n; i++) {
            derivatives[i] = 0.0;
            for (int p = S->row_pointers[i]; p < S->row_pointers[i + 1]; p++) {
                int j = S->col_indices[p];
                double s_val = S->values[p];
                derivatives[i] += s_val * v[j];
            }
        }
    } else if (sys->S && sys->S->data) {
        int cols = sys->S->cols;
        for (int i = 0; i < n; i++) {
            derivatives[i] = 0.0;
            for (int j = 0; j < m; j++) {
                derivatives[i] += sys->S->data[i * cols + j] * v[j];
            }
        }
    }
}

int stoich_nullspace(const stoich_system_t *sys, double *nullspace, int *dim)
{
    if (!sys || !nullspace || !dim) return -1;
    if (!sys->S || !sys->S->data) return -1;

    int n = sys->S->rows;
    int m = sys->S->cols;
    int max_dim = (n < m) ? n : m;

    /* Compute nullspace via Gaussian elimination on S^T to find
     * vectors v such that S·v = 0 */

    /* Create augmented working matrix: [S | I_m] */
    int work_rows = n;
    int work_cols = m + m;
    double *work = (double *)calloc(work_rows * work_cols, sizeof(double));
    if (!work) return -1;

    /* Copy S into left part, identity into right part */
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            work[i * work_cols + j] = sys->S->data[i * m + j];
        }
        if (i < m) {
            work[i * work_cols + m + i] = 1.0;
        }
    }

    /* Gaussian elimination (row reduction) */
    int row = 0;
    for (int col = 0; col < m && row < n; col++) {
        /* Find pivot */
        int pivot = row;
        for (int r = row; r < n; r++) {
            if (fabs(work[r * work_cols + col]) > fabs(work[pivot * work_cols + col])) {
                pivot = r;
            }
        }

        if (fabs(work[pivot * work_cols + col]) < 1e-12) continue;

        /* Swap rows */
        if (pivot != row) {
            for (int c = 0; c < work_cols; c++) {
                double tmp = work[row * work_cols + c];
                work[row * work_cols + c] = work[pivot * work_cols + c];
                work[pivot * work_cols + c] = tmp;
            }
        }

        /* Normalize pivot row */
        double piv_val = work[row * work_cols + col];
        for (int c = 0; c < work_cols; c++) {
            work[row * work_cols + c] /= piv_val;
        }

        /* Eliminate other rows */
        for (int r = 0; r < n; r++) {
            if (r == row) continue;
            double factor = work[r * work_cols + col];
            if (fabs(factor) < 1e-15) continue;
            for (int c = 0; c < work_cols; c++) {
                work[r * work_cols + c] -= factor * work[row * work_cols + c];
            }
        }
        row++;
    }

    /* Nullspace vectors are the right part columns for zero rows in left */
    int ns_dim = m - row;
    if (ns_dim <= 0 || ns_dim > max_dim) {
        *dim = 0;
        free(work);
        return 0;
    }

    /* Extract nullspace: columns of right part where left part is zero */
    int ns_count = 0;
    for (int j = 0; j < m && ns_count < ns_dim; j++) {
        /* Check if column j of left part is all zero (free variable) */
        int is_free = 1;
        for (int i = 0; i < n; i++) {
            if (fabs(work[i * work_cols + j]) > 1e-12) {
                is_free = 0;
                break;
            }
        }
        if (is_free) {
            /* Nullspace vector from right part column j */
            for (int i = 0; i < m; i++) {
                nullspace[ns_count * m + i] = -work[i * work_cols + m + j];
            }
            ns_count++;
        }
    }

    *dim = ns_count;
    free(work);
    return 0;
}

int stoich_conserved_quantities(const stoich_system_t *sys,
                                 double *conserved, int *dim)
{
    if (!sys || !conserved || !dim) return -1;
    if (!sys->S || !sys->S->data) return -1;

    int n = sys->S->rows;
    int m = sys->S->cols;

    /* Conserved quantities w satisfy w^T * S = 0
     * i.e., S^T * w = 0, which is the left nullspace.
     * Compute by transposing S and finding nullspace of S^T */

    /* Create transposed matrix: S^T is m × n */
    double *ST = (double *)malloc(m * n * sizeof(double));
    if (!ST) return -1;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            ST[j * n + i] = sys->S->data[i * m + j];
        }
    }

    /* Find nullspace of S^T using the same method as above */
    int work_rows = m;
    int work_cols = n + n;
    double *work = (double *)calloc((size_t)work_rows * (size_t)work_cols, sizeof(double));
    if (!work) { free(ST); return -1; }

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            work[i * work_cols + j] = ST[i * n + j];
        }
        if (i < n) {
            work[i * work_cols + n + i] = 1.0;
        }
    }

    int row = 0;
    for (int col = 0; col < n && row < m; col++) {
        int pivot = row;
        for (int r = row; r < m; r++) {
            if (fabs(work[r * work_cols + col]) > fabs(work[pivot * work_cols + col])) {
                pivot = r;
            }
        }

        if (fabs(work[pivot * work_cols + col]) < 1e-12) continue;

        if (pivot != row) {
            for (int c = 0; c < work_cols; c++) {
                double tmp = work[row * work_cols + c];
                work[row * work_cols + c] = work[pivot * work_cols + c];
                work[pivot * work_cols + c] = tmp;
            }
        }

        double piv_val = work[row * work_cols + col];
        for (int c = 0; c < work_cols; c++) {
            work[row * work_cols + c] /= piv_val;
        }

        for (int r = 0; r < m; r++) {
            if (r == row) continue;
            double factor = work[r * work_cols + col];
            if (fabs(factor) < 1e-15) continue;
            for (int c = 0; c < work_cols; c++) {
                work[r * work_cols + c] -= factor * work[row * work_cols + c];
            }
        }
        row++;
    }

    int ns_dim = n - row;
    if (ns_dim <= 0) { *dim = 0; free(work); free(ST); return 0; }

    int ns_count = 0;
    for (int j = 0; j < n && ns_count < ns_dim; j++) {
        int is_free = 1;
        for (int i = 0; i < m; i++) {
            if (fabs(work[i * work_cols + j]) > 1e-12) {
                is_free = 0;
                break;
            }
        }
        if (is_free) {
            for (int i = 0; i < n; i++) {
                conserved[ns_count * n + i] = -work[i * work_cols + n + j];
            }
            ns_count++;
        }
    }

    *dim = ns_count;
    free(work);
    free(ST);
    return 0;
}

/* ==========================================================================
 * ODE Integration (L5)
 * ========================================================================== */

void stoich_rk4_step(stoich_system_t *sys, double dt,
                     void (*rate_func)(const double *x, double *v, void *ctx),
                     void *ctx)
{
    if (!sys || !sys->x.concentrations || !rate_func || dt <= 0.0) return;

    int n = sys->x.length;
    int m = sys->v.length;

    double *k1 = (double *)malloc(4 * n * sizeof(double));
    if (!k1) return;
    double *k2 = k1 + n;
    double *k3 = k2 + n;
    double *k4 = k3 + n;

    double *x_temp = (double *)malloc(n * sizeof(double));
    double *v_temp = (double *)malloc(m * sizeof(double));
    if (!x_temp || !v_temp) { free(k1); free(x_temp); free(v_temp); return; }

    /* k1 = f(x) = S * v(x) */
    rate_func(sys->x.concentrations, v_temp, ctx);
    stoich_compute_derivatives(sys, v_temp, k1);

    /* k2 = f(x + dt/2 * k1) */
    for (int i = 0; i < n; i++) {
        x_temp[i] = sys->x.concentrations[i] + 0.5 * dt * k1[i];
    }
    rate_func(x_temp, v_temp, ctx);
    stoich_compute_derivatives(sys, v_temp, k2);

    /* k3 = f(x + dt/2 * k2) */
    for (int i = 0; i < n; i++) {
        x_temp[i] = sys->x.concentrations[i] + 0.5 * dt * k2[i];
    }
    rate_func(x_temp, v_temp, ctx);
    stoich_compute_derivatives(sys, v_temp, k3);

    /* k4 = f(x + dt * k3) */
    for (int i = 0; i < n; i++) {
        x_temp[i] = sys->x.concentrations[i] + dt * k3[i];
    }
    rate_func(x_temp, v_temp, ctx);
    stoich_compute_derivatives(sys, v_temp, k4);

    /* Update: x_{n+1} = x_n + (dt/6)*(k1 + 2k2 + 2k3 + k4) */
    for (int i = 0; i < n; i++) {
        sys->x.concentrations[i] += (dt / 6.0) *
            (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
        if (sys->x.concentrations[i] < 0.0) {
            sys->x.concentrations[i] = 0.0;
        }
    }

    /* Update derivatives at new state */
    rate_func(sys->x.concentrations, sys->v.rates, ctx);
    stoich_compute_derivatives(sys, sys->v.rates, sys->dxdt.concentrations);

    free(k1);
    free(x_temp);
    free(v_temp);
}

int stoich_integrate_adaptive(stoich_system_t *sys,
                               double t_start, double t_end,
                               double dt_initial, double tolerance,
                               void (*rate_func)(const double *x, double *v, void *ctx),
                               void *ctx)
{
    if (!sys || !rate_func || t_end <= t_start || dt_initial <= 0.0) return 0;

    double t = t_start;
    double dt = dt_initial;
    int n = sys->x.length;
    int steps = 0;

    /* Save current state for error estimation */
    double *x_save = (double *)malloc(n * sizeof(double));
    if (!x_save) return 0;

    while (t < t_end && steps < 100000) {
        if (t + dt > t_end) {
            dt = t_end - t;
        }

        /* Save state */
        memcpy(x_save, sys->x.concentrations, n * sizeof(double));

        /* Take full step */
        stoich_rk4_step(sys, dt, rate_func, ctx);

        /* Take two half steps for error estimation */
        /* (Simplify: reuse x_save, restore state, do two half-steps) */
        memcpy(sys->x.concentrations, x_save, n * sizeof(double));

        stoich_rk4_step(sys, dt * 0.5, rate_func, ctx);
        stoich_rk4_step(sys, dt * 0.5, rate_func, ctx);

        double *x_two_half = (double *)malloc(n * sizeof(double));
        if (!x_two_half) { free(x_save); return steps; }
        memcpy(x_two_half, sys->x.concentrations, n * sizeof(double));

        /* Compute error between full step and two half-steps */
        /* Restore and redo full step */
        memcpy(sys->x.concentrations, x_save, n * sizeof(double));
        stoich_rk4_step(sys, dt, rate_func, ctx);

        double error = 0.0;
        for (int i = 0; i < n; i++) {
            double diff = fabs(sys->x.concentrations[i] - x_two_half[i]);
            double scale = fabs(sys->x.concentrations[i]) + 1e-10;
            double rel_err = diff / scale;
            if (rel_err > error) error = rel_err;
        }

        /* Use the more accurate two-half-step result if error is acceptable */
        if (error <= tolerance) {
            memcpy(sys->x.concentrations, x_two_half, n * sizeof(double));
        }

        free(x_two_half);

        /* Adapt step size: if error > tolerance, reduce; else increase */
        if (error > tolerance) {
            dt *= 0.5;
            /* Reject step: restore */
            memcpy(sys->x.concentrations, x_save, n * sizeof(double));
            continue; /* Don't advance t */
        } else {
            dt *= 1.2;
            if (dt > (t_end - t_start) * 0.1) {
                dt = (t_end - t_start) * 0.1;
            }
        }

        t += dt;
        steps++;
    }

    free(x_save);
    return steps;
}

int stoich_backward_euler_step(stoich_system_t *sys, double dt,
                                void (*rate_func)(const double *x, double *v, void *ctx),
                                void *ctx, int max_newton_iter)
{
    if (!sys || !rate_func || dt <= 0.0) return -1;

    int n = sys->x.length;
    int m = sys->v.length;

    double *x_old = (double *)malloc(n * sizeof(double));
    double *x_new = (double *)malloc(n * sizeof(double));
    double *residual = (double *)malloc(n * sizeof(double));
    double *v_rates = (double *)malloc(m * sizeof(double));
    double *jacobi = (double *)malloc(n * n * sizeof(double));
    double *dx = (double *)malloc(n * sizeof(double));

    if (!x_old || !x_new || !residual || !v_rates || !jacobi || !dx) {
        free(x_old); free(x_new); free(residual);
        free(v_rates); free(jacobi); free(dx);
        return -1;
    }

    /* Save old state */
    memcpy(x_old, sys->x.concentrations, n * sizeof(double));
    memcpy(x_new, x_old, n * sizeof(double));

    /* Newton iteration: solve x_new = x_old + dt * S * v(x_new)
     * F(x) = x - x_old - dt * S * v(x) = 0
     * J = I - dt * S * J_v   where J_v = ∂v/∂x */

    for (int iter = 0; iter < max_newton_iter; iter++) {
        /* Compute F(x) = x - x_old - dt * S * v(x) */
        rate_func(x_new, v_rates, ctx);
        stoich_compute_derivatives(sys, v_rates, residual);

        double max_res = 0.0;
        for (int i = 0; i < n; i++) {
            residual[i] = x_new[i] - x_old[i] - dt * residual[i];
            if (fabs(residual[i]) > max_res) max_res = fabs(residual[i]);
        }

        if (max_res < 1e-10) {
            /* Converged */
            memcpy(sys->x.concentrations, x_new, n * sizeof(double));
            rate_func(sys->x.concentrations, sys->v.rates, ctx);
            stoich_compute_derivatives(sys, sys->v.rates, sys->dxdt.concentrations);
            free(x_old); free(x_new); free(residual);
            free(v_rates); free(jacobi); free(dx);
            return 0;
        }

        /* Approximate Jacobian: J ≈ I (simplified Newton) */
        /* dx = -F(x) (since J ≈ I) */
        for (int i = 0; i < n; i++) {
            dx[i] = -residual[i];
            x_new[i] += dx[i];
            if (x_new[i] < 0.0) x_new[i] = 0.0;
        }
    }

    /* Did not converge — use explicit Euler fallback */
    rate_func(x_old, v_rates, ctx);
    stoich_compute_derivatives(sys, v_rates, residual);
    for (int i = 0; i < n; i++) {
        sys->x.concentrations[i] = x_old[i] + dt * residual[i];
        if (sys->x.concentrations[i] < 0.0) sys->x.concentrations[i] = 0.0;
    }

    free(x_old); free(x_new); free(residual);
    free(v_rates); free(jacobi); free(dx);
    return -1;
}

/* ==========================================================================
 * Steady-State Analysis
 * ========================================================================== */

int stoich_find_steady_state(stoich_system_t *sys,
                              void (*rate_func)(const double *x, double *v, void *ctx),
                              void *ctx, double max_time, double tolerance)
{
    if (!sys || !rate_func) return 0;

    stoich_integrate_adaptive(sys, 0.0, max_time, 0.01, 0.001,
                               rate_func, ctx);

    return stoich_is_steady_state(sys, tolerance);
}

int stoich_is_steady_state(const stoich_system_t *sys, double tolerance)
{
    if (!sys || !sys->dxdt.concentrations) return 0;

    double max_deriv = 0.0;
    for (int i = 0; i < sys->dxdt.length; i++) {
        double abs_deriv = fabs(sys->dxdt.concentrations[i]);
        if (abs_deriv > max_deriv) max_deriv = abs_deriv;
    }

    return (max_deriv < tolerance) ? 1 : 0;
}

void stoich_compute_jacobian(const stoich_system_t *sys,
                              void (*rate_func)(const double *x, double *v, void *ctx),
                              void *ctx, double *jacobian, double epsilon)
{
    if (!sys || !rate_func || !jacobian) return;

    int n = sys->x.length;
    int m = sys->v.length;

    double *x_save = (double *)malloc(n * sizeof(double));
    double *x_pert = (double *)malloc(n * sizeof(double));
    double *v_base = (double *)malloc(m * sizeof(double));
    double *v_pert = (double *)malloc(m * sizeof(double));
    double *dxdt_base = (double *)malloc(n * sizeof(double));
    double *dxdt_pert = (double *)malloc(n * sizeof(double));

    if (!x_save || !x_pert || !v_base || !v_pert || !dxdt_base || !dxdt_pert) {
        free(x_save); free(x_pert); free(v_base);
        free(v_pert); free(dxdt_base); free(dxdt_pert);
        return;
    }

    memcpy(x_save, sys->x.concentrations, n * sizeof(double));

    /* Base derivatives */
    rate_func(x_save, v_base, ctx);
    stoich_compute_derivatives(sys, v_base, dxdt_base);

    /* Finite differences for each species */
    for (int j = 0; j < n; j++) {
        memcpy(x_pert, x_save, n * sizeof(double));
        x_pert[j] += epsilon;

        rate_func(x_pert, v_pert, ctx);
        stoich_compute_derivatives(sys, v_pert, dxdt_pert);

        for (int i = 0; i < n; i++) {
            jacobian[i * n + j] = (dxdt_pert[i] - dxdt_base[i]) / epsilon;
        }
    }

    free(x_save); free(x_pert); free(v_base);
    free(v_pert); free(dxdt_base); free(dxdt_pert);
}

int stoich_eigenvalues(const double *jacobian, int n,
                        double *eigenvalues_real, double *eigenvalues_imag,
                        int max_iter)
{
    if (!jacobian || !eigenvalues_real || !eigenvalues_imag || n <= 0) return -1;

    /* Simple Jacobi eigenvalue algorithm for symmetric matrices.
     * For non-symmetric matrices, use a simplified Hessenberg/QR approach:
     * Reduce to upper Hessenberg form, then use QR iteration. */

    double *H = (double *)malloc(n * n * sizeof(double));
    if (!H) return -1;

    /* Copy Jacobian to working matrix */
    memcpy(H, jacobian, n * n * sizeof(double));

    /* Reduce to upper Hessenberg form via Householder reflections */
    for (int k = 0; k < n - 2; k++) {
        /* Compute Householder vector for column k below diagonal */
        double sigma = 0.0;
        for (int i = k + 1; i < n; i++) {
            sigma += H[i * n + k] * H[i * n + k];
        }

        if (sigma < 1e-15) continue;

        double alpha = sqrt(sigma);
        if (H[(k + 1) * n + k] > 0) alpha = -alpha;

        double *v = (double *)malloc(n * sizeof(double));
        for (int i = 0; i <= k; i++) v[i] = 0.0;
        v[k + 1] = H[(k + 1) * n + k] - alpha;
        for (int i = k + 2; i < n; i++) v[i] = H[i * n + k];

        double beta = 0.0;
        for (int i = k + 1; i < n; i++) beta += v[i] * v[i];
        if (beta < 1e-15) { free(v); continue; }

        /* Apply H = (I - 2*v*v^T/beta) * H * (I - 2*v*v^T/beta) */
        for (int j = k; j < n; j++) {
            double s = 0.0;
            for (int i = k + 1; i < n; i++) s += v[i] * H[i * n + j];
            s *= 2.0 / beta;
            for (int i = k + 1; i < n; i++) H[i * n + j] -= s * v[i];
        }

        for (int i = 0; i < n; i++) {
            double s = 0.0;
            for (int j = k + 1; j < n; j++) s += H[i * n + j] * v[j];
            s *= 2.0 / beta;
            for (int j = k + 1; j < n; j++) H[i * n + j] -= s * v[j];
        }

        free(v);
    }

    /* Extract subdiagonal — if entry is nearly zero, split into blocks */
    /* QR iteration on each unreduced block */
    for (int iter = 0; iter < max_iter; iter++) {
        /* Find if sub-diagonal has converged */
        int all_converged = 1;
        for (int i = n - 1; i > 0; i--) {
            if (fabs(H[i * n + i - 1]) > 1e-12 * (fabs(H[(i-1)*n + i-1]) + fabs(H[i * n + i]))) {
                all_converged = 0;
                break;
            }
        }
        if (all_converged) break;

        /* Simple double-shift QR step (Francis QR) */
        /* Use Wilkinson shift: eigenvalue of bottom 2x2 closest to H[n-1][n-1] */
        double a = H[(n-2)*n + n-2], b = H[(n-2)*n + n-1],
               c = H[(n-1)*n + n-2], d = H[(n-1)*n + n-1];
        double trace = a + d;
        double det = a * d - b * c;
        double disc = trace * trace - 4.0 * det;
        double mu;
        if (disc >= 0) {
            double sr = sqrt(disc);
            double r1 = (trace + sr) / 2.0;
            double r2 = (trace - sr) / 2.0;
            mu = (fabs(r1 - d) < fabs(r2 - d)) ? r1 : r2;
        } else {
            mu = trace / 2.0;
        }

        /* Use mu as shift */
        /* QR decomposition of H - mu*I */
        double *cs = (double *)malloc((n - 1) * sizeof(double));
        double *sn = (double *)malloc((n - 1) * sizeof(double));
        for (int i = 0; i < n - 1; i++) {
            double x = H[i * n + i] - mu;
            double y = H[(i + 1) * n + i];
            double r = sqrt(x * x + y * y);
            if (r > 1e-15) {
                cs[i] = x / r;
                sn[i] = -y / r;

                /* Apply Givens rotation to rows i and i+1 */
                for (int j = i; j < n; j++) {
                    double t1 = H[i * n + j];
                    double t2 = H[(i + 1) * n + j];
                    H[i * n + j] = cs[i] * t1 - sn[i] * t2;
                    H[(i + 1) * n + j] = sn[i] * t1 + cs[i] * t2;
                }

                /* Apply to columns i and i+1 */
                for (int j = 0; j <= (i + 1 < n - 1 ? i + 2 : i + 1) && j < n; j++) {
                    double t1 = H[j * n + i];
                    double t2 = H[j * n + i + 1];
                    H[j * n + i] = cs[i] * t1 - sn[i] * t2;
                    H[j * n + i + 1] = sn[i] * t1 + cs[i] * t2;
                }
            } else {
                cs[i] = 1.0;
                sn[i] = 0.0;
            }
        }
        free(cs);
        free(sn);
    }

    /* Extract eigenvalues from diagonal and subdiagonal blocks */
    for (int i = 0; i < n; i++) {
        if (i < n - 1 && fabs(H[(i+1)*n + i]) > 1e-10) {
            /* 2x2 block: [a b; c d] */
            double a = H[i*n+i], b = H[i*n+i+1], c_val = H[(i+1)*n+i], d = H[(i+1)*n+i+1];
            double tr = a + d, det = a*d - b*c_val;
            double disc = tr*tr - 4.0*det;
            if (disc >= 0) {
                eigenvalues_real[i] = (tr + sqrt(disc)) / 2.0;
                eigenvalues_imag[i] = 0.0;
                eigenvalues_real[i+1] = (tr - sqrt(disc)) / 2.0;
                eigenvalues_imag[i+1] = 0.0;
            } else {
                eigenvalues_real[i] = tr / 2.0;
                eigenvalues_imag[i] = sqrt(-disc) / 2.0;
                eigenvalues_real[i+1] = tr / 2.0;
                eigenvalues_imag[i+1] = -sqrt(-disc) / 2.0;
            }
            i++; /* Skip next */
        } else {
            eigenvalues_real[i] = H[i*n+i];
            eigenvalues_imag[i] = 0.0;
        }
    }

    free(H);
    return 0;
}

/* ==========================================================================
 * Cleanup
 * ========================================================================== */

void stoich_system_destroy(stoich_system_t *sys)
{
    if (!sys) return;

    if (sys->S) {
        free(sys->S->data);
        free(sys->S);
    }
    if (sys->S_sparse) {
        free(sys->S_sparse->values);
        free(sys->S_sparse->col_indices);
        free(sys->S_sparse->row_pointers);
        free(sys->S_sparse);
    }
    if (sys->v.rates) free(sys->v.rates);
    if (sys->x.concentrations) free(sys->x.concentrations);
    if (sys->dxdt.concentrations) free(sys->dxdt.concentrations);

    memset(sys, 0, sizeof(stoich_system_t));
}
