#include "cp_core.h"
#include "cp_variety.h"
#include "cp_purpose.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Example 1: Feedback Loop ? Thermostat Temperature Regulation
 *
 * Demonstrates the core philosophical concept of closed-loop control:
 * negative feedback drives error to zero. The thermostat is the canonical
 * example from Wiener's "Cybernetics" (1948) and Rosenblueth et al.'s
 * "Behavior, Purpose and Teleology" (1943).
 *
 * A thermostat's purpose is to maintain temperature.
 * This is achieved through negative feedback:
 *   1. Measure temperature (observe)
 *   2. Compare to setpoint (compare)
 *   3. If too cold ? turn on heater (actuate)
 *   4. If too hot  ? turn off heater
 */

int main(void) {
    printf("=== Example 1: Thermostat Feedback Loop ===\n");
    printf("Wiener's Cybernetics (1948): Control as communication in the animal and the machine\n\n");

    /* Create a 1-state (temperature), 1-input (heater), 1-output system */
    ControlSystem* thermostat = cp_system_create("Thermostat", 1, 1, 1);
    thermostat->x[0] = 15.0;  /* initial temperature = 15?C */

    /* Set reference: target temperature = 22?C with ?0.5?C tolerance */
    double ref[1] = {22.0};
    double tol[1] = {0.5};
    cp_system_set_reference(thermostat, ref, tol);
    cp_system_set_feedback(thermostat, CP_BALANCING_FEEDBACK);
    thermostat->feedback_gain = 5.0;  /* aggressive heating */

    printf("Initial temperature: %.1f?C\n", thermostat->x[0]);
    printf("Target temperature:  %.1f?C (?%.1f?C)\n", ref[0], tol[0]);
    printf("\nSimulating closed-loop control (dt=0.1):\n");
    printf("Time\tTemp\tError\tHeater\n");
    printf("----\t----\t-----\t------\n");

    for (int step = 0; step < 50; step++) {
        cp_system_step(thermostat, 0.1);
        if (step % 5 == 0) {
            printf("%.1fs\t%.2f\t%.3f\t%.3f\n",
                   step * 0.1, thermostat->x[0],
                   thermostat->estimation_error[0], thermostat->u[0]);
        }
    }

    printf("\nFinal temperature: %.4f?C\n", thermostat->x[0]);
    printf("Final error: %.6f\n", thermostat->estimation_error[0]);

    double V = cp_system_lyapunov_value(thermostat);
    printf("Lyapunov function V(x) = ?x? = %.6f\n", V);

    /* Check Ashby's Law: is the regulator sufficiently varied? */
    printf("\n=== Ashby's Law of Requisite Variety ===\n");
    double V_R = cp_variety(2);  /* regulator has 2 states: ON/OFF */
    double V_D = cp_variety(10); /* disturbance: 10 possible fluctuations */
    printf("V(Regulator)  = %.4f bits\n", V_R);
    printf("V(Disturbance) = %.4f bits\n", V_D);
    double ashby = cp_ashby_index(V_R, V_D, 1.0);
    printf("Ashby Index = %.4f (%s)\n", ashby,
           ashby >= 0 ? "Sufficient variety" : "Insufficient variety - regulation may fail");

    cp_system_print(thermostat);
    cp_system_free(thermostat);

    printf("\nExample 1 complete.\n");
    return 0;
}
