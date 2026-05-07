#
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2020 Johan Ouwerkerk <jm.ouwerkerk@gmail.com>
#
set(oath_SRCS
    oath.cpp
)

add_library(oath_lib STATIC ${oath_SRCS})
set_target_properties(oath_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(oath_lib Qt::Core hmac_lib)