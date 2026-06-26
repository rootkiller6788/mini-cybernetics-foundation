/* ============================================================================
 * soc_constructivism.c — Constructivist epistemology implementations
 *
 * Covers: Conceptual networks, Piagetian assimilation/accommodation,
 * Enactive cognition (Varela/Thompson/Rosch), Social construction
 * (Berger & Luckmann), Operational closure (Maturana & Varela),
 * Structural coupling, and constructivist learning.
 *
 * "Knowledge is not passively received but actively built up by the
 * cognizing subject." — Ernst von Glasersfeld
 * ============================================================================ */

#include "soc_constructivism.h"

/* ========================================================================
 * Constructed Concepts
 * ======================================================================== */

SOCConstructedConcept* soc_concept_create(int dim, int op_dim) {
    if (dim <= 0) return NULL;
    SOCConstructedConcept* c = (SOCConstructedConcept*)calloc(1, sizeof(SOCConstructedConcept));
    if (!c) return NULL;
    c->dim = dim;
    c->concept_vector = (double*)calloc((size_t)dim, sizeof(double));
    c->op_dim = op_dim > 0 ? op_dim : dim;
    c->operational_meaning = (double*)calloc((size_t)c->op_dim, sizeof(double));
    c->viability_score = 0.5;
    c->perturbation_signature = (double*)calloc((size_t)dim, sizeof(double));
    c->sig_len = dim;
    return c;
}

void soc_concept_free(SOCConstructedConcept* c) {
    if (!c) return;
    free(c->concept_vector);
    free(c->operational_meaning);
    free(c->perturbation_signature);
    free(c);
}

void soc_concept_set_operational_meaning(SOCConstructedConcept* c,
                                          const double* operations, int n) {
    if (!c || !operations) return;
    int copy_n = (n < c->op_dim) ? n : c->op_dim;
    memcpy(c->operational_meaning, operations, (size_t)copy_n * sizeof(double));
}

/* Test a concept's viability against perturbation.
 * Knowledge point: A concept's meaning = the operations that produce it (Piaget).
 * Complexity: O(dim) */
double soc_concept_viability_test(SOCConstructedConcept* c,
                                   const double* perturbation, int dim) {
    if (!c || !perturbation) return 0.0;
    /* Viability = correlation between concept and perturbation signature */
    int n = (dim < c->dim) ? dim : c->dim;
    double corr = 0.0;
    double norm_c = soc_vector_norm(c->concept_vector, n);
    double norm_p = soc_vector_norm(perturbation, n);
    if (norm_c < 1e-15 || norm_p < 1e-15) return 0.0;
    for (int i = 0; i < n; i++) {
        corr += c->concept_vector[i] * perturbation[i];
    }
    return corr / (norm_c * norm_p);
}

/* Confirm or refute a concept based on experience.
 * Knowledge point: Viability is determined pragmatically, not by correspondence.
 * Complexity: O(1) */
bool soc_concept_confirm(SOCConstructedConcept* c, bool successful) {
    if (!c) return false;
    if (successful) {
        c->n_confirmations++;
        c->viability_score = fmin(1.0, c->viability_score + 0.1);
    } else {
        c->n_refutations++;
        c->viability_score = fmax(0.0, c->viability_score - 0.15);
    }
    return c->viability_score > 0.5;
}

void soc_concept_print(const SOCConstructedConcept* c) {
    if (!c) { printf("Concept: (null)\n"); return; }
    printf("  Concept dim=%d, Viability=%.4f, Confirmations=%d, Refutations=%d\n",
           c->dim, c->viability_score, c->n_confirmations, c->n_refutations);
}

/* ========================================================================
 * Conceptual Network
 * ======================================================================== */

SOCConceptualNetwork* soc_conceptual_network_create(int max_concepts) {
    if (max_concepts <= 0) return NULL;
    SOCConceptualNetwork* cn = (SOCConceptualNetwork*)calloc(1, sizeof(SOCConceptualNetwork));
    if (!cn) return NULL;
    cn->max_concepts = max_concepts;
    cn->concepts = (SOCConstructedConcept**)calloc((size_t)max_concepts, sizeof(SOCConstructedConcept*));
    return cn;
}

