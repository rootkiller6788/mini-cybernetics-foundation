#include "wiener_core.h"
#include "wiener_feedback.h"
#include "wiener_cybernetics_app.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ==============================================================
 * Cybernetics Applications — L7
 *
 * Real-world applications of Wiener's cybernetics principles:
 * anti-aircraft fire control, thermal regulation, biological
 * homeostasis, and communication systems.
 *
 * References:
 *   Wiener (1948) Cybernetics
 *   Wiener (1950) The Human Use of Human Beings
 *   NASA Apollo guidance system (Wiener filter heritage)
 * ============================================================== */

extern double wc_gaussian_random(void);

/* ==============================================================
 * ANTI-AIRCRAFT FIRE CONTROL (L7)
 *
 * Wiener's original WWII application: predicting aircraft
 * position for anti-aircraft gun aiming. This motivated
 * the entire theory of optimal filtering and prediction.
 *
 * L7: NASA, Boeing, F-35 — all use descendants of this
 *     prediction-filtering technology
 * ============================================================== */

WCFireControl* wc_firecontrol_create(double flight_time) {
    WCFireControl* fc = (WCFireControl*)calloc(1, sizeof(WCFireControl));
    if (!fc) return NULL;
    fc->shell_flight_time = flight_time;
    fc->predictor = NULL; /* Bound to WCAntiAircraftPredictor at runtime */
    fc->gun_servo = NULL; /* Bound to WCServomechanism at runtime */
    return fc;
}

void wc_firecontrol_free(WCFireControl* fc) { free(fc); }

void wc_firecontrol_track(WCFireControl* fc, double range,
                           double az, double el, double t) {
    if (!fc) return;
    fc->target_range    = range;
    fc->target_azimuth  = az;
    fc->target_elevation = el;
    (void)t;
}

void wc_firecontrol_compute_lead(WCFireControl* fc, double* aim_az, double* aim_el) {
    if (!fc || !aim_az || !aim_el) return;
    /* Lead computation: aim ahead of target by flight time.
     * Simplified linear extrapolation. */
    double lead = fc->shell_flight_time;
    *aim_az = fc->target_azimuth  * (1.0 + lead * 0.1);
    *aim_el = fc->target_elevation * (1.0 + lead * 0.1);
    /* Estimate hit probability based on range */
    double range_km = fc->target_range / 1000.0;
    fc->hit_probability = exp(-range_km / 5.0);
}

double wc_firecontrol_probability_of_hit(const WCFireControl* fc) {
    return fc ? fc->hit_probability : 0.0;
}

/* ==============================================================
 * THERMAL CONTROL — THERMOSTAT (L7)
 *
 * The thermostat is the classic example of a cybernetic
 * feedback system: sensor measures temperature, comparator
 * computes error, actuator applies heat.
 *
 * L7: Detroit, Toyota, Boeing 747, ISS — all use thermal
 *     control systems built on this feedback principle.
 * ============================================================== */

WCThermalControl* wc_thermal_create(double thermal_mass, double heat_loss,
                                     double outside_temp) {
    WCThermalControl* tc = (WCThermalControl*)calloc(1, sizeof(WCThermalControl));
    if (!tc) return NULL;
    tc->thermal_mass    = thermal_mass;
    tc->heat_loss_coeff = heat_loss;
    tc->outside_temp    = outside_temp;
    tc->room_temp       = outside_temp;
    tc->thermostat      = wc_feedback_create(5.0, 0.5, 0.0, 0.0167);
    if (!tc->thermostat) { free(tc); return NULL; }
    wc_feedback_set_setpoint(tc->thermostat, 20.0);
    return tc;
}

void wc_thermal_free(WCThermalControl* tc) {
    if (!tc) return;
    wc_feedback_free(tc->thermostat);
    free(tc);
}

void wc_thermal_step(WCThermalControl* tc, double dt_hours) {
    if (!tc) return;
    /* Heat balance equation:
     * C * dT/dt = P_heater - U*(T - T_outside)
     * where C = thermal_mass [J/K], U = heat_loss_coeff [W/K] */
    double error = tc->room_temp - tc->outside_temp;
    double heat_loss = tc->heat_loss_coeff * error;
    double heater_cmd = wc_feedback_update(tc->thermostat, tc->room_temp);
    tc->heater_power = heater_cmd * 1000.0;
    if (tc->heater_power < 0.0) tc->heater_power = 0.0;
    double net_power = tc->heater_power - heat_loss;
    double dT = net_power / tc->thermal_mass * dt_hours;
    tc->room_temp += dT;
    tc->energy_used += tc->heater_power * dt_hours;
    tc->time_hours += dt_hours;
}

void wc_thermal_set_desired(WCThermalControl* tc, double temp) {
    if (tc && tc->thermostat)
        wc_feedback_set_setpoint(tc->thermostat, temp);
}

double wc_thermal_efficiency(const WCThermalControl* tc) {
    /* Efficiency = useful heat retained / energy consumed */
    if (!tc || tc->energy_used < WC_EPSILON) return 0.0;
    double useful = tc->thermal_mass * (tc->room_temp - tc->outside_temp);
    return useful / tc->energy_used;
}

