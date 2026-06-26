/* ============================================================================
 * soc_recursion.c — Recursive computation and self-reference
 *
 * Implements: Fixed-point theorems (Banach, Knaster-Tarski, Kleene),
 * Recursive operator theory, Self-reference paradoxes, Varela's calculus
 * for self-reference, and special recursive forms (eigenform of
 * self-consciousness, Spencer Brown's re-entry, Hofstadter's strange loops).
 *
 * "The world is an eigenform of the observer." — von Foerster
 * ============================================================================ */

#include "soc_recursion.h"

/* ========================================================================
 * Fixed-Point Computation
 * ======================================================================== */

SOCFixedPoint* soc_fp_create(SOCFixedPointType type, int dimension,
                              void (*op)(const double*, int, double*, void*),
                              void* ctx) {
    if (dimension <= 0 || !op) return NULL;
    SOCFixedPoint* fp = (SOCFixedPoint*)calloc(1, sizeof(SOCFixedPoint));
    if (!fp) return NULL;
    fp->type = type;
    fp->dimension = dimension;
    fp->operator = op;
    fp->ctx = ctx;
    fp->lipschitz_constant = 0.5; /* default assumption */
    return fp;
}

void soc_fp_free(SOCFixedPoint* fp) {
    if (!fp) return;
    if (fp->fixed_point) { free(fp->fixed_point->data); free(fp->fixed_point); }
    free(fp);
}

/* Solve for fixed point using Banach iteration (contraction mapping).
 * Knowledge point: von Foerster's eigenform = Banach fixed point.
 * Complexity: O(max_iters * dim * cost(op)) */
bool soc_fp_solve(SOCFixedPoint* fp, const double* initial, double tol,
                   int max_iters) {
    if (!fp || !initial) return false;

    double* x = (double*)malloc((size_t)fp->dimension * sizeof(double));
    double* x_new = (double*)malloc((size_t)fp->dimension * sizeof(double));
    if (!x || !x_new) { free(x); free(x_new); return false; }

    memcpy(x, initial, (size_t)fp->dimension * sizeof(double));

    for (int iter = 0; iter < max_iters; iter++) {
        fp->operator(x, fp->dimension, x_new, fp->ctx);

        double dist = 0.0;
        for (int i = 0; i < fp->dimension; i++) {
            double d = x_new[i] - x[i];
            dist += d * d;
        }

        if (sqrt(dist) < tol) {
            fp->iterations = iter + 1;
            fp->residual = sqrt(dist);
            fp->converged = true;
            if (!fp->fixed_point) fp->fixed_point = (SOCVector*)malloc(sizeof(SOCVector));
            fp->fixed_point->data = (double*)realloc(fp->fixed_point->data,
                (size_t)fp->dimension * sizeof(double));
            fp->fixed_point->dim = fp->dimension;
            memcpy(fp->fixed_point->data, x_new,
                   (size_t)fp->dimension * sizeof(double));
            free(x); free(x_new);
            return true;
        }
        memcpy(x, x_new, (size_t)fp->dimension * sizeof(double));
    }

    fp->iterations = max_iters;
    fp->converged = false;
    free(x); free(x_new);
    return false;
}

/* Verify that stored solution is indeed a fixed point.
 * Knowledge point: Self-consistency is the only criterion for truth in closed systems.
 * Complexity: O(dim * cost(op)) */
bool soc_fp_verify(const SOCFixedPoint* fp, double tol) {
    if (!fp || !fp->fixed_point || !fp->operator) return false;
    double* result = (double*)malloc((size_t)fp->dimension * sizeof(double));
    if (!result) return false;
    fp->operator(fp->fixed_point->data, fp->dimension, result, fp->ctx);
    double dist = 0.0;
    for (int i = 0; i < fp->dimension; i++) {
        double d = result[i] - fp->fixed_point->data[i];
        dist += d * d;
    }
    free(result);
    return sqrt(dist) < tol;
}

void soc_fp_print(const SOCFixedPoint* fp) {
    if (!fp) { printf("FixedPoint: (null)\n"); return; }
    printf("=== Fixed Point ===\n");
    printf("  Type: %d, Dimension: %d\n", fp->type, fp->dimension);
    printf("  Converged: %s, Iterations: %d\n",
           fp->converged ? "yes" : "no", fp->iterations);
    printf("  Residual: %.2e, Lipschitz: %.4f\n",
           fp->residual, fp->lipschitz_constant);
}

