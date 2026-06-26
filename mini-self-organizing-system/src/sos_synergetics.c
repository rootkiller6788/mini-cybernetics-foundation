#include "sos_synergetics.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ===== Linear Stability Analysis ===== */

LinearStability* ls_analyze(const double* J, int n) {
    if (!J || n <= 0) return NULL;
    LinearStability* ls = (LinearStability*)calloc(1, sizeof(LinearStability));
    if (!ls) return NULL;
    ls->n_dims = n; ls->n_modes = 0;
    ls->modes = (Eigenmode*)malloc(n * sizeof(Eigenmode));
    ls->order_param_idx = (int*)malloc(n * sizeof(int));

    /* For a general n x n matrix, we compute eigenvalues using
     * the QR algorithm (simplified: use power iteration for dominant
     * eigenvalues + deflation for smaller systems, or just 2x2).
     * For demonstration, handle n=2 exactly and n>2 approximately. */
    if (n == 2) {
        double a=J[0], b=J[1], c=J[2], d=J[3];
        double tr=a+d, det=a*d-b*c, disc=tr*tr-4.0*det;
        for (int i = 0; i < 2; i++) {
            ls->modes[i].eigenvector = (double*)calloc(n, sizeof(double));
            ls->modes[i].eigenvector[i] = 1.0;
        }
        if (disc >= 0.0) {
            double sd = sqrt(disc);
            ls->modes[0].eigenvalue_real = (tr+sd)/2.0;
            ls->modes[0].eigenvalue_imag = 0.0;
            ls->modes[1].eigenvalue_real = (tr-sd)/2.0;
            ls->modes[1].eigenvalue_imag = 0.0;
            ls->modes[0].eigenvector[0] = -b; ls->modes[0].eigenvector[1] = a - ls->modes[0].eigenvalue_real;
            ls->modes[1].eigenvector[0] = -b; ls->modes[1].eigenvector[1] = a - ls->modes[1].eigenvalue_real;
        } else {
            ls->modes[0].eigenvalue_real = tr/2.0;
            ls->modes[0].eigenvalue_imag = sqrt(-disc)/2.0;
            ls->modes[1].eigenvalue_real = tr/2.0;
            ls->modes[1].eigenvalue_imag = -sqrt(-disc)/2.0;
        }
        ls->n_modes = 2;
    } else {
        /* For n > 2, use a simplified approach:
         * compute trace = sum of eigenvalues, use Gershgorin discs
         * to bound eigenvalue locations. */
        ls->n_modes = n;
        for (int i = 0; i < n; i++) {
            ls->modes[i].eigenvector = (double*)calloc(n, sizeof(double));
            ls->modes[i].eigenvector[i] = 1.0;
            /* Gershgorin center = diagonal element */
            ls->modes[i].eigenvalue_real = J[i*n + i];
            ls->modes[i].eigenvalue_imag = 0.0;
            /* Gershgorin radius = sum of off-diagonal magnitudes */
            double radius = 0.0;
            for (int j = 0; j < n; j++)
                if (j != i) radius += fabs(J[i*n + j]);
            /* Eigenvalue is within disc of radius around center */
            ls->modes[i].relaxation_time = (ls->modes[i].eigenvalue_real != 0.0) ?
                -1.0/ls->modes[i].eigenvalue_real : 1e308;
        }
    }
    /* Classify each mode */
    for (int i = 0; i < ls->n_modes; i++) {
        if (ls->modes[i].eigenvalue_real > 1e-6)
            ls->modes[i].type = MODE_UNSTABLE;
        else if (fabs(ls->modes[i].eigenvalue_real) <= 1e-6)
            ls->modes[i].type = MODE_MARGINAL;
        else
            ls->modes[i].type = MODE_STABLE;
        ls->modes[i].relaxation_time = (ls->modes[i].eigenvalue_real < -1e-15) ?
            -1.0/ls->modes[i].eigenvalue_real : 1e308;
        ls->modes[i].is_order_param =
            (ls->modes[i].type == MODE_MARGINAL || ls->modes[i].type == MODE_UNSTABLE) ? 1 : 0;
    }
    ls_identify_order_params(ls, 1e-6);
    return ls;
}

