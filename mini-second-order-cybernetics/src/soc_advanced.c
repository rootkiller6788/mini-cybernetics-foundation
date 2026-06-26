/* ============================================================================
 * soc_advanced.c — L8 Advanced Topics in Second-Order Cybernetics
 *
 * Covers:
 * 1. Luhmann's Social Systems Theory — autopoietic social systems,
 *    communication as the basic element, functional differentiation.
 *    Society = system of communications, not of people.
 *
 * 2. Spencer Brown's Laws of Form — the calculus of indications,
 *    primary algebra, re-entry. The mathematical foundation of
 *    distinction-based observation.
 *
 * 3. von Foerster's Ethics — the ethical imperative of second-order
 *    cybernetics: "Act always so as to increase the number of choices."
 *
 * 4. Fuzzy Cybernetics — extending distinction to degrees (fuzzy sets),
 *    the bridge between Spencer Brown's binary distinctions and
 *    continuous observation.
 *
 * 5. Game of Life as Second-Order System — Conway's cellular automaton
 *    as an example of a system where the observer must be an NTM
 *    to predict the system's behavior.
 * ============================================================================ */

#include "soc_core.h"
#include "soc_recursion.h"

/* ========================================================================
 * Luhmann's Social Systems — Autopoietic Communication
 *
 * Luhmann (1984/1995): Social systems consist of communications,
 * not persons. Each communication is an operation that connects to
 * previous communications. The system is operationally closed:
 * it produces communications from communications.
 *
 * Key insight: "Only communication can communicate." (Luhmann)
 * This is a second-order move: observe how the system observes,
 * not what it observes.
 * ======================================================================== */

typedef struct {
    double* communication_vector;    /* vector representation of a communication */
    int comm_dim;
    double* system_memory;           /* past communications that form the system */
    int mem_len;
    int mem_cap;
    double selection_pressure;       /* how the system selects what to communicate */
    double autopoiesis_strength;     /* how strongly the system produces itself */
    double* functional_codes;        /* binary codes of functional subsystems */
    int n_codes;                     /* law/illegal, pay/not-pay, truth/false, etc. */
    bool is_autopoietic;
} SOCLuhmannSystem;

SOCLuhmannSystem* soc_luhmann_create(int comm_dim, int mem_cap) {
    if (comm_dim <= 0) return NULL;
    SOCLuhmannSystem* ls = (SOCLuhmannSystem*)calloc(1, sizeof(SOCLuhmannSystem));
    if (!ls) return NULL;
    ls->comm_dim = comm_dim;
    ls->mem_cap = mem_cap > 0 ? mem_cap : 100;
    ls->communication_vector = (double*)calloc((size_t)comm_dim, sizeof(double));
    ls->system_memory = (double*)calloc((size_t)(ls->mem_cap * comm_dim), sizeof(double));
    ls->selection_pressure = 0.5;
    ls->autopoiesis_strength = 0.7;
    /* Default functional codes: economy, law, science, politics */
    ls->n_codes = 4;
    ls->functional_codes = (double*)calloc((size_t)ls->n_codes, sizeof(double));
    for (int i = 0; i < ls->n_codes; i++) ls->functional_codes[i] = 0.5;
    return ls;
}

void soc_luhmann_free(SOCLuhmannSystem* ls) {
    if (!ls) return;
    free(ls->communication_vector);
    free(ls->system_memory);
    free(ls->functional_codes);
    free(ls);
}

/* Process a communication through the social system.
 * Knowledge point: Communication connects to communication (Luhmann).
 * The system selects which communications to continue based on
 * its own history — operational closure of meaning.
 * Complexity: O(comm_dim * mem_len) */
