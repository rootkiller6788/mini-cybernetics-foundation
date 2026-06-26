#include "sos_core.h"
#include "sos_cellular.h"
#include "sos_turing.h"
#include "sos_autocatalytic.h"
#include "sos_criticality.h"
#include "sos_synergetics.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    srand((unsigned)time(NULL));
    printf("============================================\n");
    printf("  Self-Organizing Systems — Demo Overview\n");
    printf("============================================\n\n");

    /* 1. System Creation and Entropy */
    printf("--- 1. Core System and Entropy ---\n");
    SOSystem* sys = sos_system_create("DemoSystem", 100, 3);
    sos_add_state(sys, "order", 0.5);
    sos_add_state(sys, "temperature", 1.0);
    sos_add_state(sys, "coupling", 2.0);
    sos_system_set_regime(sys, SOS_REGIME_FAR_EQUILIBRIUM);
    sos_system_set_level(sys, SOS_SELF_ORGANIZING);
    sos_add_order_param(sys, "magnetization", 0.2, 0.1, -0.1, 0.3);

    double probs[4] = {0.1, 0.2, 0.3, 0.4};
    double H = sos_shannon_entropy(probs, 4);
    double C = sos_lmc_complexity(probs, 4);
    double E = sos_detect_emergence(1.5, 3.0);
    printf("  Shannon entropy: %.3f bits\n", H);
    printf("  LMC complexity: %.3f\n", C);
    printf("  Emergence strength: %.3f\n", E);
    sos_print_system(sys, stdout);
    sos_system_free(sys);
    printf("\n");

    /* 2. Cellular Automaton */
    printf("--- 2. Conway's Game of Life ---\n");
    CellularAutomaton* ca = ca_create(20, 10, "B3/S23");
    ca_randomize(ca, 0.35);
    printf("  Initial density: %.3f\n", ca_density(ca));
    for (int i = 0; i < 5; i++) ca_evolve(ca);
    printf("  After 5 generations: density=%.3f, live=%d\n",
           ca_density(ca), ca_live_count(ca));
    printf("  Langton lambda: %.3f\n", ca_lambda_parameter(ca));
    ca_free(ca);
    printf("\n");

    /* 3. Turing Pattern */
    printf("--- 3. Turing Pattern Formation ---\n");
    TuringSystem* ts = turing_create(TURING_SCHNAKENBERG, 20, 20, 5.0, 5.0);
    double tp[2] = {0.3, 0.5};
    turing_set_params(ts, tp, 2);
    turing_set_diffusion(ts, 0.1, 15.0);
    double us, vs;
    turing_homogeneous_steady_state(ts, &us, &vs);
    printf("  Steady state: u*=%.3f, v*=%.3f\n", us, vs);
    int ti = turing_check_instability(ts, us, vs);
    printf("  Turing unstable: %s\n", ti ? "YES" : "NO");
    turing_free(ts);
    printf("\n");

    /* 4. Autocatalytic Set */
    printf("--- 4. Autocatalytic Set ---\n");
    CatalyticNetwork* cn = cn_create();
    cn_add_molecule(cn, "Food", MOLECULE_FOOD);
    cn_add_molecule(cn, "Cat", MOLECULE_CATALYST);
    cn_add_molecule(cn, "Prod", MOLECULE_PRODUCT);
    cn_add_reaction(cn, 0, -1, 2, 1, 1.0, 0.001);
    int fs[1] = {0}; cn_set_food(cn, fs, 1);
    int raf = cn_find_raf(cn);
    printf("  RAF size: %d\n", raf);
    printf("  Autocatalytic: %s\n", cn_is_autocatalytic(cn) ? "YES" : "NO");
    cn_free(cn);
    printf("\n");

    /* 5. Bak-Sneppen Model */
    printf("--- 5. Bak-Sneppen Evolution Model ---\n");
    BakSneppenModel* bs = bs_create(32);
    bs_randomize(bs);
    bs_run(bs, 2000);
    printf("  Fitness threshold: %.4f (expected ~0.667)\n", bs_fitness_threshold(bs));
    printf("  Is critical: %s\n", bs_is_critical(bs) ? "YES" : "NO");
    bs_free(bs);
    printf("\n");

    /* 6. Laser Self-Organization */
    printf("--- 6. Haken's Laser — Order Parameter Emergence ---\n");
    LaserSystem* laser = laser_create(1.5, 1.0, 0.3, 2.5);
    printf("  Pump parameter: %.2f (>1 = lasing)\n", laser->threshold);
    for (int i = 0; i < 500; i++) laser_step(laser, 0.05);
    printf("  Photon number n: %.4f\n", laser->n);
    printf("  Is lasing: %s\n", laser_is_lasing(laser) ? "YES" : "NO");
    laser_free(laser);
    printf("\n");

    printf("============================================\n");
    printf("  Demo complete. Key principles illustrated:\n");
    printf("  1. Entropy/complexity measure organization\n");
    printf("  2. Local rules -> global patterns (CA)\n");
    printf("  3. Diffusion paradoxically creates structure\n");
    printf("  4. Catalytic closure -> self-sustaining sets\n");
    printf("  5. SOC: criticality without tuning\n");
    printf("  6. Order parameters enslave micro-dynamics\n");
    printf("============================================\n");
    return 0;
}
