/* -*-C++-*- */
/*
 * Heuristics.
 *
 * Copyright (C) 2002-2004 Carnegie Mellon University
 * Written by H�kan L. S. Younes.
 *
 * Permission is hereby granted to distribute this software for
 * non-commercial research purposes, provided that this copyright
 * notice is included with any such distribution.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE
 * SOFTWARE IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU
 * ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.
 *
 * $Id: heuristics.h,v 6.7 2003-12-05 23:16:12 lorens Exp $
 */
#ifndef HEURISTICS_H
#define HEURISTICS_H

#include "domains.h"
#include "formulas.h"
#include <stdexcept>

struct Action;
struct ActionList;
struct Problem;
struct ActionDomain;
struct Bindings;
struct Flaw;
struct Unsafe;
struct OpenCondition;
struct Plan;
struct Parameters;


/* ====================================================================== */
/* ActionEffectMap */

/*
 * Mapping from actions to effects.
 */
struct ActionEffectMap : public std::multimap<const Action*, const Effect*> {};


/* ====================================================================== */
/* HeuristicValue */

/*
 * A heuristic value.
 */
struct HeuristicValue {

    /* A zero heuristic value. */
    static const HeuristicValue ZERO;
    
    /* A zero cost, unit work, heuristic value. */
    static const HeuristicValue ZERO_COST_UNIT_WORK;
    
    /* An infinite heuristic value. */
    static const HeuristicValue INFINITE;

    /* Constructs a zero heuristic value. */
    HeuristicValue() : add_cost_(0.0f), add_work_(0), makespan_(0.0f) {}

    /* Constructs a heuristic value. */
    HeuristicValue(float add_cost, int add_work, float makespan)
        : add_cost_(add_cost), add_work_(add_work), makespan_(makespan) {}

    /* Returns the cost according to the additive heurisitc. */
    float add_cost() const { return add_cost_; }

    /* Returns the work according to the additive heuristic. */
    int add_work() const { return add_work_; }

    /* Returns the value according to the makespan heuristic. */
    float makespan() const { return makespan_; }

    /* Checks if this heuristic value is zero. */
    bool zero() const;

    /* Checks if this heuristic value is infinite. */
    bool infinite() const;

    /* Adds the given heuristic value to this heuristic value. */
    HeuristicValue& operator+=(const HeuristicValue& v);

    /* Increases the cost of this heuristic value. */
    void increase_cost(float x);

    /* Increments the work of this heuristic value. */
    void increment_work();

    /* Increases the makespan of this heuristic value. */
    void increase_makespan(float x);

private:

    /* Cost according to additive heuristic. */
    float add_cost_;
    
    /* Work according to additive heuristic. */
    int add_work_;
    
    /* Value according to the makespan heursitic. */
    float makespan_;
};

/* Inequality operator for heuristic values. */
bool operator!=(const HeuristicValue& v1, const HeuristicValue& v2);

/* Returns the componentwise minimum heuristic value, given two heuristic values. */
HeuristicValue min(const HeuristicValue& v1, const HeuristicValue& v2);

/* Output operator for heuristic values. */
std::ostream& operator<<(std::ostream& os, const HeuristicValue& v);


/* ====================================================================== */
/* PlanningGraph */

/*
 * A planning graph.
 */
struct PlanningGraph {
    
    /* Constructs a planning graph. */
    PlanningGraph(const Problem& problem, const Parameters& params);

    /* Deletes this planning graph. */
    ~PlanningGraph();

    /* Returns the problem associated with this planning graph. */
    const Problem& problem() const { return *problem_; }

    /* Returns the heurisitc value of an atom. */
    HeuristicValue heuristic_value(const Atom& atom, int step_id,
        const Bindings* bindings = NULL) const;

    /* Returns the heuristic value of a negated atom. */
    HeuristicValue heuristic_value(const Negation& negation, int step_id,
        const Bindings* bindings = NULL) const;

    /* Returns a set of achievers for the given literal. */
    const ActionEffectMap* literal_achievers(const Literal& literal) const;

