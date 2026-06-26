#ifndef CAS_AGENT_H
#define CAS_AGENT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* =========================================================================
 * Complex Adaptive System -- Agent Definitions (L1-L2)
 *
 * Holland's framework: Agent, Internal Model, Building Block, Tag, Fitness
 * Reference: Holland (1995) Hidden Order, Miller & Page (2007)
 * Course: MIT 6.883, Stanford MS&E 338, SFI Complexity Explorer
 * ========================================================================= */

#define CAS_AGENT_STATE_DIM_MAX        16
#define CAS_AGENT_MAX_SENSORS          16
#define CAS_AGENT_MAX_EFFECTORS        16
#define CAS_AGENT_MAX_TAGS             8
#define CAS_TAG_STR_MAX                32
#define CAS_AGENT_MAX_BUILDING_BLOCKS  32
#define CAS_AGENT_MAX_NEIGHBORS        32
#define CAS_AGENT_HISTORY_LEN          20
#define CAS_AGENT_NAME_MAX             64

/* L1: Agent type spectrum */
typedef enum {
    CAS_AGENT_REACTIVE     = 0,
    CAS_AGENT_LEARNING     = 1,
    CAS_AGENT_DELIBERATIVE = 2,
    CAS_AGENT_COEVOLVING   = 3,
    CAS_AGENT_META         = 4
} cas_agent_type_t;

/* L1: Adaptation strategy */
typedef enum {
    CAS_ADAPT_NONE         = 0,
    CAS_ADAPT_HILL_CLIMB   = 1,
    CAS_ADAPT_REINFORCE    = 2,
    CAS_ADAPT_EVOLUTIONARY = 3,
    CAS_ADAPT_BAYESIAN     = 4,
    CAS_ADAPT_GRADIENT     = 5
} cas_adapt_strategy_t;

/* L1: Building Block -- Holland's reusable component */
typedef struct {
    char     name[CAS_TAG_STR_MAX];
    double   strength;
    double   specificity;
    uint32_t use_count;
    uint32_t generation;
    uint8_t  condition[4];
    uint8_t  action[4];
    double   bid;
    double   last_reward;
} cas_building_block_t;

/* L1: Tag -- agent identification and grouping */
typedef struct {
    char     label[CAS_TAG_STR_MAX];
    double   strength;
    double   tolerance;
    uint32_t group_id;
    bool     active;
    uint32_t created_at;
} cas_agent_tag_t;

/* L1: Internal Model -- agent's representation of environment */
typedef struct {
    double   state[CAS_AGENT_STATE_DIM_MAX];
    double   confidence[CAS_AGENT_STATE_DIM_MAX];
    double   prediction[CAS_AGENT_STATE_DIM_MAX];
    double   reward_model[CAS_AGENT_STATE_DIM_MAX];
    double   gradient[CAS_AGENT_STATE_DIM_MAX];
    uint32_t state_dim;
    uint32_t update_count;
    double   learning_rate;
    double   discount_factor;
    double   prediction_error;
    double   moving_error;
    bool     initialized;
} cas_internal_model_t;

/* L1: Sensor -- input channel */
typedef struct {
    uint32_t source_id;
    double   reading;
    double   noise_std;
    double   calibration_offset;
    double   range_min;
    double   range_max;
    double   sensitivity;
    bool     active;
    double   history[16];
    uint32_t history_idx;
} cas_sensor_t;

/* L1: Effector -- output channel */
typedef struct {
    uint32_t target_id;
    double   output;
    double   saturation_min;
    double   saturation_max;
    double   energy_cost;
    double   precision;
    bool     active;
    double   wear;
} cas_effector_t;

/* L2: Complete CAS Agent -- unifies all L1 elements */
typedef struct {
    uint32_t             id;
    cas_agent_type_t     type;
    char                 name[CAS_AGENT_NAME_MAX];
    cas_agent_tag_t      tags[CAS_AGENT_MAX_TAGS];
    uint32_t             num_tags;
    uint32_t             group_id;
    cas_internal_model_t model;
    cas_sensor_t         sensors[CAS_AGENT_MAX_SENSORS];
    uint32_t             num_sensors;
    cas_effector_t       effectors[CAS_AGENT_MAX_EFFECTORS];
    uint32_t             num_effectors;
    double               fitness;
    double               fitness_history[CAS_AGENT_HISTORY_LEN];
    uint32_t             fitness_history_len;
    double               energy;
    double               max_energy;
    cas_adapt_strategy_t adapt_strategy;
    double               mutation_rate;
    double               temperature;
    double               adaptation_rate;
    cas_building_block_t building_blocks[CAS_AGENT_MAX_BUILDING_BLOCKS];
    uint32_t             num_building_blocks;
    double               reward_history[CAS_AGENT_HISTORY_LEN];
    uint32_t             reward_history_len;
    uint32_t             age;
    double               cumulative_reward;
    double               average_reward;
    double               x, y;
    double               vx, vy;
    uint32_t             neighborhood[CAS_AGENT_MAX_NEIGHBORS];
    uint32_t             num_neighbors;
    double               centrality;
    double               betweenness;
    double               genotypic_id[8];
} cas_agent_t;

/* ===== L5: Agent Lifecycle API ===== */
void cas_agent_init(cas_agent_t *agent, uint32_t id,
                    cas_agent_type_t type, uint32_t state_dim);
void cas_agent_destroy(cas_agent_t *agent);
void cas_agent_sense(cas_agent_t *agent, const double *world_state,
                     uint32_t world_dim);
uint32_t cas_agent_decide(cas_agent_t *agent, double *actions,
                          uint32_t max_actions);
void cas_agent_act(cas_agent_t *agent, const double *actions,
                   double *world_state, uint32_t world_dim);
void cas_agent_adapt(cas_agent_t *agent, double reward);
void cas_agent_step(cas_agent_t *agent, double *world_state,
                    uint32_t world_dim, double reward);
void cas_agent_interact(cas_agent_t *a, cas_agent_t *b);
double cas_agent_similarity(const cas_agent_t *a, const cas_agent_t *b);
bool cas_agent_add_tag(cas_agent_t *agent, const char *label,
                       double tolerance);
double cas_agent_has_tag(const cas_agent_t *agent, const char *label);
bool cas_agent_add_building_block(cas_agent_t *agent, const char *name,
                                  const uint8_t condition[4],
                                  const uint8_t action[4]);
int32_t cas_agent_match_block(const cas_agent_t *agent,
                              const uint8_t condition[4]);
double cas_agent_compute_fitness(const cas_agent_t *agent,
                                 const double *environment, uint32_t env_dim);
uint32_t cas_agent_tournament_select(cas_agent_t **agents,
                                     uint32_t n_agents, uint32_t k);
uint32_t cas_agent_roulette_select(cas_agent_t **agents,
                                   uint32_t n_agents, double rng);

#endif /* CAS_AGENT_H */
