#ifndef SOC_CONSTRUCTIVISM_H
#define SOC_CONSTRUCTIVISM_H

#include "soc_core.h"
#include "soc_observer.h"

/* ============================================================================
 * Second-Order Cybernetics — Constructivist Epistemology
 *
 * Radical constructivism (von Glasersfeld): Knowledge is not passively
 * received but actively built up by the cognizing subject. The function
 * of cognition is adaptive and serves the organization of the experiential
 * world, not the discovery of ontological reality.
 *
 * Key principles:
 *   1. Knowledge is not received passively but actively constructed
 *   2. Cognition serves adaptation, not truth
 *   3. Knowledge is viable (works) not true (matches reality)
 *   4. The environment is a black box — we only know perturbations
 *
 * Key references:
 *   von Glasersfeld, E. (1995). Radical Constructivism: A Way of Knowing and Learning.
 *   Maturana, H.R. (1978). Biology of Language: The Epistemology of Reality.
 *   Varela, F.J., Thompson, E., & Rosch, E. (1991). The Embodied Mind.
 *   Piaget, J. (1937/1954). The Construction of Reality in the Child.
 * ============================================================================ */

/* ---- Constructivist Knowledge Structures ---- */

typedef struct {
    double* concept_vector;
    int dim;
    double* operational_meaning; /* meaning = set of operations that produce it */
    int op_dim;
    double viability_score;
    int n_confirmations;
    int n_refutations;
    double* perturbation_signature;
    int sig_len;
} SOCConstructedConcept;

typedef struct {
    SOCConstructedConcept** concepts;
    int n_concepts;
    int max_concepts;
    double* coherence_matrix;    /* how concepts support/contradict each other */
    int matrix_dim;
    double overall_coherence;
    double overall_viability;
    double complexity;
} SOCConceptualNetwork;

typedef struct {
    SOCConceptualNetwork* network;
    double* experience_buffer;
    int exp_len;
    int exp_cap;
    double assimilation_rate;    /* Piaget: fitting new into existing schemas */
    double accommodation_rate;   /* Piaget: modifying schemas to fit new */
    double equilibration_target; /* balance between assimilation and accommodation */
    int n_adaptations;
    double adaptation_history_sum;
} SOCConstructivistAgent;

/* ---- Enaction Theory (Varela, Thompson, Rosch) ---- */

typedef enum {
    SOC_ENACTION_PERCEPTION = 0,
    SOC_ENACTION_ACTION = 1,
    SOC_ENACTION_SENSORIMOTOR = 2,
    SOC_ENACTION_COUPLING = 3
} SOCEnactiveMode;

typedef struct {
    SOCEnactiveMode mode;
    double* sensory_input;
    int sensory_dim;
    double* motor_output;
    int motor_dim;
    double* world_state;
    int world_dim;
    double sensorimotor_correlation;
    double* coupling_matrix;     /* how sensory and motor are coupled */
    double structural_coupling;  /* Maturana: history of mutual perturbation */
    bool has_enacted_world;      /* has the agent brought forth a world? */
} SOCEnactiveLoop;

/* ---- Social Construction (Berger & Luckmann, Gergen) ---- */

typedef struct {
    char* social_group;
    SOCConceptualNetwork* shared_concepts;
    double** member_beliefs;
    int n_members;
    int concept_dim;
    double consensus_level;
    double* institutionalization; /* degree to which concepts are taken for granted */
    int n_institutions;
    double* legitimization;
    double reification_level;    /* degree to which social constructs seem "natural" */
} SOCSocialConstruction;

/* ---- Operational Closure (Maturana & Varela) ---- */

typedef struct {
    double* internal_state;
    int state_dim;
    double* perturbation_channel;
    int pert_dim;
    double (*internal_dynamics)(const double* state, int dim,
                                 const double* perturbation, int p_dim,
                                 double* new_state, void* ctx);
    void* dyn_ctx;
    bool is_operationally_closed;
    double closure_index;        /* 0 = fully determined by environment, 1 = self-determined */
    double* state_history;
    int hist_len;
    int hist_cap;
} SOCOperationalClosure;

/* ---- Structural Coupling ---- */

typedef struct {
    SOCOperationalClosure* system_a;
    SOCOperationalClosure* system_b;
    double coupling_strength;    /* intensity of mutual perturbation */
    double* coupling_history;
    int hist_len;
    bool is_conserved;           /* does coupling preserve organization? */
    double perturbation_tolerance;
    bool has_structural_drift;   /* Maturana: change without loss of organization */
    double drift_rate;
} SOCStructuralCoupling;

/* ---- Constructivist Learning ---- */

typedef struct {
    SOCConstructivistAgent* agent;
    double* training_data;
    int n_samples;
    int sample_dim;
    double* current_hypothesis;
    int hyp_dim;
    double verification_score;
    int n_verified;
    int n_falsified;
    double learning_curve_slope;
    double* error_history;
    int err_len;
} SOCConstructivistLearning;

