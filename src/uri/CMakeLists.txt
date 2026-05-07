#
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2020 Johan Ouwerkerk <jm.ouwerkerk@gmail.com>
#
set(uri_SRCS
    uri.cpp
)

add_library(uri_lib STATIC ${uri_SRCS})
set_target_properties(uri_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(uri_lib Qt::Core base32_lib)