void soc_luhmann_communicate(SOCLuhmannSystem* ls, const double* new_comm) {
    if (!ls || !new_comm) return;
    /* Store communication in system memory */
    if (ls->mem_len < ls->mem_cap) {
        memcpy(ls->system_memory + ls->mem_len * ls->comm_dim,
               new_comm, (size_t)ls->comm_dim * sizeof(double));
        ls->mem_len++;
    }
    /* Update communication vector as resonance of past communications */
    if (ls->mem_len > 0) {
        double* avg = (double*)calloc((size_t)ls->comm_dim, sizeof(double));
        if (avg) {
            for (int m = 0; m < ls->mem_len; m++) {
                for (int d = 0; d < ls->comm_dim; d++) {
                    avg[d] += ls->system_memory[m * ls->comm_dim + d];
                }
            }
            for (int d = 0; d < ls->comm_dim; d++) {
                avg[d] /= (double)ls->mem_len;
                ls->communication_vector[d] =
                    ls->autopoiesis_strength * avg[d]
                    + (1.0 - ls->autopoiesis_strength) * new_comm[d];
            }
            free(avg);
        }
    }
}

/* Check if the social system is autopoietic.
 * Knowledge point: Autopoiesis of communication (Luhmann):
 * the system produces the elements of which it consists.
 * Complexity: O(mem_len * comm_dim) */
bool soc_luhmann_is_autopoietic(const SOCLuhmannSystem* ls) {
    if (!ls || ls->mem_len < 3) return false;
    /* Autopoietic if communication vector is mainly determined by
     * system memory, not by external perturbation */
    double self_determination = ls->autopoiesis_strength;
    /* Check functional differentiation: each code has clear binary distinction */
    double code_clarity = 0.0;
    for (int i = 0; i < ls->n_codes; i++) {
        code_clarity += fabs(ls->functional_codes[i] - 0.5) * 2.0;
    }
    code_clarity /= (double)ls->n_codes;
    return (self_determination > 0.5 && code_clarity > 0.3);
}

void soc_luhmann_print(const SOCLuhmannSystem* ls) {
    if (!ls) { printf("LuhmannSystem: (null)\n"); return; }
    printf("=== Luhmann Social System ===\n");
    printf("  Communications: %d / %d\n", ls->mem_len, ls->mem_cap);
    printf("  Autopoiesis strength: %.4f\n", ls->autopoiesis_strength);
    printf("  Is autopoietic: %s\n", soc_luhmann_is_autopoietic(ls) ? "yes" : "no");
    printf("  Functional codes: ");
    for (int i = 0; i < ls->n_codes; i++) printf("%.2f ", ls->functional_codes[i]);
    printf("\n");
}

/* ========================================================================
 * Spencer Brown — Laws of Form (Calculus of Indications)
 *
 * The primary algebra consists of two primitive equations:
 *   1. Position:  ((p)) = (p)    (calling twice = calling once)
 *   2. Transposition: ((pr)(qr)) = ((p)(q))(r)
 *
 * Re-entry: the form that re-enters its own indicational space.
 * This is the most profound concept in Laws of Form — it describes
 * self-referential forms that generate time and eigenbehavior.
 * ======================================================================== */

typedef struct {
    bool is_marked;               /* marked = true, unmarked = false */
    int depth;                    /* depth of nested distinctions */
    double* form_value;           /* continuous representation */
    int dim;
    bool has_re_entry;
    int re_entry_depth;
} SOCSpencerBrown;

SOCSpencerBrown* soc_spencer_brown_create(int dim) {
    SOCSpencerBrown* sb = (SOCSpencerBrown*)calloc(1, sizeof(SOCSpencerBrown));
    if (!sb) return NULL;
    sb->dim = dim > 0 ? dim : 1;
    sb->form_value = (double*)calloc((size_t)sb->dim, sizeof(double));
    sb->is_marked = false;
    sb->depth = 0;
    return sb;
}

void soc_spencer_brown_free(SOCSpencerBrown* sb) {
    if (!sb) return;
    free(sb->form_value);
    free(sb);
}

/* Apply the primary algebra axiom: Position.
 * ((p)) = (p) — calling the distinction twice is the same as calling it once.
 * Knowledge point: The simplest law of form — redundancy collapses.
 * Complexity: O(1) */
bool soc_spencer_brown_position(bool p) {
    /* ((p)) = (p) in Boolean: not(not(p)) = p */
    return p;
}

/* Apply the primary algebra axiom: Transposition.
 * ((pr)(qr)) = ((p)(q))(r)
 * Knowledge point: The distributive law of the calculus of indications.
 * Complexity: O(1) */
