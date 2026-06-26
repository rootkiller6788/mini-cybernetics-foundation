#include "cp_hierarchy.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CP_EPS 1e-12

/* ============================================================================
 * Control Layer
 * ============================================================================ */

ControlLayer* cp_layer_create(const char* name, HierarchyLevel level,
                               int state_dim, double time_scale, double time_horizon) {
    ControlLayer* layer = (ControlLayer*)calloc(1, sizeof(ControlLayer));
    if (!layer) return NULL;
    layer->name = (name && name[0]) ? strdup(name) : strdup("layer");
    layer->level = level;
    layer->time_scale = time_scale;
    layer->time_horizon = time_horizon;
    layer->state_dim = state_dim;
    if (state_dim > 0) layer->state = (double*)calloc((size_t)state_dim, sizeof(double));
    layer->n_goals = 0;
    layer->n_constraints = 0;
    layer->parent_idx = -1;
    layer->n_children = 0;
    layer->child_capacity = 8;
    layer->child_indices = (int*)malloc(8 * sizeof(int));
    return layer;
}

void cp_layer_free(ControlLayer* layer) {
    if (!layer) return;
    free(layer->name);
    free(layer->state);
    free(layer->goals);
    free(layer->constraints);
    free(layer->upward_report);
    free(layer->downward_command);
    free(layer->child_indices);
    free(layer);
}

void cp_layer_set_goals(ControlLayer* layer, const double* goals, int n) {
    if (!layer || !goals || n <= 0) return;
    free(layer->goals);
    layer->goals = (double*)malloc((size_t)n * sizeof(double));
    memcpy(layer->goals, goals, (size_t)n * sizeof(double));
    layer->n_goals = n;
}

void cp_layer_set_constraints(ControlLayer* layer, const double* constraints, int n) {
    if (!layer || !constraints || n <= 0) return;
    free(layer->constraints);
    layer->constraints = (double*)malloc((size_t)n * sizeof(double));
    memcpy(layer->constraints, constraints, (size_t)n * sizeof(double));
    layer->n_constraints = n;
}

void cp_layer_add_child(ControlLayer* parent, int child_idx) {
    if (!parent || child_idx < 0) return;
    if (parent->n_children >= parent->child_capacity) {
        parent->child_capacity *= 2;
        parent->child_indices = (int*)realloc(parent->child_indices,
            (size_t)parent->child_capacity * sizeof(int));
    }
    parent->child_indices[parent->n_children++] = child_idx;
}

void cp_layer_step(ControlLayer* layer, double dt, ControlLayer** all_layers) {
    if (!layer || !all_layers) return;

    /* Layer dynamics: each layer evolves at its own time scale */
    double tau = layer->time_scale;
    if (tau < CP_EPS) tau = 1.0;
    for (int i = 0; i < layer->state_dim; i++) {
        /* Goal-driven dynamics with relaxation time tau */
        double goal_force = 0.0;
        for (int g = 0; g < layer->n_goals && g < layer->state_dim; g++) {
            goal_force += (layer->goals[g] - layer->state[i]) / tau;
        }
        /* Constraint satisfaction forces */
        double constraint_force = 0.0;
        for (int c = 0; c < layer->n_constraints && c < layer->state_dim; c++) {
            double violation = layer->state[i] - layer->constraints[c];
            if (violation > 0) constraint_force -= violation;
        }
        /* Higher layer commands */
        double parent_force = 0.0;
        if (layer->parent_idx >= 0 && layer->parent_idx < 100) {
            ControlLayer* parent = all_layers[layer->parent_idx];
            if (parent && parent->downward_command) {
                for (int j = 0; j < parent->state_dim && j < layer->state_dim; j++) {
                    parent_force += (parent->downward_command[j] - layer->state[i]) / parent->time_scale;
                }
            }
        }

        /* Collect child reports upward */
        double child_report = 0.0;
        for (int c = 0; c < layer->n_children; c++) {
            ControlLayer* child = all_layers[layer->child_indices[c]];
            if (child && child->upward_report && child->state_dim > 0) {
                child_report += child->upward_report[0] / layer->n_children;
            }
        }

        layer->state[i] += (goal_force + constraint_force + parent_force + 0.1 * child_report) * dt;
    }

    /* Update upward report */
    if (!layer->upward_report && layer->state_dim > 0)
        layer->upward_report = (double*)calloc((size_t)layer->state_dim, sizeof(double));
    for (int i = 0; i < layer->state_dim; i++)
        layer->upward_report[i] = layer->state[i];

    /* Update downward commands to children */
    if (!layer->downward_command && layer->state_dim > 0)
        layer->downward_command = (double*)calloc((size_t)layer->state_dim, sizeof(double));
    for (int i = 0; i < layer->state_dim; i++)
        layer->downward_command[i] = (layer->goals && i < layer->n_goals)
            ? layer->goals[i] : layer->state[i];
}

