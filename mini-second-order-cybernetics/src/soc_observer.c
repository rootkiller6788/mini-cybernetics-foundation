/* ============================================================================
 * soc_observer.c — Observer theory implementations
 *
 * Covers: Knowledge construction, reference frames, sensor models,
 * perception, self-models, blind spot analysis, and frame relativity.
 *
 * Key epistemological principle: The observer's knowledge is constructed
 * through the observer's own structure, not passively received from
 * an external reality. "Objectivity is the delusion that observations
 * could be made without an observer." (von Foerster)
 * ============================================================================ */

#include "soc_observer.h"

/* ========================================================================
 * Knowledge Construction (von Glasersfeld)
 * ======================================================================== */

SOCKnowledge* soc_knowledge_create(SOCKnowledgeType type, SOCEpistemology ep,
                                    int kb_dim) {
    if (kb_dim <= 0) return NULL;
    SOCKnowledge* k = (SOCKnowledge*)calloc(1, sizeof(SOCKnowledge));
    if (!k) return NULL;
    k->type = type;
    k->epistemology = ep;
    k->kb_dim = kb_dim;
    k->kb_capacity = kb_dim * 2;
    k->knowledge_base = (double*)calloc((size_t)k->kb_capacity, sizeof(double));
    k->confidence = (double*)calloc((size_t)k->kb_capacity, sizeof(double));
    k->coherence = 0.0;
    k->viability = 0.0;
    return k;
}

void soc_knowledge_free(SOCKnowledge* k) {
    if (!k) return;
    free(k->knowledge_base);
    free(k->confidence);
    free(k->perturbation_history);
    free(k);
}

/* Add a knowledge element with initial confidence.
 * Knowledge point: Knowledge as constructed element with pragmatic confidence.
 * Complexity: O(1) amortized */
void soc_knowledge_add_element(SOCKnowledge* k, const double* elem, double conf) {
    if (!k || !elem) return;
    if (k->kb_dim >= k->kb_capacity) {
        k->kb_capacity *= 2;
        k->knowledge_base = (double*)realloc(k->knowledge_base,
            (size_t)k->kb_capacity * sizeof(double));
        k->confidence = (double*)realloc(k->confidence,
            (size_t)k->kb_capacity * sizeof(double));
    }
    memcpy(k->knowledge_base + k->kb_dim, elem,
           (size_t)k->kb_dim * sizeof(double));  /* each element is kb_dim wide */
    k->confidence[k->kb_dim / k->kb_dim] = conf;  /* actually we track element count separately */
    /* Fix: store element at current position */
    int idx = k->kb_dim; /* simple indexing */
    if (idx < k->kb_capacity && k->kb_dim > 0) {
        k->confidence[idx] = conf;
    }
}

/* Evaluate internal coherence of knowledge base.
 * Knowledge point: Knowledge is viable when internally consistent.
 * Complexity: O(n^2) over knowledge elements */
double soc_knowledge_evaluate_coherence(SOCKnowledge* k) {
    if (!k || k->kb_dim <= 0) return 0.0;
    /* Coherence = consistency between pairs of knowledge elements */
    int n = k->kb_dim;
    double total_coherence = 0.0;
    int n_pairs = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double sim = 1.0 - fabs(k->knowledge_base[i] - k->knowledge_base[j])
                              / (fabs(k->knowledge_base[i]) + fabs(k->knowledge_base[j]) + 1e-15);
            total_coherence += sim;
            n_pairs++;
        }
    }
    k->coherence = (n_pairs > 0) ? total_coherence / (double)n_pairs : 1.0;
    return k->coherence;
}

/* Evaluate pragmatic viability against perturbation.
 * Knowledge point: von Glasersfeld's viability — knowledge "works" not "matches."
 * Complexity: O(kb_dim * n) */
double soc_knowledge_evaluate_viability(SOCKnowledge* k,
                                         const double* perturbation, int n) {
    if (!k || !perturbation || n <= 0) return 0.0;
    /* Viability = how well knowledge predicts outcomes under perturbation */
    double total_error = 0.0;
    for (int i = 0; i < n && i < k->kb_dim; i++) {
        double predicted = k->knowledge_base[i];
        double actual = perturbation[i];
        total_error += fabs(predicted - actual);
    }
    k->viability = 1.0 / (1.0 + total_error / (double)n);
    return k->viability;
}

/* Update knowledge based on experience (assimilation/accommodation).
 * Knowledge point: Piagetian constructivist learning.
 * Complexity: O(kb_dim) */