void soc_conceptual_network_free(SOCConceptualNetwork* cn) {
    if (!cn) return;
    for (int i = 0; i < cn->n_concepts; i++) soc_concept_free(cn->concepts[i]);
    free(cn->concepts);
    free(cn->coherence_matrix);
    free(cn);
}

void soc_conceptual_network_add_concept(SOCConceptualNetwork* cn,
                                         SOCConstructedConcept* concept) {
    if (!cn || !concept || cn->n_concepts >= cn->max_concepts) return;
    cn->concepts[cn->n_concepts++] = concept;
}

/* Compute coherence matrix between all concept pairs.
 * Knowledge point: Coherence = concepts support rather than contradict each other.
 * Complexity: O(n_concepts^2 * dim) */
void soc_conceptual_network_compute_coherence(SOCConceptualNetwork* cn) {
    if (!cn || cn->n_concepts < 2) { if (cn) cn->overall_coherence = 1.0; return; }

    int nc = cn->n_concepts;
    cn->matrix_dim = nc;
    cn->coherence_matrix = (double*)realloc(cn->coherence_matrix,
        (size_t)(nc * nc) * sizeof(double));

    for (int i = 0; i < nc; i++) {
        cn->coherence_matrix[i * nc + i] = 1.0;
        for (int j = i + 1; j < nc; j++) {
            int dim_i = cn->concepts[i]->dim;
            int dim_j = cn->concepts[j]->dim;
            int min_dim = (dim_i < dim_j) ? dim_i : dim_j;
            double corr = 0.0;
            double ni = soc_vector_norm(cn->concepts[i]->concept_vector, min_dim);
            double nj = soc_vector_norm(cn->concepts[j]->concept_vector, min_dim);
            if (ni > 1e-15 && nj > 1e-15) {
                for (int d = 0; d < min_dim; d++) {
                    corr += cn->concepts[i]->concept_vector[d]
                            * cn->concepts[j]->concept_vector[d];
                }
                corr /= (ni * nj);
            }
            cn->coherence_matrix[i * nc + j] = corr;
            cn->coherence_matrix[j * nc + i] = corr;
        }
    }

    /* Overall coherence = mean of off-diagonal */
    double sum = 0.0;
    int count = 0;
    for (int i = 0; i < nc; i++) {
        for (int j = i + 1; j < nc; j++) {
            sum += cn->coherence_matrix[i * nc + j];
            count++;
        }
    }
    cn->overall_coherence = (count > 0) ? sum / (double)count : 1.0;
}

double soc_conceptual_network_mean_viability(const SOCConceptualNetwork* cn) {
    if (!cn || cn->n_concepts == 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < cn->n_concepts; i++) {
        sum += cn->concepts[i]->viability_score;
    }
    return sum / (double)cn->n_concepts;
}

/* Prune concepts with viability below threshold.
 * Knowledge point: Unviable concepts are abandoned (evolutionary epistemology).
 * Complexity: O(n_concepts) */
void soc_conceptual_network_prune(SOCConceptualNetwork* cn, double threshold) {
    if (!cn) return;
    int write = 0;
    for (int read = 0; read < cn->n_concepts; read++) {
        if (cn->concepts[read]->viability_score >= threshold) {
            cn->concepts[write++] = cn->concepts[read];
        } else {
            soc_concept_free(cn->concepts[read]);
        }
    }
    cn->n_concepts = write;
}

void soc_conceptual_network_print(const SOCConceptualNetwork* cn) {
    if (!cn) { printf("ConceptualNetwork: (null)\n"); return; }
    printf("=== Conceptual Network ===\n");
    printf("  Concepts: %d / %d\n", cn->n_concepts, cn->max_concepts);
    printf("  Overall coherence: %.4f, Mean viability: %.4f\n",
           cn->overall_coherence, soc_conceptual_network_mean_viability(cn));
    printf("  Complexity: %.4f\n", cn->complexity);
}

/* ========================================================================
 * Constructivist Agent (Piagetian learning)
 * ======================================================================== */