double cp_layer_autonomy_measure(const ControlLayer* layer) {
    if (!layer || layer->state_dim == 0) return 0.0;
    /* Autonomy = 1 - (influence from parent) / (total influence) */
    double self_influence = 0.0;
    for (int i = 0; i < layer->n_goals; i++) {
        self_influence += fabs(layer->goals[i]);
    }
    double parent_influence = 0.0;
    if (layer->parent_idx >= 0) {
        parent_influence = 0.5; /* estimate */
    }
    double total = self_influence + parent_influence + CP_EPS;
    return self_influence / total;
}

/* ============================================================================
 * Hierarchical Control
 * ============================================================================ */

HierarchicalControl* cp_hierarchical_create(const char* name, int capacity) {
    HierarchicalControl* hc = (HierarchicalControl*)calloc(1, sizeof(HierarchicalControl));
    if (!hc) return NULL;
    hc->name = (name && name[0]) ? strdup(name) : strdup("hierarchy");
    hc->layer_capacity = capacity;
    hc->layers = (ControlLayer**)calloc((size_t)capacity, sizeof(ControlLayer*));
    hc->n_layers = 0;
    hc->global_goal_dim = 0;
    hc->total_cost = 0.0;
    hc->coordination_overhead = 0.0;
    hc->emergence_measure = 0.0;
    return hc;
}

void cp_hierarchical_free(HierarchicalControl* hc) {
    if (!hc) return;
    /* Note: layers are freed separately by caller to avoid double-free */
    free(hc->name);
    free(hc->global_goal);
    free(hc->layers);
    free(hc);
}

int cp_hierarchical_add_layer(HierarchicalControl* hc, ControlLayer* layer) {
    if (!hc || !layer || hc->n_layers >= hc->layer_capacity) return -1;
    int idx = hc->n_layers;
    hc->layers[idx] = layer;
    hc->n_layers++;
    return idx;
}

void cp_hierarchical_set_global_goal(HierarchicalControl* hc, const double* goal, int dim) {
    if (!hc || !goal) return;
    free(hc->global_goal);
    hc->global_goal = (double*)malloc((size_t)dim * sizeof(double));
    if (hc->global_goal) {
        memcpy(hc->global_goal, goal, (size_t)dim * sizeof(double));
        hc->global_goal_dim = dim;
    }
}

void cp_hierarchical_step(HierarchicalControl* hc, double dt) {
    if (!hc) return;
    cp_hierarchical_propagate_goals(hc);
    /* Step layers from bottom to top for stability */
    for (int i = hc->n_layers - 1; i >= 0; i--) {
        cp_layer_step(hc->layers[i], dt, hc->layers);
    }
    cp_hierarchical_collect_reports(hc);
    cp_hierarchical_coordinate(hc);
}

void cp_hierarchical_coordinate(HierarchicalControl* hc) {
    if (!hc || hc->n_layers < 2) return;
    double overhead = 0.0;
    /* Coordination: resolve conflicts between sibling layers */
    for (int i = 0; i < hc->n_layers; i++) {
        ControlLayer* li = hc->layers[i];
        for (int j = i + 1; j < hc->n_layers; j++) {
            ControlLayer* lj = hc->layers[j];
            if (li->parent_idx == lj->parent_idx && li->parent_idx >= 0) {
                /* Siblings: check for goal conflicts */
                for (int g = 0; g < li->n_goals && g < lj->n_goals; g++) {
                    double conflict = fabs(li->goals[g] - lj->goals[g]);
                    overhead += conflict;
                    /* Resolve by averaging (simple coordination) */
                    double avg = 0.5 * (li->goals[g] + lj->goals[g]);
                    li->goals[g] = avg;
                    lj->goals[g] = avg;
                }
            }
        }
    }
    hc->coordination_overhead = overhead;
    hc->total_cost += overhead;
}

