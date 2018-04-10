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

namespace marl {
class agent : public client_base {
public:
    agent(uint32_t id = 0);
    response_base* process_request(const request_base* const) override;
    response_base* process_action_select(const request_base* const);
    //response_base* process_update_table(const request_base* const);
private:
    qtable_t m_q_table;
};
}
