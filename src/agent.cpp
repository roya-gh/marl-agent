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

#include "agent.hpp"
#include <marl-protocols/request-base.hpp>
#include <marl-protocols/action-select-response.hpp>

marl::agent::agent(uint32_t id):
    marl::client_base{id} {
}

marl::response_base* marl::agent::process_request(const marl::request_base* const request) {
    if(!request) {
        return nullptr;
    }
    switch(request->type()) {
        case MARL_ACTION_SELECT_REQ:
            return process_action_select(request);
            break;
        default:
            return nullptr;
            break;
    }
}

marl::response_base* marl::agent::process_action_select(const marl::request_base* const request) {
    marl::action_select_rsp* rsp = new action_select_rsp();
    rsp->agent_id = m_id;
    rsp->request_number = request->request_number;
    // Find what actions on requested state are present in current agents table
    for(const q_entry& entry : m_q_table) {
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