void cp_hierarchical_propagate_goals(HierarchicalControl* hc) {
    if (!hc) return;
    /* Top-down: each layer decomposes its goals to children */
    if (hc->global_goal && hc->n_layers > 0) {
        ControlLayer* top = hc->layers[0];
        if (top && top->n_goals == 0) {
            cp_layer_set_goals(top, hc->global_goal, hc->global_goal_dim);
        }
    }
    for (int i = 0; i < hc->n_layers; i++) {
        ControlLayer* parent = hc->layers[i];
        if (!parent || !parent->goals) continue;
        for (int c = 0; c < parent->n_children; c++) {
            int child_idx = parent->child_indices[c];
            if (child_idx < 0 || child_idx >= hc->n_layers) continue;
            ControlLayer* child = hc->layers[child_idx];
            if (!child) continue;
            /* Propagate decomposed goals */
            cp_layer_set_goals(child, parent->goals, parent->n_goals);
            cp_layer_set_constraints(child, parent->constraints, parent->n_constraints);
        }
    }
}

void cp_hierarchical_collect_reports(HierarchicalControl* hc) {
    if (!hc) return;
    /* Bottom-up: aggregate child reports to parents */
    for (int i = hc->n_layers - 1; i >= 0; i--) {
        ControlLayer* layer = hc->layers[i];
        if (!layer || layer->parent_idx < 0 || layer->parent_idx >= hc->n_layers) continue;
        ControlLayer* parent = hc->layers[layer->parent_idx];
        if (!parent || !parent->upward_report) {
            if (parent) {
                parent->upward_report = (double*)calloc((size_t)parent->state_dim, sizeof(double));
            }
        }
        if (parent && parent->upward_report && layer->upward_report) {
            for (int j = 0; j < parent->state_dim && j < layer->state_dim; j++) {
                parent->upward_report[j] += layer->upward_report[j];
            }
        }
    }
}

double cp_hierarchical_coordination_overhead(const HierarchicalControl* hc) {
    return hc ? hc->coordination_overhead : 0.0;
}

double cp_hierarchical_emergence(const HierarchicalControl* hc) {
    if (!hc || hc->n_layers == 0) return 0.0;
    /* Emergence = system-level behavior - sum(component behaviors) */
    double system_level = 0.0;
    for (int i = 0; i < hc->n_layers; i++) {
        ControlLayer* l = hc->layers[i];
        if (l && l->state) {
            for (int j = 0; j < l->state_dim; j++) {
                system_level += fabs(l->state[j]);
            }
        }
    }
    double component_sum = 0.0;
    for (int i = 0; i < hc->n_layers; i++) {
        ControlLayer* l = hc->layers[i];
        if (l && l->state && l->n_children == 0) {
            for (int j = 0; j < l->state_dim; j++) {
                component_sum += fabs(l->state[j]);
            }
        }
    }
    double emergence = system_level - component_sum;
    return (emergence > 0) ? emergence / (system_level + CP_EPS) : 0.0;
}

void cp_hierarchical_print(const HierarchicalControl* hc) {
    if (!hc) { printf("HierarchicalControl: NULL\n"); return; }
    printf("=== Hierarchical Control: %s ===\n", hc->name);
    printf("Layers: %d, Global goal dim: %d\n", hc->n_layers, hc->global_goal_dim);
    printf("Total cost: %.6f, Coordination overhead: %.6f\n",
           hc->total_cost, hc->coordination_overhead);
    printf("Emergence: %.6f\n", hc->emergence_measure);
    for (int i = 0; i < hc->n_layers; i++) {
        ControlLayer* l = hc->layers[i];
        printf("  Layer %d [%s]: level=%d, state_dim=%d, children=%d, parent=%d\n",
               i, l->name, l->level, l->state_dim, l->n_children, l->parent_idx);
    }
}

/* ============================================================================
 * Viable System Model (Stafford Beer)
 * ============================================================================ */

ViableSystemModel* cp_vsm_create(const char* identity) {
    ViableSystemModel* vsm = (ViableSystemModel*)calloc(1, sizeof(ViableSystemModel));
    if (!vsm) return NULL;
    vsm->identity = (identity && identity[0]) ? strdup(identity) : strdup("VSM");
    vsm->hc = cp_hierarchical_create(identity, 5);
    vsm->damping_factor = 0.7;
    vsm->audit_frequency = 5.0;
    vsm->planning_horizon = 50.0;
    vsm->cohesion_measure = 1.0;
    return vsm;
}

