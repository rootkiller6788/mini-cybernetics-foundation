#include "homeostat_model.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PI 3.14159265358979323846

static double urand_h(void) { return (double)rand() / (double)RAND_MAX; }
static double gauss_rand_h(void) {
    double u1 = urand_h(), u2 = urand_h();
    return sqrt(-2.0 * log(u1 + 1e-15)) * cos(2.0 * PI * u2);
}

/* ===== HomeostatUnit ===== */
HomeostatUnit* homeostat_unit_create(int unit_id, int n_positions) {
    HomeostatUnit* unit = (HomeostatUnit*)calloc(1, sizeof(HomeostatUnit));
    unit->unit_id = unit_id;
    unit->needle_angle = (urand_h() - 0.5) * 0.2;
    unit->needle_velocity = 0.0;
    unit->coil_current = 0.0;
    unit->magnet_field = 0.05;
    unit->resistance = 100.0; unit->inductance = 0.5;
    unit->damping_coefficient = 0.01; unit->spring_constant = 0.05;
    int np = (n_positions > 0) ? n_positions : 25;
    unit->n_positions = np;
    unit->resistance_values = (double*)malloc(np * sizeof(double));
    unit->inductance_values = (double*)malloc(np * sizeof(double));
    unit->damping_values = (double*)malloc(np * sizeof(double));
    unit->spring_values = (double*)malloc(np * sizeof(double));
    for (int i = 0; i < np; i++) {
        double factor = 0.2 + 1.8 * i / (np - 1.0);
        unit->resistance_values[i] = 100.0 * factor;
        unit->inductance_values[i] = 0.5 * factor;
        unit->damping_values[i] = 0.01 * factor;
        unit->spring_values[i] = 0.05 * factor;
    }
    unit->n_coupled = 0; unit->coupling_to = NULL;
    unit->angle_min = -0.5; unit->angle_max = 0.5;
    unit->damping_type = HOMEOSTAT_DAMPING_VISCOUS;
    unit->is_stable = true; unit->time_stable = 0.0;
    unit->stable_angle_center = 0.0; unit->step_count = 0;
    unit->resistance_position = unit->inductance_position = 0;
    unit->damping_position = unit->spring_position = 0;
    return unit;
}

void homeostat_unit_free(HomeostatUnit* unit) {
    if (!unit) return;
    free(unit->resistance_values); free(unit->inductance_values);
    free(unit->damping_values); free(unit->spring_values);
    free(unit->coupling_to); free(unit);
}

void homeostat_unit_set_stability_limits(HomeostatUnit* unit, double min_angle, double max_angle) {
    if (!unit) return;
    unit->angle_min = min_angle; unit->angle_max = max_angle;
}

void homeostat_unit_set_parameters(HomeostatUnit* unit, double R, double L, double D, double K) {
    if (!unit) return;
    unit->resistance = R; unit->inductance = L;
    unit->damping_coefficient = D; unit->spring_constant = K;
}

void homeostat_unit_configure_uniselector(HomeostatUnit* unit,
    const double* r_vals, const double* l_vals,
    const double* d_vals, const double* k_vals, int n) {
    if (!unit) return;
    int np = (n < unit->n_positions) ? n : unit->n_positions;
    memcpy(unit->resistance_values, r_vals, np * sizeof(double));
    memcpy(unit->inductance_values, l_vals, np * sizeof(double));
    memcpy(unit->damping_values, d_vals, np * sizeof(double));
    memcpy(unit->spring_values, k_vals, np * sizeof(double));
}

void homeostat_unit_connect(HomeostatUnit* unit, double* coupling_coeffs, int n) {
    if (!unit) return;
    free(unit->coupling_to);
    unit->n_coupled = n;
    unit->coupling_to = (double*)malloc(n * sizeof(double));
    memcpy(unit->coupling_to, coupling_coeffs, n * sizeof(double));
}

void homeostat_unit_step_parameter(HomeostatUnit* unit) {
    if (!unit) return;
    int choice = rand() % 4;
    int new_pos = rand() % unit->n_positions;
    switch (choice) {
        case 0: unit->resistance_position = new_pos;
                unit->resistance = unit->resistance_values[new_pos]; break;
        case 1: unit->inductance_position = new_pos;
                unit->inductance = unit->inductance_values[new_pos]; break;
        case 2: unit->damping_position = new_pos;
                unit->damping_coefficient = unit->damping_values[new_pos]; break;
        case 3: unit->spring_position = new_pos;
                unit->spring_constant = unit->spring_values[new_pos]; break;
    }
    unit->step_count++;
}

