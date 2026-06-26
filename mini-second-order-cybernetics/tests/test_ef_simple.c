#include "soc_core.h"
#include <stdio.h>

static void phi_copy(const double* x, int dim, double* r, void* ctx) {
    (void)ctx;
    for (int i = 0; i < dim; i++) r[i] = x[i];
}

int main(void) {
    printf("START\n"); fflush(stdout);
    
    SOCEigenform* ef = soc_eigenform_create(1, phi_copy, NULL);
    if (!ef) { printf("CREATE FAILED\n"); return 1; }
    
    printf("Created ef at %p, phi=%p\n", (void*)ef, (void*)ef->phi);
    fflush(stdout);
    
    double init[] = {42.0};
    
    printf("Calling solve...\n"); fflush(stdout);
    bool ok = soc_eigenform_solve(ef, init);
    printf("Solve returned: %d\n", ok);
    fflush(stdout);
    
    if (ef->converged) {
        printf("CONVERGED: solution=%.6f, residual=%.2e\n",
               ef->solution->data[0], ef->residual_norm);
    }
    
    soc_eigenform_free(ef);
    printf("DONE\n");
    return 0;
}
