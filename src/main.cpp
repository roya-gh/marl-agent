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
    "  -n N, --episodes=N\n"
    "                 Number of episodes in learning stage.\n"
    "                 will be ignored on exploit mode.\n"
    "  -r N, --learning-rate=N\n"
    "                 The learning rate or step size determines to what extent newly\n"
    "                 acquired information overrides old information. A factor of 0\n"
    "                 makes the agent learn nothing (exclusively exploiting prior\n"
    "                 knowledge), while a factor of 1 makes the agent consider only\n"
    "                 the most recent information (ignoring prior knowledge to\n"
    "                 explore possibilities).\n"
    "                 You must provide a number between 0 and 1.\n"
    "                 Default value is: `0.01'.\n"
    "  -d N, --discount-factor=N\n"
    "                 The discount factor determines the importance of future rewards.\n"
    "                 A factor of 0 will make the agent \"myopic\" (or short-sighted)\n"
    "                 by only considering current rewards, while a factor approaching\n"
    "                 1 will make it strive for a long-term high reward.\n"
    "                 You must provide a number between 0 and 1.\n"
    "                 Default value is: `0.5'.\n"
    "  -t N, --temperature=N\n"
    "                 Temperature of the Boltzmann distribution used to decide which \n"
    "                 action to perform amongst possibilities.\n"
    "                 A factor of 0 will make the agent \"greedy\" by only considering\n"
    "                 best looking actions. The higher values will make the agent"
    "                 \"adventurer\" that is no matter what the learned experience"
    "                 suggests, the agent will likely perform random actions.\n"
    "                 You must provide a number between 0 and infinity.\n"
    "                 Default value is: `50'.\n"
    "  -o PATH, --policy-output=PATH\n"
    "                 File name to write learned policy to.\n"
    "                 Will be ignored on exploit mode.\n"
    "  -i PATH, --policy-input=PATH\n"
    "                 File name to read learned policy from.\n"
    "                 Will be ignored on learning mode.\n"
    "  -x PATH, --stats-file=PATH\n"
    "                 File name to write eisodes statistics into.\n"
    "  -v N, --log-level=N\n"
    "                 Verbosity level of the output. Possible values are:\n"
    "                   0 (OFF): Turns off logging.\n"
    "                   1 (FATAL): Logs very severe error events that will presumably\n"
    "                              lead the application to abort\n"
    "                   2 (ERROR): Logs error events that might still allow the\n"
    "                              application to continue running.\n"
    "                   3 (WARN):  Logs potentially harmful situations.\n"
    "                   4 (INFO):  Logs informational messages that highlight the\n"
    "                              progress of the application at coarse-grained\n"
    "                              level.\n"
    "                   6 (TRACE): Logs finer-grained informational events than the\n"
    "                              DEBUG\n"
    "                   7 (ALL):   Logs all events.\n"
    "                 Default value is: `7' (ALL).\n"
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
    std::string stats_path;
    uint32_t start_state = 0;
    uint32_t id = 0;
    uint32_t episodes = 0;
    flog::level_t log_level = flog::level_t::ALL;
    float learning_rate = 0.01;
    float temperature = 50;
    float discount_factor = 0.5;
    marl::learning_mode_t learning_mode = marl::learning_mode_t::learn;
    marl::operation_mode_t operation_mode = marl::operation_mode_t::single;
    int c;
    std::map<char, bool> set_arguments;
    char all_args[] = "hSPpasmlnoirtdv";
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
            {"episodes",     required_argument, 0, 'n'},
            {"operation-mode", required_argument, 0, 'm'},
            {"learning-mode",  required_argument, 0, 'l'},
            {"policy-output",  required_argument, 0, 'o'},
            {"policy-input",   required_argument, 0, 'i'},
            {"learning-rate",   required_argument, 0, 'r'},
            {"temperature",   required_argument, 0, 't'},
            {"discount-factor",   required_argument, 0, 'd'},
            {"log-level",   required_argument, 0, 'v'},
            {"stats-file",   required_argument, 0, 'x'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "hS:P:p:a:s:m:l:n:o:i:r:t:d:v:x:", long_options, &option_index);
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
            case 'x':
                stats_path = std::string{optarg};
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
            case 'r':
                learning_rate = std::stof(std::string{optarg});
                break;
            case 't':
                temperature = std::stof(std::string{optarg});
                break;
            case 'd':
                discount_factor = std::stof(std::string{optarg});
                break;
            case 'o':
                output_path = std::string{optarg};
                break;
            case 'i':
                input_path = std::string{optarg};
                break;
            case 'n':
                episodes = std::stoi(std::string{optarg});
                break;
            case 'v':
                log_level = static_cast<flog::level_t>(std::stoi(std::string{optarg}));
                flog::logger::instance()->set_level(log_level);
                break;
            default:
                std::cerr << "Unknown argument: " << (char)c << ',' << optarg << '\n';
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
                std::cerr << "ERROR: Iteration Count must be specified! (-n, --episodes)\n";
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
    a.set_learning_rate(learning_rate);
    a.set_temperature(temperature);
    a.set_discount_factor(discount_factor);
    a.set_stats_file(stats_path);
    if(operation_mode == marl::operation_mode_t::multi) {
        a.connect(host, port);
    }
    a.set_iterations(episodes);
    a.set_q_file_path(output_path);
    a.start();
    a.wait();
    flog::logger::instance()->flush(std::chrono::minutes{1});
    return 0;
}