void homeostat_unit_dynamics(HomeostatUnit* unit, double dt) {
    if (!unit) return;
    double J = 0.001, K_motor = 0.01, V_supply = 12.0;
    double back_emf = K_motor * unit->needle_velocity;
    double di_dt = (V_supply - unit->resistance * unit->coil_current - back_emf) / unit->inductance;
    double motor_torque = K_motor * unit->coil_current;
    double damping_torque = unit->damping_coefficient * unit->needle_velocity;
    double spring_torque = unit->spring_constant * unit->needle_angle;
    double coupling_torque = 0.0;
    if (fabs(unit->needle_angle) > 1.0)
        coupling_torque = -0.02 * unit->needle_angle * (fabs(unit->needle_angle) - 1.0);
    double net_torque = motor_torque - damping_torque - spring_torque + coupling_torque;
    double domega_dt = net_torque / J;
    if (unit->damping_type == HOMEOSTAT_DAMPING_COULOMB && unit->needle_velocity != 0.0) {
        double coulomb = 0.001;
        if (unit->needle_velocity > 0) {
            if (net_torque > coulomb) net_torque -= coulomb; else domega_dt = 0.0;
        } else {
            if (net_torque < -coulomb) net_torque += coulomb; else domega_dt = 0.0;
        }
    }
    unit->coil_current += di_dt * dt;
    unit->needle_velocity += domega_dt * dt;
    unit->needle_angle += unit->needle_velocity * dt;
    if (unit->needle_angle > PI) { unit->needle_angle = PI; unit->needle_velocity *= -0.5; }
    if (unit->needle_angle < -PI) { unit->needle_angle = -PI; unit->needle_velocity *= -0.5; }
}

bool homeostat_unit_check_stability(const HomeostatUnit* unit, double threshold) {
    if (!unit) return false;
    double limit = (threshold > 0.0) ? threshold : (unit->angle_max - unit->angle_min) * 0.3;
    return fabs(unit->needle_angle - unit->stable_angle_center) < limit;
}

void homeostat_unit_print(const HomeostatUnit* unit) {
    if (!unit) return;
    printf("Unit %d: angle=%.4f rad, vel=%.4f, I=%.4f A\n",
           unit->unit_id, unit->needle_angle, unit->needle_velocity, unit->coil_current);
    printf("  R=%.1f ohm, L=%.3f H, D=%.4f, K=%.4f, stable=%s, steps=%d\n",
           unit->resistance, unit->inductance, unit->damping_coefficient,
           unit->spring_constant, unit->is_stable ? "yes" : "no", unit->step_count);
}

/* ===== HomeostatSystem ===== */
HomeostatSystem* homeostat_system_create(int n_units, int n_positions) {
    HomeostatSystem* hs = (HomeostatSystem*)calloc(1, sizeof(HomeostatSystem));
    hs->n_units = (n_units > 8) ? 8 : n_units;
    hs->units = (HomeostatUnit*)calloc(hs->n_units, sizeof(HomeostatUnit));
    for (int i = 0; i < hs->n_units; i++) {
        HomeostatUnit* u = &hs->units[i];
        u->unit_id = i; u->needle_angle = (urand_h() - 0.5) * 0.2;
        u->magnet_field = 0.05; u->resistance = 100.0; u->inductance = 0.5;
        u->damping_coefficient = 0.01; u->spring_constant = 0.05;
        u->n_positions = n_positions;
        u->resistance_values = (double*)malloc(n_positions * sizeof(double));
        u->inductance_values = (double*)malloc(n_positions * sizeof(double));
        u->damping_values = (double*)malloc(n_positions * sizeof(double));
        u->spring_values = (double*)malloc(n_positions * sizeof(double));
        for (int j = 0; j < n_positions; j++) {
            double factor = 0.3 + 1.7 * j / (n_positions - 1.0);
            u->resistance_values[j] = 100.0 * factor;
            u->inductance_values[j] = 0.5 * factor;
            u->damping_values[j] = 0.01 * factor;
            u->spring_values[j] = 0.05 * factor;
        }
        u->angle_min = -0.5; u->angle_max = 0.5;
        u->is_stable = true; u->stable_angle_center = 0.0;
    }
    hs->global_supply_voltage = 12.0; hs->stability_threshold = 0.1;
    hs->stability_time_required = 2.0; hs->step_timeout = 5.0;
    hs->time = 0.0; hs->total_steps_taken = 0; hs->max_steps_per_unit = 100;
    hs->is_converged = false; hs->convergence_time = 0.0;
    hs->energy_history = ashby_timeseries_create(1);
    hs->stability_metric_history = ashby_timeseries_create(1);
    return hs;
}

