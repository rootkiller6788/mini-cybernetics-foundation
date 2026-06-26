#include "soc_core.h"
#include <stdio.h>
static void cl_id(const double* x, int dim, double* r, void* ctx) {
    (void)ctx;
    for (int i=0;i<dim;i++) r[i]=x[i];
}
int main() {
    printf("Closure test...\n"); fflush(stdout);
    SOCCircularClosure* cl = soc_closure_create(2, cl_id, NULL);
    if(!cl) { printf("NULL\n"); return 1; }
    printf("Created, calling compute_fixed_point...\n"); fflush(stdout);
    double init[] = {1,1};
    bool ok = soc_closure_compute_fixed_point(cl, init, 100);
    printf("Result: %d, degree: %.4f\n", ok, cl->closure_degree);
    fflush(stdout);
    soc_closure_free(cl);
    printf("Done\n");
    return 0;
}
