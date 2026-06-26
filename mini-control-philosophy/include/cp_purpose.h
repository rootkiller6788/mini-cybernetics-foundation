#ifndef CP_PURPOSE_H
#define CP_PURPOSE_H

#include "cp_core.h"
#include <stdbool.h>

/* ============================================================================
 * Control Philosophy ? Teleology, Purpose, and Goal-Directed Behavior
 *
 * Rosenblueth, Wiener & Bigelow (1943):
 *   "Behavior, Purpose and Teleology"
 *   ? The foundational paper that redefined purpose as feedback-controlled
 *     negative entropy, making teleology scientifically respectable.
 *
 * Purpose is behavior controlled by negative feedback.
 * The purpose of a thermostat is to maintain temperature.
 * The purpose of a guided missile is to hit its target.
 *
 * References:
 *   Rosenblueth, Wiener & Bigelow (1943). Philosophy of Science, 10(1), 18-24.
 *   Ashby, W.R. (1952). Design for a Brain.
 *   Powers, W.T. (1973). Behavior: The Control of Perception.
 * ============================================================================ */

/** A goal specification: what the system aims to achieve. */
typedef struct {
    char*   description;      /* human-readable goal name */
    double* target_values;    /* desired output values */
    double* tolerances;       /* acceptable error per dimension */
    int     n_dimensions;     /* dimensionality of output space */
    double  priority;         /* 0..1, for goal arbitration */
    bool    is_active;        /* currently pursued */
    bool    is_achieved;      /* goal satisfaction flag */
    double  achievement_ratio;/* how close: 1.0 = fully achieved */
} GoalSpec;

/** A value/utility function for evaluating outcomes.
 *  U : Y ? R ? R, where Y = outcome space, R = reference. */
typedef struct {
    int     n_dimensions;
    double* weights;          /* relative importance of each dimension */
    double  risk_aversion;    /* > 1: risk-averse, < 1: risk-seeking */
    double  discount_factor;  /* ? ? (0,1] for future rewards */
    bool    is_quadratic;     /* true: U = -? w_i (y_i - r_i)? */
    bool    is_linear;        /* true: U = ? w_i y_i */
    bool    is_saturating;    /* true: U = ? w_i tanh(y_i - r_i) */
} UtilityFunction;

/** A teleological system: a control system with explicit purpose. */
typedef struct {
    ControlSystem* sys;       /* underlying control system */
    GoalSpec*      goals;     /* array of goals */
    int            n_goals;
    GoalSpec*      active_goal;/* currently selected goal */
    UtilityFunction utility;   /* how outcomes are evaluated */
    double         cumulative_utility; /* total utility accumulated */
    int            goal_switches;  /* number of times goal changed */
    double*        outcome_history; /* ring buffer of recent outcomes */
    int            history_len;
    int            history_cap;
} TeleologicalSystem;

/** A purposive behavior analyzer using the Rosenblueth-Wiener-Bigelow taxonomy. */
typedef struct {
    TeleologicalMode   mode;
    bool               has_feedback;       /* active purposeful behavior */
    bool               has_extrapolation;  /* predictive purpose */
    int                prediction_horizon;
    double             purpose_efficiency; /* achieved utility / max possible */
    double*            trajectory_deviation; /* ||actual - intended|| over time */
    int                trajectory_len;
} PurposiveAnalysis;

/* --- API: Goal Specification --- */

GoalSpec* cp_goal_create(const char* description, int n_dims,
                          const double* targets, const double* tolerances);
void cp_goal_free(GoalSpec* goal);
void cp_goal_set_priority(GoalSpec* goal, double priority);
bool cp_goal_check_satisfaction(const GoalSpec* goal, const double* current_output);
double cp_goal_achievement_ratio(const GoalSpec* goal, const double* current_output);

/* --- API: Utility Function --- */

UtilityFunction* cp_utility_create_quadratic(int n_dims, const double* weights);
UtilityFunction* cp_utility_create_linear(int n_dims, const double* weights);
UtilityFunction* cp_utility_create_saturating(int n_dims, const double* weights);
void cp_utility_free(UtilityFunction* uf);
double cp_utility_evaluate(const UtilityFunction* uf, const double* y, const double* r);
double cp_utility_certainty_equivalent(const UtilityFunction* uf, const double* y, const double* r);
void cp_utility_set_risk_aversion(UtilityFunction* uf, double gamma);

/* --- API: Teleological System --- */

TeleologicalSystem* cp_teleo_create(ControlSystem* sys, int max_goals);
void cp_teleo_free(TeleologicalSystem* ts);
void cp_teleo_add_goal(TeleologicalSystem* ts, const char* desc,
                        const double* targets, const double* tolerances, int n_dims);
int  cp_teleo_select_goal(TeleologicalSystem* ts, int goal_idx);
void cp_teleo_arbitrate(TeleologicalSystem* ts);
void cp_teleo_step(TeleologicalSystem* ts, double dt);
void cp_teleo_record_outcome(TeleologicalSystem* ts, const double* outcome);
void cp_teleo_print(const TeleologicalSystem* ts);

/* --- API: Purposive Analysis --- */

PurposiveAnalysis* cp_purposive_analyze(const TeleologicalSystem* ts);
void cp_purposive_free(PurposiveAnalysis* pa);
const char* cp_teleological_mode_name(TeleologicalMode mode);
void cp_purposive_print(const PurposiveAnalysis* pa);

/* --- API: Wertfunktion (Value of Control) --- */

/**
 * Compute the value of information in a control context:
 * expected utility with information minus expected utility without.
 * [Source: Howard, "Information Value Theory" (1966)]
 * Complexity: O(n_outcomes * n_actions)
 */
double cp_value_of_information(const double* prior, const double** likelihood,
                                const double** utility_matrix,
                                int n_states, int n_actions, int n_observations);

/**
 * Compute the control premium: how much better closed-loop is than open-loop.
 * CP = E[U_closed_loop] - E[U_open_loop].
 * [Source: Bar-Shalom & Tse, "Dual Effect and Certainty Equivalence" (1974)]
 */
double cp_control_premium(double util_closed_loop, double util_open_loop);

#endif /* CP_PURPOSE_H */