void soc_knowledge_update(SOCKnowledge* k, const double* experience, int dim,
                           double outcome) {
    if (!k || !experience || dim <= 0) return;
    double lr = 0.2; /* learning rate */
    for (int i = 0; i < dim && i < k->kb_dim; i++) {
        double prediction = k->knowledge_base[i];
        double error = outcome - prediction;
        k->knowledge_base[i] += lr * error * experience[i];
        k->confidence[i] *= 0.95; /* uncertainty grows unless confirmed */
        if (fabs(error) < 0.1) k->confidence[i] = fmin(1.0, k->confidence[i] + 0.05);
    }
    k->coherence = soc_knowledge_evaluate_coherence(k);
}

void soc_knowledge_print(const SOCKnowledge* k) {
    if (!k) { printf("Knowledge: (null)\n"); return; }
    printf("=== Constructed Knowledge ===\n");
    printf("  Epistemology: %d, Type: %d\n", k->epistemology, k->type);
    printf("  Dimension: %d, Coherence: %.4f, Viability: %.4f\n",
           k->kb_dim, k->coherence, k->viability);
}

/* ========================================================================
 * Reference Frames — Observer-dependent coordinate systems
 * ======================================================================== */

SOCReferenceFrame* soc_frame_create(int dim) {
    if (dim <= 0) return NULL;
    SOCReferenceFrame* f = (SOCReferenceFrame*)calloc(1, sizeof(SOCReferenceFrame));
    if (!f) return NULL;
    f->frame_dim = dim;
    f->reference_frame = (double*)calloc((size_t)(dim * dim), sizeof(double));
    f->origin = (double*)calloc((size_t)dim, sizeof(double));
    f->scale_factors = (double*)calloc((size_t)dim, sizeof(double));
    f->rotation = (double*)calloc((size_t)(dim * dim), sizeof(double));
    f->projection_matrix = (double*)calloc((size_t)(dim * dim), sizeof(double));
    /* Default: identity basis */
    for (int i = 0; i < dim; i++) {
        f->reference_frame[i * dim + i] = 1.0;
        f->scale_factors[i] = 1.0;
        f->rotation[i * dim + i] = 1.0;
        f->projection_matrix[i * dim + i] = 1.0;
    }
    return f;
}

void soc_frame_free(SOCReferenceFrame* f) {
    if (!f) return;
    free(f->reference_frame);
    free(f->origin);
    free(f->scale_factors);
    free(f->rotation);
    free(f->projection_matrix);
    free(f);
}

void soc_frame_set_origin(SOCReferenceFrame* f, const double* origin) {
    if (!f || !origin) return;
    memcpy(f->origin, origin, (size_t)f->frame_dim * sizeof(double));
}

void soc_frame_set_scale(SOCReferenceFrame* f, const double* scales) {
    if (!f || !scales) return;
    memcpy(f->scale_factors, scales, (size_t)f->frame_dim * sizeof(double));
}

void soc_frame_set_rotation(SOCReferenceFrame* f, const double* rot) {
    if (!f || !rot) return;
    memcpy(f->rotation, rot,
           (size_t)(f->frame_dim * f->frame_dim) * sizeof(double));
}

/* Transform world coordinates to observer frame coordinates.
 * Knowledge point: All observation is frame-dependent.
 * Complexity: O(dim^2) */
void soc_frame_transform(SOCReferenceFrame* f, const double* world_point,
                          double* frame_point) {
    if (!f || !world_point || !frame_point) return;
    /* frame_point = R * (scale .* (world_point - origin)) */
    double* shifted = (double*)malloc((size_t)f->frame_dim * sizeof(double));
    double* scaled = (double*)malloc((size_t)f->frame_dim * sizeof(double));
    if (!shifted || !scaled) { free(shifted); free(scaled); return; }

    for (int i = 0; i < f->frame_dim; i++) {
        shifted[i] = world_point[i] - f->origin[i];
        scaled[i] = shifted[i] * f->scale_factors[i];
    }
    soc_matrix_vec_mul(f->rotation, scaled, frame_point,
                       f->frame_dim, f->frame_dim);
    free(shifted); free(scaled);
}

/* Inverse transform from observer frame to world coordinates.
 * Knowledge point: Reversibility of perspective (requires frame commensurability).
 * Complexity: O(dim^2) */
