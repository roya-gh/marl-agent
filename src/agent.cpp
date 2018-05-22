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
#include "agent.hpp"
#include <marl-protocols/request-base.hpp>
#include <marl-protocols/action-select-request.hpp>
#include <marl-protocols/action-select-response.hpp>
#include <marl-protocols/state.hpp>
#include <marl-protocols/action.hpp>
#include <flog/flog.hpp>

marl::agent::agent(operation_mode_t mode, learning_mode_t lm):
    m_request_sequence{0},
    m_operation_mode{mode},
    m_learning_mode{lm} {
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

uint32_t marl::agent::iterations() const {
    return m_iterations;
}

void marl::agent::set_iterations(uint32_t i) {
    m_iterations = i;
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

}

void marl::agent::learn_multi() {
    // Initialize constants
    m_tau = 0.4;
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
    //while(m_is_running.load()) {
    // Ask other agents
    action_select_req r;
    r.agent_id = m_id;
    r.confidence = m_visits.at(m_current_state->id());
    r.request_number = (++m_request_sequence);
    r.state_id = m_current_state->id();
    send_message(r);
    //}
}

void marl::agent::exploit() {

}

float marl::agent::q(marl::state* s, marl::action* a) const {
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