    /* Returns the parameter domain for the given action, or NULL if the
       parameter domain is empty. */
    const ActionDomain* action_domain(const std::string& name) const;

private:

    /* Atom value map. */
    struct AtomValueMap : public std::map<const Atom*, HeuristicValue> {};

    /* Mapping of literals to actions. */
    struct LiteralAchieverMap : public std::map<const Literal*, ActionEffectMap> {};

    /* Mapping of predicate names to ground atoms. */
    struct PredicateAtomsMap : public std::multimap<Predicate, const Atom*> {};

    /* Mapping of action name to parameter domain. */
    struct ActionDomainMap : public std::map<std::string, ActionDomain*> {};

    /* Problem associated with this planning graph. */
    const Problem* problem_;
    
    /* Atom values. */
    AtomValueMap atom_values_;
    
    /* Negated atom values. */
    AtomValueMap negation_values_;
    
    /* Maps formulas to actions that achieve those formulas. */
    LiteralAchieverMap achievers_;
    
    /* Maps predicates to ground atoms. */
    PredicateAtomsMap predicate_atoms_;
    
    /* Maps predicates to negated ground atoms. */
    PredicateAtomsMap predicate_negations_;
    
    /* Maps action names to possible parameter lists. */
    ActionDomainMap action_domains_;

    /* Finds an element in a LiteralActionsMap. */
    bool find(const LiteralAchieverMap& m, const Literal& l,
        const Action& a, const Effect& e) const;
};


/* ====================================================================== */
/* InvalidHeuristic */

/*
 * An invalid heuristic exception.
 */
struct InvalidHeuristic : public std::runtime_error {
    
    /* Constructs an invalid heuristic exception. */
    InvalidHeuristic(const std::string& name);

};


/* ====================================================================== */
/* Heuristic */

/*
 * Heuristic for ranking plans.
 *
 * LIFO gives priority to plans created later.
 * FIFO gives priority to plans created earlier.
 * OC gives priority to plans with few open conditions.
 * UC gives priority to plans with few threatened links.
 * BUC gives priority to plans with no threatened links.
 * S+OC uses h(p) = |S(p)| + w*|OC(p)|.
 * UCPOP uses h(p) = |S(p)| + w*(|OC(p)| + |UC(p)|).
 * ADD_COST uses the additive cost heuristic.
 * ADD_WORK uses the additive work heuristic.
 * ADD uses h(p) = |S(p)| + w*ADD_COST.
 * ADDR is like ADD, but tries to take reuse into account.
 * MAKESPAN gives priority to plans with low makespan.
 */
struct Heuristic {
    
    /* Constructs a heuristic from a name. */
    Heuristic(const std::string& name = "UCPOP");

    /* Selects a heuristic from a name. */
    Heuristic& operator=(const std::string& name);

    /* Checks if this heuristic needs a planning graph. */
    bool needs_planning_graph() const;

    /* Fills the provided vector with the ranks for the given plan. */
    void plan_rank(std::vector<float>& rank, const Plan& plan,
        float weight, const Domain& domain,
        const PlanningGraph* planning_graph) const;

private:
    
    /* Heuristics. */
    typedef enum {
        LIFO, FIFO, OC, UC, BUC, S_PLUS_OC, UCPOP,
        ADD, ADD_COST, ADD_WORK, ADDR, ADDR_COST, ADDR_WORK,
        MAKESPAN
    } HVal;

    /* The selected heuristics. */
    std::vector<HVal> h_;
    
    /* Whether a planning graph is needed by this heuristic. */
    bool needs_pg_;
};


/* ====================================================================== */
/* InvalidFlawSelectionOrder */

/*
 * An invalid flaw selection order exception.
 */
struct InvalidFlawSelectionOrder : public std::runtime_error {
    
    /* Constructs an invalid flaw selection order exception. */
    InvalidFlawSelectionOrder(const std::string& name);

};


/* ====================================================================== */
/* SelectionCriterion */

/*
   A selection criterion. We adopt the following notation to succinctly specify selection criteria:
   {flaw types}[max refinements]ordering criterion (inspired by Pollack, Joslin, and Paolucci 1997).
   See README.md file for details on flaw types and ordering criteria supported by Recurve.
 */
