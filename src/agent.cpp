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

#ifdef __DBL_DECIMAL_DIG__
#define OP_DBL_DIGS (__DBL_DECIMAL_DIG__)
#else
#ifdef __DECIMAL_DIG__
#define OP_DBL_DIGS (__DECIMAL_DIG__)
#else
#define OP_DBL_DIGS (__DBL_DIG__ + 3)
#endif
#endif

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

void marl::agent::set_stats_file(const std::string& path) {
    m_stats_file_path = path;
}

void marl::agent::set_learning_rate(float r) {
    m_learning_rate = r;
}

void marl::agent::set_temperature(float f) {
    m_temperature = f;
}

void marl::agent::set_discount_factor(float d) {
    m_discount = d;
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
    myfile << std::setprecision(std::numeric_limits<double>::digits10 + 1) << std::fixed;
    for(const q_entry_t& e : m_q_table) {
        myfile << e.state << '\t' << e.action << '\t'
               << e.value << '\t' << e.confidence << '\n';
    }
    myfile.close();
}

void marl::agent::run() {
    switch(m_operation_mode) {
        case operation_mode_t::multi:
            run_multi();
            break;
        case operation_mode_t::single:
            run_single();
            break;
        default:
            break;
    }
    terminate();
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

void marl::agent::run_multi() {
    /* Running agent in multi-agent and exploit modes simulatenously does not
     * make sense
     */
    learn_multi();
}

void marl::agent::learn_single() {
    flog::logger* l = flog::logger::instance();
    // Initialize random engine
    static std::random_device r;
    static std::default_random_engine e1(r());
    static std::uniform_int_distribution<int> uniform_dist(0, m_env.states().size() - 1);
    // Initialize Q-Table
    for(const marl::action* a : m_env.actions()) {
        q_entry_t e;
        e.action = a->id();
        e.state = a->from()->id();
        e.confidence = 0;
        e.value = 0.0;
        m_q_table.push_back(e);
        for(transition* t : a->transitions()) {
            l->log(flog::level_t::TRACE, "From %d to %d reward is: %f",
                   a->from()->id(), t->to()->id(), t->reward());
        }
    }
    // Initialize current state to a random number
    if(m_start_index == -1) {
        m_current_state = m_env.states().at(uniform_dist(e1));
    } else {
        m_current_state = m_env.states().at(m_start_index);
    }
    // Open Statistics File
    std::ofstream stat_file;
    stat_file.open(m_stats_file_path, std::ios_base::out | std::ios_base::trunc);
    stat_file << "#Episode Steps\n";
    // Run algorithm
    uint32_t episode = 1;
    uint32_t step = 0;
    while(episode < m_iterations) {
        l->log(flog::level_t::TRACE, "Running step: %d", step++);
        // Compute action probabilities
        l->logc(flog::level_t::TRACE, "Current State: %d", m_current_state->id());
        std::vector<float> m_qs;
        for(const action* a : m_current_state->actions()) {
            m_qs.push_back(q(m_current_state, a));
        }
        size_t selection = boltzmann_d(m_qs);
        action* selected_action = m_current_state->actions().at(selection);
        l->logc(flog::level_t::TRACE, "Selected Action: %d", selected_action->id());
        // Perform the move
        // TODO: Check for non-stattionary problems
        state* old_state = m_current_state;
        const transition* t = selected_action->transitions().at(0);
        m_current_state = t->to();
        float reward = t->reward();
        l->logc(flog::level_t::TRACE, "Observed Reward: %f", reward);
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
            //l->logc(flog::level_t::TRACE, "New Max: %f", new_max);
            max_q = (new_max > max_q) ? new_max : max_q;
        }
        l->logc(flog::level_t::TRACE, "Maximum Q: %f", max_q);
        item->value = (1.0f - m_learning_rate) * q(old_state, selected_action)
                      + m_learning_rate * (reward + m_discount * max_q);
        item->confidence += 0.001;
        l->logc(flog::level_t::TRACE, "Updated Q(%d, %d): %f",
                m_current_state->id(), selected_action->id(), item->value);
        //print_q_table();
        if(reward == 1.0) {
            l->log(flog::level_t::INFO, "Episode: %d Value: %d", episode, step);
            stat_file << std::setprecision(17);
            stat_file << std::setprecision(17) << std::fixed << episode << ' ' << step << '\n';
            if(episode % 100 == 0) {
                stat_file.flush();
            }
            episode++;
            step = 0;
            l->log(flog::level_t::INFO, "Goal reached. Trying a new state.");
            m_current_state = m_env.states().at(uniform_dist(e1));
            l->log(flog::level_t::INFO, "New state is: %d.", m_current_state->id());
            // print_q_table();
        }
    }
    stat_file.close();
    save_q_table();
}

