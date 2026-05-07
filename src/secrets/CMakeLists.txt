#
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2020-2021 Johan Ouwerkerk <jm.ouwerkerk@gmail.com>
#
set(secret_SRCS
    secrets.cpp
)

add_library(secrets_lib STATIC ${secret_SRCS})
set_target_properties(secrets_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(secrets_lib Qt::Core base32_lib sodium)