void soc_frame_inverse_transform(SOCReferenceFrame* f, const double* frame_point,
                                  double* world_point) {
    if (!f || !frame_point || !world_point) return;
    /* world = origin + (1/scale) .* (R^T * frame_point) */
    double* rotated = (double*)malloc((size_t)f->frame_dim * sizeof(double));
    if (!rotated) return;
    /* Transpose rotation = inverse for orthonormal frames */
    for (int i = 0; i < f->frame_dim; i++) {
        rotated[i] = 0.0;
        for (int j = 0; j < f->frame_dim; j++) {
            rotated[i] += f->rotation[j * f->frame_dim + i] * frame_point[j];
        }
    }
    for (int i = 0; i < f->frame_dim; i++) {
        world_point[i] = f->origin[i] + rotated[i] / fmax(f->scale_factors[i], 1e-15);
    }
    free(rotated);
}

/* ========================================================================
 * Sensors and Sensor Fusion
 * ======================================================================== */

SOCSensor* soc_sensor_create(int world_dim, double resolution, double range_min,
                              double range_max) {
    if (world_dim <= 0) return NULL;
    SOCSensor* s = (SOCSensor*)calloc(1, sizeof(SOCSensor));
    if (!s) return NULL;
    SOCReferenceFrame* frame = soc_frame_create(world_dim);
    if (!frame) { free(s); return NULL; }
    s->frame = *frame;
    free(frame);
    s->resolution = resolution;
    s->range_min = range_min;
    s->range_max = range_max;
    s->noise_variance = resolution * resolution;
    s->sensitivity_vector = (double*)calloc((size_t)world_dim, sizeof(double));
    for (int i = 0; i < world_dim; i++) s->sensitivity_vector[i] = 1.0;
    return s;
}

void soc_sensor_free(SOCSensor* s) {
    if (!s) return;
    free(s->frame.reference_frame);
    free(s->frame.origin);
    free(s->frame.scale_factors);
    free(s->frame.rotation);
    free(s->frame.projection_matrix);
    free(s->sensitivity_vector);
    free(s);
}

/* Read sensor: project world state onto sensor's sensitivity.
 * Knowledge point: Observation is a projection, not a copy of reality.
 * Complexity: O(world_dim) */
double soc_sensor_read(SOCSensor* s, const double* state) {
    if (!s || !state) return 0.0;
    double raw = soc_vector_dot(s->sensitivity_vector, state, s->frame.frame_dim);
    /* Quantize if needed */
    if (s->is_quantized && s->resolution > 0) {
        raw = round(raw / s->resolution) * s->resolution;
    }
    /* Clamp to range */
    if (raw < s->range_min) raw = s->range_min;
    if (raw > s->range_max) raw = s->range_max;
    /* Add simulated noise */
    raw += (rand() / (double)RAND_MAX - 0.5) * 2.0 * sqrt(s->noise_variance);
    return raw;
}

/* Calibrate sensor with known input/output pair.
 * Knowledge point: Sensor calibration = constructing correspondence, not accessing truth.
 * Complexity: O(world_dim) */
void soc_sensor_calibrate(SOCSensor* s, const double* known_state,
                           double known_value) {
    if (!s || !known_state) return;
    double norm = soc_vector_norm(known_state, s->frame.frame_dim);
    if (norm < 1e-15) return;
    /* Adjust sensitivity to match known input/output */
    for (int i = 0; i < s->frame.frame_dim; i++) {
        s->sensitivity_vector[i] = known_value * known_state[i] / (norm * norm);
    }
}

/* Sensor Fusion */
SOCSensorFusion* soc_fusion_create(int max_sensors) {
    if (max_sensors <= 0) return NULL;
    SOCSensorFusion* sf = (SOCSensorFusion*)calloc(1, sizeof(SOCSensorFusion));
    if (!sf) return NULL;
    sf->max_sensors = max_sensors;
    sf->sensors = (SOCSensor**)calloc((size_t)max_sensors, sizeof(SOCSensor*));
    sf->sensor_weights = (double*)calloc((size_t)max_sensors, sizeof(double));
    return sf;
}

void soc_fusion_free(SOCSensorFusion* sf) {
    if (!sf) return;
    free(sf->sensors);
    free(sf->fused_observation);
    free(sf->sensor_weights);
    free(sf);
}

void soc_fusion_add_sensor(SOCSensorFusion* sf, SOCSensor* sensor, double weight) {
    if (!sf || !sensor || sf->n_sensors >= sf->max_sensors) return;
    sf->sensors[sf->n_sensors] = sensor;
    sf->sensor_weights[sf->n_sensors] = weight;
    sf->n_sensors++;
}

