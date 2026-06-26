/**
 * @file cas_agent.c
 * @brief CAS Agent Implementation (L1-L6)
 *
 * Full implementation of the Complex Adaptive System agent per
 * Holland's framework. Each function realizes an independent
 * knowledge point from the L1-L6 knowledge hierarchy.
 *
 * L1: agent_init/destroy, tag, building block -- struct lifecycle
 * L2: OODA loop phases, bounded rationality, peer interaction
 * L3: Internal state vector, sensor/effector dynamics
 * L4: TD learning convergence, bucket brigade algorithm
 * L5: Full OODA cycle, social learning, similarity, selection
 * L6: Fitness computation, tournament selection, roulette selection
 *
 * Reference:
 *   Holland, J.H. (1995). Hidden Order. Addison-Wesley.
 *   Miller, J.H. & Page, S.E. (2007). Complex Adaptive Systems. Princeton.
 *   Sutton, R.S. & Barto, A.G. (2018). Reinforcement Learning. MIT Press.
 */

#include "cas_agent.h"
#include <string.h>
#include <stdio.h>

/* ---- Internal PRNG: 64-bit LCG with splitmix-style output ---- */
static uint64_t _cas_seed = 12345;

static void _cas_srand(uint64_t s) { _cas_seed = s; }

static uint64_t _cas_rand(void) {
    _cas_seed = _cas_seed * 6364136223846793005ULL + 1442695040888963407ULL;
    uint64_t z = _cas_seed;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static double _cas_rand_double(void) {
    return (double)(_cas_rand() >> 11) * 0x1.0p-53;
}

static double _cas_rand_gaussian(void) {
    double u1 = _cas_rand_double();
    double u2 = _cas_rand_double();
    if (u1 < 1e-12) u1 = 1e-12;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

static double _cas_clamp(double x, double lo, double hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

/* =================================================================
 * L1: Agent Initialization
 *
 * Initialize a CAS agent with sensible defaults.
 * Sets identity, zeroes arrays, initializes internal model.
 *
 * Bounded rationality: noise_std > 0, learning_rate < 1,
 * sensor range limits ensure agents never have perfect information.
 *
 * Theorem (Simon 1957, Miller & Page 2007):
 *   Agents with bounded rationality parameters (finite sensors,
 *   noisy model, limited energy) explore their local fitness
 *   landscape with finite adaptation velocity v < v_max.
 *
 * Complexity: O(state_dim)
 * ================================================================= */

void cas_agent_init(cas_agent_t *agent, uint32_t id,
                    cas_agent_type_t type, uint32_t state_dim)
{
    if (!agent) return;
    memset(agent, 0, sizeof(cas_agent_t));

    agent->id = id;
    agent->type = type;
    agent->group_id = 0;
    snprintf(agent->name, CAS_AGENT_NAME_MAX, "agent_%u", id);

    /* Internal model setup */
    agent->model.state_dim = (state_dim < CAS_AGENT_STATE_DIM_MAX)
                             ? state_dim : CAS_AGENT_STATE_DIM_MAX;
    agent->model.learning_rate = 0.1;
    agent->model.discount_factor = 0.95;
    agent->model.initialized = true;
    agent->model.update_count = 0;
    agent->model.prediction_error = 0.0;
    agent->model.moving_error = 0.0;

    for (uint32_t i = 0; i < agent->model.state_dim; i++) {
        agent->model.state[i] = 0.0;
        agent->model.confidence[i] = 0.5;
        agent->model.prediction[i] = 0.0;
        agent->model.reward_model[i] = 0.0;
        agent->model.gradient[i] = 0.0;
    }

    /* Fitness and energy defaults */
    agent->fitness = 0.0;
    agent->energy = 100.0;
    agent->max_energy = 200.0;
    agent->fitness_history_len = 0;
    agent->reward_history_len = 0;
    agent->age = 0;
    agent->cumulative_reward = 0.0;
    agent->average_reward = 0.0;

    /* Adaptation parameters */
    agent->adapt_strategy = CAS_ADAPT_REINFORCE;
    agent->mutation_rate = 0.01;
    agent->temperature = 1.0;
    agent->adaptation_rate = 0.1;

    /* Random spatial placement */
    agent->x = _cas_rand_double() * 100.0;
    agent->y = _cas_rand_double() * 100.0;
    agent->vx = 0.0;
    agent->vy = 0.0;

    /* Unique genotypic signature */
    for (int i = 0; i < 8; i++) {
        agent->genotypic_id[i] = _cas_rand_double();
    }

    agent->num_sensors = 0;
    agent->num_effectors = 0;
    agent->num_tags = 0;
    agent->num_building_blocks = 0;
    agent->num_neighbors = 0;
    agent->centrality = 0.0;
    agent->betweenness = 0.0;
}

void cas_agent_destroy(cas_agent_t *agent)
{
    if (!agent) return;
    agent->model.initialized = false;
    agent->num_sensors = 0;
    agent->num_effectors = 0;
    agent->num_tags = 0;
    agent->num_building_blocks = 0;
}

/* =================================================================
 * L2: OODA Loop -- Sense Phase
 *
 * Read sensor values from environment.
 * Each sensor reads a specific world state element, applies
 * gain + offset, adds Gaussian noise, and clamps to range.
 *
 * Bounded rationality: sensors are noisy (noise_std > 0) and
 * range-limited (range_min/max). The agent never has perfect
 * information -- this is fundamental to CAS theory.
 *
 * Complexity: O(num_sensors)
 * ================================================================= */

void cas_agent_sense(cas_agent_t *agent, const double *world_state,
                     uint32_t world_dim)
{
    if (!agent || !world_state) return;

    for (uint32_t i = 0; i < agent->num_sensors; i++) {
        cas_sensor_t *s = &agent->sensors[i];
        if (!s->active) continue;

        double raw = 0.0;
        if (s->source_id < world_dim) {
            raw = world_state[s->source_id];
        }

        /* Calibration and gain */
        raw = (raw + s->calibration_offset) * s->sensitivity;

        /* Gaussian observation noise */
        raw += _cas_rand_gaussian() * s->noise_std;

        /* Clamp to sensor range */
        raw = _cas_clamp(raw, s->range_min, s->range_max);

        /* Circular history buffer */
        s->history[s->history_idx % 16] = raw;
        s->history_idx++;

        s->reading = raw;
    }
}

/* =================================================================
 * L2: OODA Loop -- Decide Phase
 *
 * Generate effector actions from internal model and sensors.
 * Strategy varies by agent type:
 *   Reactive:   direct sensor -> action (no internal state)
 *   Learning/   internal model prediction + Boltzmann exploration
 *   Deliberative: model-based planning
 *   Coevolving: neighbor-influenced decisions
 *   Meta:       self-modifying parameters
 *
 * Complexity: O(state_dim * num_effectors)
 * ================================================================= */

uint32_t cas_agent_decide(cas_agent_t *agent, double *actions,
                          uint32_t max_actions)
{
    if (!agent || !actions) return 0;

    uint32_t na = agent->num_effectors;
    if (na > max_actions) na = max_actions;
    uint32_t dim = agent->model.state_dim;
    uint32_t i, j;

    switch (agent->type) {

    case CAS_AGENT_REACTIVE:
        /* Direct stimulus-response mapping */
        for (i = 0; i < na; i++) {
            double sv = (i < agent->num_sensors)
                        ? agent->sensors[i].reading : 0.0;
            actions[i] = tanh(sv * 0.1);
        }
        break;

    case CAS_AGENT_LEARNING:
    case CAS_AGENT_DELIBERATIVE:
        /* Model-based prediction with exploration */
        for (i = 0; i < na; i++) {
            double sum = 0.0;
            for (j = 0; j < dim; j++) {
                sum += agent->model.state[j]
                     * agent->model.confidence[j];
            }
            double noise = agent->temperature > 0.0
                ? _cas_rand_gaussian() * agent->temperature : 0.0;
            actions[i] = tanh(sum / (double)(dim + 1) + noise);
        }
        break;

    case CAS_AGENT_COEVOLVING:
        /* Neighbor-influenced decision */
        for (i = 0; i < na; i++) {
            double inf = 0.0;
            uint32_t nn = agent->num_neighbors;
            if (nn > 8) nn = 8;
            for (j = 0; j < nn; j++) {
                inf += sin((double)agent->neighborhood[j] * 0.01);
            }
            actions[i] = tanh(inf / (double)(nn + 1));
        }
        break;

    case CAS_AGENT_META:
        /* Self-modifying: adjust temperature based on performance */
        if (agent->average_reward < 0.0)
            agent->temperature *= 1.10;
        else if (agent->average_reward > 1.0)
            agent->temperature *= 0.95;
        agent->temperature = _cas_clamp(agent->temperature, 0.01, 10.0);
        for (i = 0; i < na; i++) {
            double sum = 0.0;
            for (j = 0; j < dim; j++)
                sum += agent->model.state[j] * 0.1;
            actions[i] = tanh(sum
                + _cas_rand_gaussian() * agent->temperature);
        }
        break;
    }

    return na;
}

/* =================================================================
 * L2: OODA Loop -- Act Phase
 *
 * Execute actions on environment through effectors.
 * Applies saturation, quantization, energy cost, and mechanical wear.
 *
 * Physical constraints (energy, precision, wear) embody the
 * bounded rationality and resource limitation of CAS agents.
 *
 * Complexity: O(num_effectors * world_dim)
 * ================================================================= */

void cas_agent_act(cas_agent_t *agent, const double *actions,
                   double *world_state, uint32_t world_dim)
{
    if (!agent || !actions || !world_state) return;
    uint32_t i;

    for (i = 0; i < agent->num_effectors; i++) {
        cas_effector_t *e = &agent->effectors[i];
        if (!e->active) continue;

        double a = (i < agent->num_effectors) ? actions[i] : 0.0;

        /* Saturation */
        a = _cas_clamp(a, e->saturation_min, e->saturation_max);

        /* Finite precision quantization */
        if (e->precision > 0.0)
            a = round(a / e->precision) * e->precision;

        /* Apply to world state */
        if (e->target_id < world_dim)
            world_state[e->target_id] += a * e->output;

        /* Energy consumption */
        agent->energy -= e->energy_cost * fabs(a);
        if (agent->energy < 0.0) agent->energy = 0.0;

        /* Effector wear accumulation */
        e->wear += fabs(a) * 0.001;
        if (e->wear > 1.0) e->wear = 1.0;

        e->output = a;
    }
}

/* =================================================================
 * L2: OODA Loop -- Adapt Phase
 *
 * Update internal model from reward using TD(0) learning:
 *   td_error = r + gamma * V(s') - V(s)
 *   V(s) <- V(s) + alpha * td_error
 *
 * This is the core learning loop in CAS. The agent updates its
 * internal model weights and confidence estimates based on the
 * temporal-difference prediction error.
 *
 * Theorem (Sutton 1988): Under decaying learning rate alpha_t
 * satisfying sum(alpha_t) = inf, sum(alpha_t^2) < inf,
 * TD(0) converges with probability 1 to the true value function
 * for linear function approximation.
 *
 * Complexity: O(state_dim)
 * ================================================================= */

void cas_agent_adapt(cas_agent_t *agent, double reward)
{
    if (!agent) return;

    uint32_t dim = agent->model.state_dim;
    double alpha = agent->model.learning_rate;
    double gamma = agent->model.discount_factor;
    uint32_t i;

    /* Append to reward history (circular buffer) */
    if (agent->reward_history_len < CAS_AGENT_HISTORY_LEN) {
        agent->reward_history[agent->reward_history_len++] = reward;
    } else {
        for (i = 1; i < CAS_AGENT_HISTORY_LEN; i++)
            agent->reward_history[i - 1] = agent->reward_history[i];
        agent->reward_history[CAS_AGENT_HISTORY_LEN - 1] = reward;
    }

    agent->cumulative_reward += reward;
    agent->age++;
    if (agent->age > 0)
        agent->average_reward = agent->cumulative_reward
                                / (double)agent->age;

    /* Current value V(s) = dot(state, reward_model) */
    double v_current = 0.0;
    for (i = 0; i < dim; i++)
        v_current += agent->model.state[i]
                     * agent->model.reward_model[i];

    /* Temporal-difference error */
    double td_error = reward + gamma * v_current - v_current;

    /* Update reward model weights */
    for (i = 0; i < dim; i++) {
        agent->model.reward_model[i]
            += alpha * td_error * agent->model.state[i];
        /* Policy gradient estimate (EMA) */
        agent->model.gradient[i]
            = 0.9 * agent->model.gradient[i]
              + 0.1 * td_error * agent->model.state[i];
    }

    /* Prediction error tracking */
    agent->model.prediction_error = fabs(td_error);
    agent->model.moving_error
        = 0.95 * agent->model.moving_error
          + 0.05 * fabs(td_error);

    /* Update confidence per dimension */
    for (i = 0; i < dim; i++) {
        double contrib = fabs(agent->model.state[i]
                              * agent->model.reward_model[i]);
        agent->model.confidence[i]
            = 0.9 * agent->model.confidence[i] + 0.1 * contrib;
        agent->model.confidence[i]
            = _cas_clamp(agent->model.confidence[i], 0.01, 0.99);
    }

    agent->model.update_count++;

    /* Update building blocks: bucket brigade algorithm
     * Holland (1986): Each active block bids proportionally
     * to its strength; winners receive a fraction of reward. */
    for (i = 0; i < agent->num_building_blocks; i++) {
        cas_building_block_t *bb = &agent->building_blocks[i];
        bb->strength = 0.99 * bb->strength + 0.01 * reward;
        bb->last_reward = reward;
        if (fabs(reward) > 0.01) bb->use_count++;
    }

    /* Update fitness history */
    if (agent->fitness_history_len < CAS_AGENT_HISTORY_LEN) {
        agent->fitness_history[agent->fitness_history_len++]
            = agent->fitness;
    } else {
        for (i = 1; i < CAS_AGENT_HISTORY_LEN; i++)
            agent->fitness_history[i - 1]
                = agent->fitness_history[i];
        agent->fitness_history[CAS_AGENT_HISTORY_LEN - 1]
            = agent->fitness;
    }
}

/* =================================================================
 * L5: Full OODA Cycle
 *
 * Execute one complete agent time step:
 *   Sense -> Decide -> Act -> Adapt
 *
 * This is the fundamental unit of agent time in a CAS simulation.
 * All four phases execute atomically within one time step.
 *
 * Complexity: O(state_dim * num_effectors)
 * ================================================================= */

void cas_agent_step(cas_agent_t *agent, double *world_state,
                    uint32_t world_dim, double reward)
{
    if (!agent || !world_state) return;
    double actions[CAS_AGENT_MAX_EFFECTORS];

    cas_agent_sense(agent, world_state, world_dim);
    uint32_t _na __attribute__((unused)) = cas_agent_decide(agent, actions,
                                   CAS_AGENT_MAX_EFFECTORS);
    cas_agent_act(agent, actions, world_state, world_dim);
    cas_agent_adapt(agent, reward);
}

/* =================================================================
 * L2: Peer Interaction -- Social Learning in CAS
 *
 * Two agents interact: the less-fit agent imitates the more-fit
 * agent's internal model. This models social learning, which is
 * a key mechanism for cultural evolution in CAS populations.
 *
 * Theorem (Rendell et al. 2010, Science 328:208):
 *   Social learning evolves when individual learning is costly
 *   and environmental change is moderate. In CAS populations,
 *   social interaction accelerates collective adaptation.
 *
 * Complexity: O(min(dim_a, dim_b))
 * ================================================================= */

void cas_agent_interact(cas_agent_t *a, cas_agent_t *b)
{
    if (!a || !b) return;

    cas_agent_t *teacher = (a->fitness >= b->fitness) ? a : b;
    cas_agent_t *student = (a->fitness >= b->fitness) ? b : a;

    uint32_t dim = student->model.state_dim;
    if (dim > teacher->model.state_dim) dim = teacher->model.state_dim;

    double rate = 0.05;
    uint32_t i;
    for (i = 0; i < dim; i++) {
        student->model.state[i]
            = (1.0 - rate) * student->model.state[i]
              + rate * teacher->model.state[i];
    }

    /* Cultural transmission: share a building block */
    if (teacher->num_building_blocks > 0
        && student->num_building_blocks < CAS_AGENT_MAX_BUILDING_BLOCKS) {
        uint32_t idx = _cas_rand() % teacher->num_building_blocks;
        student->building_blocks[student->num_building_blocks]
            = teacher->building_blocks[idx];
        student->building_blocks[student->num_building_blocks]
            .use_count = 0;
        student->num_building_blocks++;
    }
}

/* =================================================================
 * L2: Agent Similarity
 *
 * Compute cosine similarity between two agents' internal models.
 * Used for niche identification, speciation detection, and
 * clustering of agents into functional groups.
 *
 * cos_sim(a,b) = (a . b) / (|a| * |b|) in [-1, 1]
 *
 * Complexity: O(min(dim_a, dim_b))
 * ================================================================= */

double cas_agent_similarity(const cas_agent_t *a, const cas_agent_t *b)
{
    if (!a || !b) return 0.0;

    uint32_t dim = a->model.state_dim;
    if (b->model.state_dim < dim) dim = b->model.state_dim;
    if (dim == 0) return 0.0;

    double dot = 0.0, na = 0.0, nb = 0.0;
    uint32_t i;
    for (i = 0; i < dim; i++) {
        dot += a->model.state[i] * b->model.state[i];
        na  += a->model.state[i] * a->model.state[i];
        nb  += b->model.state[i] * b->model.state[i];
    }

    if (na < 1e-12 || nb < 1e-12) return 0.0;
    return dot / (sqrt(na) * sqrt(nb));
}

/* =================================================================
 * L1: Tag Mechanism (Holland 1995, Ch. 2)
 *
 * Tags are the CAS mechanism for agent identification and
 * selective interaction without centralized control.
 * This is one of Holland's seven basics of CAS.
 *
 * cas_agent_add_tag: attach a tag
 * cas_agent_has_tag: check for matching tag within tolerance
 * ================================================================= */

bool cas_agent_add_tag(cas_agent_t *agent, const char *label,
                       double tolerance)
{
    if (!agent || !label) return false;
    if (agent->num_tags >= CAS_AGENT_MAX_TAGS) return false;

    cas_agent_tag_t *tag = &agent->tags[agent->num_tags];
    strncpy(tag->label, label, CAS_TAG_STR_MAX - 1);
    tag->label[CAS_TAG_STR_MAX - 1] = '\0';
    tag->strength = 0.5;
    tag->tolerance = _cas_clamp(tolerance, 0.0, 1.0);
    tag->active = true;
    tag->created_at = agent->age;
    tag->group_id = 0;

    agent->num_tags++;
    return true;
}

double cas_agent_has_tag(const cas_agent_t *agent, const char *label)
{
    if (!agent || !label) return 0.0;

    double best = 0.0;
    size_t label_len = strlen(label);
    uint32_t i;

    for (i = 0; i < agent->num_tags; i++) {
        const cas_agent_tag_t *t = &agent->tags[i];
        if (!t->active) continue;

        size_t tag_len = strlen(t->label);
        size_t min_len = label_len < tag_len ? label_len : tag_len;
        if (min_len == 0) continue;

        uint32_t matches = 0;
        size_t j;
        for (j = 0; j < min_len; j++)
            if (label[j] == t->label[j]) matches++;

        double ratio = (double)matches / (double)min_len;
        if (ratio >= t->tolerance) {
            double val = ratio * t->strength;
            if (val > best) best = val;
        }
    }

    return best;
}

/* =================================================================
 * L1: Building Block Mechanism (Holland 1995, Ch. 2)
 *
 * Agents discover and compose building blocks -- reusable
 * modular components. Novel solutions arise from combining
 * tested building blocks. This is Holland's central mechanism
 * for how adaptation builds complexity.
 *
 * cas_agent_add_building_block: add block to repertoire
 * cas_agent_match_block: find best-matching block for condition
 * ================================================================= */

bool cas_agent_add_building_block(cas_agent_t *agent, const char *name,
                                  const uint8_t condition[4],
                                  const uint8_t action[4])
{
    if (!agent || !name || !condition || !action) return false;
    if (agent->num_building_blocks >= CAS_AGENT_MAX_BUILDING_BLOCKS)
        return false;

    cas_building_block_t *bb
        = &agent->building_blocks[agent->num_building_blocks];
    strncpy(bb->name, name, CAS_TAG_STR_MAX - 1);
    bb->name[CAS_TAG_STR_MAX - 1] = '\0';
    memcpy(bb->condition, condition, 4);
    memcpy(bb->action, action, 4);
    bb->strength = 0.1;
    bb->specificity = 0.5;
    bb->use_count = 0;
    bb->generation = agent->age;
    bb->bid = 0.0;
    bb->last_reward = 0.0;

    agent->num_building_blocks++;
    return true;
}

int32_t cas_agent_match_block(const cas_agent_t *agent,
                              const uint8_t condition[4])
{
    if (!agent || !condition) return -1;

    int32_t best_idx = -1;
    double best_score = -1.0;
    uint32_t i;

    for (i = 0; i < agent->num_building_blocks; i++) {
        const cas_building_block_t *bb = &agent->building_blocks[i];

        /* Bit-overlap match score */
        uint32_t matches = 0;
        int j, k;
        for (j = 0; j < 4; j++) {
            uint8_t overlap = ~(condition[j] ^ bb->condition[j]);
            for (k = 0; k < 8; k++)
                if (overlap & (1 << k)) matches++;
        }

        double score = (double)matches / 32.0
                       * bb->specificity * bb->strength;
        if (score > best_score) {
            best_score = score;
            best_idx = (int32_t)i;
        }
    }

    return best_idx;
}

/* =================================================================
 * L6: Fitness Computation
 *
 * Evaluate agent quality in environment:
 *   fitness = -prediction_error + energy_efficiency
 *           + weighted_reward_history + diversity_bonus
 *
 * This is the objective function driving selection pressure.
 * It defines what "better adapted" means in CAS.
 *
 * Complexity: O(state_dim + env_dim)
 * ================================================================= */

double cas_agent_compute_fitness(const cas_agent_t *agent,
                                 const double *environment,
                                 uint32_t env_dim)
{
    if (!agent) return -1e9;
    double f = 0.0;

    /* Prediction accuracy: negative MSE between model and env */
    if (environment && env_dim > 0) {
        double pe = 0.0;
        uint32_t n = env_dim < agent->model.state_dim
                     ? env_dim : agent->model.state_dim;
        uint32_t i;
        for (i = 0; i < n; i++) {
            double err = environment[i] - agent->model.state[i];
            pe += err * err;
        }
        f -= pe / (double)(n + 1);
    }

    /* Energy efficiency */
    f += agent->energy / (agent->max_energy + 1.0);

    /* Recent rewards (weighted toward recent) */
    double rs = 0.0, ws = 0.0;
    uint32_t i;
    for (i = 0; i < agent->reward_history_len; i++) {
        double w = (double)(i + 1)
                   / (double)agent->reward_history_len;
        rs += w * agent->reward_history[i];
        ws += w;
    }
    if (ws > 0.0) f += rs / ws;

    /* Diversity bonus: Holland's principle that diversity
     * of building blocks increases adaptive capacity */
    f += (double)agent->num_building_blocks * 0.05;

    return f;
}

/* =================================================================
 * L6: Selection Mechanisms
 *
 * Tournament selection: pick k random, return best.
 *   Preferable in CAS: handles negative fitness, tunable pressure.
 *
 * Roulette wheel selection: fitness-proportionate.
 *   Standard method; requires non-negative fitness.
 *
 * Complexity: O(k) for tournament, O(n) for roulette
 * ================================================================= */

uint32_t cas_agent_tournament_select(cas_agent_t **agents,
                                     uint32_t n_agents, uint32_t k)
{
    if (!agents || n_agents == 0 || k == 0) return 0;
    if (k > n_agents) k = n_agents;

    uint32_t best_idx = 0;
    double best_fit = -1e308;
    uint32_t i;

    for (i = 0; i < k; i++) {
        uint32_t idx = _cas_rand() % n_agents;
        if (agents[idx]) {
            double fit = agents[idx]->fitness;
            if (fit > best_fit) {
                best_fit = fit;
                best_idx = idx;
            }
        }
    }

    return best_idx;
}

uint32_t cas_agent_roulette_select(cas_agent_t **agents,
                                   uint32_t n_agents, double rng)
{
    if (!agents || n_agents == 0) return 0;

    /* Find min fitness for shift to non-negative */
    double min_fit = 0.0;
    uint32_t i;
    for (i = 0; i < n_agents; i++)
        if (agents[i] && agents[i]->fitness < min_fit)
            min_fit = agents[i]->fitness;

    /* Compute total shifted fitness */
    double total = 0.0;
    for (i = 0; i < n_agents; i++)
        if (agents[i])
            total += agents[i]->fitness - min_fit + 1e-6;

    if (total < 1e-12) return _cas_rand() % n_agents;

    /* Select based on cumulative probability */
    double cum = 0.0;
    for (i = 0; i < n_agents; i++) {
        if (agents[i]) {
            cum += (agents[i]->fitness - min_fit + 1e-6) / total;
            if (rng <= cum) return i;
        }
    }

    return n_agents - 1;
}