SOCConstructivistAgent* soc_agent_create(int exp_cap, int concept_dim) {
    SOCConstructivistAgent* a = (SOCConstructivistAgent*)calloc(1, sizeof(SOCConstructivistAgent));
    if (!a) return NULL;
    a->exp_cap = exp_cap > 0 ? exp_cap : 100;
    a->experience_buffer = (double*)calloc((size_t)(a->exp_cap * concept_dim), sizeof(double));
    a->network = soc_conceptual_network_create(20);
    a->assimilation_rate = 0.3;
    a->accommodation_rate = 0.1;
    a->equilibration_target = 0.5;
    return a;
}

void soc_agent_free(SOCConstructivistAgent* a) {
    if (!a) return;
    soc_conceptual_network_free(a->network);
    free(a->experience_buffer);
    free(a);
}

/* Record an experience.
 * Knowledge point: Experience is the perturbation that triggers adaptation.
 * Complexity: O(concept_dim) */
void soc_agent_experience(SOCConstructivistAgent* a, const double* exp, int dim) {
    if (!a || !exp) return;
    int cdim = (a->network && a->network->n_concepts > 0)
               ? a->network->concepts[0]->dim : dim;
    if (a->exp_len < a->exp_cap) {
        memcpy(a->experience_buffer + a->exp_len * cdim,
               exp, (size_t)cdim * sizeof(double));
        a->exp_len++;
    }
}

/* Adapt: equilibrate between assimilation and accommodation (Piaget).
 * Knowledge point: Adaptation = equilibrium between fitting world to self
 * and fitting self to world.
 * Complexity: O(concept_dim * n_concepts) */
void soc_agent_adapt(SOCConstructivistAgent* a) {
    if (!a || a->exp_len == 0) return;
    a->n_adaptations++;
    /* Balance: randomly choose assimilation or accommodation based on equilibration */
    double coin = (double)rand() / (double)RAND_MAX;
    if (coin < a->equilibration_target) {
        /* Assimilation: fit experience into existing schema */
        for (int i = 0; i < a->network->n_concepts; i++) {
            soc_concept_confirm(a->network->concepts[i], coin < 0.7);
        }
    } else {
        /* Accommodation: modify schema to fit experience */
        for (int i = 0; i < a->network->n_concepts; i++) {
            if (a->network->concepts[i]->viability_score < 0.3) {
                /* Modify concept based on recent experience */
                int last_exp = (a->exp_len - 1) * a->network->concepts[i]->dim;
                if (a->exp_len > 0 && a->experience_buffer) {
                    memcpy(a->network->concepts[i]->concept_vector,
                           a->experience_buffer + last_exp,
                           (size_t)a->network->concepts[i]->dim * sizeof(double));
                }
                a->network->concepts[i]->viability_score = 0.5;
            }
        }
    }
    a->adaptation_history_sum += 1.0;
}

/* Assimilation: interpret new experience using existing schema.
 * Knowledge point: Assimilation maintains cognitive stability (Piaget).
 * Complexity: O(concept_dim * n_concepts) */
double soc_agent_assimilate(SOCConstructivistAgent* a, const double* experience,
                              int dim) {
    if (!a || !experience || !a->network || a->network->n_concepts == 0) return 0.0;
    /* Find the concept that best matches this experience */
    double best_match = -1.0;
    for (int i = 0; i < a->network->n_concepts; i++) {
        double match = soc_concept_viability_test(a->network->concepts[i],
                                                   experience, dim);
        if (match > best_match) best_match = match;
    }
    return fmax(0.0, best_match);
}

/* Accommodation: modify schema to better fit experience.
 * Knowledge point: Accommodation enables cognitive growth (Piaget).
 * Complexity: O(concept_dim * n_concepts) */
double soc_agent_accommodate(SOCConstructivistAgent* a, const double* experience,
                               int dim) {
    if (!a || !experience || !a->network) return 0.0;
    /* Create a new concept from the experience if it's novel enough */
    double novelty = soc_agent_assimilate(a, experience, dim);
    if (novelty < 0.3 && a->network->n_concepts < a->network->max_concepts) {
        SOCConstructedConcept* new_c = soc_concept_create(dim, dim);
        memcpy(new_c->concept_vector, experience, (size_t)dim * sizeof(double));
        soc_conceptual_network_add_concept(a->network, new_c);
        return 0.5; /* accommodation successful */
    }
    return fmax(0.0, 1.0 - novelty); /* difficulty is inverse of assimilation success */
}