void homeostat_system_free(HomeostatSystem* hs) {
    if (!hs) return;
    for (int i = 0; i < hs->n_units; i++) {
        free(hs->units[i].resistance_values); free(hs->units[i].inductance_values);
        free(hs->units[i].damping_values); free(hs->units[i].spring_values);
        free(hs->units[i].coupling_to);
    }
    free(hs->units);
    if (hs->parameter_pool) {
        for (int i = 0; i < hs->pool_size; i++) free(hs->parameter_pool[i]);
        free(hs->parameter_pool);
    }
    free(hs->param_pool_indices);
    if (hs->energy_history) ashby_timeseries_free(hs->energy_history);
    if (hs->stability_metric_history) ashby_timeseries_free(hs->stability_metric_history);
    free(hs);
}

void homeostat_system_configure_parameter_pool(HomeostatSystem* hs, double** pool, int pool_size) {
    if (!hs) return;
    if (hs->parameter_pool) {
        for (int i = 0; i < hs->pool_size; i++) free(hs->parameter_pool[i]);
        free(hs->parameter_pool);
    }
    hs->pool_size = pool_size;
    hs->parameter_pool = (double**)malloc(pool_size * sizeof(double*));
    for (int i = 0; i < pool_size; i++) {
        hs->parameter_pool[i] = (double*)malloc(4 * sizeof(double));
        memcpy(hs->parameter_pool[i], pool[i], 4 * sizeof(double));
    }
    free(hs->param_pool_indices);
    hs->param_pool_indices = (int*)calloc(pool_size, sizeof(int));
}

void homeostat_system_set_connectivity(HomeostatSystem* hs, const double* coupling_matrix, int n) {
    if (!hs) return;
    for (int i = 0; i < hs->n_units && i < n; i++) {
        free(hs->units[i].coupling_to);
        hs->units[i].n_coupled = hs->n_units - 1;
        hs->units[i].coupling_to = (double*)malloc((hs->n_units - 1) * sizeof(double));
        int idx = 0;
        for (int j = 0; j < hs->n_units; j++)
            if (i != j) hs->units[i].coupling_to[idx++] = coupling_matrix[i * hs->n_units + j];
    }
}
void homeostat_system_step(HomeostatSystem* hs, double dt) {
    if (!hs) return;
    double* coupling_torques = (double*)calloc(hs->n_units, sizeof(double));
    for (int i = 0; i < hs->n_units; i++) {
        HomeostatUnit* ui = &hs->units[i];
        int ci = 0;
        for (int j = 0; j < hs->n_units; j++) {
            if (i == j) continue;
            if (ci < ui->n_coupled)
                coupling_torques[i] += ui->coupling_to[ci++] * (hs->units[j].needle_angle - ui->needle_angle);
        }
    }
    for (int i = 0; i < hs->n_units; i++) {
        HomeostatUnit* unit = &hs->units[i];
        double J = 0.001, K_motor = 0.01, V_supply = hs->global_supply_voltage;
        double back_emf = K_motor * unit->needle_velocity;
        double di_dt = (V_supply - unit->resistance * unit->coil_current - back_emf) / unit->inductance;
        double motor_torque = K_motor * unit->coil_current;
        double damping_torque = unit->damping_coefficient * unit->needle_velocity;
        double spring_torque = unit->spring_constant * unit->needle_angle;
        double net_torque = motor_torque - damping_torque - spring_torque + coupling_torques[i];
        double domega_dt = net_torque / J;
        unit->coil_current += di_dt * dt;
        unit->needle_velocity += domega_dt * dt;
        unit->needle_angle += unit->needle_velocity * dt;
        if (unit->needle_angle > PI) { unit->needle_angle = PI; unit->needle_velocity *= -0.5; }
        if (unit->needle_angle < -PI) { unit->needle_angle = -PI; unit->needle_velocity *= -0.5; }
        unit->is_stable = homeostat_unit_check_stability(unit, hs->stability_threshold);
        if (unit->is_stable) {
            unit->time_stable += dt;
            unit->stable_angle_center = unit->stable_angle_center * 0.95 + unit->needle_angle * 0.05;
        }
    }
    free(coupling_torques);
    hs->total_energy = homeostat_energy_kinetic(hs) + homeostat_energy_magnetic(hs) + homeostat_energy_potential(hs);
    hs->power_dissipation = homeostat_power_loss(hs);
    hs->is_converged = homeostat_system_all_stable(hs, hs->stability_threshold);
    if (hs->is_converged && hs->convergence_time == 0.0) hs->convergence_time = hs->time;
    ashby_timeseries_record(hs->energy_history, hs->total_energy);
    ashby_timeseries_record(hs->stability_metric_history, homeostat_system_stability_index(hs));
    hs->time += dt;
}