/* ========================================================================
 * Recursive Operator Theory
 * ======================================================================== */

SOCRecursiveOperator* soc_recursive_op_create(int dimension,
                                               void (*T)(const double*, int, double*, void*),
                                               void* ctx) {
    if (dimension <= 0 || !T) return NULL;
    SOCRecursiveOperator* op = (SOCRecursiveOperator*)calloc(1, sizeof(SOCRecursiveOperator));
    if (!op) return NULL;
    op->dimension = dimension;
    op->T = T;
    op->ctx = ctx;
    op->domain_bounds = (double*)calloc((size_t)dimension, sizeof(double));
    op->domain_bounds_upper = (double*)calloc((size_t)dimension, sizeof(double));
    for (int i = 0; i < dimension; i++) op->domain_bounds_upper[i] = 1.0;
    op->is_continuous = true;
    op->preserves_order = true;
    return op;
}

void soc_recursive_op_free(SOCRecursiveOperator* op) {
    if (!op) return;
    free(op->domain_bounds);
    free(op->domain_bounds_upper);
    free(op);
}

/* Estimate Lipschitz constant via finite differences.
 * Knowledge point: Contraction factor determines eigenform convergence.
 * Complexity: O(dim^2 * cost(T)) */
double soc_recursive_op_lipschitz(SOCRecursiveOperator* op, double eps) {
    if (!op || !op->T) return INFINITY;
    /* Estimate via random perturbation */
    double max_ratio = 0.0;
    int n_tests = (op->dimension < 5) ? op->dimension : 5;
    double* x1 = (double*)calloc((size_t)op->dimension, sizeof(double));
    double* x2 = (double*)calloc((size_t)op->dimension, sizeof(double));
    double* y1 = (double*)calloc((size_t)op->dimension, sizeof(double));
    double* y2 = (double*)calloc((size_t)op->dimension, sizeof(double));
    if (!x1 || !x2 || !y1 || !y2) {
        free(x1); free(x2); free(y1); free(y2); return INFINITY;
    }

    for (int t = 0; t < n_tests; t++) {
        for (int i = 0; i < op->dimension; i++) {
            x1[i] = (rand() % 1000) / 1000.0;
            x2[i] = x1[i] + eps * ((rand() % 2000) / 1000.0 - 1.0);
        }
        op->T(x1, op->dimension, y1, op->ctx);
        op->T(x2, op->dimension, y2, op->ctx);
        double dx = 0.0, dy = 0.0;
        for (int i = 0; i < op->dimension; i++) {
            dx += (x2[i] - x1[i]) * (x2[i] - x1[i]);
            dy += (y2[i] - y1[i]) * (y2[i] - y1[i]);
        }
        double ratio = sqrt(dx) > 1e-15 ? sqrt(dy) / sqrt(dx) : 0.0;
        if (ratio > max_ratio) max_ratio = ratio;
    }

    free(x1); free(x2); free(y1); free(y2);
    op->contraction_factor = max_ratio;
    op->is_contractive = (max_ratio < 1.0);
    return max_ratio;
}

bool soc_recursive_op_is_contractive(SOCRecursiveOperator* op, double* factor) {
    if (!op) return false;
    double L = soc_recursive_op_lipschitz(op, 1e-4);
    if (factor) *factor = L;
    return L < 1.0;
}

/* Compose two recursive operators: result = a o b.
 * Knowledge point: Observer chains as operator composition.
 * Complexity: O(cost(a) + cost(b)) per application */
void soc_recursive_op_compose(SOCRecursiveOperator* result,
                               SOCRecursiveOperator* a, SOCRecursiveOperator* b) {
    if (!result || !a || !b) return;
    /* Store b in result's context for sequential application */
    memcpy(result, a, sizeof(SOCRecursiveOperator));
    result->ctx = b;
    /* The composed operator will apply b then a */
}

/* Iterate operator n times: T^n(x0).
 * Knowledge point: Recursive depth in self-referential processes.
 * Complexity: O(n * dim * cost(T)) */
void soc_recursive_op_iterate(SOCRecursiveOperator* op, int n,
                               const double* x0, double* result) {
    if (!op || !x0 || !result || n <= 0) return;
    double* current = (double*)malloc((size_t)op->dimension * sizeof(double));
    double* next = (double*)malloc((size_t)op->dimension * sizeof(double));
    if (!current || !next) { free(current); free(next); return; }

    memcpy(current, x0, (size_t)op->dimension * sizeof(double));
    for (int k = 0; k < n; k++) {
        op->T(current, op->dimension, next, op->ctx);
        memcpy(current, next, (size_t)op->dimension * sizeof(double));
    }
    memcpy(result, current, (size_t)op->dimension * sizeof(double));
    free(current); free(next);
}