void cp_vsm_free(ViableSystemModel* vsm) {
    if (!vsm) return;
    /* Free layers owned by VSM */
    if (vsm->hc) {
        for (int i = 0; i < vsm->hc->n_layers; i++)
            cp_layer_free(vsm->hc->layers[i]);
        cp_hierarchical_free(vsm->hc);
    }
    free(vsm->identity);
    free(vsm->unit_autonomy);
    free(vsm->resource_allocation);
    free(vsm->environment_model);
    free(vsm);
}

void cp_vsm_set_operational_units(ViableSystemModel* vsm, int n_units) {
    if (!vsm || n_units <= 0) return;
    vsm->n_operational_units = n_units;
    vsm->unit_autonomy = (double*)malloc((size_t)n_units * sizeof(double));
    vsm->resource_allocation = (double*)malloc((size_t)n_units * sizeof(double));
    if (vsm->unit_autonomy) {
        for (int i = 0; i < n_units; i++) vsm->unit_autonomy[i] = 0.7;
    }
    if (vsm->resource_allocation) {
        for (int i = 0; i < n_units; i++) vsm->resource_allocation[i] = 1.0 / n_units;
    }
}

void cp_vsm_set_autonomy(ViableSystemModel* vsm, int unit, double autonomy) {
    if (!vsm || !vsm->unit_autonomy || unit < 0 || unit >= vsm->n_operational_units) return;
    vsm->unit_autonomy[unit] = (autonomy < 0.0) ? 0.0 : ((autonomy > 1.0) ? 1.0 : autonomy);
}

void cp_vsm_allocate_resources(ViableSystemModel* vsm, const double* allocation, int n) {
    if (!vsm || !allocation || !vsm->resource_allocation) return;
    int n_copy = (n < vsm->n_operational_units) ? n : vsm->n_operational_units;
    memcpy(vsm->resource_allocation, allocation, (size_t)n_copy * sizeof(double));
}

void cp_vsm_step(ViableSystemModel* vsm, double dt) {
    if (!vsm) return;
    /* System 2: Anti-oscillation ? dampen inter-unit oscillations */
    if (vsm->n_operational_units > 1) {
        double total_oscillation = 0.0;
        for (int i = 0; i < vsm->n_operational_units; i++) {
            for (int j = i + 1; j < vsm->n_operational_units; j++) {
                double diff = fabs(vsm->unit_autonomy[i] - vsm->unit_autonomy[j]);
                total_oscillation += diff * exp(-vsm->damping_factor * dt);
            }
        }
        vsm->oscillation_amplitude = total_oscillation / vsm->n_operational_units;
    }

    /* System 3: Resource reallocation */
    for (int i = 0; i < vsm->n_operational_units; i++) {
        if (vsm->resource_allocation) {
            vsm->resource_allocation[i] *= (1.0 - 0.01 * dt);
            vsm->resource_allocation[i] += 0.01 * vsm->unit_autonomy[i] * dt;
        }
    }

    /* System 5: Identity maintenance */
    vsm->cohesion_measure *= (1.0 - 0.001 * dt);
    vsm->cohesion_measure += 0.001 * (1.0 - vsm->oscillation_amplitude) * dt;
    if (vsm->cohesion_measure < 0.0) vsm->cohesion_measure = 0.0;
    if (vsm->cohesion_measure > 1.0) vsm->cohesion_measure = 1.0;
}

double cp_vsm_cohesion(const ViableSystemModel* vsm) {
    return vsm ? vsm->cohesion_measure : 0.0;
}

bool cp_vsm_is_viable(const ViableSystemModel* vsm) {
    if (!vsm) return false;
    /* Viability conditions:
       1. All units have some autonomy
       2. Cohesion > threshold
       3. Oscillation not diverging */
    if (vsm->cohesion_measure < 0.3) return false;
    for (int i = 0; i < vsm->n_operational_units; i++) {
        if (vsm->unit_autonomy[i] < 0.1) return false;
    }
    return vsm->oscillation_amplitude < 1.0;
}

void cp_vsm_print(const ViableSystemModel* vsm) {
    if (!vsm) { printf("ViableSystemModel: NULL\n"); return; }
    printf("=== Viable System Model: %s ===\n", vsm->identity);
    printf("Operational units: %d\n", vsm->n_operational_units);
    printf("Damping factor: %.4f, Oscillation: %.6f\n",
           vsm->damping_factor, vsm->oscillation_amplitude);
    printf("Cohesion: %.4f, Viable: %s\n",
           vsm->cohesion_measure, cp_vsm_is_viable(vsm) ? "YES" : "NO");
    printf("Autonomy: ");
    for (int i = 0; i < vsm->n_operational_units && i < 10; i++)
        printf("%.3f ", vsm->unit_autonomy ? vsm->unit_autonomy[i] : 0.0);
    printf("\n");
}