void soc_agent_print(const SOCConstructivistAgent* a) {
    if (!a) { printf("ConstructivistAgent: (null)\n"); return; }
    printf("=== Constructivist Agent ===\n");
    printf("  Experiences: %d, Adaptations: %d\n", a->exp_len, a->n_adaptations);
    printf("  Assimilation rate: %.3f, Accommodation rate: %.3f\n",
           a->assimilation_rate, a->accommodation_rate);
    printf("  Equilibration: %.3f\n", a->equilibration_target);
    soc_conceptual_network_print(a->network);
}

/* ========================================================================
 * Enactive Loop (Varela, Thompson, Rosch)
 * ======================================================================== */

SOCEnactiveLoop* soc_enactive_create(int sensory_dim, int motor_dim,
                                      int world_dim) {
    if (sensory_dim <= 0 || motor_dim <= 0 || world_dim <= 0) return NULL;
    SOCEnactiveLoop* el = (SOCEnactiveLoop*)calloc(1, sizeof(SOCEnactiveLoop));
    if (!el) return NULL;
    el->sensory_dim = sensory_dim;
    el->motor_dim = motor_dim;
    el->world_dim = world_dim;
    el->sensory_input = (double*)calloc((size_t)sensory_dim, sizeof(double));
    el->motor_output = (double*)calloc((size_t)motor_dim, sizeof(double));
    el->world_state = (double*)calloc((size_t)world_dim, sizeof(double));
    el->coupling_matrix = (double*)calloc((size_t)(sensory_dim * motor_dim), sizeof(double));
    for (int i = 0; i < sensory_dim && i < motor_dim; i++) {
        el->coupling_matrix[i * motor_dim + i] = 1.0;
    }
    return el;
}

void soc_enactive_free(SOCEnactiveLoop* el) {
    if (!el) return;
    free(el->sensory_input);
    free(el->motor_output);
    free(el->world_state);
    free(el->coupling_matrix);
    free(el);
}

/* Sense: world state produces sensory input.
 * Knowledge point: Perception is sensorimotor action, not passive reception.
 * Complexity: O(sensory_dim * world_dim) */
void soc_enactive_sense(SOCEnactiveLoop* el, const double* world) {
    if (!el || !world) return;
    memcpy(el->world_state, world, (size_t)el->world_dim * sizeof(double));
    /* Sensory input = f(world, motor_state) — enactive coupling */
    for (int i = 0; i < el->sensory_dim; i++) {
        el->sensory_input[i] = 0.0;
        for (int j = 0; j < el->world_dim && j < el->sensory_dim; j++) {
            el->sensory_input[i] += world[j] * (1.0 + 0.1 * el->motor_output[i % el->motor_dim]);
        }
    }
}

/* Act: sensory input generates motor output.
 * Knowledge point: Action shapes perception, perception shapes action.
 * Complexity: O(motor_dim * sensory_dim) */
void soc_enactive_act(SOCEnactiveLoop* el, double* motor_cmd) {
    if (!el || !motor_cmd) return;
    soc_matrix_vec_mul(el->coupling_matrix, el->sensory_input, motor_cmd,
                       el->motor_dim, el->sensory_dim);
    memcpy(el->motor_output, motor_cmd, (size_t)el->motor_dim * sizeof(double));
}

/* Couple: run the enactive loop for n_steps.
 * Knowledge point: The world is "enacted" through sensorimotor coupling.
 * Complexity: O(n_steps * (sensory_dim * world_dim + motor_dim * sensory_dim)) */
void soc_enactive_couple(SOCEnactiveLoop* el, int n_steps) {
    if (!el) return;
    double* motor = (double*)malloc((size_t)el->motor_dim * sizeof(double));
    if (!motor) return;
    for (int t = 0; t < n_steps; t++) {
        soc_enactive_sense(el, el->world_state);
        soc_enactive_act(el, motor);
        /* World changes due to motor action (simple model) */
        for (int i = 0; i < el->world_dim && i < el->motor_dim; i++) {
            el->world_state[i] += 0.01 * motor[i];
        }
    }
    free(motor);
    el->has_enacted_world = true;
}

/* Correlation between sensory and motor channels.
 * Knowledge point: Sensorimotor correlation = structural coupling.
 * Complexity: O(sensory_dim * motor_dim) */
