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

marl::agent::agent():
    m_request_sequence{0} {

}

marl::action_select_rsp marl::agent::process_request(const action_select_req&) {
}

marl::response_base* marl::agent::process_action_select(const marl::request_base* const request) {
    marl::action_select_rsp* rsp = new action_select_rsp();
    rsp->agent_id = m_id;
    rsp->request_number = request->request_number;
    // Find what actions on requested state are present in current agents table
    for(const q_entry_t& entry : m_q_table) {
        if(entry.state == request->state_id) {
            action_info i;
            i.state = entry.state;
            i.action = entry.action;
            i.confidence = entry.confidence;
            i.q_value = entry.value;
            rsp->info.push_back(i);
        }
    }
    return rsp;
}

void marl::agent::set_ask_treshold(float value) {
    m_ask_treshold = value;
}

float marl::agent::ask_treshold() const {
    return m_ask_treshold;
}

void marl::agent::run() {
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
    // Run algorithm
    while(m_is_running.load()) {
        // Ask other agents
        action_select_req r;
        r.agent_id = m_id;
        r.confidence = m_visits.at(m_current_state->id());
        r.request_number = (++m_request_sequence);
        r.state_id = m_current_state->id();
        send_message<action_select_req>(r);
    }
}

float marl::agent::q(marl::state* s, marl::action* a) const {
    flog::logger* l = flog::logger::instance();
    if(!s || !a) {
        l->log(flog::level_t::ERROR_, "Invalid state or action pointer!");
        return 0.0;
    }
    // Validate if action actually belongs to this state
    bool found = std::any_of(s->actions().cbegin(),
                             s->actions().cend(),
    [&s, &a](const action*& aa)->bool {
        return aa->id() == a->id();
    });
    if(!found) {
        l->log(flog::level_t::ERROR_, "Action can not be initiated from state!");
        return 0;
    }
    // Find Q-Value and return it.
    for(const q_entry_t& qe : m_q_table) {
        if(qe.state == s->id(), qe.action == a->id()) {
            return qe.value;
        }
    }
    return q(s->id(), a->id());
}

float marl::agent::q(uint32_t s, uint32_t a) const {
    flog::logger* l = flog::logger::instance();
    // Find Q-Value and return it.
    for(const q_entry_t& qe : m_q_table) {
        if(qe.state == s, qe.action == a) {
            return qe.value;
        }
    }
    l->log(flog::level_t::ERROR_,
           "Can not find Q-Value for (%d, %d)...", s, a);
    return 0;
}
