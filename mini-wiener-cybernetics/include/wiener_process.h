#ifndef WIENER_PROCESS_H
#define WIENER_PROCESS_H
#include "wiener_core.h"

/* ==============================================================
 * Wiener Process & Brownian Motion
 * Based on: Wiener (1923) Differential-Space
 *           Einstein (1905) Brownian Motion
 *           Ito (1944) Stochastic Integral
 * ============================================================== */

/* L3: Mathematical Structures */

typedef struct {
    double* path;
    double* times;
    int     n_points;
    double  dt;
    double  t_end;
    double  drift;
    double  diffusion;
    int     seed;
} WCWienerPath;

typedef struct {
    double* mean_path;
    double* var_path;
    double* skewness;
    double* kurtosis;
    int     n_times;
    int     n_paths;
} WCWienerEnsemble;

typedef struct {
    double  (*drift_func)(double x, double t, void* params);
    double  (*diffusion_func)(double x, double t, void* params);
    void*   params;
    double  current_x;
    double  current_t;
} WCItoProcess;

typedef struct {
    double  mu;
    double  sigma;
    double  S0;
} WCGeometricBM;

typedef struct {
    double  theta;
    double  mu;
    double  sigma;
    double  X0;
} WCOrnsteinUhlenbeck;

typedef struct {
    double  sigma;
    double  dt;
    int     seed;
    double  (*custom_dist)(void);
} WCWhiteNoise;

/* Wiener Path API */
WCWienerPath* wc_wiener_path_create(int n_points, double t_end, int seed);
void      wc_wiener_path_free(WCWienerPath* wp);
void      wc_wiener_path_generate(WCWienerPath* wp);
void      wc_wiener_path_generate_drifted(WCWienerPath* wp, double drift, double diff);
double    wc_wiener_path_max(const WCWienerPath* wp);
double    wc_wiener_path_min(const WCWienerPath* wp);
double    wc_wiener_path_terminal(const WCWienerPath* wp);
double    wc_wiener_path_quadratic_variation(const WCWienerPath* wp);
double    wc_wiener_path_first_hit(const WCWienerPath* wp, double barrier);
void      wc_wiener_path_print(const WCWienerPath* wp, const char* label);

/* Ensemble Statistics API */
WCWienerEnsemble* wc_wiener_ensemble_create(int n_times, int n_paths, double t_end, int seed);
void      wc_wiener_ensemble_free(WCWienerEnsemble* we);
void      wc_wiener_ensemble_generate(WCWienerEnsemble* we);
double    wc_wiener_ensemble_mean_at(const WCWienerEnsemble* we, int t_idx);
double    wc_wiener_ensemble_var_at(const WCWienerEnsemble* we, int t_idx);
bool      wc_wiener_ensemble_verify(const WCWienerEnsemble* we, double tolerance);

/* Ito Process API */
WCItoProcess* wc_ito_create(double x0, double t0,
                            double (*drift)(double,double,void*),
                            double (*diff)(double,double,void*),
                            void* params);
void      wc_ito_free(WCItoProcess* ip);
void      wc_ito_step(WCItoProcess* ip, double dt);
WCWienerPath* wc_ito_simulate(WCItoProcess* ip, int n_steps, double dt);

/* Special Process APIs */
WCGeometricBM* wc_gbm_create(double mu, double sigma, double S0);
void       wc_gbm_free(WCGeometricBM* gbm);
WCWienerPath* wc_gbm_simulate(const WCGeometricBM* gbm, int n_steps, double t_end, int seed);

WCOrnsteinUhlenbeck* wc_ou_create(double theta, double mu, double sigma, double X0);
void       wc_ou_free(WCOrnsteinUhlenbeck* ou);
WCWienerPath* wc_ou_simulate(const WCOrnsteinUhlenbeck* ou, int n_steps, double t_end, int seed);

/* White Noise API */
WCWhiteNoise* wc_whitenoise_create(double sigma, double dt, int seed);
void      wc_whitenoise_free(WCWhiteNoise* wn);
double    wc_whitenoise_sample(WCWhiteNoise* wn);
void      wc_whitenoise_fill(WCWhiteNoise* wn, double* buffer, int n);
WCSignal* wc_whitenoise_signal(WCWhiteNoise* wn, int length);

/* Advanced Wiener Process Functions (L8) */
double wc_first_passage_probability(double barrier, double drift, double sigma, double t);
double wc_running_max_probability(double level, double t);
double wc_arcsin_probability(double x);
double wc_volterra_second_order(const double* x, int n, const double* h1, int h1_len, const double* h2, int h2_len, int t_idx);
void   wc_wiener_g_functional(const double* input, int n, int order, double* output);
double wc_stochastic_resonance_simulate(double A, double omega, double sigma, double dt, int n_steps, double* output);
double wc_stochastic_resonance_snr(const double* output, int n, double omega, double dt);
double wc_generalized_autocorrelation(const double* x, int n, int t, int tau);
void   wc_wigner_ville(const double* x, int n, int t, double* freq, double* wv, int n_freq);
int    wc_fokker_planck_step(double* p, int n_x, double dx, double dt, double (*drift)(double,void*), double (*diffusion)(double,void*), void* params);
double wc_entropy_rate_gaussian(const double* spectrum, int n_freq, double df);
double wc_lyapunov_stochastic(const double* trajectory, int n, double (*drift_deriv)(double,void*), void* params);

#endif
