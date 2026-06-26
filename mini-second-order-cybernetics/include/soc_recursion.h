#ifndef SOC_RECURSION_H
#define SOC_RECURSION_H

#include "soc_core.h"

/* ============================================================================
 * Second-Order Cybernetics — Recursive Computation Theory
 *
 * Self-reference and recursion are at the heart of second-order cybernetics.
 * This header covers:
 *   - Fixed-point computation (Banach, Knaster-Tarski, Kleene)
 *   - Eigenform algorithms
 *   - Circular closure dynamics
 *   - Self-referential paradox resolution
 *   - Recursive operator theory
 *
 * Key references:
 *   von Foerster, H. (2003). Understanding Understanding.
 *   Kauffman, L.H. (1987). Self-reference and recursive forms.
 *   Varela, F.J. (1975). A calculus for self-reference.
 * ============================================================================ */

/* ---- Fixed-Point Theorems ---- */

typedef enum {
    SOC_FP_BANACH = 0,          /* Contraction mapping */
    SOC_FP_BROUWER = 1,         /* Continuous map on convex compact set */
    SOC_FP_KNaster_TARSKI = 2,  /* Monotone on complete lattice */
    SOC_FP_KLEENE = 3,          /* Continuous on CPO */
    SOC_FP_SCHAUDER = 4         /* Compact map on Banach space */
} SOCFixedPointType;

typedef struct {
    SOCFixedPointType type;
    void (*operator)(const double* x, int dim, double* result, void* ctx);
    void* ctx;
    int dimension;
    double lipschitz_constant;  /* for Banach: must be < 1 */
    bool is_monotone;           /* for Knaster-Tarski */
    bool is_compact;            /* for Schauder */
    SOCVector* fixed_point;
    double residual;
    int iterations;
    bool converged;
} SOCFixedPoint;

/* ---- Recursive Operator Theory ---- */

typedef struct {
    void (*T)(const double* F, int dim, double* T_F, void* ctx);
    void* ctx;
    int dimension;
    double* domain_bounds;      /* lower bounds */
    double* domain_bounds_upper;/* upper bounds */
    bool is_contractive;
    double contraction_factor;
    bool is_monotone;
    bool is_continuous;
    bool preserves_order;
} SOCRecursiveOperator;

typedef struct {
    SOCRecursiveOperator* op;
    double* current_value;
    int dimension;
    double* history;
    int hist_len;
    int hist_cap;
    double convergence_rate;
    bool is_converging;
    bool is_oscillating;
    bool is_chaotic;
    int period;
} SOCRecursiveProcess;

/* ---- Self-Reference Structures ---- */

typedef enum {
    SOC_PARADOX_LIAR = 0,
    SOC_PARADOX_RUSSELL = 1,
    SOC_PARADOX_GRELLING = 2,
    SOC_PARADOX_BERRY = 3,
    SOC_PARADOX_RICHARD = 4,
    SOC_PARADOX_CURRY = 5
} SOCParadoxType;

typedef struct {
    SOCParadoxType type;
    char* statement;
    bool is_paradoxical;
    bool is_resolvable;
    char* resolution_strategy;
    double* truth_value;        /* for multi-valued logic resolution */
    int n_truth_values;
} SOCParadox;

/* ---- Multi-Level Self-Reference ---- */

typedef struct {
    int n_levels;
    double** level_values;      /* values at each level of reference */
    int level_dim;
    bool* is_well_founded;
    int* grounding_depth;       /* depth to ground at each level */
    double* inter_level_map;    /* how level i maps to level j */
} SOCReferenceHierarchy;

/* ---- Varela's Calculus for Self-Reference ---- */

typedef enum {
    SOC_VARELA_AUTONOMY = 0,
    SOC_VARELA_CLOSURE = 1,
    SOC_VARELA_SELF_REFERENCE = 2,
    SOC_VARELA_COMPLEMENTARITY = 3
} SOCVarelaOperator;

typedef struct {
    SOCVarelaOperator op_type;
    void (*star_operator)(const double* x, int dim, double* result, void* ctx);
    void* ctx;
    int dimension;
    bool has_fixed_point;
    SOCVector* fixed_point;
    bool is_productive;         /* does self-reference generate novelty? */
} SOCVarelaForm;

/* ---- API: Fixed-Point Computation ---- */

SOCFixedPoint* soc_fp_create(SOCFixedPointType type, int dimension,
                              void (*op)(const double*, int, double*, void*),
                              void* ctx);
void soc_fp_free(SOCFixedPoint* fp);
bool soc_fp_solve(SOCFixedPoint* fp, const double* initial, double tol,
                   int max_iters);
bool soc_fp_verify(const SOCFixedPoint* fp, double tol);
void soc_fp_print(const SOCFixedPoint* fp);

/* ---- API: Recursive Operators ---- */

SOCRecursiveOperator* soc_recursive_op_create(int dimension,
                                               void (*T)(const double*, int, double*, void*),
                                               void* ctx);
void soc_recursive_op_free(SOCRecursiveOperator* op);
double soc_recursive_op_lipschitz(SOCRecursiveOperator* op, double eps);
bool soc_recursive_op_is_contractive(SOCRecursiveOperator* op, double* factor);
void soc_recursive_op_compose(SOCRecursiveOperator* result,
                               SOCRecursiveOperator* a, SOCRecursiveOperator* b);
void soc_recursive_op_iterate(SOCRecursiveOperator* op, int n,
                               const double* x0, double* result);

SOCRecursiveProcess* soc_recursive_process_create(SOCRecursiveOperator* op,
                                                    const double* x0);
void soc_recursive_process_free(SOCRecursiveProcess* rp);
void soc_recursive_process_step(SOCRecursiveProcess* rp);
void soc_recursive_process_analyze(SOCRecursiveProcess* rp);
void soc_recursive_process_print(const SOCRecursiveProcess* rp);

/* ---- API: Self-Reference Analysis ---- */

SOCParadox* soc_paradox_create(SOCParadoxType type, const char* statement);
void soc_paradox_free(SOCParadox* p);
bool soc_paradox_detect(const char* statement);
bool soc_paradox_resolve(SOCParadox* p);
void soc_paradox_print(const SOCParadox* p);

SOCReferenceHierarchy* soc_refhier_create(int n_levels, int level_dim);
void soc_refhier_free(SOCReferenceHierarchy* rh);
void soc_refhier_set_level(SOCReferenceHierarchy* rh, int level,
                            const double* values);
bool soc_refhier_is_grounded(const SOCReferenceHierarchy* rh);
void soc_refhier_print(const SOCReferenceHierarchy* rh);

/* ---- API: Varela Calculus ---- */

SOCVarelaForm* soc_varela_create(SOCVarelaOperator type, int dimension,
                                  void (*star)(const double*, int, double*, void*),
                                  void* ctx);
void soc_varela_free(SOCVarelaForm* vf);
bool soc_varela_compute_closure(SOCVarelaForm* vf);
bool soc_varela_is_autonomous(const SOCVarelaForm* vf);
void soc_varela_print(const SOCVarelaForm* vf);

/* ---- Special Recursive Forms ---- */

/* The "I am" operator: I = observe(I) — the eigenform of self-consciousness */
bool soc_eigenform_self_consciousness(double* result, int dim, double kappa,
                                       int max_iters);

/* Spencer Brown's re-entry: the form that re-enters its own indicational space */
bool soc_spencer_brown_reentry(int depth, bool* result);

/* Hofstadter's strange loop: tangled hierarchy detection */
bool soc_detect_strange_loop(const double* graph, int n, double threshold);

#endif /* SOC_RECURSION_H */