void marl::agent::learn_multi() {
    flog::logger* l = flog::logger::instance();
    // Initialize random engine
    static std::random_device r;
    static std::default_random_engine e1(r());
    static std::uniform_int_distribution<int> uniform_dist(0, m_env.states().size() - 1);
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
    // Initialize current state to a random number
    if(m_start_index == -1) {
        m_current_state = m_env.states().at(uniform_dist(e1));
    } else {
        m_current_state = m_env.states().at(m_start_index);
    }
    l->log(flog::level_t::INFO, "Starting at state: %d", m_current_state->id());
    // Run algorithm
    uint32_t episode = 1;
    uint32_t step = 0;
    l->log(flog::level_t::TRACE, "Iteration count: %d", m_iterations);
    // Open Statistics File
    std::ofstream stat_file;
    stat_file.open(m_stats_file_path, std::ios_base::out | std::ios_base::trunc);
    stat_file << "#Episode Steps\n";

    while(m_is_running.load() && episode < m_iterations) {
        l->log(flog::level_t::TRACE, "Running episode: %d", step++);
        // Ask other agents
        action_select_req r;
        r.agent_id = m_id;
        r.confidence = m_visits.at(m_current_state->id());
        r.request_number = (++m_request_sequence);
        r.state_id = m_current_state->id();

        this->set_rendezvous(r.request_number);
        send_message(r);
        //        l->log(flog::level_t::TRACE,
        //               "Waiting for response for request: %d", r.request_number);
        std::unique_ptr<marl::response_base> r_base = this->get_response(r.request_number);
        marl::action_select_rsp* response = dynamic_cast<marl::action_select_rsp*>(r_base.get());
        if(!response) {
            // TODO: error
            continue;
        }
        // Sanity check
        if(response->request_number != r.request_number) {
            l->log(flog::level_t::ERROR_,
                   "Requests does not match! %d!=%d",
                   response->request_number, r.request_number);
            // TODO: handle error
            continue;
        } else {
            l->log(flog::level_t::TRACE,
                   "Responce received from server. Details: ");
            l->logc(flog::level_t::TRACE,
                    "Request Number: %d", response->request_number);
            l->logc(flog::level_t::TRACE,
                    "Size: %z", response->info.size());
            for(const action_info& reply : response->info) {
                l->logc(flog::level_t::TRACE,
                        "Action: %d, State: %d, Confidence: %f, Q-Value: %f",
                        reply.action, reply.confidence, reply.q_value, reply.state);
            }
        }
        for(const action* a : m_current_state->actions()) {
            action_info i;
            i.action = a->id();
            i.state = m_current_state->id();
            i.q_value = q(m_current_state->id(), a->id());
            i.confidence = c(m_current_state, a);
            response->info.push_back(i);
        }
        // perform action based on the reply
        std::vector<action_info> concensus = aggregate_and_normalize(response->info);
        // Update Q-Table based on new information
        for(q_entry_t& e: m_q_table) {
            for(action_info& a: concensus) {
                if(e.action == a.action) {
                    e.value = a.q_value;
                    break;
                }
            }
        }

        std::vector<float> m_qs;
        for(const action_info a : concensus) {
            m_qs.push_back(a.q_value);
        }
        size_t selection = boltzmann_d(m_qs);
        //        l->log(flog::level_t::INFO, "Boltzman: %zd of %zd",
        //               selection, concensus.size());
        uint32_t action_id = concensus.at(selection).action;
        action* selected_action = nullptr;
        for(action* a  : m_current_state->actions()) {
            if(a->id() == action_id) {
                selected_action = a;
                break;
            }
        }
        if(!selected_action) {
            // TODO check for error source,
            continue;
        }
        // perform actual action
        l->logc(flog::level_t::TRACE, "Selected Action: %d", selected_action->id());
        state* old_state = m_current_state;
        const transition* t = selected_action->transitions().at(0);
        m_current_state = t->to();
        float reward = t->reward();
        l->logc(flog::level_t::TRACE, "Observed Reward: %f", reward);
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
            l->logc(flog::level_t::TRACE, "New Max: %f", new_max);
            max_q = (new_max > max_q) ? new_max : max_q;
        }
        //        l->logc(flog::level_t::TRACE, "Maximum Q: %f", max_q);
        item->value = (1.0f - m_learning_rate) * q(old_state, selected_action)
                      + m_learning_rate * (reward + m_discount * max_q);
        item->confidence += 0.1;
        l->logc(flog::level_t::TRACE, "Updated Q(%d, %d): %f",
                m_current_state->id(), selected_action->id(), item->value);
        if(reward == 1.0) {
            l->log(flog::level_t::INFO, "Episode: %d Value: %d", episode, step);
            stat_file << episode << ' ' << step << '\n';
            if(episode % 100 == 0) {
                stat_file.flush();
            }
            episode++;
            step = 0;
            l->log(flog::level_t::INFO, "Goal reached. Trying a new state.");
            m_current_state = m_env.states().at(uniform_dist(e1));
            l->log(flog::level_t::INFO, "New state is: %d.", m_current_state->id());
            // print_q_table();
        }
    }
    stat_file.close();
    save_q_table();
}