bool soc_spencer_brown_transposition(bool p, bool q, bool r) {
    /* ((pr)(qr)) = ((p)(q))(r)
     * LHS: NOT(NOT(p AND r) AND NOT(q AND r)) = (p AND r) OR (q AND r) = (p OR q) AND r
     * RHS: NOT(NOT(p AND q)) AND r = (p AND q) AND r
     * Wait, let me verify from Laws of Form:
     * Actually in Boolean: LHS = (p∧r)∨(q∧r) = (p∨q)∧r
     * If the axiom holds strictly: check both sides
     */
    bool lhs = !((!(!(p && r))) && (!(!(q && r))));  /* ((pr)(qr)) */
    /* Actually the notation in Laws of Form: ab = conjunction, (a) = negation
     * ((pr)(qr)) = NOT (NOT(p AND r) AND NOT(q AND r)) = (p AND r) OR (q AND r)
     * ((p)(q))r = NOT(NOT p AND NOT q) AND r = (p OR q) AND r
     * These are equivalent in Boolean algebra: (p∨q)∧r = (p∧r)∨(q∧r) */
    bool rhs = (!(!p && !q)) && r;
    return lhs == rhs;
}

/* Simulate re-entry: the form f such that f = (f).
 * Knowledge point: Self-referential form is the generator of time.
 * Complexity: O(max_depth) */
void soc_spencer_brown_simulate_reentry(SOCSpencerBrown* sb, int max_depth) {
    if (!sb) return;
    /* f = (f) means f = NOT(f). The re-entry oscillates:
     * depth 0: f = unmarked (arbitrary start)
     * depth 1: f = (unmarked) = marked
     * depth 2: f = (marked) = unmarked
     * depth 3: f = (unmarked) = marked ...
     */
    bool state = false;
    for (int d = 0; d < max_depth; d++) {
        state = !state;
        if (d < sb->dim) {
            sb->form_value[d] = state ? 1.0 : 0.0;
        }
    }
    sb->is_marked = state;
    sb->re_entry_depth = max_depth;
    sb->has_re_entry = true;
}

void soc_spencer_brown_print(const SOCSpencerBrown* sb) {
    if (!sb) { printf("SpencerBrown: (null)\n"); return; }
    printf("=== Spencer Brown Form ===\n");
    printf("  Dim: %d, State: %s\n", sb->dim, sb->is_marked ? "marked" : "unmarked");
    printf("  Depth: %d, Re-entry: %s\n",
           sb->depth, sb->has_re_entry ? "yes" : "no");
    if (sb->has_re_entry) {
        printf("  Re-entry depth: %d, Final value: %.1f\n",
               sb->re_entry_depth, sb->is_marked ? 1.0 : 0.0);
    }
}

/* ========================================================================
 * von Foerster's Ethical Imperative
 *
 * "Act always so as to increase the number of choices."
 * (von Foerster, 1973)
 *
 * The ethical imperative is a direct consequence of second-order
 * cybernetics: if the observer is part of the system, then the
 * observer's actions shape the system. The ethical choice is to
 * increase possibilities for others, not reduce them.
 *
 * This is opposed to the first-order cybernetic imperative:
 * "Act so as to achieve the goal." The second-order imperative
 * recognizes that goals are observer-dependent.
 * ======================================================================== */

typedef struct {
    double* choice_set;              /* currently available choices */
    int n_choices;
    int max_choices;
    double* ethical_value;           /* how much each choice increases options */
    double imperative_violation;     /* 0 = fully ethical, 1 = fully unethical */
    SOCEigenform* ethical_eigenform; /* ethics as eigenbehavior */
} SOCEthicalImperative;

SOCEthicalImperative* soc_ethics_create(int max_choices) {
    SOCEthicalImperative* ei = (SOCEthicalImperative*)calloc(1, sizeof(SOCEthicalImperative));
    if (!ei) return NULL;
    ei->max_choices = max_choices > 0 ? max_choices : 10;
    ei->choice_set = (double*)calloc((size_t)ei->max_choices, sizeof(double));
    ei->ethical_value = (double*)calloc((size_t)ei->max_choices, sizeof(double));
    ei->imperative_violation = 0.0;
    /* Default: each choice has positive ethical value */
    for (int i = 0; i < ei->max_choices; i++) {
        ei->choice_set[i] = (double)i / (double)ei->max_choices;
        ei->ethical_value[i] = 1.0;
    }
    ei->n_choices = ei->max_choices;
    return ei;
}