struct SelectionCriterion {
    
    /* A selection order. */
    typedef enum {
        LIFO, FIFO, RANDOM, LR, MR,
        NEW, REUSE, LC, MC, LW, MW
    } OrderType;
    
    /* A heuristic. */
    typedef enum { ADD, MAKESPAN } RankHeuristic;

    /* Whether this criterion applies to non-separable threats. */
    bool non_separable;
    
    /* Whether this criterion applies to separable threats. */
    bool separable;
    
    /* Whether this criterion applies to open conditions. */
    bool open_cond;
    
    /* Whether this criterion applies to local open conditions. */
    bool local_open_cond;
    
    /* Whether this criterion applies to static open conditions. */
    bool static_open_cond;
    
    /* Whether this criterion applies to unsafe open conditions. */
    bool unsafe_open_cond;
    
    /* Whether this criterion applies to unexpanded composite steps. */
    bool unexpanded_composite_step;

    /* The maximum number of refinements allowed for a flaw that this criterion applies to. */
    int max_refinements;

    /* The ordering criterion. */
    OrderType order;
    
    /* Heuristic used to rank open conditions (if applicable). */
    RankHeuristic heuristic;
    
    /* Whether the above heuristic should take reuse into account. */
    bool reuse;
};


/* ====================================================================== */
/* FlawSelectionOrder */

/*
 * Flaw selection order.
 *
 * This is basically a list of selection criteria.
 */
struct FlawSelectionOrder
{
    /* Constructs a default flaw selection order. */
    FlawSelectionOrder(const std::string& name = "UCPOP");

    /* Selects a flaw selection order from a name. */
    FlawSelectionOrder& operator=(const std::string& name);

    /* Returns the name of this flaw selection order. */
    const std::string name() const { return name_; }

    /* Checks if this flaw order needs a planning graph. */
    bool needs_planning_graph() const;

    /* Checks if this flaw order is POCL complete: every POCL flaw matches at least one selection
       criterion in the flaw order. */
    bool is_pocl_complete() const;

    /* Checks if this flaw order is decomposition complete: it is POCL complete and it has at least
       one selection criterion that can handle unexpanded composite steps. */
    bool is_decomposition_complete() const;

    /* Selects a flaw from the flaws of the given plan. */
    const Flaw& select(const Plan& plan, const Problem& problem, const PlanningGraph* pg) const;

private:

    /* A flaw selection. */
    struct FlawSelection {
    
        /* The selected flaw. */
        const Flaw* flaw;
        
        /* Index of criterion used to select this flaw. */
        int criterion;
        
        /* Rank of this flaw if selected by a ranking criterion. */
        float rank;
        
        /* Counts the length of a streak, for use with random order. */
        int streak;
    };

    /* Selection criteria. */
    std::vector<SelectionCriterion> selection_criteria_;

    /* The name of this flaw selection order.*/
    std::string name_;
    
    /* Whether a planning graph is needed by this flaw selection order. */
    bool needs_pg_;
    
    /* Index of the first selection criterion involving threats. */
    int first_unsafe_criterion_;
    
    /* Index of the last selection criterion involving threats. */
    int last_unsafe_criterion_;
    
    /* Index of the first selection criterion involving open conditions. */
    int first_open_cond_criterion_;
    
    /* Index of the last selection criterion involving open conditions. */
    int last_open_cond_criterion_;

    /* Seaches threats for a flaw to select. */
    int select_unsafe(FlawSelection& selection, const Plan& plan,
        const Problem& problem,
        int first_criterion, int last_criterion) const;

    /* Seaches open conditions for a flaw to select. */
    int select_open_cond(FlawSelection& selection, const Plan& plan,
        const Problem& problem, const PlanningGraph* pg,
        int first_criterion, int last_criterion) const;

    /* Searches unexpanded abstract steps for a flaw to select. */
    int select_unexpanded_step(FlawSelection& selection, const Plan& plan,
        const Problem& problem,
        int first_criterion, int last_criterion) const;
};


#endif /* HEURISTICS_H */