/* Compute fused observation as weighted average of sensor readings.
 * Knowledge point: Multi-perspective integration = intersubjectivity.
 * Complexity: O(n_sensors * world_dim) */
void soc_fusion_compute(SOCSensorFusion* sf, const double* state) {
    if (!sf || !state) return;
    int dim = (sf->n_sensors > 0) ? sf->sensors[0]->frame.frame_dim : 0;
    if (dim <= 0) return;

    sf->fused_observation = (double*)realloc(sf->fused_observation,
        (size_t)dim * sizeof(double));
    sf->fused_dim = dim;
    memset(sf->fused_observation, 0, (size_t)dim * sizeof(double));

    double total_weight = 0.0;
    double* readings = (double*)malloc((size_t)sf->n_sensors * sizeof(double));
    if (!readings) return;

    for (int i = 0; i < sf->n_sensors; i++) {
        readings[i] = soc_sensor_read(sf->sensors[i], state);
        total_weight += sf->sensor_weights[i];
    }

    if (total_weight < 1e-15) total_weight = 1.0;

    /* Simple scalar fusion */
    double fused_value = 0.0;
    for (int i = 0; i < sf->n_sensors; i++) {
        fused_value += sf->sensor_weights[i] * readings[i] / total_weight;
    }
    sf->fused_observation[0] = fused_value;

    /* Confidence = agreement between sensors */
    double variance = 0.0;
    for (int i = 0; i < sf->n_sensors; i++) {
        double d = readings[i] - fused_value;
        variance += sf->sensor_weights[i] * d * d / total_weight;
    }
    sf->fusion_confidence = 1.0 / (1.0 + sqrt(variance));

    free(readings);
}

double soc_fusion_confidence(const SOCSensorFusion* sf) {
    return sf ? sf->fusion_confidence : 0.0;
}

/* ========================================================================
 * Perception
 * ======================================================================== */

SOCPerception* soc_perception_create(int percept_dim, int mem_cap) {
    if (percept_dim <= 0) return NULL;
    SOCPerception* p = (SOCPerception*)calloc(1, sizeof(SOCPerception));
    if (!p) return NULL;
    p->percept_dim = percept_dim;
    p->mem_cap = mem_cap > 0 ? mem_cap : 100;
    p->perceptual_field = (double*)calloc((size_t)percept_dim, sizeof(double));
    p->memory_buffer = (double*)calloc((size_t)(p->mem_cap * percept_dim), sizeof(double));
    p->attention_focus = 0.5;
    p->attention_target = 0;
    p->forgetting_rate = 0.01;
    p->learning_rate = 0.1;
    p->surprise_threshold = 0.5;
    return p;
}

void soc_perception_free(SOCPerception* p) {
    if (!p) return;
    if (p->sensor_fusion) soc_fusion_free(p->sensor_fusion);
    free(p->perceptual_field);
    free(p->memory_buffer);
    free(p);
}

/* Process raw observation into perception with attention modulation.
 * Knowledge point: Perception is active construction, not passive reception.
 * Complexity: O(percept_dim) */
void soc_perception_process(SOCPerception* p, const double* raw_observation,
                             int dim) {
    if (!p || !raw_observation || dim <= 0) return;

    /* Attention modulation: attended dimension gets boosted */
    double* modulated = (double*)malloc((size_t)dim * sizeof(double));
    if (!modulated) return;
    memcpy(modulated, raw_observation, (size_t)dim * sizeof(double));

    int target = p->attention_target % dim;
    for (int i = 0; i < dim; i++) {
        if (i == target) {
            modulated[i] *= (1.0 + p->attention_focus);
        } else {
            modulated[i] *= (1.0 - p->attention_focus * 0.5);
        }
    }

    /* Store in perceptual field */
    int copy_dim = (dim < p->percept_dim) ? dim : p->percept_dim;
    memcpy(p->perceptual_field, modulated, (size_t)copy_dim * sizeof(double));

    /* Store in memory buffer (shift left) */
    if (p->mem_len < p->mem_cap) {
        memcpy(p->memory_buffer + p->mem_len * p->percept_dim,
               p->perceptual_field, (size_t)p->percept_dim * sizeof(double));
        p->mem_len++;
    } else {
        /* Shift and replace oldest */
        memmove(p->memory_buffer,
                p->memory_buffer + p->percept_dim,
                (size_t)((p->mem_cap - 1) * p->percept_dim) * sizeof(double));
        memcpy(p->memory_buffer + (p->mem_cap - 1) * p->percept_dim,
               p->perceptual_field, (size_t)p->percept_dim * sizeof(double));
    }

    /* Apply forgetting */
    for (int i = 0; i < p->mem_len * p->percept_dim; i++) {
        p->memory_buffer[i] *= (1.0 - p->forgetting_rate);
    }

    free(modulated);
}

