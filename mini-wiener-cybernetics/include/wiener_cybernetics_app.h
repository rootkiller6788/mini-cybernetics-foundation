#ifndef WIENER_CYBERNETICS_APP_H
#define WIENER_CYBERNETICS_APP_H
#include "wiener_core.h"
#include "wiener_feedback.h"
#include "wiener_prediction.h"

/* Cybernetics Applications — L7
 * Based on: Wiener's original WWII and later applications
 */

/* Anti-aircraft fire control (Wiener's WWII project) */
typedef struct {
    void* predictor;
    void* gun_servo;
    double target_range, target_azimuth, target_elevation;
    double shell_flight_time, hit_probability;
} WCFireControl;

/* Thermostat — classic cybernetic demo */
typedef struct {
    double room_temp, outside_temp, thermal_mass;
    double heat_loss_coeff, heater_power, energy_used, time_hours;
    WCFeedbackLoop* thermostat;
} WCThermalControl;

/* Biological homeostasis — body temperature regulation */
typedef struct {
    double core_temp, skin_temp, metabolic_rate;
    double sweat_rate, blood_flow, shivering, setpoint;
    bool is_fever;
} WCBiologicalHomeostasis;

/* Communication system with feedback */
typedef struct {
    WCSignal *transmitted, *received, *noise;
    void* channel;
    double snr_db, ber, throughput;
} WCCommSystem;

WCFireControl* wc_firecontrol_create(double flight_time);
void wc_firecontrol_free(WCFireControl* fc);
void wc_firecontrol_track(WCFireControl* fc, double range, double az, double el, double t);
void wc_firecontrol_compute_lead(WCFireControl* fc, double* aim_az, double* aim_el);
double wc_firecontrol_probability_of_hit(const WCFireControl* fc);

WCThermalControl* wc_thermal_create(double thermal_mass, double heat_loss, double outside_temp);
void wc_thermal_free(WCThermalControl* tc);
void wc_thermal_step(WCThermalControl* tc, double dt_hours);
void wc_thermal_set_desired(WCThermalControl* tc, double temp);
double wc_thermal_efficiency(const WCThermalControl* tc);

WCBiologicalHomeostasis* wc_biohomeo_create(void);
void wc_biohomeo_free(WCBiologicalHomeostasis* bh);
void wc_biohomeo_step(WCBiologicalHomeostasis* bh, double ambient_temp, double dt);
void wc_biohomeo_exercise(WCBiologicalHomeostasis* bh, double intensity);
void wc_biohomeo_fever(WCBiologicalHomeostasis* bh, double fever_temp);

WCCommSystem* wc_commsys_create(double snr_db);
void wc_commsys_free(WCCommSystem* cs);
void wc_commsys_transmit(WCCommSystem* cs, const WCSignal* msg);
double wc_commsys_capacity(const WCCommSystem* cs);

#endif