void soc_ethics_free(SOCEthicalImperative* ei) {
    if (!ei) return;
    free(ei->choice_set);
    free(ei->ethical_value);
    soc_eigenform_free(ei->ethical_eigenform);
    free(ei);
}

/* Evaluate an action against von Foerster's ethical imperative.
 * Knowledge point: "Act always so as to increase the number of choices."
 * Complexity: O(n_choices) */
double soc_ethics_evaluate(SOCEthicalImperative* ei, int action_idx) {
    if (!ei || action_idx < 0 || action_idx >= ei->n_choices) return 0.0;
    /* Does this action increase or decrease the choice space? */
    double before = (double)ei->n_choices;
    /* Simulate: action may constrain or expand future choices */
    double after = before + ei->ethical_value[action_idx] - 0.5;
    double delta = after - before;
    /* Ethical imperative violation if choices decrease */
    if (delta < 0) ei->imperative_violation = fmin(1.0, ei->imperative_violation + fabs(delta));
    return delta;
}

/* Compute the ethical eigenform: the pattern of action that maximizes choices.
 * Knowledge point: Ethics as eigenbehavior — stable pattern of choice-increasing action.
 * Complexity: O(max_choices^2) */
void soc_ethics_compute_eigenform(SOCEthicalImperative* ei, int max_iters) {
    (void)max_iters;
    if (!ei) return;
    /* Prefer actions with highest ethical_value */
    double best_val = ei->ethical_value[0];
    for (int i = 1; i < ei->n_choices; i++) {
        if (ei->ethical_value[i] > best_val) {
            best_val = ei->ethical_value[i];
        }
    }
    /* The eigenform is to choose the action that maximizes future choices */
    ei->imperative_violation = 1.0 - best_val;
}

void soc_ethics_print(const SOCEthicalImperative* ei) {
    if (!ei) { printf("Ethics: (null)\n"); return; }
    printf("=== von Foerster's Ethical Imperative ===\n");
    printf("  Current choices: %d / %d\n", ei->n_choices, ei->max_choices);
    printf("  Imperative violation: %.4f (0=ethical, 1=unethical)\n",
           ei->imperative_violation);
    printf("  Mean ethical value: %.4f\n",
           ei->n_choices > 0 ? soc_vector_norm(ei->ethical_value, ei->n_choices)
                              / sqrt((double)ei->n_choices) : 0.0);
}

/* ========================================================================
 * Fuzzy Cybernetics — Degrees of Observation
 *
 * Spencer Brown's distinctions are binary (marked/unmarked).
 * Fuzzy cybernetics allows degrees of markedness, bridging
 * between the binary calculus of indications and continuous
 * observation. This is particularly relevant for:
 * - Partial observation (partially transparent blind spot)
 * - Fuzzy set membership in social systems (Luhmann)
 * - Graded distinctions in constructivist learning
 * ======================================================================== */

typedef struct {
    double* membership;          /* fuzzy membership in marked/unmarked */
    int n_elements;
    double* fuzzy_distinction;   /* how sharply the distinction is drawn (0=blurred, 1=crisp) */
    double alpha_cut;            /* threshold for crisp distinction */
    double fuzziness_index;      /* overall measure of distinction blur */
} SOCFuzzyDistinction;

SOCFuzzyDistinction* soc_fuzzy_create(int n_elements) {
    if (n_elements <= 0) return NULL;
    SOCFuzzyDistinction* fd = (SOCFuzzyDistinction*)calloc(1, sizeof(SOCFuzzyDistinction));
    if (!fd) return NULL;
    fd->n_elements = n_elements;
    fd->membership = (double*)calloc((size_t)n_elements, sizeof(double));
    fd->fuzzy_distinction = (double*)calloc((size_t)n_elements, sizeof(double));
    for (int i = 0; i < n_elements; i++) {
        fd->membership[i] = 0.5;          /* neither fully marked nor unmarked */
        fd->fuzzy_distinction[i] = 1.0;   /* crisp by default */
    }
    fd->alpha_cut = 0.5;
    fd->fuzziness_index = 0.0;
    return fd;
}