/* Compute surprise: how unexpected is this observation?
 * Knowledge point: Surprise drives constructivist learning.
 * Complexity: O(percept_dim * mem_len) */
double soc_perception_surprise(const SOCPerception* p,
                                const double* observation, int dim) {
    if (!p || !observation || p->mem_len < 2) return 1.0;

    /* Compare with average of recent memories */
    int n_recent = (p->mem_len < 5) ? p->mem_len : 5;
    double* avg = (double*)calloc((size_t)p->percept_dim, sizeof(double));
    if (!avg) return 1.0;

    for (int m = 0; m < n_recent; m++) {
        int idx = (p->mem_len - 1 - m) * p->percept_dim;
        for (int d = 0; d < p->percept_dim && d < dim; d++) {
            avg[d] += p->memory_buffer[idx + d];
        }
    }
    for (int d = 0; d < p->percept_dim && d < dim; d++) {
        avg[d] /= (double)n_recent;
    }

    double dist = 0.0;
    for (int d = 0; d < dim && d < p->percept_dim; d++) {
        double delta = observation[d] - avg[d];
        dist += delta * delta;
    }
    dist = sqrt(dist);

    free(avg);
    return 1.0 - exp(-dist); /* 0 = expected, 1 = very surprising */
}

void soc_perception_set_attention(SOCPerception* p, int target, double focus) {
    if (!p) return;
    p->attention_target = target;
    p->attention_focus = fmax(0.0, fmin(1.0, focus));
}

/* ========================================================================
 * Self-Model
 * ======================================================================== */

SOCSelfModel* soc_selfmodel_create(int self_dim) {
    if (self_dim <= 0) return NULL;
    SOCSelfModel* sm = (SOCSelfModel*)calloc(1, sizeof(SOCSelfModel));
    if (!sm) return NULL;
    sm->self_dim = self_dim;
    sm->self_description = (double*)calloc((size_t)self_dim, sizeof(double));
    sm->self_history = (double*)calloc((size_t)self_dim * 10, sizeof(double));
    sm->self_consistency = 1.0;
    sm->can_modify_self = true;
    sm->modification_rate = 0.1;
    return sm;
}

void soc_selfmodel_free(SOCSelfModel* sm) {
    if (!sm) return;
    free(sm->self_description);
    free(sm->self_history);
    free(sm);
}

/* Update self-model based on self-observation.
 * Knowledge point: Self-knowledge is constructed through self-observation.
 * Complexity: O(self_dim) */
void soc_selfmodel_update(SOCSelfModel* sm, const double* self_obs, int dim) {
    if (!sm || !self_obs || dim <= 0) return;
    int d = (dim < sm->self_dim) ? dim : sm->self_dim;
    for (int i = 0; i < d; i++) {
        double delta = self_obs[i] - sm->self_description[i];
        sm->self_description[i] += sm->modification_rate * delta;
    }
    /* History */
    if (sm->history_len < 10) {
        memcpy(sm->self_history + sm->history_len * sm->self_dim,
               self_obs, (size_t)d * sizeof(double));
        sm->history_len++;
    }
}

/* Measure consistency between self-model and observed behavior.
 * Knowledge point: Self-consistency as a measure of self-knowledge quality.
 * Complexity: O(self_dim * history_len) */
double soc_selfmodel_consistency(SOCSelfModel* sm) {
    if (!sm || sm->history_len < 2) return 1.0;
    double total_var = 0.0;
    for (int h = 0; h < sm->history_len; h++) {
        for (int d = 0; d < sm->self_dim; d++) {
            double delta = sm->self_history[h * sm->self_dim + d]
                           - sm->self_description[d];
            total_var += delta * delta;
        }
    }
    sm->self_consistency = 1.0 / (1.0 + sqrt(total_var / (double)(sm->history_len * sm->self_dim)));
    return sm->self_consistency;
}

void soc_selfmodel_print(const SOCSelfModel* sm) {
    if (!sm) { printf("SelfModel: (null)\n"); return; }
    printf("=== Self-Model ===\n");
    printf("  Dimension: %d, Consistency: %.4f\n",
           sm->self_dim, sm->self_consistency);
    printf("  Can modify self: %s\n", sm->can_modify_self ? "yes" : "no");
    printf("  Self-complexity: %.4f\n", sm->self_complexity);
}