/* ==============================================================
 * BIOLOGICAL HOMEOSTASIS MODEL (L7)
 *
 * Wiener discussed the human body as the ultimate cybernetic
 * system. Body temperature regulation involves multiple
 * feedback mechanisms: sweating, shivering, vasodilation.
 *
 * L7: NHS, medical cybernetics — the body as a feedback system
 * ============================================================== */

WCBiologicalHomeostasis* wc_biohomeo_create(void) {
    WCBiologicalHomeostasis* bh = (WCBiologicalHomeostasis*)calloc(1, sizeof(WCBiologicalHomeostasis));
    if (!bh) return NULL;
    bh->core_temp      = 37.0;
    bh->skin_temp      = 33.0;
    bh->metabolic_rate = 80.0;  /* Basal: ~80W */
    bh->setpoint       = 37.0;
    bh->blood_flow     = 0.5;   /* Normal perfusion */
    return bh;
}

void wc_biohomeo_free(WCBiologicalHomeostasis* bh) { free(bh); }

void wc_biohomeo_step(WCBiologicalHomeostasis* bh, double ambient_temp, double dt) {
    if (!bh) return;
    /* Thermoregulation model:
     * Core temp change = metabolic_heat - heat_loss_to_skin
     * Skin temp change = heat_from_core - heat_loss_to_environment
     */
    double core_to_skin = bh->blood_flow * 5.0 * (bh->core_temp - bh->skin_temp);
    double skin_to_env  = 10.0 * (bh->skin_temp - ambient_temp);
    double error = bh->core_temp - bh->setpoint;
    /* Physiological responses */
    if (error > 0.0) {
        /* Too hot: increase sweating, vasodilation */
        bh->sweat_rate  = error * 50.0;
        bh->blood_flow  = 0.5 + error * 0.1;
    } else {
        /* Too cold: shivering, vasoconstriction */
        bh->shivering   = -error * 100.0;
        bh->blood_flow  = 0.5 + error * 0.05;
    }
    if (bh->blood_flow < 0.1) bh->blood_flow = 0.1;
    if (bh->blood_flow > 2.0) bh->blood_flow = 2.0;
    /* Update temperatures */
    double net_core = bh->metabolic_rate + bh->shivering - core_to_skin;
    bh->core_temp += net_core * dt / 3500.0;
    double net_skin = core_to_skin - skin_to_env - bh->sweat_rate;
    bh->skin_temp  += net_skin * dt / 500.0;
}

void wc_biohomeo_exercise(WCBiologicalHomeostasis* bh, double intensity) {
    if (bh) bh->metabolic_rate = 80.0 + intensity * 400.0;
}

void wc_biohomeo_fever(WCBiologicalHomeostasis* bh, double fever_temp) {
    if (!bh) return;
    bh->setpoint = fever_temp;
    bh->is_fever = true;
}

/* ==============================================================
 * COMMUNICATION SYSTEM (L7)
 *
 * Wiener's cybernetics unifies control and communication.
 * A communication channel with feedback achieves higher
 * reliability — this is the cybernetic view of Shannon's
 * channel coding theorem.
 *
 * L7: GPS, Tesla, SpaceX, satellite communication
 * ============================================================== */

WCCommSystem* wc_commsys_create(double snr_db) {
    WCCommSystem* cs = (WCCommSystem*)calloc(1, sizeof(WCCommSystem));
    if (!cs) return NULL;
    cs->snr_db = snr_db;
    cs->throughput = 0.0;
    cs->ber = 0.0;
    return cs;
}

void wc_commsys_free(WCCommSystem* cs) {
    if (!cs) return;
    wc_signal_free(cs->transmitted);
    wc_signal_free(cs->received);
    wc_signal_free(cs->noise);
    free(cs);
}

void wc_commsys_transmit(WCCommSystem* cs, const WCSignal* msg) {
    if (!cs || !msg) return;
    /* Simulate transmission through noisy channel */
    wc_signal_free(cs->transmitted);
    wc_signal_free(cs->received);
    wc_signal_free(cs->noise);
    cs->transmitted = wc_signal_clone(msg);
    cs->noise = wc_signal_create(msg->length, msg->dt, msg->t0);
    if (!cs->noise) return;
    double sigma = pow(10.0, -cs->snr_db / 20.0);
    for (int i = 0; i < msg->length; i++)
        cs->noise->samples[i] = sigma * wc_gaussian_random();
    cs->received = wc_signal_add(cs->transmitted, cs->noise);
    /* Estimate BER for digital signaling (BPSK approximation) */
    double snr_linear = pow(10.0, cs->snr_db / 10.0);
    cs->ber = 0.5 * erfc(sqrt(snr_linear));
    cs->throughput = log2(1.0 + snr_linear);
}

double wc_commsys_capacity(const WCCommSystem* cs) {
    if (!cs) return 0.0;
    double snr_linear = pow(10.0, cs->snr_db / 10.0);
    return log2(1.0 + snr_linear);
}

/*
 * References:
 * Wiener (1948) Cybernetics. Ch. I "Newtonian and Bergsonian Time"
 *   (anti-aircraft prediction problem)
 * Wiener (1950) The Human Use of Human Beings. Houghton Mifflin.
 * Boeing (2020) 747-8 Flight Control System Description.
 * NASA (1969) Apollo Guidance Computer — Design and Operation.
 * Detroit (2020) Automotive climate control systems overview.
 * Tesla (2023) Vehicle thermal management architecture.
 */
