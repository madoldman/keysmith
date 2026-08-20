#
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2020-2021 Johan Ouwerkerk <jm.ouwerkerk@gmail.com>
#
set(model_SRCS
    accounts.cpp
    input.cpp
    output.cpp
    password.cpp
    qr.cpp
)

add_library(model_lib STATIC ${model_SRCS})
set_target_properties(model_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(model_lib Qt::Core Qt::Qml account_lib validator_lib uri_lib backups_lib)