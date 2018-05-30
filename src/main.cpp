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
#include <flog/flog.hpp>
#include <getopt.h>
#include <string.h>

static std::string usage_message =
    //                                                                            >>>|
    "Usage: marl-agent [OPTION]...\n"
    "\n"
    "  -S ADDRESS, --server=ADDRESS\n"
    "                 Host name or IP address to connect to server.\n"
    "                 Will be ignored on single-agent mode.\n"
    "  -P N, --port=N\n"
    "                 Port number of server's main control connection (tcp).\n"
    "                 Will be ignored on single-agent mode.\n"
    "  -p PATH, --problem=PATH\n"
    "                 File name of the markov decision problem in mdpx format.\n"
    "  -a N, --agent-id=N\n"
    "                 Agent ID. Must be unique between all connectiong agents.\n"
    "                 Will be ignored on single-agent mode.\n"
    "  -s N, --start=N\n"
    "                 Starting point to solve problem. This is the index of the \n"
    "                 state to start to explore the state space. In case the given\n"
    "                 number is zero, then the start state will be assigned to a \n"
    "                 random number.\n"
    "  -m [single|multi], --operation-mode=[single|multi]\n"
    "                 Operation mode of the agent. Host and Port Number must be \n"
    "                 specified on multi-agent mode.\n"
    "                 Default mode is \"multi\".\n"
    "  -l [learn|exploit], --learning-mode=[learn|exploit]\n"
    "                 Specifies either the agent is learning a new policy or using a\n"
    "                 previously learned policy as an input. If learning mode is\n"
    "                 \"learn\" then '--policy-output' must be specified too. If\n"
    "                 learning mode is \"exploit\" then '--policy-input' parameter\n"
    "                 must be set.\n"
    "                 Default mode is \"learn\".\n"
    "  -n N, --iterations=N\n"
    "                 Number of iterations in learning stage.\n"
    "                 will be ignored on exploit mode.\n"
    "  -o PATH, --policy-output=PATH\n"
    "                 File name to write learned policy to.\n"
    "                 Will be ignored on exploit mode.\n"
    "  -i PATH, --policy-input=PATH\n"
    "                 File name to read learned policy from.\n"
    "                 Will be ignored on learning mode.\n"
    "  -h, --help\n"
    "                 Prints this message and exits.\n"
    ;

void print_usage() {
    std::cerr << usage_message;
}

int main(int argc, char* argv[]) {
    // Options:
    std::string host;
    uint16_t port = 0;
    std::string problem_path;
    std::string input_path;
    std::string output_path;
    uint32_t start_state = 0;
    uint32_t id = 0;
    uint32_t iterations = 0;
    marl::learning_mode_t learning_mode = marl::learning_mode_t::learn;
    marl::operation_mode_t operation_mode = marl::operation_mode_t::single;
    int c;
    std::map<char, bool> set_arguments;
    char all_args[] = "hSPpasmlnoi";
    for(size_t i = 0; i < strlen(all_args); ++i) {
        set_arguments[all_args[i]] = false;
    }
    while(true) {
        static struct option long_options[] = {
            {"help",     no_argument, 0, 'h'},
            {"server",   required_argument, 0, 'S'},
            {"port",     required_argument, 0, 'P'},
            {"problem",  required_argument, 0, 'p'},
            {"agent-id", required_argument, 0, 'a'},
            {"start",    required_argument, 0, 's'},
            {"operation-mode", required_argument, 0, 'm'},
            {"learning-mode",  required_argument, 0, 'l'},
            {"iterations",     required_argument, 0, 'n'},
            {"policy-output",  required_argument, 0, 'o'},
            {"policy-input",   required_argument, 0, 'i'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "hS:P:p:a:s:m:l:n:o:i", long_options, &option_index);
        if(c == -1) {
            break;
        }
        set_arguments[c] = true;
        switch(c) {
            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
                break;
            case 'S':
                host = std::string{optarg};
                break;
            case 'P':
                port = std::stoi(std::string{optarg});
                break;
            case 'p':
                problem_path = std::string{optarg};
                break;
            case 's':
                start_state = std::stoi(std::string{optarg});
                break;
            case 'a':
                id = std::stoi(std::string{optarg});
                break;
            case 'm':
                if(strcmp(optarg, "single") == 0) {
                    operation_mode = marl::operation_mode_t::single;
                } else if(strcmp(optarg, "multi") == 0) {
                    operation_mode = marl::operation_mode_t::multi;
                } else {
                    std::cerr << "Unknown operation mode: `" << optarg << "'!\n";
                    exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                if(strcmp(optarg, "learn") == 0) {
                    learning_mode = marl::learning_mode_t::learn;
                } else if(strcmp(optarg, "exploit") == 0) {
                    learning_mode = marl::learning_mode_t::exploit;
                } else {
                    std::cerr << "Unknown learning mode: `" << optarg << "'!\n";
                    exit(EXIT_FAILURE);
                }
                break;
            case 'n':
                iterations = std::stoi(std::string{optarg});
                break;
            case 'o':
                output_path = std::string{optarg};
                break;
            case 'i':
                input_path = std::string{optarg};
                break;
            default:
                std::cerr << "Unknown argument!\n";
                exit(EXIT_FAILURE);
        }
    }
    // Perform an initial sanity check
    if(!set_arguments.at('p')) {
        std::cerr << "ERROR: Problem file must be specified! ()\n";
        exit(EXIT_FAILURE);
    }
    if(!set_arguments.at('s')) {
        std::cerr << "ERROR: Initial state must be specified!\n";
        exit(EXIT_FAILURE);
    }
    switch(operation_mode) {
        case marl::operation_mode_t::multi:
            if(!set_arguments.at('S')) {
                std::cerr << "ERROR: Server address must be specified in multi-agent mode! (-S, --server)\n";
                exit(EXIT_FAILURE);
            }
            if(!set_arguments.at('P')) {
                std::cerr << "ERROR: Port Number must be specified in multi-agent mode! (-P, --port)\n";
                exit(EXIT_FAILURE);
            }
            if(learning_mode == marl::learning_mode_t::exploit) {
                std::cerr << "ERROR: Exploit mode is not allowed in multi-agent environment! (-l, --learning-mode)\n";
                exit(EXIT_FAILURE);
            }
            break;
        case marl::operation_mode_t::single:
            // nothing to do
            break;
        default:
            break;
    }
    switch(learning_mode) {
        case marl::learning_mode_t::learn:
            if(!set_arguments.at('o')) {
                std::cerr << "ERROR: Output Policy file must be specified! (-o, --policy-output)\n";
                exit(EXIT_FAILURE);
            }
            if(!set_arguments.at('n')) {
                std::cerr << "ERROR: Iteration Count must be specified! (-n, --iterations)\n";
                exit(EXIT_FAILURE);
            }
            break;
        case marl::learning_mode_t::exploit:
            if(!set_arguments.at('i')) {
                std::cerr << "ERROR: Input Policy file must be specified! (-i, --policy-input)\n";
                exit(EXIT_FAILURE);
            }
            break;
        default:
            break;
    }
    marl::agent a;
    a.initialize(operation_mode, learning_mode);
    a.initialize(problem_path, start_state, id);
    if(operation_mode == marl::operation_mode_t::multi) {
        a.connect(host, port);
    }
    a.set_iterations(iterations);
    a.set_q_file_path(output_path);
    a.start();
    a.wait();
    a.stop();
    flog::logger::instance()->flush(std::chrono::minutes{1});
    return 0;
}
