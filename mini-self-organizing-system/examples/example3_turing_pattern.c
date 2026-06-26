#include "sos_turing.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    srand((unsigned)time(NULL));
    printf("=== Turing Pattern Formation — Self-Organizing Spatial Patterns ===\n\n");
    printf("Alan Turing (1952) showed that coupled reaction-diffusion systems\n");
    printf("can spontaneously produce spatial patterns from a homogeneous state.\n");
    printf("This 'diffusion-driven instability' explains animal coat patterns,\n");
    printf("shell pigmentation, and chemical morphogenesis.\n\n");

    int nx = 40, ny = 40;
    double Lx = 10.0, Ly = 10.0;

    TuringSystem* ts = turing_create(TURING_SCHNAKENBERG, nx, ny, Lx, Ly);

    /* Schnakenberg model parameters: a=0.3, b=0.5 */
    double params[2] = {0.3, 0.5};
    turing_set_params(ts, params, 2);

    /* Key: inhibitor must diffuse faster than activator */
    turing_set_diffusion(ts, 0.1, 20.0);

    printf("Schnakenberg model: a=%.1f, b=%.1f\n", params[0], params[1]);
    printf("Diffusion: Du=%.1f, Dv=%.1f (Dv/Du = %.0f)\n",
           ts->Du, ts->Dv, ts->Dv/ts->Du);

    /* Check Turing instability condition */
    double u_star, v_star;
    turing_homogeneous_steady_state(ts, &u_star, &v_star);
    printf("Homogeneous steady state: u*=%.3f, v*=%.3f\n", u_star, v_star);

    int is_turing = turing_check_instability(ts, u_star, v_star);
    printf("Turing instability condition: %s\n",
           is_turing ? "UNSTABLE -> Pattern forms!" : "STABLE -> No pattern");

    double k_dom = turing_dominant_wavenumber(ts, u_star, v_star);
    double dom_wavelength = (k_dom > 1e-15) ? 2.0 * 3.14159265358979323846 / k_dom : 0.0;
    printf("Dominant wavenumber: %.3f, wavelength: %.3f\n",
           k_dom, dom_wavelength);

    /* Initialize with random perturbation */
    turing_random_init(ts, u_star, v_star, 0.01);

    printf("\nEvolving for 5000 timesteps...\n");
    turing_evolve(ts, 5000);

    int patterned = turing_pattern_formed(ts, 0.005);
    double wavelength = turing_pattern_wavelength(ts);
    int ptype = turing_classify_pattern(ts);
    const char* type_names[] = {"none", "spots", "stripes", "labyrinth", "traveling_wave"};

    printf("\nPattern analysis:\n");
    printf("  Pattern formed: %s\n", patterned ? "YES" : "NO");
    printf("  Wavelength: %.3f\n", wavelength);
    printf("  Pattern type: %s\n", type_names[ptype]);

    /* Print a cross-section of the activator field */
    printf("\nActivator field cross-section (row %d):\n", ny/2);
    printf("  ");
    for (int x = 0; x < nx; x++) {
        double val = ts->u[(ny/2)*nx + x];
        char c = (val > u_star + 0.1) ? '#' : (val < u_star - 0.1) ? '.' : '-';
        printf("%c", c);
    }
    printf("\n  # = high activator (peak), . = low activator (valley)\n");

    printf("\nKey insight: Diffusion, usually a homogenizing force, can\n");
    printf("paradoxically create structure when coupled with nonlinear reactions.\n");
    printf("This is a universal mechanism of biological self-organization.\n");

    turing_free(ts);
    return 0;
}
