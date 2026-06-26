#include "sos_turing.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===== Lifecycle ===== */

TuringSystem* turing_create(TuringSystemType type, int nx, int ny,
                             double Lx, double Ly) {
    TuringSystem* ts = (TuringSystem*)calloc(1, sizeof(TuringSystem));
    if (!ts) return NULL;
    ts->type = type;
    ts->grid.nx = nx; ts->grid.ny = ny;
    ts->grid.Lx = Lx; ts->grid.Ly = Ly;
    ts->grid.dx = Lx / nx; ts->grid.dy = Ly / ny;
    int N = nx * ny;
    ts->u = (double*)calloc(N, sizeof(double));
    ts->v = (double*)calloc(N, sizeof(double));
    ts->u_next = (double*)calloc(N, sizeof(double));
    ts->v_next = (double*)calloc(N, sizeof(double));
    ts->Du = 1.0; ts->Dv = 10.0;
    ts->dt = 0.001; ts->t = 0.0; ts->step = 0;
    ts->turing_condition = 0.0;
    ts->dominant_wavelength = 0.0;
    ts->is_pattern = 0;
    return ts;
}

void turing_free(TuringSystem* ts) {
    if (!ts) return;
    free(ts->u); free(ts->v);
    free(ts->u_next); free(ts->v_next);
    free(ts);
}

void turing_set_params(TuringSystem* ts, const double* params, int n) {
    if (!ts || !params || n <= 0 || n > 8) return;
    ts->n_params = n;
    memcpy(ts->params, params, n * sizeof(double));
}

void turing_set_diffusion(TuringSystem* ts, double Du, double Dv) {
    if (!ts) return;
    ts->Du = Du; ts->Dv = Dv;
}

void turing_random_init(TuringSystem* ts, double u0, double v0, double noise) {
    if (!ts) return;
    int N = ts->grid.nx * ts->grid.ny;
    for (int i = 0; i < N; i++) {
        ts->u[i] = u0 + noise * ((double)rand() / RAND_MAX - 0.5);
        ts->v[i] = v0 + noise * ((double)rand() / RAND_MAX - 0.5);
    }
}

void turing_set_initial(TuringSystem* ts, const double* u_init, const double* v_init) {
    if (!ts) return;
    int N = ts->grid.nx * ts->grid.ny;
    if (u_init) memcpy(ts->u, u_init, N * sizeof(double));
    if (v_init) memcpy(ts->v, v_init, N * sizeof(double));
}

/* ===== Reaction Kinetics ===== */

void turing_reaction_rhs(TuringSystem* ts, double u, double v,
                         double* du_dt, double* dv_dt) {
    if (!ts) { *du_dt = 0.0; *dv_dt = 0.0; return; }
    switch (ts->type) {
    case TURING_SCHNAKENBERG: {
        double a = ts->params[0], b = ts->params[1];
        *du_dt = a - u + u*u*v;
        *dv_dt = b - u*u*v;
        break;
    }
    case TURING_GIERER_MEINHARDT: {
        double rA = ts->params[0], muA = ts->params[1];
        double rH = ts->params[2], muH = ts->params[3];
        *du_dt = rA * u*u / (1.0 + ts->params[4]*u*u) - muA*u;
        *dv_dt = rH * u*u - muH*v;
        break;
    }
    case TURING_GRAY_SCOTT: {
        double F = ts->params[0], k = ts->params[1];
        *du_dt = -u*v*v + F*(1.0 - u);
        *dv_dt = u*v*v - (F + k)*v;
        break;
    }
    case TURING_BRUSSELATOR: {
        double A = ts->params[0], B = ts->params[1];
        *du_dt = A - (B + 1.0)*u + u*u*v;
        *dv_dt = B*u - u*u*v;
        break;
    }
    default:
        *du_dt = 0.0; *dv_dt = 0.0;
    }
}

static double laplacian_5pt(const double* field, int nx, int ny,
                            double dx, double dy, int idx) {
    int xi = idx % nx, yi = idx / nx;
    double center = field[idx];
    double left  = (xi > 0)    ? field[idx-1]    : field[idx];
    double right = (xi < nx-1) ? field[idx+1]    : field[idx];
    double up    = (yi > 0)    ? field[idx-nx]   : field[idx];
    double down  = (yi < ny-1) ? field[idx+nx]   : field[idx];
    double d2x = (left - 2.0*center + right) / (dx*dx);
    double d2y = (up - 2.0*center + down) / (dy*dy);
    return d2x + d2y;
}