double soc_enactive_correlation(const SOCEnactiveLoop* el) {
    if (!el) return 0.0;
    double corr = 0.0;
    for (int i = 0; i < el->sensory_dim && i < el->motor_dim; i++) {
        corr += el->sensory_input[i] * el->motor_output[i];
    }
    double ns = soc_vector_norm(el->sensory_input, el->sensory_dim);
    double nm = soc_vector_norm(el->motor_output, el->motor_dim);
    if (ns < 1e-15 || nm < 1e-15) return 0.0;
    return corr / (ns * nm);
}

void soc_enactive_print(const SOCEnactiveLoop* el) {
    if (!el) { printf("EnactiveLoop: (null)\n"); return; }
    printf("=== Enactive Loop ===\n");
    printf("  Sensory: %d, Motor: %d, World: %d\n",
           el->sensory_dim, el->motor_dim, el->world_dim);
    printf("  Correlation: %.4f, Enacted world: %s\n",
           soc_enactive_correlation(el),
           el->has_enacted_world ? "yes" : "no");
}

/* ========================================================================
 * Social Construction (Berger & Luckmann)
 * ======================================================================== */

SOCSocialConstruction* soc_social_create(const char* group, int n_members,
                                          int concept_dim) {
    if (n_members <= 0 || concept_dim <= 0) return NULL;
    SOCSocialConstruction* sc = (SOCSocialConstruction*)calloc(1, sizeof(SOCSocialConstruction));
    if (!sc) return NULL;
    sc->social_group = group ? _strdup(group) : _strdup("unnamed_group");
    sc->n_members = n_members;
    sc->concept_dim = concept_dim;
    sc->member_beliefs = (double**)calloc((size_t)n_members, sizeof(double*));
    for (int i = 0; i < n_members; i++) {
        sc->member_beliefs[i] = (double*)calloc((size_t)concept_dim, sizeof(double));
    }
    sc->shared_concepts = soc_conceptual_network_create(20);
    sc->institutionalization = (double*)calloc((size_t)concept_dim, sizeof(double));
    sc->legitimization = (double*)calloc((size_t)concept_dim, sizeof(double));
    return sc;
}

void soc_social_free(SOCSocialConstruction* sc) {
    if (!sc) return;
    free(sc->social_group);
    soc_conceptual_network_free(sc->shared_concepts);
    for (int i = 0; i < sc->n_members; i++) free(sc->member_beliefs[i]);
    free(sc->member_beliefs);
    free(sc->institutionalization);
    free(sc->legitimization);
    free(sc);
}

void soc_social_set_member_belief(SOCSocialConstruction* sc, int member,
                                   const double* beliefs) {
    if (!sc || member < 0 || member >= sc->n_members || !beliefs) return;
    memcpy(sc->member_beliefs[member], beliefs,
           (size_t)sc->concept_dim * sizeof(double));
}

/* Measure consensus among group members.
 * Knowledge point: Social reality = shared beliefs (Berger & Luckmann).
 * Complexity: O(n_members^2 * concept_dim) */
double soc_social_measure_consensus(SOCSocialConstruction* sc) {
    if (!sc || sc->n_members < 2) return sc ? 1.0 : 0.0;
    double total_sim = 0.0;
    int n_pairs = 0;
    for (int i = 0; i < sc->n_members; i++) {
        for (int j = i + 1; j < sc->n_members; j++) {
            double sim = soc_cosine_similarity(sc->member_beliefs[i],
                                                sc->member_beliefs[j],
                                                sc->concept_dim);
            total_sim += sim;
            n_pairs++;
        }
    }
    sc->consensus_level = (n_pairs > 0) ? total_sim / (double)n_pairs : 1.0;
    return sc->consensus_level;
}

/* Institutionalize a concept: it becomes "taken for granted."
 * Knowledge point: Institutions = habitualized shared typifications (Berger & Luckmann).
 * Complexity: O(1) */
void soc_social_institutionalize(SOCSocialConstruction* sc, int concept_idx) {
    if (!sc || concept_idx < 0 || concept_idx >= sc->concept_dim) return;
    sc->institutionalization[concept_idx] = fmin(1.0,
        sc->institutionalization[concept_idx] + 0.2);
    sc->n_institutions++;
}

