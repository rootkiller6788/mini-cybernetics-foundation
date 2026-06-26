#ifndef HOMEOSTAT_MODEL_H
#define HOMEOSTAT_MODEL_H

#include "ashby_homeostasis.h"

/* ============================================================================
 * Homeostat Device Model — Ashby's Electromechanical Homeostat
 *
 * Based on Ashby, "Design for a Brain" (1952/1960), Chapters 7-10.
 *
 * The Homeostat is a device built from four identical interconnected units.
 * Each unit contains a magnet, coil, vane (needle), potentiometer, and
 * commutator driving a uniselector. The device demonstrates ultrastability:
 * when disturbed, it searches through parameter configurations until it
 * finds one that keeps its essential variables within limits.
 *
 * Key insight: the Homeostat does NOT know what to do when disturbed.
 * It randomly changes its parameters (via the step-function mechanism)
 * until stability is regained. This "blind" search is what Ashby called
 * "trial-and-error" or ultrastable adaptation.
 * ============================================================================ */

/* --- Homeostat Unit --- */

/** Maximum number of parameter values a uniselector can hold */
#define HOMEOSTAT_UNISELECTOR_POSITIONS 25

/** Maximum number of units in a homeostat */
#define HOMEOSTAT_MAX_UNITS 8

/** Damping methods for the homeostat unit */
typedef enum {
    HOMEOSTAT_DAMPING_VISCOUS    = 0,  /* Velocity-proportional damping */
    HOMEOSTAT_DAMPING_COULOMB    = 1,  /* Constant friction independent of speed */
    HOMEOSTAT_DAMPING_STRUCTURAL = 2,  /* Displacement-proportional energy loss */
    HOMEOSTAT_DAMPING_RAYLEIGH   = 3   /* Combined mass and stiffness proportional */
} HomeostatDampingType;

/** Single homeostat unit representing one electromagnetic assembly */
typedef struct {
    int unit_id;

    /* Electromechanical state */
    double needle_angle;       /* Current angular position of vane/needle (rad) */
    double needle_velocity;    /* Angular velocity (rad/s) */
    double coil_current;       /* Current through the coil (A) */
    double magnet_field;       /* Magnetic field strength (T) */

    /* Parameters adjustable by the uniselector (step-function mechanism) */
    double resistance;         /* Coil resistance (ohm) — adjustable */
    double inductance;         /* Coil inductance (H) — adjustable */
    double damping_coefficient;/* Mechanical damping (N·m·s/rad) — adjustable */
    double spring_constant;    /* Restoring spring constant (N·m/rad) — adjustable */

    /* Uniselector state: the step-function mechanism */
    int resistance_position;   /* Current position (0..N-1) */
    int inductance_position;
    int damping_position;
    int spring_position;
    double* resistance_values; /* Array of possible resistance values */
    double* inductance_values;
    double* damping_values;
    double* spring_values;
    int n_positions;           /* Number of uniselector positions used */

    /* Interconnection: how this unit couples to others */
    double* coupling_to;       /* Coupling coefficients to each other unit */
    int n_coupled;             /* Number of coupled units */

    /* Stability criterion */
    double angle_min;          /* Minimum acceptable needle angle */
    double angle_max;          /* Maximum acceptable needle angle */
    HomeostatDampingType damping_type;

    /* State tracking */
    bool is_stable;            /* Whether needle is within limits */
    double time_stable;        /* Accumulated time in stable state */
    double stable_angle_center;/* Mean angle when stable (setpoint) */
    int step_count;            /* Number of uniselector steps performed */
} HomeostatUnit;

/* --- Homeostat Assembly --- */

/** The complete 4-unit (or N-unit) homeostat system */
typedef struct {
    HomeostatUnit* units;
    int n_units;

    /* Global configuration */
    double global_supply_voltage;  /* Power supply voltage for all units */
    double stability_threshold;    /* Needle angle must stay within this range */
    double stability_time_required;/* Must be stable for this long (s) to trigger success */
    double step_timeout;           /* Max time before triggering a parameter step */

    /* System-wide energy */
    double total_energy;           /* Sum of kinetic + potential + magnetic energy */
    double power_dissipation;      /* Total power lost as heat */

    /* Behavior tracking */
    double time;                   /* Simulation time */
    int total_steps_taken;         /* Cumulative uniselector steps across all units */
    int max_steps_per_unit;        /* Search budget per unit */
    bool is_converged;             /* True when all units are stable */
    double convergence_time;       /* Time to reach convergence */

    /* Configuration from step-function parameter spaces */
    double** parameter_pool;       /* Pool of parameter values for step-function */
    int* param_pool_indices;       /* Current index in pool for each parameter */
    int pool_size;                 /* Number of parameter sets */

    /* History */
    AHTimeSeries* energy_history;
    AHTimeSeries* stability_metric_history;
} HomeostatSystem;

/* --- Homeostat API --- */

/* Unit lifecycle */
HomeostatUnit* homeostat_unit_create(int unit_id, int n_positions);
void homeostat_unit_free(HomeostatUnit* unit);
void homeostat_unit_set_stability_limits(HomeostatUnit* unit,
                                          double min_angle, double max_angle);
void homeostat_unit_set_parameters(HomeostatUnit* unit,
                                    double R, double L, double D, double K);
void homeostat_unit_configure_uniselector(HomeostatUnit* unit,
    const double* r_vals, const double* l_vals,
    const double* d_vals, const double* k_vals, int n);
void homeostat_unit_connect(HomeostatUnit* unit, double* coupling_coeffs, int n);
void homeostat_unit_step_parameter(HomeostatUnit* unit);
void homeostat_unit_dynamics(HomeostatUnit* unit, double dt);
bool homeostat_unit_check_stability(const HomeostatUnit* unit, double threshold);
void homeostat_unit_print(const HomeostatUnit* unit);

/* System lifecycle */
HomeostatSystem* homeostat_system_create(int n_units, int n_positions);
void homeostat_system_free(HomeostatSystem* hs);
void homeostat_system_configure_parameter_pool(HomeostatSystem* hs,
    double** pool, int pool_size);
void homeostat_system_set_connectivity(HomeostatSystem* hs,
    const double* coupling_matrix, int n);

/* Dynamics and simulation */
void homeostat_system_step(HomeostatSystem* hs, double dt);
void homeostat_system_simulate(HomeostatSystem* hs,
                                double duration, double dt);
void homeostat_system_apply_disturbance(HomeostatSystem* hs,
    int unit_idx, double impulse_magnitude);
bool homeostat_system_all_stable(const HomeostatSystem* hs, double threshold);
double homeostat_system_stability_index(const HomeostatSystem* hs);
void homeostat_system_trigger_search(HomeostatSystem* hs);
void homeostat_system_print(const HomeostatSystem* hs);

/* Analysis */
double homeostat_energy_kinetic(const HomeostatSystem* hs);
double homeostat_energy_magnetic(const HomeostatSystem* hs);
double homeostat_energy_potential(const HomeostatSystem* hs);
double homeostat_power_loss(const HomeostatSystem* hs);
void homeostat_unit_torque_breakdown(const HomeostatUnit* unit,
    double* spring_torque, double* damping_torque,
    double* magnetic_torque, double* coupling_torque,
    const double* other_angles, int n_other);
double homeostat_convergence_rate(const HomeostatSystem* hs);

#endif /* HOMEOSTAT_MODEL_H */