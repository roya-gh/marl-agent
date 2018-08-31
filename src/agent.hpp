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

class agent : public client_base {
public:
    agent();
    action_select_rsp process_request(const action_select_req&) override;
    //response_base* process_request(const request_base* const) override;
    //response_base* process_action_select(const request_base* const);
    //response_base* process_update_table(const request_base* const);
    void set_ask_treshold(float value);
    float ask_treshold() const;
    void load_q_table();
    void save_q_table();
    void set_q_file_path(const std::string& path);
    void set_stats_file(const std::string& path);
    void set_learning_rate(float);
    void set_temperature(float);
    void set_discount_factor(float);
protected:
    void print_q_table();
    void run() override;
    void run_single();
    void run_multi();
    void learn_single();
    void learn_multi();
    void exploit();
    // Helper functions
    float c(const state* s, const action* a) const;
    float q(const state* s, const action* a) const;
    float q(uint32_t s, uint32_t a) const;
    // Boltzmann distribution function for softmax selection
    size_t boltzmann_d(const std::vector<float> &values) const;
private:
    std::vector<action_info> aggregate_and_normalize(const std::vector<action_info>& v);

    std::string m_q_file_path;
    std::string m_stats_file_path;
    std::vector<q_entry_t> m_q_table;
    state_stats_t m_visits;
    float m_ask_treshold;
    float m_discount;           // gamma
    float m_learning_rate;      // alpha
    float m_temperature;        // tau
    marl::state* m_current_state;
    uint32_t m_request_sequence;
};
}