/* Reification: how much social constructions seem "natural" rather than made.
 * Knowledge point: Reification = forgetting that social reality is human-made.
 * Complexity: O(concept_dim) */
double soc_social_reification_level(SOCSocialConstruction* sc) {
    if (!sc) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < sc->concept_dim; i++) {
        sum += sc->institutionalization[i];
    }
    sc->reification_level = sum / (double)sc->concept_dim;
    return sc->reification_level;
}

void soc_social_print(const SOCSocialConstruction* sc) {
    if (!sc) { printf("SocialConstruction: (null)\n"); return; }
    printf("=== Social Construction: %s ===\n", sc->social_group);
    printf("  Members: %d, Concept dim: %d\n", sc->n_members, sc->concept_dim);
    printf("  Consensus: %.4f, Reification: %.4f\n",
           sc->consensus_level, sc->reification_level);
    printf("  Institutions: %d\n", sc->n_institutions);
}

/* ========================================================================
 * Operational Closure (Maturana & Varela)
 * ======================================================================== */

SOCOperationalClosure* soc_opclosure_create(int state_dim, int pert_dim,
                                              double (*dyn)(const double*, int,
                                                             const double*, int,
                                                             double*, void*),
                                              void* ctx) {
    if (state_dim <= 0 || pert_dim < 0 || !dyn) return NULL;
    SOCOperationalClosure* oc = (SOCOperationalClosure*)calloc(1, sizeof(SOCOperationalClosure));
    if (!oc) return NULL;
    oc->state_dim = state_dim;
    oc->pert_dim = pert_dim;
    oc->internal_state = (double*)calloc((size_t)state_dim, sizeof(double));
    oc->perturbation_channel = (double*)calloc((size_t)pert_dim, sizeof(double));
    oc->internal_dynamics = dyn;
    oc->dyn_ctx = ctx;
    oc->hist_cap = 100;
    oc->state_history = (double*)calloc((size_t)(oc->hist_cap * state_dim), sizeof(double));
    return oc;
}

void soc_opclosure_free(SOCOperationalClosure* oc) {
    if (!oc) return;
    free(oc->internal_state);
    free(oc->perturbation_channel);
    free(oc->state_history);
    free(oc);
}

void soc_opclosure_perturb(SOCOperationalClosure* oc, const double* pert) {
    if (!oc || !pert) return;
    memcpy(oc->perturbation_channel, pert,
           (size_t)oc->pert_dim * sizeof(double));
}

void soc_opclosure_step(SOCOperationalClosure* oc) {
    if (!oc || !oc->internal_dynamics) return;
    double* new_state = (double*)malloc((size_t)oc->state_dim * sizeof(double));
    if (!new_state) return;
    oc->internal_dynamics(oc->internal_state, oc->state_dim,
                           oc->perturbation_channel, oc->pert_dim,
                           new_state, oc->dyn_ctx);
    /* Record history */
    if (oc->hist_len < oc->hist_cap) {
        memcpy(oc->state_history + oc->hist_len * oc->state_dim,
               new_state, (size_t)oc->state_dim * sizeof(double));
        oc->hist_len++;
    }
    memcpy(oc->internal_state, new_state, (size_t)oc->state_dim * sizeof(double));
    free(new_state);
}

/* Measure operational closure: how much internal dynamics (vs. perturbation) determines state.
 * Knowledge point: Operational closure = the system's state is determined by its own structure.
 * Complexity: O(state_dim) */
double soc_opclosure_measure_closure(SOCOperationalClosure* oc) {
    if (!oc || oc->hist_len < 2) return 0.0;
    /* Compare state change variance in response to perturbations */
    double self_determination = 0.0;
    if (oc->pert_dim > 0) {
        double pert_power = soc_vector_norm(oc->perturbation_channel, oc->pert_dim);
        double state_change = 0.0;
        if (oc->hist_len >= 2) {
            double* prev = oc->state_history + (oc->hist_len - 2) * oc->state_dim;
            double* curr = oc->state_history + (oc->hist_len - 1) * oc->state_dim;
            for (int i = 0; i < oc->state_dim; i++) {
                double d = curr[i] - prev[i];
                state_change += d * d;
            }
        }
        self_determination = 1.0 / (1.0 + pert_power / (sqrt(state_change) + 1e-15));
    } else {
        self_determination = 1.0; /* No perturbation = fully closed */
    }
    oc->closure_index = self_determination;
    return self_determination;
}