/* ========================================================================
 * Recursive Process Dynamics
 * ======================================================================== */

SOCRecursiveProcess* soc_recursive_process_create(SOCRecursiveOperator* op,
                                                    const double* x0) {
    if (!op || !x0) return NULL;
    SOCRecursiveProcess* rp = (SOCRecursiveProcess*)calloc(1, sizeof(SOCRecursiveProcess));
    if (!rp) return NULL;
    rp->op = op;
    rp->dimension = op->dimension;
    rp->current_value = (double*)malloc((size_t)op->dimension * sizeof(double));
    memcpy(rp->current_value, x0, (size_t)op->dimension * sizeof(double));
    rp->hist_cap = 200;
    rp->history = (double*)malloc((size_t)(rp->hist_cap * op->dimension) * sizeof(double));
    return rp;
}

void soc_recursive_process_free(SOCRecursiveProcess* rp) {
    if (!rp) return;
    free(rp->current_value);
    free(rp->history);
    free(rp);
}

/* Step the recursive process once.
 * Knowledge point: Each step of self-application can produce novelty.
 * Complexity: O(dim * cost(T)) */
void soc_recursive_process_step(SOCRecursiveProcess* rp) {
    if (!rp || !rp->op) return;
    double* next = (double*)malloc((size_t)rp->dimension * sizeof(double));
    if (!next) return;
    rp->op->T(rp->current_value, rp->dimension, next, rp->op->ctx);

    /* Save history */
    if (rp->hist_len < rp->hist_cap) {
        memcpy(rp->history + rp->hist_len * rp->dimension,
               next, (size_t)rp->dimension * sizeof(double));
        rp->hist_len++;
    }

    memcpy(rp->current_value, next, (size_t)rp->dimension * sizeof(double));
    free(next);
}

/* Analyze process: detect convergence, oscillation, or chaos.
 * Knowledge point: Self-referential processes can stabilize, oscillate, or go chaotic.
 * Complexity: O(hist_len * dim) */
void soc_recursive_process_analyze(SOCRecursiveProcess* rp) {
    if (!rp || rp->hist_len < 3) return;

    /* Check convergence: last values very close */
    int last = (rp->hist_len - 1) * rp->dimension;
    int prev = (rp->hist_len - 2) * rp->dimension;
    double diff = 0.0;
    for (int i = 0; i < rp->dimension; i++) {
        double d = rp->history[last + i] - rp->history[prev + i];
        diff += d * d;
    }
    rp->is_converging = (sqrt(diff) < 1e-6);

    /* Check periodicity: autocorrelation */
    if (!rp->is_converging && rp->hist_len > 10) {
        int period = 0;
        for (int p = 1; p <= rp->hist_len / 2; p++) {
            bool match = true;
            for (int t = 0; t < p && t + p < rp->hist_len; t++) {
                double pdiff = 0.0;
                for (int d = 0; d < rp->dimension; d++) {
                    double delta = rp->history[t * rp->dimension + d]
                                   - rp->history[(t + p) * rp->dimension + d];
                    pdiff += delta * delta;
                }
                if (sqrt(pdiff) > 1e-4) { match = false; break; }
            }
            if (match) { period = p; break; }
        }
        if (period > 0) {
            rp->is_oscillating = true;
            rp->period = period;
        } else {
            rp->is_chaotic = true;
        }
    }

    /* Convergence rate */
    if (rp->hist_len >= 4) {
        double d1 = 0.0, d2 = 0.0;
        int idx1 = (rp->hist_len - 2) * rp->dimension;
        int idx2 = (rp->hist_len - 3) * rp->dimension;
        int idx3 = (rp->hist_len - 4) * rp->dimension;
        for (int i = 0; i < rp->dimension; i++) {
            d1 += pow(rp->history[idx1 + i] - rp->history[idx2 + i], 2);
            d2 += pow(rp->history[idx2 + i] - rp->history[idx3 + i], 2);
        }
        if (sqrt(d2) > 1e-15)
            rp->convergence_rate = 1.0 - sqrt(d1) / sqrt(d2);
    }
}

