#ifndef WIENER_PREDICTION_H
#define WIENER_PREDICTION_H
#include "wiener_core.h"

/* Wiener Prediction Theory — L4, L5
 * Based on: Wiener (1949) Extrapolation, Interpolation, and Smoothing
 *           Yule (1927), Walker (1931), Kalman (1960)
 *
 * L4: Yule-Walker equations R*a = -r
 * L5: Levinson-Durbin O(N^2) solver
 * L5: Burg maximum-entropy method
 */

typedef struct { double* coeffs; double noise_var; int order; double bic, aic; } WARModel;
typedef struct { double* coeffs; double noise_var; int order; } WMAModel;
typedef struct { double *ar_coeffs, *ma_coeffs; int ar_order, ma_order; double noise_var; } WARMA;
typedef struct { double* coeffs; int order; double mse, gain; } WCLinearPredictor;
typedef struct { double *x_hat, *P, *A, *B, *C, *Q, *R; int n, m, p; double dt; } WCKalmanPredictor;
typedef struct { double* coeffs; int degree; double lead_time, last_value, last_derivative; } WCPolyPredictor;

WARModel* wc_ar_create(int order);
void wc_ar_free(WARModel* ar);
int wc_ar_fit_yule_walker(WARModel* ar, const WCSignal* sig);
int wc_ar_fit_burg(WARModel* ar, const WCSignal* sig);
int wc_ar_fit_levinson(WARModel* ar, const double* autocorr, int order);
WCSignal* wc_ar_simulate(const WARModel* ar, int length, int seed);
double wc_ar_predict_one(const WARModel* ar, const double* past);
WCSignal* wc_ar_predict_n(const WARModel* ar, const WCSignal* sig, int n_ahead);
double wc_ar_log_likelihood(const WARModel* ar, const WCSignal* sig);

WMAModel* wc_ma_create(int order);
void wc_ma_free(WMAModel* ma);
int wc_ma_fit_innovations(WMAModel* ma, const WCSignal* sig);
WCSignal* wc_ma_simulate(const WMAModel* ma, int length, int seed);

WARMA* wc_arma_create(int ar_order, int ma_order);
void wc_arma_free(WARMA* arma);
int wc_arma_fit_hannan_rissanen(WARMA* arma, const WCSignal* sig);
WCSignal* wc_arma_simulate(const WARMA* arma, int length, int seed);
double wc_arma_predict_one(const WARMA* arma, const double* sig, const double* noise, int pos);

WCLinearPredictor* wc_lpredictor_create(int order);
void wc_lpredictor_free(WCLinearPredictor* lp);
int wc_lpredictor_design(WCLinearPredictor* lp, const WCSignal* sig);
double wc_lpredictor_predict(const WCLinearPredictor* lp, const double* past);
WCSignal* wc_lpredictor_predict_signal(const WCLinearPredictor* lp, const WCSignal* sig, int ahead);

WCKalmanPredictor* wc_kalman_create(int n, int m, int p, double dt);
void wc_kalman_free(WCKalmanPredictor* kp);
void wc_kalman_set_model(WCKalmanPredictor* kp, const double* A, const double* B, const double* C, const double* Q, const double* R);
void wc_kalman_predict(WCKalmanPredictor* kp);
void wc_kalman_update(WCKalmanPredictor* kp, const double* measurement);
void wc_kalman_get_state(const WCKalmanPredictor* kp, double* x_hat);

WCPolyPredictor* wc_polypred_create(int degree, double lead_time);
void wc_polypred_free(WCPolyPredictor* pp);
void wc_polypred_update(WCPolyPredictor* pp, double t, double value);
double wc_polypred_predict(WCPolyPredictor* pp, double t);

#endif