/* Check if the system is autopoietic: self-producing.
 * Knowledge point: Autopoiesis = the network produces itself (Maturana & Varela).
 * Complexity: O(state_dim) */
bool soc_opclosure_is_autopoietic(const SOCOperationalClosure* oc) {
    if (!oc || oc->hist_len < 3) return false;
    /* Simple criterion: state returns to similar region after perturbation */
    double* early = oc->state_history; /* first recorded state */
    double* late = oc->state_history + (oc->hist_len - 1) * oc->state_dim;
    double dist = 0.0;
    for (int i = 0; i < oc->state_dim; i++) {
        double d = early[i] - late[i];
        dist += d * d;
    }
    return sqrt(dist) < 0.5 * soc_vector_norm(oc->internal_state, oc->state_dim);
}

void soc_opclosure_print(const SOCOperationalClosure* oc) {
    if (!oc) { printf("OpClosure: (null)\n"); return; }
    printf("=== Operational Closure ===\n");
    printf("  State dim: %d, Pert dim: %d\n", oc->state_dim, oc->pert_dim);
    printf("  Closure index: %.4f\n", oc->closure_index);
    printf("  Autopoietic: %s\n", soc_opclosure_is_autopoietic(oc) ? "yes" : "no");
}

/* ========================================================================
 * Structural Coupling
 * ======================================================================== */

SOCStructuralCoupling* soc_coupling_create(SOCOperationalClosure* a,
                                            SOCOperationalClosure* b,
                                            double strength) {
    if (!a || !b) return NULL;
    SOCStructuralCoupling* sc = (SOCStructuralCoupling*)calloc(1, sizeof(SOCStructuralCoupling));
    if (!sc) return NULL;
    sc->system_a = a;
    sc->system_b = b;
    sc->coupling_strength = strength;
    sc->perturbation_tolerance = 0.5;
    return sc;
}

void soc_coupling_free(SOCStructuralCoupling* sc) {
    if (!sc) return;
    free(sc->coupling_history);
    free(sc);
}

/* Evolve the coupled systems for n_steps.
 * Knowledge point: Structural coupling = mutual perturbation history.
 * Complexity: O(n_steps * (cost_a + cost_b)) */
void soc_coupling_evolve(SOCStructuralCoupling* sc, int n_steps) {
    if (!sc || n_steps <= 0) return;
    sc->coupling_history = (double*)realloc(sc->coupling_history,
        (size_t)(n_steps * 2) * sizeof(double));
    for (int t = 0; t < n_steps; t++) {
        /* A perturbs B, B perturbs A */
        soc_opclosure_perturb(sc->system_b, sc->system_a->internal_state);
        soc_opclosure_step(sc->system_b);
        soc_opclosure_perturb(sc->system_a, sc->system_b->internal_state);
        soc_opclosure_step(sc->system_a);

        /* Measure coupling effect */
        double ca = soc_opclosure_measure_closure(sc->system_a);
        double cb = soc_opclosure_measure_closure(sc->system_b);
        sc->coupling_history[t * 2] = ca;
        sc->coupling_history[t * 2 + 1] = cb;
        sc->hist_len++;
    }
    sc->is_conserved = (sc->hist_len > 0);
}

bool soc_coupling_is_conserved(const SOCStructuralCoupling* sc) {
    return sc ? sc->is_conserved : false;
}

double soc_coupling_drift(const SOCStructuralCoupling* sc) {
    if (!sc || sc->hist_len < 2) return 0.0;
    /* Drift = change in closure over time */
    double early = sc->coupling_history[0];
    double late = sc->coupling_history[(sc->hist_len - 1) * 2];
    return fabs(late - early);
}

void soc_coupling_print(const SOCStructuralCoupling* sc) {
    if (!sc) { printf("Coupling: (null)\n"); return; }
    printf("=== Structural Coupling ===\n");
    printf("  Strength: %.4f, Steps: %d\n", sc->coupling_strength, sc->hist_len);
    printf("  Conserved: %s, Drift: %.4f\n",
           sc->is_conserved ? "yes" : "no", soc_coupling_drift(sc));
}

