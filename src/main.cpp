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
#include <iostream>
#include <string>
#include <algorithm>
#include <bitset>
#include <cstdint>
#include <getopt.h>

static std::string usage_message =
    "Usage: marl-agent [OPTION]...\n"
    "\n"
    "  -h ADDRESS, --host=ADDRESS\n"
    "                 Host name or IP address to start server on.\n"
    "  -p N, --port=N\n"
    "                 port number of server's main control connection (tcp).\n"
    "  -m PATH, --problem=PATH\n"
    "                 file name of the markov decision problem in mdpx format.\n"
    "  -s N, --start=N\n"
    "                 Starting point to solve problem. This is the index of the "
    "                 state to start to explore the state space. In case the given"
    "                 number is zero, then the start state will be assigned to a "
    "                 random number.\n"
    ;

void print_usage() {
    std::cerr << usage_message;
}

int main(int argc, char* argv[]) {
    // Options:
    std::string host;
    uint16_t port = 0;
    std::string problem_path;
    uint32_t start_state = 0;
    int c;
    std::bitset<4> args;
    while(true) {
        static struct option long_options[] = {
            {"host",     required_argument, 0, 'h'},
            {"port",     required_argument, 0, 'p'},
            {"problem",  required_argument, 0, 'm'},
            {"start",    required_argument, 0, 's'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "h:p:m:s:", long_options, &option_index);
        if(c == -1) {
            break;
        }
        switch(c) {
            case 'h':
                host = std::string{optarg};
                args[0] = true;
                break;
            case 'p':
                port = std::stoi(std::string{optarg});
                args[1] = true;
                break;
            case 'm':
                problem_path = std::string{optarg};
                args[2] = true;
                break;
            case 's':
                start_state = std::stoi(std::string{optarg});
                args[3] = true;
                break;
            default:
                std::cerr << "Unknown argument!\n";
                abort();
        }
    }
    if(!args.all()) {
        print_usage();
        return -1;
    }
    marl::agent a;
    a.initialize(problem_path, start_state);
    a.connect(host, port);
    a.start();
    a.wait();
    a.stop();
    return 0;
}
