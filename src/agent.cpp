/*
 * Copyright 2017 - Roya Ghasemzade <roya@ametisco.ir>
 * Copyright 2017 - Soroush Rabiei <soroush@ametisco.ir>
 *
 * This file is part of marl_agent.
 *
 * marl_agent is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * marl_agent is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with marl_agent.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <cmath>
#include <sstream>
#include <marl-protocols/request-base.hpp>
#include <marl-protocols/action-select-request.hpp>
#include <marl-protocols/action-select-response.hpp>
#include <marl-protocols/state.hpp>
#include <marl-protocols/action.hpp>
#include <flog/flog.hpp>
#include "prettyprint.hpp"
#include "agent.hpp"

marl::agent::agent():
    m_request_sequence{0} {
}

marl::action_select_rsp marl::agent::process_request(const action_select_req& request) {
    marl::action_select_rsp rsp;
    rsp.agent_id = m_id;
    rsp.request_number = request.request_number;
    rsp.requester_id = request.agent_id;
    // Find what actions on requested state are present in current agents table
    for(const q_entry_t& entry : m_q_table) {
        if(entry.state == request.state_id) {
            action_info i;
            i.state = entry.state;
            i.action = entry.action;
            i.confidence = entry.confidence;
            i.q_value = entry.value;
            rsp.info.push_back(i);
        }
    }
    return rsp;
}


//marl::action_select_rsp marl::agent::process_request(const action_select_req& req) {
//    action_select_rsp rsp;
//    rsp.agent_id = m_id;
//    rsp.request_number = req.request_number;
//    rsp.requester_id = req.agent_id;

//    // Fetch asker agent's state
//    auto finder = [&](const state * s)->bool { return s->id() == req.state_id; };
//    auto found_state_it = std::find_if(m_env.states().cbegin(), m_env.states().cend(), finder);
//    state* found_state = *found_state_it;

//    // Iterate over possible actions
//    for(action* a : found_state->actions()) {
//        action_info i;
//        i.state = req.state_id;
//        i.action = a->id();
//        i.confidence = this->confidence(found_state, a);
//        i.q_value = this->q(found_state, a);
//        rsp.info.push_back(i);
//    }

//    return rsp;
//}

//marl::response_base* marl::agent::process_action_select(const marl::request_base* const request) {
//    marl::action_select_rsp* rsp = new action_select_rsp();
//    rsp->agent_id = m_id;
//    rsp->request_number = request->request_number;
//    // Find what actions on requested state are present in current agents table
//    for(const q_entry_t& entry : m_q_table) {
//        if(entry.state == request->state_id) {
//            action_info i;
//            i.state = entry.state;
//            i.action = entry.action;
//            i.confidence = entry.confidence;
//            i.q_value = entry.value;
//            rsp->info.push_back(i);
//        }
//    }
//    return rsp;
//}

void marl::agent::set_ask_treshold(float value) {
    m_ask_treshold = value;
}

float marl::agent::ask_treshold() const {
    return m_ask_treshold;
}

void marl::agent::set_q_file_path(const std::string& path) {
    m_q_file_path = path;
    if(m_learning_mode == learning_mode_t::exploit) {
        load_q_table();
    }
}

void marl::agent::load_q_table() {
    std::ifstream file;
    file.open(m_q_file_path, std::ios_base::in);
    if(!file.is_open()) {
        flog::logger* l = flog::logger::instance();
        l->log(flog::level_t::ERROR_, "Unable to open file `%s'!",
               m_q_file_path.c_str());
        return;
    }
    q_entry_t e;
    while(file >> e.state >> e.action
            >> e.value >> e.confidence) {
        m_q_table.push_back(e);
    }
    file.close();
}

void marl::agent::print_q_table() {
    flog::logger* l = flog::logger::instance();
    l->log(flog::level_t::INFO, "Q Table is:");
    l->logc(flog::level_t::INFO, "#State\tAction\tValue\tConfidence\n");
    for(const q_entry_t& e : m_q_table) {
        l->logc(flog::level_t::INFO, "%d\t%d\t%f\t%f",
                e.state, e.action, e.value, e.confidence);
    }
}

void marl::agent::save_q_table() {
    std::ofstream myfile;
    myfile.open(m_q_file_path, std::ios_base::out);
    myfile << "#State\tAction\tValue\tConfidence\n";
    for(const q_entry_t& e : m_q_table) {
        myfile << e.state << '\t' << e.action << '\t'
               << e.value << '\t' << e.confidence << '\n';
    }
    myfile.close();
}

void marl::agent::run() {
    switch(m_operation_mode) {
        case operation_mode_t::multi:
            learn_multi();
            break;
        case operation_mode_t::single:
            run_single();
            break;
        default:
            break;
    }
}

void marl::agent::run_single() {
    switch(m_learning_mode) {
        case learning_mode_t::learn:
            learn_single();
            break;
        case learning_mode_t::exploit:
            exploit();
            break;
        default:
            break;
    }
}

void marl::agent::learn_single() {
    flog::logger* l = flog::logger::instance();
    // Initialize constants
    m_learning_factor = 0.1;
    m_learning_rate = 0.2;
    m_discount = 0.9;
    // Initialize Q-Table
    for(const marl::action* a : m_env.actions()) {
        q_entry_t e;
        e.action = a->id();
        e.state = a->from()->id();
        e.confidence = 0;
        e.value = 0.5;
        m_q_table.push_back(e);
        for(transition* t : a->transitions()) {
            l->log(flog::level_t::INFO, "From %d to %d reward is: %f",
                   a->from()->id(), t->to()->id(), t->reward());
        }
    }
    save_q_table();
    // Initialize current state
    m_current_state = m_env.states().at(m_start_index);
    // Run algorithm
    for(uint32_t i = 1; i <= m_iterations; ++i) {
        l->log(flog::level_t::INFO, "Iteration %d starts..", i);
        // Compute action probabilities
        l->logc(flog::level_t::INFO, "Current State: %d", m_current_state->id());
        std::vector<float> m_qs;
        for(const action* a : m_current_state->actions()) {
            m_qs.push_back(q(m_current_state, a));
        }
        size_t selection = boltzmann_d(m_qs);
        action* selected_action = m_current_state->actions().at(selection);
        l->logc(flog::level_t::INFO, "Selected Action: %d", selected_action->id());
        // Perform the move
        // TODO: Check for non-stattionary problems
        state* old_state = m_current_state;
        const transition* t = selected_action->transitions().at(0);
        m_current_state = t->to();
        float reward = t->reward();
        l->logc(flog::level_t::INFO, "Observed Reward: %f", reward);
        // Find entry to update
        auto finder = [&selected_action, &old_state](const q_entry_t& e) {
            return e.action == selected_action->id()
                   && e.state == old_state->id();
        };
        // Update Q Table
        auto item = std::find_if(m_q_table.begin(), m_q_table.end(),
                                 finder);
        if(item == m_q_table.end()) {
            l->log(flog::level_t::ERROR_, "Invalid or incomplete Q-Table!");
            break;
        }
        // calculate max_q for current state (which is after move)
        float max_q = 0;
        for(action* a : m_current_state->actions()) {
            float new_max = q(m_current_state, a);
            //l->logc(flog::level_t::INFO, "New Max: %f", new_max);
            max_q = (new_max > max_q) ? new_max : max_q;
        }
        l->logc(flog::level_t::INFO, "Maximum Q: %f", max_q);
        item->value = (1.0f - m_learning_rate) * q(old_state, selected_action)
                      + m_learning_rate * (reward + m_discount * max_q);
        item->confidence += 0.001;
        l->logc(flog::level_t::INFO, "Updated Q(%d, %d): %f",
                m_current_state->id(), selected_action->id(), item->value);
        print_q_table();
    }
    save_q_table();
}

void marl::agent::learn_multi() {
    // Initialize constants
    m_learning_factor = 0.4;
    // Initialize visits
    m_visits.clear();
    for(const marl::state* s : m_env.states()) {
        m_visits.emplace(s->id(), 0);
    }
    // Initialize Q-Table
    for(const marl::action* a : m_env.actions()) {
        q_entry_t e;
        e.action = a->id();
        e.state = a->from()->id();
        e.confidence = 0.0;
        e.value = 0;
        m_q_table.push_back(e);
    }
    // Initialize current state
    m_current_state = m_env.states().at(m_start_index);
    // Run algorithm
    uint32_t i = 0;
    while(m_is_running.load() && i < m_iterations) {
        // Ask other agents
        action_select_req r;
        r.agent_id = m_id;
        r.confidence = m_visits.at(m_current_state->id());
        r.request_number = (++m_request_sequence);
        r.state_id = m_current_state->id();
        send_message(r);
    }
}

void marl::agent::exploit() {

}

float marl::agent::q(const state* s, const marl::action* a) const {
    flog::logger* l = flog::logger::instance();
    if(!s || !a) {
        l->log(flog::level_t::ERROR_, "Invalid state or action pointer!");
        return 0.0;
    }
    // Validate if action actually belongs to this state
    auto finder = [&](action * aa)->bool {
        return aa->id() == a->id();
    };
    bool found = std::any_of(s->actions().cbegin(),
                             s->actions().cend(), finder);
    if(!found) {
        l->log(flog::level_t::ERROR_, "Action can not be initiated from state!");
        return 0;
    }
    // Find Q-Value and return it.
    for(const q_entry_t& qe : m_q_table) {
        if(qe.state == s->id() && qe.action == a->id()) {
            return qe.value;
        }
    }
    l->log(flog::level_t::ERROR_, "Can not find Q-Value for %d and %d!",
           s->id(), a->id());
    return q(s->id(), a->id());
}

float marl::agent::q(uint32_t s, uint32_t a) const {
    flog::logger* l = flog::logger::instance();
    // Find Q-Value and return it.
    for(const q_entry_t& qe : m_q_table) {
        if(qe.state == s && qe.action == a) {
            return qe.value;
        }
    }
    l->log(flog::level_t::ERROR_,
           "Can not find Q-Value for (%d, %d)...", s, a);
    return 0;
}

size_t marl::agent::boltzmann_d(const std::vector<float> values) const {
    static std::random_device rd;
    static std::mt19937 e2(rd());
    static std::uniform_real_distribution<> dist(0, 1);

    flog::logger* l = flog::logger::instance();

    std::vector<float> probabilites;
    float total = 0.0;
    for(float value : values) {
        total += exp(value / m_learning_factor);
    }
    for(float value : values) {
        const float p = exp(value / m_learning_factor) / total;
        probabilites.push_back(p);
    }
    float rand = dist(e2);
    //    l->log(flog::level_t::INFO, "Input size: %zd", values.size());
    //    l->logc(flog::level_t::INFO, "Random seed: %f", rand);
    float max_p = 0;
    size_t selected = 0;
    for(float value : probabilites) {
        max_p += value;
        //        l->logc(flog::level_t::INFO, "Value= %f, Max(P)=%f", value, max_p);
        if(rand < max_p) {
            break;
        } else {
            selected++;
        }
    }
    std::stringstream ss;
    ss << values;
    //    l->logc(flog::level_t::INFO, "   input: %s", ss.str().c_str());
    ss.str("");
    ss << probabilites;
    //    l->logc(flog::level_t::INFO, "P vector: %s", ss.str().c_str());
    ss.str("");
    //    l->logc(flog::level_t::INFO, "Selected: %zd", selected);
    //    l->logc(flog::level_t::INFO, "Selected: (%f)", values.at(selected));
    return selected;
}