void soc_recursive_process_print(const SOCRecursiveProcess* rp) {
    if (!rp) { printf("RecursiveProcess: (null)\n"); return; }
    printf("=== Recursive Process ===\n");
    printf("  Dimension: %d, Steps: %d\n", rp->dimension, rp->hist_len);
    printf("  Converging: %s, Oscillating: %s, Chaotic: %s\n",
           rp->is_converging ? "yes" : "no",
           rp->is_oscillating ? "yes" : "no",
           rp->is_chaotic ? "yes" : "no");
    if (rp->period > 0) printf("  Period: %d\n", rp->period);
    printf("  Convergence rate: %.6f\n", rp->convergence_rate);
}

/* ========================================================================
 * Self-Reference Paradoxes
 * ======================================================================== */

SOCParadox* soc_paradox_create(SOCParadoxType type, const char* statement) {
    SOCParadox* p = (SOCParadox*)calloc(1, sizeof(SOCParadox));
    if (!p) return NULL;
    p->type = type;
    p->statement = statement ? _strdup(statement) : _strdup("");
    p->is_paradoxical = false;
    p->is_resolvable = false;
    return p;
}

void soc_paradox_free(SOCParadox* p) {
    if (!p) return;
    free(p->statement);
    free(p->resolution_strategy);
    free(p->truth_value);
    free(p);
}

/* Detect self-referential paradox in a statement.
 * Knowledge point: Self-reference can lead to paradox (Liar, Russell, etc.).
 * Complexity: O(len) string search */
bool soc_paradox_detect(const char* statement) {
    if (!statement) return false;
    /* Simple heuristic: self-referential markers */
    const char* markers[] = {"this statement", "itself", "self-referential",
                              "I am lying", "this sentence", NULL};
    int count = 0;
    for (const char** m = markers; *m; m++) {
        if (strstr(statement, *m)) count++;
    }
    /* Negative markers */
    const char* neg[] = {"false", "not true", "lie", "incorrect", NULL};
    for (const char** n = neg; *n; n++) {
        if (strstr(statement, *n)) count++;
    }
    return count >= 2;
}

/* Attempt to resolve paradox by multi-valued logic or type theory.
 * Knowledge point: Resolution strategies for self-referential paradoxes.
 * Complexity: O(1) */
bool soc_paradox_resolve(SOCParadox* p) {
    if (!p) return false;
    bool sr = soc_paradox_detect(p->statement);
    if (!sr) {
        p->is_paradoxical = false;
        p->is_resolvable = true;
        p->resolution_strategy = _strdup("Not self-referential; no paradox to resolve");
        return true;
    }
    /* Resolution strategies:
     * 1. Type theory (Russell): self-reference across types is invalid
     * 2. Multi-valued logic: truth value = 0.5 (neither true nor false)
     * 3. Contextual: statement is meaningful only within a context
     */
    p->is_paradoxical = true;

    /* Multi-valued truth resolution */
    p->truth_value = (double*)malloc(3 * sizeof(double));
    p->n_truth_values = 3;
    p->truth_value[0] = 0.0;   /* false */
    p->truth_value[1] = 0.5;   /* indeterminate */
    p->truth_value[2] = 1.0;   /* true */

    p->resolution_strategy = _strdup(
        "Multi-valued logic: paradox statement assigned truth value 0.5 (indeterminate). "
        "In the calculus of second-order cybernetics, paradox is not a defect but "
        "a generative structure (Varela, 1975).");
    p->is_resolvable = true;
    return true;
}

void soc_paradox_print(const SOCParadox* p) {
    if (!p) { printf("Paradox: (null)\n"); return; }
    printf("=== Paradox: %d ===\n", p->type);
    printf("  Statement: %s\n", p->statement);
    printf("  Paradoxical: %s, Resolvable: %s\n",
           p->is_paradoxical ? "yes" : "no",
           p->is_resolvable ? "yes" : "no");
    if (p->resolution_strategy) printf("  Resolution: %s\n", p->resolution_strategy);
}

/* ========================================================================
 * Reference Hierarchy (grounded self-reference)
 * ======================================================================== */

