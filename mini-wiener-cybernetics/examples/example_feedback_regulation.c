#include "wiener_core.h"
#include "wiener_feedback.h"
#include "wiener_cybernetics_app.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Example: Feedback Regulation — Thermostat & Servo
 *
 * Demonstrates Wiener's fundamental principle: negative
 * feedback enables goal-directed behavior. Two classic
 * cybernetic examples are shown:
 * 1. Thermostat — temperature regulation (homeostasis)
 * 2. Servomechanism — position control (purposeful action)
 *
 * L6: These are canonical cybernetics problems from
 *     Wiener (1948), Ch. IV "Feedback and Oscillation"
 */

int main(void) {
    printf("=== Cybernetic Feedback Regulation ===\n\n");

    /* Part 1: Thermostat — The Classic Cybernetic System
     * Wiener used the thermostat as the prototypical
     * example of negative feedback in action. */
    printf("--- Part 1: Thermostat ---\n");
    printf("Room with thermal mass 10000 J/K, heat loss 200 W/K\n");
    printf("Outside temperature: 5 C, Desired: 20 C\n\n");

    WCThermalControl* tc = wc_thermal_create(10000.0, 200.0, 5.0);
    wc_thermal_set_desired(tc, 20.0);
    printf("Time(h)  T_room(C)  Heater(W)  Energy(kWh)\n");
    printf("------  ---------  ---------  -----------\n");
    for (int hour = 0; hour <= 5; hour++) {
        for (int step = 0; step < 10; step++)
            wc_thermal_step(tc, 0.1);
        printf("%4d     %7.1f    %8.0f    %9.3f\n",
               hour, tc->room_temp, tc->heater_power,
               tc->energy_used / 3.6e6);
    }
    printf("\nFinal efficiency: %.2f%%\n", wc_thermal_efficiency(tc) * 100.0);
    wc_thermal_free(tc);

    /* Part 2: Servomechanism — Wiener's WWII Problem
     * The servo is the fundamental actuator of cybernetic
     * systems. Wiener worked on servo control for radar
     * antenna positioning and gun aiming. */
    printf("\n--- Part 2: Servomechanism ---\n");
    printf("Inertia=1.0, Damping=0.5, Stiffness=2.0\n");
    printf("Target position: 1.0 rad\n\n");

    WCServomechanism* sv = wc_servo_create(1.0, 0.5, 2.0);
    printf("Step   Position  Velocity  Torque\n");
    printf("----   --------  --------  ------\n");
    for (int step = 0; step <= 50; step += 5) {
        for (int k = 0; k < 5; k++)
            wc_servo_position_control(sv, 1.0, 0.01);
        if (step % 10 == 0)
            printf("%4d   %8.4f  %8.4f  %6.3f\n",
                   step, sv->position, sv->velocity, sv->torque);
    }
    printf("\nFinal position: %.4f rad (target: 1.0)\n", sv->position);
    double steady_error = fabs(sv->position - 1.0);
    printf("Steady-state error: %.4f rad\n", steady_error);
    wc_servo_free(sv);

    /* Part 3: Feedback analysis */
    printf("\n--- Part 3: Feedback Loop Analysis ---\n");
    WCFeedbackLoop* fb = wc_feedback_create(5.0, 2.0, 0.5, 0.01);
    wc_feedback_set_setpoint(fb, 1.0);
    printf("PID: Kp=%.1f Ki=%.1f Kd=%.1f\n", fb->kp, fb->ki, fb->kd);
    double x = 0.0;
    printf("Step   Output   Error    Control\n");
    printf("----   ------   -----    -------\n");
    for (int i = 0; i <= 30; i += 5) {
        for (int k = 0; k < 5; k++) {
            double u = wc_feedback_update(fb, x);
            x += u * 0.01;
        }
        if (i % 10 == 0)
            printf("%4d   %6.4f  %6.4f  %7.4f\n",
                   i, x, fb->error, fb->control);
    }
    printf("Overshoot: %.1f%%\n", wc_feedback_overshoot(fb));
    printf("Settling time (2%%): %.3f s\n",
           wc_feedback_settling_time(fb, 0.02));
    wc_feedback_free(fb);

    printf("\nDone.\n");
    return 0;
}