void turing_rhs_full(TuringSystem* ts, double* du_dt, double* dv_dt) {
    if (!ts || !du_dt || !dv_dt) return;
    int N = ts->grid.nx * ts->grid.ny;
    for (int i = 0; i < N; i++) {
        double fu, fv;
        turing_reaction_rhs(ts, ts->u[i], ts->v[i], &fu, &fv);
        du_dt[i] = fu + ts->Du * laplacian_5pt(ts->u, ts->grid.nx, ts->grid.ny,
                                                 ts->grid.dx, ts->grid.dy, i);
        dv_dt[i] = fv + ts->Dv * laplacian_5pt(ts->v, ts->grid.nx, ts->grid.ny,
                                                 ts->grid.dx, ts->grid.dy, i);
    }
}

void turing_step_euler(TuringSystem* ts) {
    if (!ts) return;
    int N = ts->grid.nx * ts->grid.ny;
    turing_rhs_full(ts, ts->u_next, ts->v_next);
    for (int i = 0; i < N; i++) {
        ts->u[i] += ts->dt * ts->u_next[i];
        ts->v[i] += ts->dt * ts->v_next[i];
    }
    ts->t += ts->dt; ts->step++;
}

void turing_step_rk2(TuringSystem* ts) {
    if (!ts) return;
    int N = ts->grid.nx * ts->grid.ny;
    double* k1u = (double*)malloc(N * sizeof(double));
    double* k1v = (double*)malloc(N * sizeof(double));
    double* k2u = (double*)malloc(N * sizeof(double));
    double* k2v = (double*)malloc(N * sizeof(double));
    double* utmp = (double*)malloc(N * sizeof(double));
    double* vtmp = (double*)malloc(N * sizeof(double));

    /* Stage 1 */
    turing_rhs_full(ts, k1u, k1v);
    /* Stage 2 */
    for (int i = 0; i < N; i++) {
        utmp[i] = ts->u[i] + ts->dt * k1u[i];
        vtmp[i] = ts->v[i] + ts->dt * k1v[i];
    }
    /* Temporarily swap */
    double* save_u = ts->u, *save_v = ts->v;
    ts->u = utmp; ts->v = vtmp;
    turing_rhs_full(ts, k2u, k2v);
    ts->u = save_u; ts->v = save_v;
    /* Update */
    for (int i = 0; i < N; i++) {
        ts->u[i] += ts->dt * 0.5 * (k1u[i] + k2u[i]);
        ts->v[i] += ts->dt * 0.5 * (k1v[i] + k2v[i]);
    }
    ts->t += ts->dt; ts->step++;
    free(k1u); free(k1v); free(k2u); free(k2v); free(utmp); free(vtmp);
}

void turing_evolve(TuringSystem* ts, int n_steps) {
    for (int i = 0; i < n_steps; i++)
        turing_step_euler(ts);
}

/* ===== Turing Instability Analysis ===== */

void turing_homogeneous_steady_state(const TuringSystem* ts,
                                     double* u_star, double* v_star) {
    if (!ts) { *u_star = 0.0; *v_star = 0.0; return; }
    switch (ts->type) {
    case TURING_SCHNAKENBERG: {
        double a = ts->params[0], b = ts->params[1];
        *u_star = a + b;
        *v_star = b / ((a + b) * (a + b));
        break;
    }
    case TURING_GRAY_SCOTT: {
        double F = ts->params[0], k = ts->params[1];
        double denom = F + k;
        *u_star = (denom > 1e-15) ? 1.0 / (1.0 + k/(F*F)) : 0.0;
        *v_star = (denom > 1e-15) ? F / denom : 0.0;
        break;
    }
    default:
        *u_star = 1.0; *v_star = 1.0;
    }
}

void turing_reaction_jacobian(const TuringSystem* ts,
                              double u_star, double v_star, double* J) {
    if (!ts) { J[0]=0; J[1]=0; J[2]=0; J[3]=0; return; }
    switch (ts->type) {
    case TURING_SCHNAKENBERG: {
        J[0] = -1.0 + 2.0*u_star*v_star;
        J[1] = u_star*u_star;
        J[2] = -2.0*u_star*v_star;
        J[3] = -u_star*u_star;
        break;
    }
    default:
        J[0] = -1.0; J[1] = 1.0; J[2] = -1.0; J[3] = -1.0;
    }
}

int turing_check_instability(const TuringSystem* ts,
                             double u_star, double v_star) {
    if (!ts) return 0;
    double J[4]; turing_reaction_jacobian(ts, u_star, v_star, J);
    double fu=J[0], fv=J[1], gu=J[2], gv=J[3];
    double trace = fu + gv;
    double det = fu*gv - fv*gu;
    if (trace >= 0.0) return 0;
    if (det <= 0.0) return 0;
    double lhs = ts->Dv*fu + ts->Du*gv;
    double rhs = 2.0 * sqrt(ts->Du * ts->Dv * det);
    return (lhs > rhs) ? 1 : 0;
}

