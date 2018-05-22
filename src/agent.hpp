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

#include <marl-protocols/client-base.hpp>
#include "q-table.hpp"
#include <marl-protocols/state.hpp>

namespace marl {

enum class operation_mode_t {
    single,
    multi,
};

enum class learning_mode_t {
    learn,
    exploit,
};

class agent : public client_base {
public:
    agent(operation_mode_t op = operation_mode_t::multi,
          learning_mode_t lm = learning_mode_t::learn);
    action_select_rsp process_request(const action_select_req&) override;
    //response_base* process_request(const request_base* const) override;
    //response_base* process_action_select(const request_base* const);
    //response_base* process_update_table(const request_base* const);
    void set_ask_treshold(float value);
    float ask_treshold() const;
    uint32_t iterations() const;
    void set_iterations(uint32_t);
protected:
    void run() override;
    void run_single();
    void learn_single();
    void learn_multi();
    void exploit();
    // Helper functions
    float q(state* s, action* a) const;
    float q(uint32_t s, uint32_t a) const;
private:
    qtable_t m_q_table;
    state_stats_t m_visits;
    float m_ask_treshold;
    float m_tau;
    marl::state* m_current_state;
    uint32_t m_request_sequence;
    operation_mode_t m_operation_mode;
    learning_mode_t m_learning_mode;
    uint32_t m_iterations;
};
}