void soc_fuzzy_free(SOCFuzzyDistinction* fd) {
    if (!fd) return;
    free(fd->membership);
    free(fd->fuzzy_distinction);
    free(fd);
}

/* Apply fuzzy distinction: continuous membership in marked state.
 * Knowledge point: Observation as fuzzy, not crisp (graded distinctions).
 * Complexity: O(n_elements) */
void soc_fuzzy_set_membership(SOCFuzzyDistinction* fd, int idx, double degree) {
    if (!fd || idx < 0 || idx >= fd->n_elements) return;
    fd->membership[idx] = fmax(0.0, fmin(1.0, degree));
}

/* Alpha-cut: crisp distinction at threshold.
 * Knowledge point: Any crisp observation is a special case of fuzzy observation.
 * Complexity: O(n_elements) */
void soc_fuzzy_alpha_cut(SOCFuzzyDistinction* fd, double alpha, bool* crisp_result) {
    if (!fd || !crisp_result) return;
    for (int i = 0; i < fd->n_elements; i++) {
        crisp_result[i] = (fd->membership[i] >= alpha);
    }
    fd->alpha_cut = alpha;
}

/* Measure how fuzzy (vs. crisp) the distinction is.
 * Knowledge point: Fuzziness = distance from nearest crisp membership vector.
 * Complexity: O(n_elements) */
double soc_fuzzy_measure_fuzziness(SOCFuzzyDistinction* fd) {
    if (!fd) return 0.0;
    double fuzz = 0.0;
    for (int i = 0; i < fd->n_elements; i++) {
        /* Distance to nearest crisp value (0 or 1) */
        double d = fmin(fd->membership[i], 1.0 - fd->membership[i]);
        fuzz += d;
    }
    fd->fuzziness_index = 2.0 * fuzz / (double)fd->n_elements;
    return fd->fuzziness_index;
}

void soc_fuzzy_print(const SOCFuzzyDistinction* fd) {
    if (!fd) { printf("FuzzyDistinction: (null)\n"); return; }
    printf("=== Fuzzy Distinction ===\n");
    printf("  Elements: %d, Alpha-cut: %.3f\n", fd->n_elements, fd->alpha_cut);
    printf("  Fuzziness index: %.4f (0=crisp, 1=maximum fuzziness)\n",
           fd->fuzziness_index);
}

/* ========================================================================
 * Game of Life — Cellular Automaton as Second-Order System
 *
 * Conway's Game of Life (1970) is a cellular automaton that exemplifies
 * second-order cybernetics principles:
 *
 * 1. The system is non-trivial: predicting its state requires
 *    simulating it — no closed-form solution.
 * 2. The observer must become an NTM to "understand" the Game of Life.
 * 3. Emergence: complex patterns (gliders, oscillators) emerge from
 *    simple rules — these are eigenbehaviors of the CA.
 * 4. Self-reference: the Game of Life is Turing-complete — it can
 *    simulate itself (von Neumann's universal constructor).
 *
 * The CA rule (B3/S23):
 *   - Birth: dead cell with exactly 3 live neighbors becomes live
 *   - Survival: live cell with 2 or 3 live neighbors stays live
 *   - Death: all other live cells die
 * ======================================================================== */

typedef struct {
    bool* grid;
    int width;
    int height;
    int generation;
    double* density_history;
    int hist_len;
    int hist_cap;
    bool* eigenpattern;        /* detected stable pattern */
    int pattern_x, pattern_y;
    int pattern_w, pattern_h;
} SOCGameOfLife;