double turing_dispersion_relation(const TuringSystem* ts,
                                  double u_star, double v_star, double k) {
    if (!ts) return 0.0;
    double J[4]; turing_reaction_jacobian(ts, u_star, v_star, J);
    double D[4] = {ts->Du, 0.0, 0.0, ts->Dv};
    /* M = J - k^2 D */
    double m11 = J[0] - k*k*D[0], m12 = J[1];
    double m21 = J[2], m22 = J[3] - k*k*D[3];
    double tr = m11 + m22;
    double det = m11*m22 - m12*m21;
    double disc = tr*tr - 4.0*det;
    if (disc < 0.0) return tr/2.0;
    /* Return the larger real eigenvalue */
    double l1 = (tr + sqrt(disc))/2.0;
    double l2 = (tr - sqrt(disc))/2.0;
    return (l1 > l2) ? l1 : l2;
}

double turing_dominant_wavenumber(const TuringSystem* ts,
                                  double u_star, double v_star) {
    if (!ts) return 0.0;
    double best_k = 0.0, best_lambda = -1e308;
    double k_min = 0.1, k_max = 100.0;
    int n_k = 200;
    for (int i = 0; i < n_k; i++) {
        double k = k_min + (k_max - k_min) * i / (n_k - 1);
        double lam = turing_dispersion_relation(ts, u_star, v_star, k);
        if (lam > best_lambda) { best_lambda = lam; best_k = k; }
    }
    return best_k;
}

/* ===== Pattern Analysis ===== */

int turing_pattern_formed(const TuringSystem* ts, double threshold) {
    if (!ts) return 0;
    int N = ts->grid.nx * ts->grid.ny;
    double mean = 0.0;
    for (int i = 0; i < N; i++) mean += ts->u[i];
    mean /= N;
    double var = 0.0;
    for (int i = 0; i < N; i++) {
        double d = ts->u[i] - mean;
        var += d*d;
    }
    var /= N;
    return (var > threshold) ? 1 : 0;
}

double turing_pattern_wavelength(const TuringSystem* ts) {
    /* Approximate wavelength by autocorrelation first zero-crossing */
    if (!ts) return 0.0;
    int nx = ts->grid.nx;
    double* profile = (double*)malloc(nx * sizeof(double));
    int mid_y = ts->grid.ny / 2;
    for (int x = 0; x < nx; x++)
        profile[x] = ts->u[mid_y * nx + x];
    double mean = 0.0;
    for (int x = 0; x < nx; x++) mean += profile[x];
    mean /= nx;
    double denom = 0.0;
    for (int x = 0; x < nx; x++) {
        double d = profile[x] - mean;
        denom += d*d;
    }
    if (denom < 1e-15) { free(profile); return 0.0; }
    /* Find first zero crossing of autocorrelation */
    for (int lag = 1; lag < nx/2; lag++) {
        double ac = 0.0;
        for (int x = 0; x < nx-lag; x++)
            ac += (profile[x]-mean)*(profile[x+lag]-mean);
        ac /= denom;
        if (ac < 0.0) {
            free(profile);
            return 2.0 * lag * ts->grid.dx;
        }
    }
    free(profile);
    return 0.0;
}

int turing_classify_pattern(const TuringSystem* ts) {
    if (!ts) return 0;
    if (!turing_pattern_formed(ts, 0.001)) return 0;
    /* Simple heuristic based on spatial FFT analysis:
     * Check if power is concentrated in one vs two directions */
    double wl = turing_pattern_wavelength(ts);
    if (wl < 1e-15) return 0;
    /* Check FFT peak count to distinguish spots vs stripes */
    int nx = ts->grid.nx, ny = ts->grid.ny;
    double* power = (double*)calloc(nx*ny, sizeof(double));
    /* Compute 2D FFT (naive, for analysis purposes) */
    for (int ky = 0; ky < ny; ky++) {
        for (int kx = 0; kx < nx; kx++) {
            double re = 0.0, im = 0.0;
            for (int y = 0; y < ny; y++)
                for (int x = 0; x < nx; x++) {
                    double theta = 2.0 * M_PI * (kx*x/(double)nx + ky*y/(double)ny);
                    re += ts->u[y*nx+x] * cos(theta);
                    im -= ts->u[y*nx+x] * sin(theta);
                }
            power[ky*nx+kx] = re*re + im*im;
        }
    }
    /* Find dominant modes */
    int peaks = 0;
    double max_power = 0.0;
    for (int i = 1; i < nx*ny; i++)
        if (power[i] > max_power) max_power = power[i];
    double threshold = max_power * 0.5;
    for (int i = 1; i < nx*ny; i++)
        if (power[i] > threshold) peaks++;
    free(power);
    if (peaks <= 2) return 1; /* spots: few dominant modes */
    if (peaks <= 6) return 2; /* stripes */
    return 3; /* labyrinth */
}