void homeostat_system_simulate(HomeostatSystem* hs, double duration, double dt) {
    if (!hs) return;
    int steps = (int)(duration / dt);
    for (int i = 0; i < steps; i++) homeostat_system_step(hs, dt);
}

void homeostat_system_apply_disturbance(HomeostatSystem* hs, int unit_idx, double impulse_magnitude) {
    if (!hs || unit_idx < 0 || unit_idx >= hs->n_units) return;
    hs->units[unit_idx].needle_velocity += impulse_magnitude / 0.001;
}

bool homeostat_system_all_stable(const HomeostatSystem* hs, double threshold) {
    if (!hs) return false;
    for (int i = 0; i < hs->n_units; i++)
        if (!homeostat_unit_check_stability(&hs->units[i], threshold)) return false;
    return true;
}

double homeostat_system_stability_index(const HomeostatSystem* hs) {
    if (!hs || hs->n_units == 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < hs->n_units; i++) {
        double range = hs->units[i].angle_max - hs->units[i].angle_min + 1e-6;
        double angle_norm = fabs(hs->units[i].needle_angle) / range;
        sum += 1.0 - ((angle_norm < 1.0) ? angle_norm : 1.0);
    }
    return sum / hs->n_units;
}

void homeostat_system_trigger_search(HomeostatSystem* hs) {
    if (!hs) return;
    for (int i = 0; i < hs->n_units; i++) {
        if (!hs->units[i].is_stable && hs->units[i].step_count < hs->max_steps_per_unit) {
            homeostat_unit_step_parameter(&hs->units[i]);
            hs->total_steps_taken++;
        }
    }
}

void homeostat_system_print(const HomeostatSystem* hs) {
    if (!hs) return;
    printf("=== Homeostat System: %d units ===\n", hs->n_units);
    printf("Time: %.3f, Total steps: %d, Converged: %s (at t=%.3f)\n",
           hs->time, hs->total_steps_taken, hs->is_converged ? "yes" : "no", hs->convergence_time);
    printf("Energy: %.4f, Power dissipation: %.4f, Stability index: %.4f\n",
           hs->total_energy, hs->power_dissipation, homeostat_system_stability_index(hs));
    for (int i = 0; i < hs->n_units; i++) homeostat_unit_print(&hs->units[i]);
}

double homeostat_energy_kinetic(const HomeostatSystem* hs) {
    if (!hs) return 0.0;
    double E_k = 0.0;
    for (int i = 0; i < hs->n_units; i++)
        E_k += 0.5 * 0.001 * hs->units[i].needle_velocity * hs->units[i].needle_velocity;
    return E_k;
}

double homeostat_energy_magnetic(const HomeostatSystem* hs) {
    if (!hs) return 0.0;
    double E_m = 0.0;
    for (int i = 0; i < hs->n_units; i++)
        E_m += 0.5 * hs->units[i].inductance * hs->units[i].coil_current * hs->units[i].coil_current;
    return E_m;
}

double homeostat_energy_potential(const HomeostatSystem* hs) {
    if (!hs) return 0.0;
    double E_p = 0.0;
    for (int i = 0; i < hs->n_units; i++)
        E_p += 0.5 * hs->units[i].spring_constant * hs->units[i].needle_angle * hs->units[i].needle_angle;
    return E_p;
}

double homeostat_power_loss(const HomeostatSystem* hs) {
    if (!hs) return 0.0;
    double P = 0.0;
    for (int i = 0; i < hs->n_units; i++) {
        P += hs->units[i].coil_current * hs->units[i].coil_current * hs->units[i].resistance;
        P += hs->units[i].damping_coefficient * hs->units[i].needle_velocity * hs->units[i].needle_velocity;
    }
    return P;
}

void homeostat_unit_torque_breakdown(const HomeostatUnit* unit,
    double* spring_torque, double* damping_torque,
    double* magnetic_torque, double* coupling_torque,
    const double* other_angles, int n_other) {
    if (!unit) { *spring_torque=*damping_torque=*magnetic_torque=*coupling_torque=0.0; return; }
    *spring_torque = -unit->spring_constant * unit->needle_angle;
    *damping_torque = -unit->damping_coefficient * unit->needle_velocity;
    *magnetic_torque = 0.01 * unit->coil_current;
    *coupling_torque = 0.0;
    int n = (n_other < unit->n_coupled) ? n_other : unit->n_coupled;
    for (int i = 0; i < n; i++)
        *coupling_torque += unit->coupling_to[i] * (other_angles[i] - unit->needle_angle);
}

double homeostat_convergence_rate(const HomeostatSystem* hs) {
    if (!hs || hs->total_steps_taken == 0) return 0.0;
    return (double)hs->n_units / (hs->total_steps_taken + 1.0);
}