void marl::agent::exploit() {

}

float marl::agent::c(const marl::state* s, const marl::action* a) const {
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
    // Find confidence and return it.
    for(const q_entry_t& qe : m_q_table) {
        if(qe.state == s->id() && qe.action == a->id()) {
            return qe.confidence;
        }
    }
    l->log(flog::level_t::ERROR_, "Can not find Q-Value for %d and %d!",
           s->id(), a->id());
    return 0;
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

size_t marl::agent::boltzmann_d(const std::vector<float>& values) const {
    static std::random_device rd;
    static std::mt19937 e2(rd());
    static std::uniform_real_distribution<> dist(0, 1);

    flog::logger* l = flog::logger::instance();

    std::vector<float> probabilites;
    float total = 0.0;
    for(float value : values) {
        total += exp(value / m_temperature);
    }
    for(float value : values) {
        const float p = exp(value / m_temperature) / total;
        probabilites.push_back(p);
    }
    float rand = dist(e2);
    l->log(flog::level_t::TRACE, "Input size: %zd", values.size());
    l->logc(flog::level_t::TRACE, "Random seed: %f", rand);
    float max_p = 0;
    size_t selected = 0;

    for(float value : probabilites) {
        max_p += value;
        l->logc(flog::level_t::TRACE, "Value= %f, Max(P)=%f", value, max_p);
        if(rand <= max_p) {
            break;
        } else {
            selected++;
        }
    }
    if(selected >= values.size()) {
        selected = values.size() - 1;
    }
    /*
    std::stringstream ss;
    ss << values;
    l->logc(flog::level_t::INFO, "   input: %s", ss.str().c_str());
    ss.str("");
    ss << probabilites;
    l->logc(flog::level_t::INFO, "P vector: %s", ss.str().c_str());
    ss.str("");
    l->logc(flog::level_t::INFO, "Selected: %zd", selected);
    l->logc(flog::level_t::INFO, "Selected: (%f)", values.at(selected));
    */
    return selected;
}


std::vector<marl::action_info> marl::agent::aggregate_and_normalize(const std::vector<marl::action_info>& v) {
    std::vector<marl::action_info> r;
    for(const action_info& i : v) {
        auto finder =
        [&i](const action_info & item) -> bool {
            return item.action == i.action;
        };
        auto found = std::find_if(r.begin(), r.end(), finder);
        if(found == r.end()) {
            action_info first = i;
            first.q_value = first.confidence * first.q_value;
            r.push_back(first);
        } else {
            found->q_value += i.confidence * i.q_value;
            found->confidence += i.confidence;
        }
    }
    // TODO: remove actions which are not accepted in current state
    // add self-opinion on the consensus
    for(action* a : m_current_state->actions()) {
        for(action_info& ai : r) {
            if(ai.action == a->id()) {
                ai.confidence += c(m_current_state, a) ;
                ai.q_value += c(m_current_state, a) * q(m_current_state, a);
            }
        }
    }
    // normalize
    for(action_info& ai : r) {
        if(ai.confidence != 0.0) {
            ai.q_value = ai.q_value / ai.confidence;
        } else {
            ai.q_value = 0.0;
        }
    }
    return r;
}
