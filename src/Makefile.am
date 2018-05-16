# Copyright 2017 - Roya Ghasemzade <roya@ametisco.ir>
# Copyright 2017 - Soroush Rabiei <soroush@ametisco.ir>
#
# This file is part of marl_agent.
#
# marl_agent is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# marl_agent is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with marl_agent.  If not, see <http://www.gnu.org/licenses/>.

AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

AM_CXXFLAGS = --pedantic -Wall -Werror

bin_PROGRAMS = marl-agent

marl_agent_CPPFLAGS = \
    $(MARL_PROTOCOLS_CFLAGS) \
    $(FLOG_CFLAGS) \
    $(CPNET_CFLAGS)

marl_agent_LDADD = \
    -lpthread \
    $(MARL_PROTOCOLS_LIBS) \
    $(FLOG_LIBS) \
    $(CPNET_LIBS)

marl_agent_SOURCES = \
    agent.cpp \
    agent.hpp \
    q-table.cpp \
    q-table.hpp \
    main.cpp