/* ============================================================================
 * Emergence Analysis
 * ============================================================================ */

EmergenceAnalysis* cp_emergence_analyze(const HierarchicalControl* hc) {
    if (!hc) return NULL;
    EmergenceAnalysis* ea = (EmergenceAnalysis*)calloc(1, sizeof(EmergenceAnalysis));
    if (!ea) return NULL;

    ea->weak_emergence = cp_hierarchical_emergence(hc);

    /* Strong emergence: system-level properties irreducible to components */
    double component_complexity = 0.0;
    for (int i = 0; i < hc->n_layers; i++) {
        if (hc->layers[i] && hc->layers[i]->state) {
            component_complexity += (double)hc->layers[i]->state_dim;
        }
    }
    double system_complexity = component_complexity + hc->coordination_overhead;
    ea->complexity_gap = system_complexity - component_complexity;
    ea->strong_emergence = (ea->complexity_gap > 1.0) ? (ea->complexity_gap / system_complexity) : 0.0;

    /* Downward causation: higher layers constrain lower layers */
    ea->downward_causation = 0.0;
    for (int i = 0; i < hc->n_layers; i++) {
        ControlLayer* l = hc->layers[i];
        if (l && l->n_constraints > 0 && l->parent_idx >= 0) {
            double constraint_strength = 0.0;
            for (int c = 0; c < l->n_constraints; c++)
                constraint_strength += fabs(l->constraints[c]);
            ea->downward_causation += constraint_strength;
        }
    }
    ea->downward_causation /= (hc->n_layers + CP_EPS);

    /* Self-organization */
    ea->self_organization = (ea->weak_emergence > CP_EPS && ea->strong_emergence < 0.5)
        ? 1.0 - ea->downward_causation / (ea->weak_emergence + CP_EPS) : 0.0;

    ea->has_emergent_behavior = (ea->weak_emergence > 0.01 || ea->strong_emergence > 0.01);
    return ea;
}

void cp_emergence_free(EmergenceAnalysis* ea) {
    free(ea);
}

void cp_emergence_print(const EmergenceAnalysis* ea) {
    if (!ea) { printf("EmergenceAnalysis: NULL\n"); return; }
    printf("=== Emergence Analysis ===\n");
    printf("Weak emergence: %.4f\n", ea->weak_emergence);
    printf("Strong emergence: %.4f\n", ea->strong_emergence);
    printf("Downward causation: %.4f\n", ea->downward_causation);
    printf("Self-organization: %.4f\n", ea->self_organization);
    printf("Complexity gap: %.4f\n", ea->complexity_gap);
    printf("Has emergent behavior: %s\n", ea->has_emergent_behavior ? "YES" : "NO");
}

/* ============================================================================
 * Goal Tree
 * ============================================================================ */

GoalTree* cp_goal_tree_create(int capacity) {
    GoalTree* gt = (GoalTree*)calloc(1, sizeof(GoalTree));
    if (!gt) return NULL;
    gt->capacity = capacity;
    gt->nodes = (GoalNode*)calloc((size_t)capacity, sizeof(GoalNode));
    gt->n_nodes = 0;
    gt->tree_coherence = 1.0;
    return gt;
}

void cp_goal_tree_free(GoalTree* gt) {
    if (!gt) return;
    for (int i = 0; i < gt->n_nodes; i++) {
        free(gt->nodes[i].name);
        free(gt->nodes[i].goal_vector);
        free(gt->nodes[i].child_indices);
    }
    free(gt->nodes);
    free(gt);
}

int cp_goal_tree_add_goal(GoalTree* gt, const char* name,
                           const double* goal, int dim, double priority, int parent) {
    if (!gt || gt->n_nodes >= gt->capacity) return -1;
    int idx = gt->n_nodes;
    GoalNode* node = &gt->nodes[idx];
    node->name = (name && name[0]) ? strdup(name) : strdup("goal");
    node->goal_dim = dim;
    node->priority = priority;
    node->parent_index = parent;
    node->is_decomposable = false;
    node->child_capacity = 4;
    node->child_indices = (int*)malloc(4 * sizeof(int));
    node->n_children = 0;
    node->decomposition_loss = 0.0;
    if (dim > 0) {
        node->goal_vector = (double*)malloc((size_t)dim * sizeof(double));
        if (node->goal_vector && goal)
            memcpy(node->goal_vector, goal, (size_t)dim * sizeof(double));
    }
    gt->n_nodes++;
    return idx;
}