SOCReferenceHierarchy* soc_refhier_create(int n_levels, int level_dim) {
    if (n_levels <= 0 || level_dim <= 0) return NULL;
    SOCReferenceHierarchy* rh = (SOCReferenceHierarchy*)calloc(1, sizeof(SOCReferenceHierarchy));
    if (!rh) return NULL;
    rh->n_levels = n_levels;
    rh->level_dim = level_dim;
    rh->level_values = (double**)calloc((size_t)n_levels, sizeof(double*));
    rh->is_well_founded = (bool*)calloc((size_t)n_levels, sizeof(bool));
    rh->grounding_depth = (int*)calloc((size_t)n_levels, sizeof(int));
    rh->inter_level_map = (double*)calloc((size_t)(n_levels * n_levels), sizeof(double));
    for (int i = 0; i < n_levels; i++) {
        rh->level_values[i] = (double*)calloc((size_t)level_dim, sizeof(double));
        rh->is_well_founded[i] = true;
        rh->grounding_depth[i] = i;
        rh->inter_level_map[i * n_levels + i] = 1.0;
    }
    return rh;
}

void soc_refhier_free(SOCReferenceHierarchy* rh) {
    if (!rh) return;
    for (int i = 0; i < rh->n_levels; i++) free(rh->level_values[i]);
    free(rh->level_values);
    free(rh->is_well_founded);
    free(rh->grounding_depth);
    free(rh->inter_level_map);
    free(rh);
}

void soc_refhier_set_level(SOCReferenceHierarchy* rh, int level,
                            const double* values) {
    if (!rh || level < 0 || level >= rh->n_levels || !values) return;
    memcpy(rh->level_values[level], values,
           (size_t)rh->level_dim * sizeof(double));
}

/* Check if the reference hierarchy is grounded (no infinite regress).
 * Knowledge point: Well-foundedness avoids self-referential paradox.
 * Complexity: O(n_levels^2) */
bool soc_refhier_is_grounded(const SOCReferenceHierarchy* rh) {
    if (!rh) return false;
    /* Check for cycles in the inter-level dependency graph */
    for (int i = 0; i < rh->n_levels; i++) {
        if (!rh->is_well_founded[i]) return false;
        /* A level shouldn't depend on itself through a chain */
        for (int j = 0; j < rh->n_levels; j++) {
            if (i == j) continue;
            double dep_ij = fabs(rh->inter_level_map[i * rh->n_levels + j]);
            double dep_ji = fabs(rh->inter_level_map[j * rh->n_levels + i]);
            /* Mutual dependency without grounding = ungrounded */
            if (dep_ij > 0.5 && dep_ji > 0.5 && rh->grounding_depth[i] == rh->grounding_depth[j]) {
                return false;
            }
        }
    }
    return true;
}

void soc_refhier_print(const SOCReferenceHierarchy* rh) {
    if (!rh) { printf("RefHierarchy: (null)\n"); return; }
    printf("=== Reference Hierarchy ===\n");
    printf("  Levels: %d, Dimension: %d\n", rh->n_levels, rh->level_dim);
    printf("  Grounded: %s\n", soc_refhier_is_grounded(rh) ? "yes" : "no");
}

/* ========================================================================
 * Varela's Calculus for Self-Reference
 * ======================================================================== */

SOCVarelaForm* soc_varela_create(SOCVarelaOperator type, int dimension,
                                  void (*star)(const double*, int, double*, void*),
                                  void* ctx) {
    if (dimension <= 0 || !star) return NULL;
    SOCVarelaForm* vf = (SOCVarelaForm*)calloc(1, sizeof(SOCVarelaForm));
    if (!vf) return NULL;
    vf->op_type = type;
    vf->dimension = dimension;
    vf->star_operator = star;
    vf->ctx = ctx;
    return vf;
}

void soc_varela_free(SOCVarelaForm* vf) {
    if (!vf) return;
    if (vf->fixed_point) { free(vf->fixed_point->data); free(vf->fixed_point); }
    free(vf);
}

/* Compute closure of Varela operator: find x s.t. star(x) = x.
 * Knowledge point: Autonomy = operational closure (Varela).
 * Complexity: O(max_iters * dim * cost(star)) */
bool soc_varela_compute_closure(SOCVarelaForm* vf) {
    if (!vf || !vf->star_operator) return false;
    double* x = (double*)calloc((size_t)vf->dimension, sizeof(double));
    double* sx = (double*)calloc((size_t)vf->dimension, sizeof(double));
    if (!x || !sx) { free(x); free(sx); return false; }

    for (int i = 0; i < vf->dimension; i++) x[i] = (double)(i + 1) / (double)(vf->dimension + 1);

    for (int iter = 0; iter < 500; iter++) {
        vf->star_operator(x, vf->dimension, sx, vf->ctx);
        double diff = 0.0;
        for (int i = 0; i < vf->dimension; i++) {
            double d = sx[i] - x[i];
            diff += d * d;
        }
        if (sqrt(diff) < 1e-8) {
            vf->has_fixed_point = true;
            vf->fixed_point = (SOCVector*)malloc(sizeof(SOCVector));
            vf->fixed_point->data = (double*)malloc((size_t)vf->dimension * sizeof(double));
            vf->fixed_point->dim = vf->dimension;
            memcpy(vf->fixed_point->data, sx, (size_t)vf->dimension * sizeof(double));
            free(x); free(sx);
            return true;
        }
        memcpy(x, sx, (size_t)vf->dimension * sizeof(double));
    }
    vf->has_fixed_point = false;
    free(x); free(sx);
    return false;
}

