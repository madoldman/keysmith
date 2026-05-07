#
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2020 Johan Ouwerkerk <jm.ouwerkerk@gmail.com>
#
set(base32_SRCS
    base32.cpp
)

add_library(base32_lib STATIC ${base32_SRCS})
set_target_properties(base32_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(base32_lib Qt::Core)