void ls_free(LinearStability* ls) {
    if (!ls) return;
    for (int i = 0; i < ls->n_modes; i++)
        free(ls->modes[i].eigenvector);
    free(ls->modes);
    free(ls->order_param_idx);
    free(ls);
}

int ls_identify_order_params(LinearStability* ls, double epsilon) {
    if (!ls) return 0;
    ls->n_order_params = 0;
    for (int i = 0; i < ls->n_modes; i++) {
        if (ls->modes[i].eigenvalue_real >= -epsilon) {
            ls->modes[i].is_order_param = 1;
            ls->order_param_idx[ls->n_order_params++] = i;
        } else {
            ls->modes[i].is_order_param = 0;
        }
    }
    return ls->n_order_params;
}

void ls_project_order_params(const LinearStability* ls,
                             const double* state,
                             double* order_param_amplitudes) {
    if (!ls || !state || !order_param_amplitudes) return;
    for (int i = 0; i < ls->n_order_params; i++) {
        int idx = ls->order_param_idx[i];
        double amp = 0.0;
        for (int j = 0; j < ls->n_dims; j++)
            amp += state[j] * ls->modes[idx].eigenvector[j];
        order_param_amplitudes[i] = amp;
    }
}

void ls_reconstruct_state(const LinearStability* ls,
                          const double* order_params,
                          SlavingResult* slave,
                          double* reconstructed_state) {
    if (!ls || !reconstructed_state) return;
    for (int i = 0; i < ls->n_dims; i++) reconstructed_state[i] = 0.0;
    /* Contribution from order parameters */
    for (int p = 0; p < ls->n_order_params; p++) {
        int idx = ls->order_param_idx[p];
        for (int i = 0; i < ls->n_dims; i++)
            reconstructed_state[i] += order_params[p] * ls->modes[idx].eigenvector[i];
    }
    /* Contribution from enslaved fast variables (if available) */
    if (slave) {
        for (int f = 0; f < slave->n_fast_vars; f++)
            for (int p = 0; p < slave->n_order_params; p++) {
                int fast_idx = ls->n_order_params + f;
                if (fast_idx < ls->n_dims)
                    reconstructed_state[fast_idx] +=
                        slave->slaving_coeffs[f][p] * order_params[p];
            }
    }
}

/* ===== Slaving Principle ===== */

SlavingResult* slaving_compute(const LinearStability* ls) {
    if (!ls || ls->n_modes < 2) return NULL;
    SlavingResult* sr = (SlavingResult*)calloc(1, sizeof(SlavingResult));
    if (!sr) return NULL;
    sr->n_order_params = ls->n_order_params;
    sr->n_fast_vars = ls->n_modes - ls->n_order_params;
    sr->order_param_values = (double*)calloc(sr->n_order_params, sizeof(double));
    sr->slaving_coeffs = (double**)malloc(sr->n_fast_vars * sizeof(double*));
    for (int f = 0; f < sr->n_fast_vars; f++) {
        sr->slaving_coeffs[f] = (double*)calloc(sr->n_order_params, sizeof(double));
        int fast_idx = ls->order_param_idx[ls->n_order_params + f];
        /* c_{f,p} = - coupling(f,p) / lambda_fast
         * Linear coupling approximated from eigenvalue ratio */
        double lambda_fast = ls->modes[fast_idx].eigenvalue_real;
        if (fabs(lambda_fast) > 1e-15) {
            for (int p = 0; p < sr->n_order_params; p++) {
                int slow_idx = ls->order_param_idx[p];
                /* Coupling approximated by eigenvector overlap */
                double overlap = 0.0;
                for (int i = 0; i < ls->n_dims; i++)
                    overlap += ls->modes[fast_idx].eigenvector[i] *
                               ls->modes[slow_idx].eigenvector[i];
                sr->slaving_coeffs[f][p] = overlap / (-lambda_fast);
            }
        }
    }
    sr->adiabatic_error = 0.0;
    return sr;
}

