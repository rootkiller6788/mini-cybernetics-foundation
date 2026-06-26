#ifndef CP_HIERARCHY_H
#define CP_HIERARCHY_H

#include "cp_core.h"
#include <stdbool.h>

/* ============================================================================
 * Control Philosophy ? Hierarchical Control Architecture
 *
 * Hierarchical control decomposes a complex control problem into layers
 * of abstraction, where higher layers set goals and constraints for lower
 * layers, and lower layers execute and report status.
 *
 * Key references:
 *   Mesarovic, Macko, Takahara (1970). Theory of Hierarchical, Multilevel Systems.
 *   Findeisen et al. (1980). Control and Coordination in Hierarchical Systems.
 *   Albus, J.S. (1991). Outline for a Theory of Intelligence (RCS architecture).
 *   Beer, S. (1972). Brain of the Firm (Viable System Model).
 * ============================================================================ */

/** A single layer in the control hierarchy.
 *  Each layer operates at its own time scale and abstraction level. */
typedef struct {
    char*           name;
    HierarchyLevel  level;
    double          time_scale;       /* characteristic time constant */
    double          time_horizon;     /* planning horizon */
    int             state_dim;
    double*         state;
    double*         goals;            /* goals received from above */
    double*         constraints;      /* constraints received from above */
    int             n_goals;
    int             n_constraints;
    /* Vertical communication */
    double*         upward_report;    /* status reported to higher layer */
    double*         downward_command; /* commands issued to lower layer */
    int             parent_idx;
    int*            child_indices;
    int             n_children;
    int             child_capacity;
    /* Performance */
    double          layer_cost;
    double          coordination_error;
} ControlLayer;

/** A full hierarchical control system.
 *  Layers form a tree where higher layers are more abstract and slower. */
typedef struct {
    char*           name;
    ControlLayer**  layers;
    int             n_layers;
    int             layer_capacity;
    double*         global_goal;
    int             global_goal_dim;
    double          total_cost;
    double          coordination_overhead; /* cost of inter-layer communication */
    double          emergence_measure;     /* how much system behavior exceeds sum of parts */
} HierarchicalControl;

/** A Viable System Model (VSM) as defined by Stafford Beer.
 *  System 1-5: Implementation, Coordination, Control, Intelligence, Policy. */
typedef struct {
    HierarchicalControl* hc;
    /* System 1: Operational units (autonomous implementation) */
    int             n_operational_units;
    double*         unit_autonomy;  /* per-unit autonomy level */
    /* System 2: Anti-oscillation coordination */
    double          damping_factor;
    double          oscillation_amplitude;
    /* System 3: Internal control (audit, resource allocation) */
    double*         resource_allocation;
    double          audit_frequency;
    /* System 3*: Monitoring (sporadic audit channel) */
    bool            monitoring_active;
    double          monitoring_interval;
    /* System 4: Intelligence (future planning, adaptation) */
    double          planning_horizon;
    double*         environment_model;
    int             env_model_dim;
    /* System 5: Policy (ultimate authority, identity) */
    char*           identity;
    double          cohesion_measure;
} ViableSystemModel;

/** Emergence analysis for hierarchical systems.
 *  Emergent properties: those not deducible from components alone. */
typedef struct {
    double  weak_emergence;     /* unexpected but explainable by simulation */
    double  strong_emergence;   /* irreducible to component interactions */
    double  downward_causation; /* higher-level constraints affecting lower levels */
    double  self_organization;  /* spontaneous order formation */
    double  complexity_gap;     /* C(system) - sum(C(components)) */
    bool    has_emergent_behavior;
} EmergenceAnalysis;

/** A hierarchical goal decomposition tree. */
typedef struct {
    char*           name;
    double*         goal_vector;
    int             goal_dim;
    double          priority;
    int             parent_index;
    int*            child_indices;
    int             n_children;
    int             child_capacity;
    bool            is_decomposable;  /* can be factored into sub-goals */
    double          decomposition_loss; /* suboptimality from decomposition */
} GoalNode;