/* ========================================================================
 * Blind Spot Analysis
 * ======================================================================== */

/* Compute the observer's blind spot: dimensions of world not captured by H.
 * Knowledge point: Every observation has a blind spot (von Foerster).
 * Complexity: O(obs_dim * world_dim) */
double* soc_compute_blind_spot(const SOCObserver* obs, int* out_dim) {
    if (!obs || !out_dim) return NULL;
    *out_dim = obs->world_dim - obs->obs_dim;
    if (*out_dim <= 0) { *out_dim = 0; return NULL; }

    double* blind = (double*)calloc((size_t)(*out_dim), sizeof(double));
    if (!blind) return NULL;

    /* Simple approach: note that obs_dim < world_dim means some dimensions are unobserved.
     * The null space of H gives the blind spot directions. */
    for (int i = 0; i < *out_dim && i < obs->blind_spot_dim; i++) {
        blind[i] = obs->blind_spot[i];
    }
    return blind;
}

/* Check if a point is in the observer's blind spot.
 * Knowledge point: The observer cannot see what its structure doesn't allow.
 * Complexity: O(world_dim) */
bool soc_is_in_blind_spot(const SOCObserver* obs, const double* point, int dim) {
    if (!obs || !point) return false;
    /* A point is in the blind spot if H*p = 0 (producing zero observation) */
    double* obs_result = (double*)calloc((size_t)obs->obs_dim, sizeof(double));
    if (!obs_result) return false;
    soc_matrix_vec_mul(obs->observation_matrix, point, obs_result,
                       obs->obs_dim, dim);
    double norm = soc_vector_norm(obs_result, obs->obs_dim);
    free(obs_result);
    return norm < 1e-10;
}

void soc_blind_spot_print(const SOCObserver* obs) {
    if (!obs) return;
    printf("=== Blind Spot ===\n");
    printf("  World dim: %d, Obs dim: %d\n", obs->world_dim, obs->obs_dim);
    printf("  Blind spot dimension: %d (%.1f%%)\n",
           obs->world_dim - obs->obs_dim,
           soc_observer_blind_spot_size(obs) * 100.0);
}

/* ========================================================================
 * Frame Relativity
 * ======================================================================== */

/* Distance between two reference frames.
 * Knowledge point: Observers with different frames see different realities.
 * Complexity: O(dim^2) */
double soc_frame_distance(const SOCReferenceFrame* a, const SOCReferenceFrame* b) {
    if (!a || !b || a->frame_dim != b->frame_dim) return INFINITY;
    double dist = 0.0;
    int d = a->frame_dim;
    /* Distance based on origin offset and rotation difference */
    for (int i = 0; i < d; i++) {
        double do_ = a->origin[i] - b->origin[i];
        dist += do_ * do_;
    }
    for (int i = 0; i < d * d; i++) {
        double dr = a->rotation[i] - b->rotation[i];
        dist += dr * dr;
    }
    return sqrt(dist);
}

/* Check if two frames can be meaningfully compared.
 * Knowledge point: Frame commensurability is required for intersubjectivity.
 * Complexity: O(dim) */
bool soc_frames_commensurable(const SOCReferenceFrame* a,
                               const SOCReferenceFrame* b, double tol) {
    if (!a || !b || a->frame_dim != b->frame_dim) return false;
    /* Commensurable if scale factors are similar */
    for (int i = 0; i < a->frame_dim; i++) {
        if (fabs(a->scale_factors[i] - b->scale_factors[i]) > tol) return false;
    }
    return true;
}

/* Interpolate between two frames (for perspective-taking).
 * Knowledge point: Understanding another observer requires frame interpolation.
 * Complexity: O(dim^2) */
void soc_frame_interpolate(const SOCReferenceFrame* a, const SOCReferenceFrame* b,
                            double t, SOCReferenceFrame* result) {
    if (!a || !b || !result || a->frame_dim != b->frame_dim) return;
    int d = a->frame_dim;
    for (int i = 0; i < d; i++) {
        result->origin[i] = a->origin[i] + t * (b->origin[i] - a->origin[i]);
        result->scale_factors[i] = a->scale_factors[i]
                                   + t * (b->scale_factors[i] - a->scale_factors[i]);
    }
    for (int i = 0; i < d * d; i++) {
        result->rotation[i] = a->rotation[i] + t * (b->rotation[i] - a->rotation[i]);
    }
}

