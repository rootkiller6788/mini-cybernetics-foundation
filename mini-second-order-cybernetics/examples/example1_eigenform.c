/* ============================================================================
 * Example 1: von Foerster's Eigenform Computation
 *
 * Demonstrates the computation of eigenforms — stable self-referential
 * patterns — using fixed-point iteration. This is the core mathematical
 * operation of second-order cybernetics.
 *
 * The eigenform equation: x = Phi(x)
 *
 * We demonstrate three cases:
 * 1. Identity: Phi(x) = x — any x is an eigenform
 * 2. Contraction: Phi(x) = 0.5*x — zero is the unique eigenform
 * 3. Nonlinear: Phi(x) = 0.5*(x + 1/x) — Babylonian sqrt, eigenform at x = 1
 * ============================================================================ */

#include <stdio.h>
#include <math.h>
#include "../include/soc_core.h"

/* --- Phi operators for eigenform computation --- */

/* Phi_identity: Phi(x) = x. Every point is an eigenform. */
static void phi_identity(const double* x, int dim, double* result, void* ctx) {
    (void)ctx;
    for (int i = 0; i < dim; i++) result[i] = x[i];
}

/* Phi_contraction: Phi(x) = a * x. Eigenform at x = 0. */
static void phi_contraction(const double* x, int dim, double* result, void* ctx) {
    double a = ctx ? *(double*)ctx : 0.5;
    for (int i = 0; i < dim; i++) result[i] = a * x[i];
}

/* Phi_heron: Phi(x) = 0.5*(x + S/x). Babylonian method for sqrt(S).
 * Eigenform at x = sqrt(S). This is a nonlinear fixed point. */
static void phi_heron(const double* x, int dim, double* result, void* ctx) {
    double S = ctx ? *(double*)ctx : 2.0;
    for (int i = 0; i < dim; i++) {
        if (fabs(x[i]) < 1e-15) {
            result[i] = S; /* handle x=0 gracefully */
        } else {
            result[i] = 0.5 * (x[i] + S / x[i]);
        }
    }
}

int main(void) {
    printf("============================================\n");
    printf("  Example 1: Eigenform Computation\n");
    printf("  von Foerster: x = Phi(x)\n");
    printf("============================================\n\n");

    /* --- Case 1: Identity Phi --- */
    printf("--- Case 1: Phi(x) = x (Identity) ---\n");
    SOCEigenform* ef1 = soc_eigenform_create(3, phi_identity, NULL);
    double init1[] = {1.0, 2.0, 3.0};
    if (soc_eigenform_solve(ef1, init1)) {
        printf("  Converged in %d iterations\n", ef1->iterations_used);
        printf("  Solution: [%.4f, %.4f, %.4f]\n",
               ef1->solution->data[0], ef1->solution->data[1],
               ef1->solution->data[2]);
        printf("  Residual: %.2e\n", ef1->residual_norm);
        SOCEigenStability stab = soc_eigenform_classify(ef1, 1e-4);
        printf("  Stability: %d (0=stable, 1=unstable, ...)\n", stab);
        printf("  Interpretation: Identity eigenform = self-identity is stable.\n");
    } else {
        printf("  Failed to converge.\n");
    }
    soc_eigenform_print(ef1);
    soc_eigenform_free(ef1);

    printf("\n--- Case 2: Phi(x) = 0.5*x (Contraction) ---\n");
    SOCEigenform* ef2 = soc_eigenform_create(1, phi_contraction, NULL);
    double init2[] = {100.0};
    if (soc_eigenform_solve(ef2, init2)) {
        printf("  Converged in %d iterations (from x0=100.0)\n",
               ef2->iterations_used);
        printf("  Solution: %.6f\n", ef2->solution->data[0]);
        printf("  Expected: 0.0 (the unique eigenform)\n");
        printf("  Interpretation: All observations decay to zero = "
               "the observer who sees nothing.\n");
    }
    soc_eigenform_free(ef2);

    printf("\n--- Case 3: Phi(x) = 0.5*(x + 2/x) (Heron / Babylonian sqrt) ---\n");
    double S = 2.0;
    SOCEigenform* ef3 = soc_eigenform_create(1, phi_heron, &S);
    double init3[] = {10.0};
    if (soc_eigenform_solve(ef3, init3)) {
        printf("  Converged in %d iterations\n", ef3->iterations_used);
        printf("  Solution: %.10f\n", ef3->solution->data[0]);
        printf("  sqrt(2) =  %.10f (expected)\n", sqrt(2.0));
        printf("  Error:    %.2e\n",
               fabs(ef3->solution->data[0] - sqrt(2.0)));
        printf("  Interpretation: The eigenform captures sqrt(2) — "
               "knowledge as fixed point.\n");
    }
    SOCEigenStability stab3 = soc_eigenform_classify(ef3, 1e-4);
    if (stab3 == SOC_EIGEN_STABLE) {
        printf("  Eigenform is STABLE — the observer settles on sqrt(2).\n");
    }
    soc_eigenform_free(ef3);

    /* --- Multi-solution case --- */
    printf("\n--- Case 4: Multiple Starting Points ---\n");
    SOCEigenform* ef4 = soc_eigenform_create(1, phi_identity, NULL);
    double starts[] = {1.0, 5.0, -3.0, 42.0, 0.0, 100.0};
    soc_eigenform_find_all(ef4, 6, starts);
    printf("  Found %d distinct eigenforms (expected: 6 for identity Phi)\n",
           ef4->n_solutions);
    for (int i = 0; i < ef4->n_solutions; i++) {
        printf("  Solution %d: %.4f\n", i, ef4->all_solutions[i]->data[0]);
    }
    soc_eigenform_free(ef4);

    printf("\n=== Key Insight ===\n");
    printf("von Foerster: \"Eigenvalues represent the externally observable\n");
    printf("behavior of a recursively operating system, while eigenvectors\n");
    printf("represent the internal organization that generates that behavior.\"\n");
    printf("\nThe eigenform equation x = Phi(x) is the foundation of:\n");
    printf("  - Self-reference (the self observing itself)\n");
    printf("  - Cognition (perception as eigenform computation)\n");
    printf("  - Autonomy (operational closure = eigenform)\n");

    return 0;
}