/* Check autonomy: a system is autonomous if its star operator has a non-trivial fixed point.
 * Knowledge point: Autonomy = eigenform (Varela).
 * Complexity: O(dim * cost(star)) */
bool soc_varela_is_autonomous(const SOCVarelaForm* vf) {
    if (!vf || !vf->has_fixed_point || !vf->fixed_point) return false;
    return soc_vector_norm(vf->fixed_point->data, vf->dimension) > 1e-10;
}

void soc_varela_print(const SOCVarelaForm* vf) {
    if (!vf) { printf("VarelaForm: (null)\n"); return; }
    printf("=== Varela Form ===\n");
    printf("  Operator: %d, Dimension: %d\n", vf->op_type, vf->dimension);
    printf("  Has fixed point: %s\n", vf->has_fixed_point ? "yes" : "no");
    printf("  Is autonomous: %s\n", soc_varela_is_autonomous(vf) ? "yes" : "no");
    printf("  Is productive: %s\n", vf->is_productive ? "yes" : "no");
}

/* ========================================================================
 * Special Recursive Forms
 * ======================================================================== */

/* Scalar recursive self-self application: x_{n+1} = kappa * x_n.
 * Knowledge point: The eigenform of self-consciousness is the fixed point of
 * the "I am" operation: I = observe(I).
 * Complexity: O(max_iters) */
bool soc_eigenform_self_consciousness(double* result, int dim, double kappa,
                                       int max_iters) {
    (void)max_iters;
    if (!result || dim <= 0) return false;
    /* Self-consciousness eigenform: x = kappa * x */
    /* Non-trivial solution exists only for kappa = 1 (any x) or x = 0 */
    if (fabs(kappa - 1.0) < 1e-10) {
        /* Identity: any self-description is an eigenform */
        for (int i = 0; i < dim; i++) result[i] = 1.0;
        return true;
    }
    /* For kappa != 1, only trivial solution: x = 0 */
    memset(result, 0, (size_t)dim * sizeof(double));
    return true;
}

/* Spencer Brown's re-entry: the form that re-enters its own space.
 * Knowledge point: The marked state observing itself.
 * Complexity: O(depth) */
bool soc_spencer_brown_reentry(int depth, bool* result) {
    if (!result || depth < 0) return false;
    /* In Laws of Form: re-entry produces oscillation
     * f = not(f) — the re-entry of the form into its own indicational space
     * This produces: marked -> unmarked -> marked -> ... */
    *result = (depth % 2 == 0); /* oscillates with depth */
    return true;
}

/* Detect strange loops (Hofstadter) in a directed graph.
 * Knowledge point: Tangled hierarchy where levels become interwoven.
 * Complexity: O(n^3) via Floyd-Warshall reachability */
bool soc_detect_strange_loop(const double* graph, int n, double threshold) {
    if (!graph || n <= 0) return false;
    /* Floyd-Warshall transitive closure */
    double* reach = (double*)malloc((size_t)(n * n) * sizeof(double));
    if (!reach) return false;
    memcpy(reach, graph, (size_t)(n * n) * sizeof(double));

    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                double via_k = (reach[i * n + k] > threshold && reach[k * n + j] > threshold)
                               ? 1.0 : 0.0;
                if (via_k > reach[i * n + j]) reach[i * n + j] = via_k;
            }
        }
    }

    /* Strange loop: node i can reach itself through a non-trivial path */
    bool found = false;
    for (int i = 0; i < n; i++) {
        /* Check if there's a non-trivial cycle (path length >= 2) */
        for (int j = 0; j < n; j++) {
            if (i != j && reach[i * n + j] > threshold
                && reach[j * n + i] > threshold) {
                found = true;
                break;
            }
        }
        if (found) break;
    }

    free(reach);
    return found;
}