/* ---- API: Constructed Concepts ---- */

SOCConstructedConcept* soc_concept_create(int dim, int op_dim);
void soc_concept_free(SOCConstructedConcept* c);
void soc_concept_set_operational_meaning(SOCConstructedConcept* c,
                                          const double* operations, int n);
double soc_concept_viability_test(SOCConstructedConcept* c,
                                   const double* perturbation, int dim);
bool soc_concept_confirm(SOCConstructedConcept* c, bool successful);
void soc_concept_print(const SOCConstructedConcept* c);

/* ---- API: Conceptual Network ---- */

SOCConceptualNetwork* soc_conceptual_network_create(int max_concepts);
void soc_conceptual_network_free(SOCConceptualNetwork* cn);
void soc_conceptual_network_add_concept(SOCConceptualNetwork* cn,
                                         SOCConstructedConcept* concept);
void soc_conceptual_network_compute_coherence(SOCConceptualNetwork* cn);
double soc_conceptual_network_mean_viability(const SOCConceptualNetwork* cn);
void soc_conceptual_network_prune(SOCConceptualNetwork* cn, double threshold);
void soc_conceptual_network_print(const SOCConceptualNetwork* cn);

/* ---- API: Constructivist Agent ---- */

SOCConstructivistAgent* soc_agent_create(int exp_cap, int concept_dim);
void soc_agent_free(SOCConstructivistAgent* a);
void soc_agent_experience(SOCConstructivistAgent* a, const double* exp, int dim);
void soc_agent_adapt(SOCConstructivistAgent* a);
double soc_agent_assimilate(SOCConstructivistAgent* a, const double* experience,
                              int dim);
double soc_agent_accommodate(SOCConstructivistAgent* a, const double* experience,
                               int dim);
void soc_agent_print(const SOCConstructivistAgent* a);

/* ---- API: Enactive Loop ---- */

SOCEnactiveLoop* soc_enactive_create(int sensory_dim, int motor_dim,
                                      int world_dim);
void soc_enactive_free(SOCEnactiveLoop* el);
void soc_enactive_sense(SOCEnactiveLoop* el, const double* world);
void soc_enactive_act(SOCEnactiveLoop* el, double* motor_cmd);
void soc_enactive_couple(SOCEnactiveLoop* el, int n_steps);
double soc_enactive_correlation(const SOCEnactiveLoop* el);
void soc_enactive_print(const SOCEnactiveLoop* el);

/* ---- API: Social Construction ---- */

SOCSocialConstruction* soc_social_create(const char* group, int n_members,
                                          int concept_dim);
void soc_social_free(SOCSocialConstruction* sc);
void soc_social_set_member_belief(SOCSocialConstruction* sc, int member,
                                   const double* beliefs);
double soc_social_measure_consensus(SOCSocialConstruction* sc);
void soc_social_institutionalize(SOCSocialConstruction* sc, int concept_idx);
double soc_social_reification_level(SOCSocialConstruction* sc);
void soc_social_print(const SOCSocialConstruction* sc);

/* ---- API: Operational Closure ---- */

SOCOperationalClosure* soc_opclosure_create(int state_dim, int pert_dim,
                                              double (*dyn)(const double*, int,
                                                             const double*, int,
                                                             double*, void*),
                                              void* ctx);
void soc_opclosure_free(SOCOperationalClosure* oc);
void soc_opclosure_perturb(SOCOperationalClosure* oc, const double* pert);
void soc_opclosure_step(SOCOperationalClosure* oc);
double soc_opclosure_measure_closure(SOCOperationalClosure* oc);
bool soc_opclosure_is_autopoietic(const SOCOperationalClosure* oc);
void soc_opclosure_print(const SOCOperationalClosure* oc);

/* ---- API: Structural Coupling ---- */

SOCStructuralCoupling* soc_coupling_create(SOCOperationalClosure* a,
                                            SOCOperationalClosure* b,
                                            double strength);
void soc_coupling_free(SOCStructuralCoupling* sc);
void soc_coupling_evolve(SOCStructuralCoupling* sc, int n_steps);
bool soc_coupling_is_conserved(const SOCStructuralCoupling* sc);
double soc_coupling_drift(const SOCStructuralCoupling* sc);
void soc_coupling_print(const SOCStructuralCoupling* sc);

/* ---- API: Constructivist Learning ---- */

SOCConstructivistLearning* soc_clearning_create(SOCConstructivistAgent* agent,
                                                  int sample_dim);
void soc_clearning_free(SOCConstructivistLearning* cl);
void soc_clearning_add_sample(SOCConstructivistLearning* cl,
                               const double* sample, int dim);
void soc_clearning_train(SOCConstructivistLearning* cl, int epochs);
double soc_clearning_evaluate(SOCConstructivistLearning* cl,
                               const double* test_sample, int dim);
void soc_clearning_print(const SOCConstructivistLearning* cl);

#endif /* SOC_CONSTRUCTIVISM_H */