void cp_goal_tree_decompose(GoalTree* gt, int node_idx) {
    if (!gt || node_idx < 0 || node_idx >= gt->n_nodes) return;
    GoalNode* node = &gt->nodes[node_idx];
    if (node->goal_dim < 2) return;
    node->is_decomposable = true;
    /* Simple decomposition: split goal vector into individual scalar goals */
    node->decomposition_loss = 0.0;
    for (int d = 0; d < node->goal_dim; d++) {
        double scalar_goal[1] = { node->goal_vector[d] };
        int child_idx = cp_goal_tree_add_goal(gt, "sub-goal", scalar_goal, 1,
                                                node->priority / node->goal_dim, node_idx);
        if (child_idx >= 0) {
            node->child_indices[node->n_children++] = child_idx;
            node->decomposition_loss += 0.01; /* small loss from decoupling */
        }
    }
}

double cp_goal_tree_coherence(const GoalTree* gt) {
    if (!gt || gt->n_nodes == 0) return 1.0;
    /* Coherence = 1 - (average decomposition loss) */
    double total_loss = 0.0;
    int decomposable = 0;
    for (int i = 0; i < gt->n_nodes; i++) {
        if (gt->nodes[i].is_decomposable) {
            total_loss += gt->nodes[i].decomposition_loss;
            decomposable++;
        }
    }
    if (decomposable == 0) return 1.0;
    double avg_loss = total_loss / decomposable;
    double coherence = 1.0 - avg_loss;
    return (coherence < 0.0) ? 0.0 : ((coherence > 1.0) ? 1.0 : coherence);
}

double cp_goal_tree_decomposition_loss(const GoalTree* gt, int node_idx) {
    if (!gt || node_idx < 0 || node_idx >= gt->n_nodes) return 0.0;
    return gt->nodes[node_idx].decomposition_loss;
}

void cp_goal_tree_print(const GoalTree* gt) {
    if (!gt) { printf("GoalTree: NULL\n"); return; }
    printf("=== Goal Tree: %d nodes ===\n", gt->n_nodes);
    printf("Coherence: %.4f\n", gt->tree_coherence);
    for (int i = 0; i < gt->n_nodes; i++) {
        GoalNode* n = &gt->nodes[i];
        printf("  [%d] %s: dim=%d, priority=%.2f, parent=%d, children=%d, decomp=%d\n",
               i, n->name, n->goal_dim, n->priority, n->parent_index, n->n_children,
               n->is_decomposable);
    }
}

/* ============================================================================
 * Span of Control & Near-Decomposability (Herbert Simon)
 * ============================================================================ */

double cp_optimal_span_of_control(int n_leaves, double coordination_cost_per_link,
                                   double delay_per_level) {
    if (n_leaves <= 1) return 1.0;
    /* b* minimizes: b * (hierarchy_depth) * cost_per_link
       where depth ? log_b(n_leaves)
       Total cost = cost_per_link * b * log_b(n) + delay_per_level * log_b(n)
       Search over candidate b values */
    double best_b = 2.0;
    double best_cost = 1e18;
    for (double b = 2.0; b <= 20.0 && b <= n_leaves; b += 1.0) {
        double depth = log((double)n_leaves) / log(b);
        double cost = coordination_cost_per_link * b * depth + delay_per_level * depth;
        if (cost < best_cost) {
            best_cost = cost;
            best_b = b;
        }
    }
    return best_b;
}

double cp_near_decomposability(const double* interaction_matrix, int n,
                                const int* partition, double intra_weight) {
    if (!interaction_matrix || !partition || n <= 0) return 0.0;
    /* Near-decomposability = intra-block interaction / total interaction */
    double intra = 0.0, inter = 0.0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double w = fabs(interaction_matrix[i * n + j]);
            if (partition[i] == partition[j]) {
                intra += w * intra_weight;
            } else {
                inter += w;
            }
        }
    }
    double total = intra + inter + CP_EPS;
    return intra / total;
}