/* ========================================================================
 * Constructivist Learning
 * ======================================================================== */

SOCConstructivistLearning* soc_clearning_create(SOCConstructivistAgent* agent,
                                                  int sample_dim) {
    if (!agent || sample_dim <= 0) return NULL;
    SOCConstructivistLearning* cl = (SOCConstructivistLearning*)calloc(1, sizeof(SOCConstructivistLearning));
    if (!cl) return NULL;
    cl->agent = agent;
    cl->sample_dim = sample_dim;
    cl->current_hypothesis = (double*)calloc((size_t)sample_dim, sizeof(double));
    cl->hyp_dim = sample_dim;
    cl->error_history = (double*)calloc(100, sizeof(double));
    return cl;
}

void soc_clearning_free(SOCConstructivistLearning* cl) {
    if (!cl) return;
    free(cl->training_data);
    free(cl->current_hypothesis);
    free(cl->error_history);
    free(cl);
}

void soc_clearning_add_sample(SOCConstructivistLearning* cl,
                               const double* sample, int dim) {
    if (!cl || !sample) return;
    int copy_dim = (dim < cl->sample_dim) ? dim : cl->sample_dim;
    cl->training_data = (double*)realloc(cl->training_data,
        (size_t)((cl->n_samples + 1) * cl->sample_dim) * sizeof(double));
    memcpy(cl->training_data + cl->n_samples * cl->sample_dim,
           sample, (size_t)copy_dim * sizeof(double));
    cl->n_samples++;
}

/* Train by constructivist assimilation of samples.
 * Knowledge point: Learning = constructing viable hypotheses, not finding truth.
 * Complexity: O(epochs * n_samples * sample_dim) */
void soc_clearning_train(SOCConstructivistLearning* cl, int epochs) {
    if (!cl || cl->n_samples == 0) return;
    for (int ep = 0; ep < epochs; ep++) {
        /* Simple Hebbian-like update: hypothesis = mean of samples */
        for (int d = 0; d < cl->sample_dim; d++) {
            double sum = 0.0;
            for (int s = 0; s < cl->n_samples; s++) {
                sum += cl->training_data[s * cl->sample_dim + d];
            }
            cl->current_hypothesis[d] = sum / (double)cl->n_samples;
        }
        /* Record error (distance from last sample as test) */
        if (cl->n_samples > 0 && cl->err_len < 100) {
            double* last = cl->training_data + (cl->n_samples - 1) * cl->sample_dim;
            double err = 0.0;
            for (int d = 0; d < cl->sample_dim; d++) {
                double diff = cl->current_hypothesis[d] - last[d];
                err += diff * diff;
            }
            cl->error_history[cl->err_len++] = sqrt(err);
        }
    }
    cl->learning_curve_slope = (cl->err_len >= 2)
        ? cl->error_history[cl->err_len - 1] - cl->error_history[0] : 0.0;
}

/* Evaluate hypothesis on a test sample.
 * Knowledge point: Viability, not truth, is the measure of knowledge.
 * Complexity: O(sample_dim) */
double soc_clearning_evaluate(SOCConstructivistLearning* cl,
                               const double* test_sample, int dim) {
    if (!cl || !test_sample) return 0.0;
    int d = (dim < cl->sample_dim) ? dim : cl->sample_dim;
    double err = 0.0;
    for (int i = 0; i < d; i++) {
        double diff = cl->current_hypothesis[i] - test_sample[i];
        err += diff * diff;
    }
    double rmse = sqrt(err / (double)d);
    cl->verification_score = 1.0 / (1.0 + rmse);
    if (cl->verification_score > 0.7) cl->n_verified++;
    else cl->n_falsified++;
    return cl->verification_score;
}

void soc_clearning_print(const SOCConstructivistLearning* cl) {
    if (!cl) { printf("CLearning: (null)\n"); return; }
    printf("=== Constructivist Learning ===\n");
    printf("  Samples: %d, Dim: %d\n", cl->n_samples, cl->sample_dim);
    printf("  Verified: %d, Falsified: %d\n", cl->n_verified, cl->n_falsified);
    printf("  Verification score: %.4f\n", cl->verification_score);
    printf("  Learning curve slope: %.6f\n", cl->learning_curve_slope);
}

