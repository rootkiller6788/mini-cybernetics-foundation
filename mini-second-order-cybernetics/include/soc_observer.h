#ifndef SOC_OBSERVER_H
#define SOC_OBSERVER_H

#include "soc_core.h"

/* ============================================================================
 * Second-Order Cybernetics — Observer Theory
 *
 * The observer is the central concept of second-order cybernetics.
 * This header extends soc_core.h with detailed observer models,
 * observation functions, frame-of-reference analysis, and the
 * constructivist epistemology of von Glasersfeld.
 *
 * Key references:
 *   von Foerster, H. (1981). Observing Systems.
 *   Maturana, H.R. & Varela, F.J. (1987). The Tree of Knowledge.
 *   von Glasersfeld, E. (1995). Radical Constructivism.
 *   Luhmann, N. (1984/1995). Social Systems.
 * ============================================================================ */

/* ---- Constructivist Knowledge Model ---- */

typedef enum {
    SOC_KNOWLEDGE_EMPIRICAL = 0,
    SOC_KNOWLEDGE_CONSTRUCTED = 1,
    SOC_KNOWLEDGE_CONSENSUAL = 2,
    SOC_KNOWLEDGE_OPERATIONAL = 3,
    SOC_KNOWLEDGE_EIGENFORM = 4
} SOCKnowledgeType;

typedef enum {
    SOC_REALISM_NAIVE = 0,
    SOC_REALISM_CRITICAL = 1,
    SOC_CONSTRUCTIVISM_RADICAL = 2,
    SOC_CONSTRUCTIVISM_SOCIAL = 3,
    SOC_SOLIPSISM = 4
} SOCEpistemology;

typedef struct {
    SOCKnowledgeType type;
    SOCEpistemology epistemology;
    double* knowledge_base;
    int kb_dim;
    int kb_capacity;
    double* confidence;         /* confidence in each knowledge element */
    double coherence;           /* internal consistency of knowledge */
    double viability;           /* pragmatic viability (von Glasersfeld) */
    double* perturbation_history;
    int pert_len;
} SOCKnowledge;

typedef struct {
    double* reference_frame;    /* basis vectors defining the observer's frame */
    int frame_dim;
    double* origin;
    double* scale_factors;
    double* rotation;           /* rotation matrix relative to canonical frame */
    double* projection_matrix;  /* from world frame to observer frame */
} SOCReferenceFrame;

typedef struct {
    SOCReferenceFrame frame;
    double (*observation_function)(const double* state, int dim, void* ctx);
    void* obs_ctx;
    double noise_variance;
    double resolution;          /* minimum detectable difference */
    double range_min;
    double range_max;
    bool is_quantized;
    int n_quantization_levels;
    double* sensitivity_vector; /* which dimensions the sensor is sensitive to */
} SOCSensor;

typedef struct {
    SOCSensor** sensors;
    int n_sensors;
    int max_sensors;
    double* fused_observation;
    int fused_dim;
    double fusion_confidence;
    double* sensor_weights;
} SOCSensorFusion;

/* ---- Observer Perception Model ---- */

typedef struct {
    SOCReferenceFrame frame;
    SOCSensorFusion* sensor_fusion;
    double* perceptual_field;   /* what the observer currently perceives */
    int percept_dim;
    double attention_focus;     /* 0 = diffuse, 1 = fully focused */
    int attention_target;       /* index of attended dimension */
    double* memory_buffer;      /* past perceptions */
    int mem_len;
    int mem_cap;
    double forgetting_rate;     /* exponential decay of memories */
    double learning_rate;       /* how fast beliefs update */
    double surprise_threshold;  /* triggers belief revision when exceeded */
} SOCPerception;

/* ---- Observer Self-Model ---- */

typedef struct {
    double* self_description;   /* how the observer models itself */
    int self_dim;
    double self_consistency;    /* agreement between self-model and behavior */
    double* self_history;       /* record of self-observations */
    int history_len;
    double self_complexity;     /* estimated complexity of self */
    bool can_modify_self;       /* can the observer change its own structure? */
    double modification_rate;
} SOCSelfModel;

/* ---- API: Knowledge ---- */

SOCKnowledge* soc_knowledge_create(SOCKnowledgeType type, SOCEpistemology ep,
                                    int kb_dim);
void soc_knowledge_free(SOCKnowledge* k);
void soc_knowledge_add_element(SOCKnowledge* k, const double* elem, double conf);
double soc_knowledge_evaluate_coherence(SOCKnowledge* k);
double soc_knowledge_evaluate_viability(SOCKnowledge* k,
                                         const double* perturbation, int n);
void soc_knowledge_update(SOCKnowledge* k, const double* experience, int dim,
                           double outcome);
void soc_knowledge_print(const SOCKnowledge* k);

/* ---- API: Reference Frame ---- */

SOCReferenceFrame* soc_frame_create(int dim);
void soc_frame_free(SOCReferenceFrame* f);
void soc_frame_set_origin(SOCReferenceFrame* f, const double* origin);
void soc_frame_set_scale(SOCReferenceFrame* f, const double* scales);
void soc_frame_set_rotation(SOCReferenceFrame* f, const double* rot);
void soc_frame_transform(SOCReferenceFrame* f, const double* world_point,
                          double* frame_point);
void soc_frame_inverse_transform(SOCReferenceFrame* f, const double* frame_point,
                                  double* world_point);

/* ---- API: Sensor and Fusion ---- */

SOCSensor* soc_sensor_create(int world_dim, double resolution, double range_min,
                              double range_max);
void soc_sensor_free(SOCSensor* s);
double soc_sensor_read(SOCSensor* s, const double* state);
void soc_sensor_calibrate(SOCSensor* s, const double* known_state,
                           double known_value);

SOCSensorFusion* soc_fusion_create(int max_sensors);
void soc_fusion_free(SOCSensorFusion* sf);
void soc_fusion_add_sensor(SOCSensorFusion* sf, SOCSensor* sensor, double weight);
void soc_fusion_compute(SOCSensorFusion* sf, const double* state);
double soc_fusion_confidence(const SOCSensorFusion* sf);

/* ---- API: Perception ---- */

SOCPerception* soc_perception_create(int percept_dim, int mem_cap);
void soc_perception_free(SOCPerception* p);
void soc_perception_process(SOCPerception* p, const double* raw_observation,
                             int dim);
double soc_perception_surprise(const SOCPerception* p,
                                const double* observation, int dim);
void soc_perception_set_attention(SOCPerception* p, int target, double focus);

/* ---- API: Self-Model ---- */

SOCSelfModel* soc_selfmodel_create(int self_dim);
void soc_selfmodel_free(SOCSelfModel* sm);
void soc_selfmodel_update(SOCSelfModel* sm, const double* self_obs, int dim);
double soc_selfmodel_consistency(SOCSelfModel* sm);
void soc_selfmodel_print(const SOCSelfModel* sm);

/* ---- Blind Spot Analysis ---- */

double* soc_compute_blind_spot(const SOCObserver* obs, int* out_dim);
bool soc_is_in_blind_spot(const SOCObserver* obs, const double* point, int dim);
void soc_blind_spot_print(const SOCObserver* obs);

/* ---- Frame Relativity ---- */

double soc_frame_distance(const SOCReferenceFrame* a, const SOCReferenceFrame* b);
bool soc_frames_commensurable(const SOCReferenceFrame* a,
                               const SOCReferenceFrame* b, double tol);
void soc_frame_interpolate(const SOCReferenceFrame* a, const SOCReferenceFrame* b,
                            double t, SOCReferenceFrame* result);

#endif /* SOC_OBSERVER_H */
