#include "ashby_homeostasis.h"
#include "homeostat_model.h"
#include "ultrastability.h"
#include "variety.h"
#include "adaptive_regulator.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Real-World Applications of Ashby Homeostasis Theory
 *
 * L7 Applications: Demonstrating homeostasis principles in practical domains:
 *   1. Biological homeostasis (body temperature regulation)
 *   2. Organizational cybernetics (Beer's Viable System Model)
 *   3. Climate control (building HVAC as homeostat)
 *   4. Autonomous robotics (navigation with essential variables)
 *   5. Supply chain stability (inventory as homeostatic variable)
 * ============================================================================ */

/* ===== 1. Biological Homeostasis: Body Temperature Regulation ===== */

typedef struct {
    AshbySystem* system;
    double core_temp;       /* degrees Celsius */
    double skin_temp;
    double metabolic_rate;  /* Watts */
    double sweat_rate;      /* g/s */
    double shiver_intensity;/* 0-1 */
    double ambient_temp;    /* environment temperature */
} BodyThermoregulation;

BodyThermoregulation* body_thermo_create(double ambient_temp) {
    BodyThermoregulation* bt = (BodyThermoregulation*)calloc(1, sizeof(BodyThermoregulation));
    bt->system = ashby_system_create("HumanThermoregulation", 3);
    ashby_ev_add(bt->system->essential_vars, "core_temperature", 36.0, 38.0, true);
    ashby_ev_add(bt->system->essential_vars, "skin_temperature", 30.0, 37.0, false);
    ashby_ev_add(bt->system->essential_vars, "blood_pH", 7.35, 7.45, true);
    bt->core_temp = 37.0; bt->skin_temp = 33.0; bt->metabolic_rate = 80.0;
    bt->ambient_temp = ambient_temp;
    ashby_system_set_regulator(bt->system, REGULATOR_ERROR_DRIVEN);
    bt->system->regulatory_capacity = 5.0;
    return bt;
}

void body_thermo_free(BodyThermoregulation* bt) {
    if (!bt) return;
    ashby_system_free(bt->system); free(bt);
}

void body_thermo_step(BodyThermoregulation* bt, double dt) {
    if (!bt) return;
    /* Heat transfer: Newton's law of cooling */
    double heat_loss = 0.05 * (bt->core_temp - bt->ambient_temp);
    /* Metabolic heat production */
    double heat_production = bt->metabolic_rate * 0.01;
    /* Sweating: evaporative cooling */
    double evap_cooling = bt->sweat_rate * 2.43; /* latent heat of water */
    /* Shivering: increased metabolic heat */
    double shiver_heat = bt->shiver_intensity * 50.0;
    /* Regulation: maintain 37C */
    double temp_error = 37.0 - bt->core_temp;
    if (temp_error > 0) {
        /* Too cold: reduce sweat, increase shiver */
        bt->shiver_intensity += 0.1 * temp_error * dt;
        bt->sweat_rate -= 0.5 * temp_error * dt;
    } else {
        /* Too hot: increase sweat, reduce shiver */
        bt->sweat_rate -= 0.5 * temp_error * dt;
        bt->shiver_intensity += 0.1 * temp_error * dt;
    }
    if (bt->shiver_intensity < 0.0) bt->shiver_intensity = 0.0;
    if (bt->shiver_intensity > 1.0) bt->shiver_intensity = 1.0;
    if (bt->sweat_rate < 0.0) bt->sweat_rate = 0.0;
    double net_heat = heat_production + shiver_heat - heat_loss - evap_cooling;
    bt->core_temp += net_heat * 0.001 * dt;
    bt->skin_temp += (bt->ambient_temp - bt->skin_temp) * 0.01 * dt;
    bt->system->state_vector[0] = bt->core_temp;
    bt->system->state_vector[1] = bt->skin_temp;
    bt->system->state_vector[2] = 7.4; /* pH assumed stable */
    ashby_system_step(bt->system);
}

void body_thermo_simulate(BodyThermoregulation* bt, double duration, double dt) {
    for (double t = 0; t < duration; t += dt) body_thermo_step(bt, dt);
}

void body_thermo_print(const BodyThermoregulation* bt) {
    if (!bt) return;
    printf("=== Body Thermoregulation ===\n");
    printf("Core: %.2f C, Skin: %.2f C, Ambient: %.2f C\n", bt->core_temp, bt->skin_temp, bt->ambient_temp);
    printf("Sweat: %.2f g/s, Shiver: %.2f, Metabolic: %.1f W\n", bt->sweat_rate, bt->shiver_intensity, bt->metabolic_rate);
    ashby_system_print_status(bt->system);
}

/* ===== 2. Organizational Cybernetics: Viable System Model-inspired ===== */

typedef struct {
    AshbySystem* system;
    double resource_level;     /* Essential variable: resource reserves */
    double demand_rate;        /* External demand on the system */
    double production_rate;    /* Internal production capacity */
    double adaptation_budget;  /* Resources allocated to adaptation */
    int n_subsystems;          /* Number of operational units */
    double* subsystem_perf;    /* Performance of each subsystem */
} OrganizationalHomeostat;

OrganizationalHomeostat* org_homeostat_create(int n_subsystems) {
    OrganizationalHomeostat* oh = (OrganizationalHomeostat*)calloc(1, sizeof(OrganizationalHomeostat));
    oh->system = ashby_system_create("OrgHomeostat", 2);
    ashby_ev_add(oh->system->essential_vars, "resource_level", 10.0, 100.0, true);
    ashby_ev_add(oh->system->essential_vars, "performance_index", 0.5, 1.0, false);
    oh->resource_level = 50.0; oh->demand_rate = 5.0; oh->production_rate = 5.0;
    oh->adaptation_budget = 10.0; oh->n_subsystems = n_subsystems;
    oh->subsystem_perf = (double*)calloc(n_subsystems, sizeof(double));
    for (int i = 0; i < n_subsystems; i++) oh->subsystem_perf[i] = 0.8;
    ashby_system_set_regulator(oh->system, REGULATOR_ADAPTIVE);
    oh->system->regulatory_capacity = 10.0;
    return oh;
}

void org_homeostat_free(OrganizationalHomeostat* oh) {
    if (!oh) return;
    ashby_system_free(oh->system); free(oh->subsystem_perf); free(oh);
}

void org_homeostat_step(OrganizationalHomeostat* oh, double dt) {
    if (!oh) return;
    double total_perf = 0.0;
    for (int i = 0; i < oh->n_subsystems; i++) total_perf += oh->subsystem_perf[i];
    double avg_perf = total_perf / oh->n_subsystems;
    oh->production_rate = avg_perf * oh->resource_level * 0.1;
    /* Resource dynamics: production - demand + adaptation investment */
    double net_change = oh->production_rate - oh->demand_rate;
    /* Adaptation: if resources are sufficient, invest in improvement */
    if (oh->resource_level > 30.0 && oh->adaptation_budget > 0.0) {
        for (int i = 0; i < oh->n_subsystems; i++)
            oh->subsystem_perf[i] += 0.01 * oh->adaptation_budget * dt / oh->n_subsystems;
        oh->adaptation_budget -= 0.5 * dt;
    }
    oh->resource_level += net_change * dt;
    if (oh->resource_level < 0.0) oh->resource_level = 0.0;
    oh->system->state_vector[0] = oh->resource_level;
    oh->system->state_vector[1] = avg_perf;
    ashby_system_step(oh->system);
}

/* ===== 3. Climate Control: Building HVAC as Homeostat ===== */

typedef struct {
    AshbySystem* system;
    double indoor_temp;
    double indoor_humidity;
    double outdoor_temp;
    double outdoor_humidity;
    double heating_power;    /* kW */
    double cooling_power;    /* kW */
    double humidifier_output;/* g/hr */
    double dehumidifier_rate;/* g/hr */
} ClimateHomeostat;

ClimateHomeostat* climate_homeostat_create(double outdoor_temp, double outdoor_humidity) {
    ClimateHomeostat* ch = (ClimateHomeostat*)calloc(1, sizeof(ClimateHomeostat));
    ch->system = ashby_system_create("BuildingHVAC", 2);
    ashby_ev_add(ch->system->essential_vars, "indoor_temp", 18.0, 26.0, false);
    ashby_ev_add(ch->system->essential_vars, "indoor_humidity", 30.0, 60.0, false);
    ch->indoor_temp = 22.0; ch->indoor_humidity = 45.0;
    ch->outdoor_temp = outdoor_temp; ch->outdoor_humidity = outdoor_humidity;
    ch->heating_power = 0.0; ch->cooling_power = 0.0;
    ch->humidifier_output = 0.0; ch->dehumidifier_rate = 0.0;
    ashby_system_set_regulator(ch->system, REGULATOR_PREDICTIVE);
    ch->system->regulatory_capacity = 3.0;
    return ch;
}

void climate_homeostat_free(ClimateHomeostat* ch) {
    if (!ch) return;
    ashby_system_free(ch->system); free(ch);
}

void climate_homeostat_step(ClimateHomeostat* ch, double dt) {
    if (!ch) return;
    /* Thermal model */
    double heat_transfer = 0.02 * (ch->outdoor_temp - ch->indoor_temp);
    double solar_gain = (ch->outdoor_temp > 25.0) ? 0.5 : 0.0;
    /* Humidity model */
    double moisture_transfer = 0.01 * (ch->outdoor_humidity - ch->indoor_humidity);
    /* Regulation */
    double temp_error = 22.0 - ch->indoor_temp;
    if (temp_error > 0) { ch->heating_power = temp_error; ch->cooling_power = 0.0; }
    else { ch->cooling_power = -temp_error; ch->heating_power = 0.0; }
    double hum_error = 45.0 - ch->indoor_humidity;
    if (hum_error > 0) { ch->humidifier_output = hum_error; ch->dehumidifier_rate = 0.0; }
    else { ch->dehumidifier_rate = -hum_error; ch->humidifier_output = 0.0; }
    ch->indoor_temp += (heat_transfer + solar_gain + ch->heating_power*0.1 - ch->cooling_power*0.1) * dt;
    ch->indoor_humidity += (moisture_transfer + ch->humidifier_output*0.05 - ch->dehumidifier_rate*0.05) * dt;
    ch->system->state_vector[0] = ch->indoor_temp;
    ch->system->state_vector[1] = ch->indoor_humidity;
    ashby_system_step(ch->system);
}

/* ===== 4. Autonomous Robotics: Navigation Homeostat ===== */

typedef struct {
    AshbySystem* system;
    double battery_level;      /* Essential variable: energy */
    double distance_to_goal;
    double obstacle_proximity; /* 0=clear, 1=imminent collision */
    double speed;
    double heading;            /* radians */
    double goal_heading;
    double* sensor_readings;
    int n_sensors;
} RobotHomeostat;

RobotHomeostat* robot_homeostat_create(int n_sensors) {
    RobotHomeostat* rh = (RobotHomeostat*)calloc(1, sizeof(RobotHomeostat));
    rh->system = ashby_system_create("RobotHomeostat", 3);
    ashby_ev_add(rh->system->essential_vars, "battery_level", 20.0, 100.0, true);
    ashby_ev_add(rh->system->essential_vars, "obstacle_proximity", 0.0, 0.3, true);
    ashby_ev_add(rh->system->essential_vars, "distance_to_goal", 0.0, 100.0, false);
    rh->battery_level = 100.0; rh->distance_to_goal = 50.0;
    rh->obstacle_proximity = 0.0; rh->speed = 1.0;
    rh->heading = 0.0; rh->goal_heading = 0.5;
    rh->n_sensors = n_sensors;
    rh->sensor_readings = (double*)calloc(n_sensors, sizeof(double));
    ashby_system_set_regulator(rh->system, REGULATOR_PREDICTIVE);
    return rh;
}

void robot_homeostat_free(RobotHomeostat* rh) {
    if (!rh) return;
    ashby_system_free(rh->system); free(rh->sensor_readings); free(rh);
}

void robot_homeostat_step(RobotHomeostat* rh, double dt) {
    if (!rh) return;
    rh->battery_level -= rh->speed * 0.5 * dt;
    /* Heading correction toward goal */
    double heading_error = rh->goal_heading - rh->heading;
    rh->heading += 0.5 * heading_error * dt;
    rh->distance_to_goal -= rh->speed * cos(heading_error) * dt;
    if (rh->distance_to_goal < 0.0) rh->distance_to_goal = 0.0;
    /* Obstacle avoidance (simulated) */
    rh->obstacle_proximity = 0.0;
    for (int i = 0; i < rh->n_sensors; i++) {
        rh->sensor_readings[i] = 0.1 * (double)(rand() % 10) / 10.0;
        if (rh->sensor_readings[i] > rh->obstacle_proximity)
            rh->obstacle_proximity = rh->sensor_readings[i];
    }
    rh->system->state_vector[0] = rh->battery_level;
    rh->system->state_vector[1] = rh->obstacle_proximity;
    rh->system->state_vector[2] = rh->distance_to_goal;
    ashby_system_step(rh->system);
}

/* ===== 5. Supply Chain Stability ===== */

typedef struct {
    AshbySystem* system;
    double inventory_level;    /* Essential variable */
    double order_backlog;
    double supplier_reliability;/* 0-1 */
    double demand_variability; /* coefficient of variation */
    double lead_time_days;
    double reorder_point;
    double safety_stock;
} SupplyChainHomeostat;

SupplyChainHomeostat* supply_chain_create(double demand_var, double lead_time) {
    SupplyChainHomeostat* sc = (SupplyChainHomeostat*)calloc(1, sizeof(SupplyChainHomeostat));
    sc->system = ashby_system_create("SupplyChain", 2);
    ashby_ev_add(sc->system->essential_vars, "inventory_level", 50.0, 500.0, true);
    ashby_ev_add(sc->system->essential_vars, "order_backlog", 0.0, 50.0, false);
    sc->inventory_level = 200.0; sc->order_backlog = 0.0;
    sc->supplier_reliability = 0.95; sc->demand_variability = demand_var;
    sc->lead_time_days = lead_time; sc->reorder_point = 100.0; sc->safety_stock = 50.0;
    ashby_system_set_regulator(sc->system, REGULATOR_LEARNING);
    return sc;
}

void supply_chain_free(SupplyChainHomeostat* sc) {
    if (!sc) return;
    ashby_system_free(sc->system); free(sc);
}

void supply_chain_step(SupplyChainHomeostat* sc, double dt) {
    if (!sc) return;
    double base_demand = 10.0;
    double noise = ((double)rand()/RAND_MAX - 0.5) * 2.0 * sc->demand_variability * base_demand;
    double demand = base_demand + noise;
    if (demand < 1.0) demand = 1.0;
    double supply = (sc->inventory_level > 0) ? 8.0 * sc->supplier_reliability : 0.0;
    sc->inventory_level += supply * dt - demand * dt;
    if (sc->inventory_level < sc->reorder_point) sc->order_backlog += (sc->reorder_point - sc->inventory_level) * 0.1;
    if (sc->inventory_level < 0.0) sc->inventory_level = 0.0;
    sc->system->state_vector[0] = sc->inventory_level;
    sc->system->state_vector[1] = sc->order_backlog;
    ashby_system_step(sc->system);
}

/* ===== Utility: Run all application demos ===== */

void homeostasis_applications_demo(void) {
    printf("\n========================================\n");
    printf("  Homeostasis Application Demonstrations\n");
    printf("========================================\n\n");

    /* 1. Body Thermoregulation */
    printf("--- 1. Biological Homeostasis: Body Temperature ---\n");
    BodyThermoregulation* bt = body_thermo_create(10.0); /* cold environment */
    for (int i = 0; i < 50; i++) {
        body_thermo_step(bt, 0.1);
        if (i % 10 == 0) body_thermo_print(bt);
    }
    body_thermo_free(bt);
    printf("\n");

    /* 2. Organizational */
    printf("--- 2. Organizational Homeostat ---\n");
    OrganizationalHomeostat* oh = org_homeostat_create(5);
    for (int i = 0; i < 30; i++) {
        org_homeostat_step(oh, 0.2);
        if (i % 10 == 0) ashby_system_print_status(oh->system);
    }
    org_homeostat_free(oh);
    printf("\n");

    /* 3. Climate Control */
    printf("--- 3. Building HVAC Homeostat ---\n");
    ClimateHomeostat* ch = climate_homeostat_create(35.0, 70.0); /* hot humid */
    for (int i = 0; i < 30; i++) {
        climate_homeostat_step(ch, 0.2);
        if (i % 10 == 0)
            printf("[t=%.1f] Indoor: %.1fC, %.0f%% RH\n", i*0.2, ch->indoor_temp, ch->indoor_humidity);
    }
    climate_homeostat_free(ch);
    printf("\n");

    /* 4. Robot */
    printf("--- 4. Robot Navigation Homeostat ---\n");
    RobotHomeostat* rh = robot_homeostat_create(3);
    for (int i = 0; i < 30; i++) {
        robot_homeostat_step(rh, 0.2);
        if (i % 10 == 0)
            printf("[t=%.1f] Battery=%.1f, Obstacle=%.2f, Dist=%.1f\n",
                   i*0.2, rh->battery_level, rh->obstacle_proximity, rh->distance_to_goal);
    }
    robot_homeostat_free(rh);
    printf("\n");

    /* 5. Supply Chain */
    printf("--- 5. Supply Chain Homeostat ---\n");
    SupplyChainHomeostat* sc = supply_chain_create(0.3, 5.0);
    for (int i = 0; i < 30; i++) {
        supply_chain_step(sc, 0.5);
        if (i % 10 == 0) ashby_system_print_status(sc->system);
    }
    supply_chain_free(sc);
    printf("\n");

    printf("All demonstrations complete.\n");
}