SOCGameOfLife* soc_gol_create(int width, int height) {
    if (width <= 0 || height <= 0) return NULL;
    SOCGameOfLife* gol = (SOCGameOfLife*)calloc(1, sizeof(SOCGameOfLife));
    if (!gol) return NULL;
    gol->width = width;
    gol->height = height;
    gol->grid = (bool*)calloc((size_t)(width * height), sizeof(bool));
    gol->hist_cap = 200;
    gol->density_history = (double*)calloc((size_t)gol->hist_cap, sizeof(double));
    return gol;
}

void soc_gol_free(SOCGameOfLife* gol) {
    if (!gol) return;
    free(gol->grid);
    free(gol->density_history);
    free(gol->eigenpattern);
    free(gol);
}

/* Set a cell to alive.
 * Complexity: O(1) */
void soc_gol_set_cell(SOCGameOfLife* gol, int x, int y, bool alive) {
    if (!gol || x < 0 || x >= gol->width || y < 0 || y >= gol->height) return;
    gol->grid[y * gol->width + x] = alive;
}

/* Count live neighbors of a cell (toroidal boundary).
 * Complexity: O(1) */
static int gol_count_neighbors(const SOCGameOfLife* gol, int x, int y) {
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = (x + dx + gol->width) % gol->width;
            int ny = (y + dy + gol->height) % gol->height;
            if (gol->grid[ny * gol->width + nx]) count++;
        }
    }
    return count;
}

/* Evolve one generation.
 * Knowledge point: Emergence of complexity from simple recursive rules.
 * Complexity: O(width * height) */
void soc_gol_step(SOCGameOfLife* gol) {
    if (!gol) return;

    bool* new_grid = (bool*)calloc((size_t)(gol->width * gol->height), sizeof(bool));
    if (!new_grid) return;

    int live_count = 0;
    for (int y = 0; y < gol->height; y++) {
        for (int x = 0; x < gol->width; x++) {
            int n = gol_count_neighbors(gol, x, y);
            int idx = y * gol->width + x;
            if (gol->grid[idx]) {
                /* Survival: S23 */
                new_grid[idx] = (n == 2 || n == 3);
            } else {
                /* Birth: B3 */
                new_grid[idx] = (n == 3);
            }
            if (new_grid[idx]) live_count++;
        }
    }

    memcpy(gol->grid, new_grid, (size_t)(gol->width * gol->height) * sizeof(bool));
    free(new_grid);

    gol->generation++;

    /* Record density */
    if (gol->hist_len < gol->hist_cap) {
        gol->density_history[gol->hist_len] =
            (double)live_count / (double)(gol->width * gol->height);
        gol->hist_len++;
    }
}

/* Detect if the Game of Life has reached a stable eigenbehavior.
 * Knowledge point: Eigenpattern = stable repeating form (periodic or static).
 * Complexity: O(width * height * hist_len) */
bool soc_gol_detect_eigenpattern(const SOCGameOfLife* gol, int lookback) {
    if (!gol || gol->hist_len < 2) return false;
    /* Check if density has stabilized */
    if (gol->hist_len >= lookback) {
        double last_d = gol->density_history[gol->hist_len - 1];
        bool stable = true;
        for (int i = 1; i < lookback && (gol->hist_len - 1 - i) >= 0; i++) {
            if (fabs(gol->density_history[gol->hist_len - 1 - i] - last_d) > 0.01) {
                stable = false;
                break;
            }
        }
        return stable;
    }
    return false;
}

void soc_gol_print(const SOCGameOfLife* gol) {
    if (!gol) { printf("GameOfLife: (null)\n"); return; }
    printf("=== Conway's Game of Life ===\n");
    printf("  Size: %d x %d, Generation: %d\n",
           gol->width, gol->height, gol->generation);
    printf("  Current density: %.4f\n",
           gol->hist_len > 0 ? gol->density_history[gol->hist_len - 1] : 0.0);
    printf("  Eigenpattern: %s\n",
           soc_gol_detect_eigenpattern(gol, 5) ? "detected" : "not detected");

    /* Print grid */
    printf("  Grid:\n");
    for (int y = 0; y < gol->height && y < 20; y++) {
        printf("    ");
        for (int x = 0; x < gol->width && x < 40; x++) {
            printf("%c", gol->grid[y * gol->width + x] ? 'O' : '.');
        }
        printf("\n");
    }
}