void slaving_free(SlavingResult* sr) {
    if (!sr) return;
    free(sr->order_param_values);
    for (int f = 0; f < sr->n_fast_vars; f++)
        free(sr->slaving_coeffs[f]);
    free(sr->slaving_coeffs);
    free(sr);
}

void slaving_evolve_order_params(SlavingResult* sr,
                                 const double* linear_rates,
                                 const double* cubic_coeffs,
                                 double dt) {
    if (!sr || !linear_rates) return;
    for (int i = 0; i < sr->n_order_params; i++) {
        double xi = sr->order_param_values[i];
        double linear = linear_rates[i] * xi;
        double cubic = (cubic_coeffs ? cubic_coeffs[i] : 0.0) * xi*xi*xi;
        sr->order_param_values[i] += dt * (linear - cubic);
    }
}

double slaving_error_estimate(const SlavingResult* sr,
                              const double* order_param_values) {
    if (!sr) return 0.0;
    (void)order_param_values;
    return sr->adiabatic_error;
}

/* ===== Laser System ===== */

LaserSystem* laser_create(double G, double kappa, double gamma, double N_0) {
    LaserSystem* laser = (LaserSystem*)calloc(1, sizeof(LaserSystem));
    if (!laser) return NULL;
    laser->G = G; laser->kappa = kappa; laser->gamma = gamma;
    laser->N_0 = N_0;
    laser->threshold = (2.0*kappa > 1e-15) ? G*N_0/(2.0*kappa) : 0.0;
    laser->n = 0.001; /* Small initial photon number (spontaneous emission) */
    laser->N = N_0;
    laser->noise = 0.01;
    return laser;
}

void laser_free(LaserSystem* laser) { free(laser); }

void laser_step(LaserSystem* laser, double dt) {
    if (!laser) return;
    /* Stochastic laser rate equations (Haken 1970):
     * dn/dt = G*N*n - 2*kappa*n + noise*sqrt(n)*xi
     * dN/dt = gamma*(N_0 - N) - G*N*n */
    double noise_term = laser->noise * sqrt(fabs(laser->n)) *
                        ((double)rand()/RAND_MAX - 0.5);
    double dn = (laser->G * laser->N * laser->n
                 - 2.0*laser->kappa*laser->n) * dt + noise_term * sqrt(dt);
    double dN = (laser->gamma * (laser->N_0 - laser->N)
                 - laser->G * laser->N * laser->n) * dt;
    laser->n += dn;
    laser->N += dN;
    if (laser->n < 0.0) laser->n = 0.0;
    if (laser->N < 0.0) laser->N = 0.0;
}

double laser_steady_state_photons(const LaserSystem* laser) {
    if (!laser) return 0.0;
    double pump = laser->threshold;
    if (pump <= 1.0) return 0.0;
    return (laser->gamma / laser->G) * (pump - 1.0);
}

int laser_is_lasing(const LaserSystem* laser) {
    if (!laser) return 0;
    return (laser->threshold > 1.0 && laser->n > 0.001) ? 1 : 0;
}

/* ===== Landau Theory ===== */

double landau_free_energy(double xi, double a, double b) {
    return (a/2.0)*xi*xi + (b/4.0)*xi*xi*xi*xi;
}

double landau_equilibrium(double a, double b) {
    if (a >= 0.0) return 0.0;
    if (b <= 1e-15) return 0.0;
    return sqrt(-a / b);
}

double landau_susceptibility(double a) {
    /* chi = 1/a for a > 0 (Curie-Weiss law)
     * For a <= 0, susceptibility diverges (critical point) */
    if (a <= 1e-15) return 1e308;
    return 1.0 / a;
}