/** Goal tree for hierarchical planning. */
typedef struct {
    GoalNode*       nodes;
    int             n_nodes;
    int             capacity;
    double          tree_coherence;   /* internal consistency of goal hierarchy */
} GoalTree;

/* --- API: Control Layer --- */

ControlLayer* cp_layer_create(const char* name, HierarchyLevel level,
                               int state_dim, double time_scale, double time_horizon);
void cp_layer_free(ControlLayer* layer);
void cp_layer_set_goals(ControlLayer* layer, const double* goals, int n);
void cp_layer_set_constraints(ControlLayer* layer, const double* constraints, int n);
void cp_layer_add_child(ControlLayer* parent, int child_idx);
void cp_layer_step(ControlLayer* layer, double dt, ControlLayer** all_layers);
double cp_layer_autonomy_measure(const ControlLayer* layer);

/* --- API: Hierarchical Control --- */

HierarchicalControl* cp_hierarchical_create(const char* name, int capacity);
void cp_hierarchical_free(HierarchicalControl* hc);
int  cp_hierarchical_add_layer(HierarchicalControl* hc, ControlLayer* layer);
void cp_hierarchical_set_global_goal(HierarchicalControl* hc, const double* goal, int dim);
void cp_hierarchical_step(HierarchicalControl* hc, double dt);
void cp_hierarchical_coordinate(HierarchicalControl* hc);
void cp_hierarchical_propagate_goals(HierarchicalControl* hc);
void cp_hierarchical_collect_reports(HierarchicalControl* hc);
double cp_hierarchical_coordination_overhead(const HierarchicalControl* hc);
double cp_hierarchical_emergence(const HierarchicalControl* hc);
void cp_hierarchical_print(const HierarchicalControl* hc);

/* --- API: Viable System Model --- */

ViableSystemModel* cp_vsm_create(const char* identity);
void cp_vsm_free(ViableSystemModel* vsm);
void cp_vsm_set_operational_units(ViableSystemModel* vsm, int n_units);
void cp_vsm_set_autonomy(ViableSystemModel* vsm, int unit, double autonomy);
void cp_vsm_allocate_resources(ViableSystemModel* vsm, const double* allocation, int n);
void cp_vsm_step(ViableSystemModel* vsm, double dt);
double cp_vsm_cohesion(const ViableSystemModel* vsm);
bool cp_vsm_is_viable(const ViableSystemModel* vsm);
void cp_vsm_print(const ViableSystemModel* vsm);

/* --- API: Emergence Analysis --- */

EmergenceAnalysis* cp_emergence_analyze(const HierarchicalControl* hc);
void cp_emergence_free(EmergenceAnalysis* ea);
void cp_emergence_print(const EmergenceAnalysis* ea);

/* --- API: Goal Tree --- */

GoalTree* cp_goal_tree_create(int capacity);
void cp_goal_tree_free(GoalTree* gt);
int  cp_goal_tree_add_goal(GoalTree* gt, const char* name,
                            const double* goal, int dim, double priority, int parent);
void cp_goal_tree_decompose(GoalTree* gt, int node_idx);
double cp_goal_tree_coherence(const GoalTree* gt);
double cp_goal_tree_decomposition_loss(const GoalTree* gt, int node_idx);
void cp_goal_tree_print(const GoalTree* gt);

/**
 * Compute the "span of control" principle: optimal branching factor b*
 * that minimizes total coordination cost.
 * b* = argmin_b [C_vertical(b) + C_horizontal(b)]
 * [Source: Simon, "The Architecture of Complexity" (1962)]
 * Complexity: O(1)
 */
double cp_optimal_span_of_control(int n_leaves, double coordination_cost_per_link,
                                   double delay_per_level);

/**
 * Compute the near-decomposability index: how well a system can be
 * decomposed into weakly-interacting subsystems.
 * [Source: Simon (1962); Courtois (1977)]
 * Complexity: O(n?)
 */
double cp_near_decomposability(const double* interaction_matrix, int n,
                                const int* partition, double intra_weight);

#endif /* CP_HIERARCHY_H */
