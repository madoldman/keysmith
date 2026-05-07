#
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2020 Johan Ouwerkerk <jm.ouwerkerk@gmail.com>
#
set(validator_SRCS
    countervalidator.cpp
    datetimevalidator.cpp
    issuervalidator.cpp
    namevalidator.cpp
    secretvalidator.cpp
    util.cpp
)

add_library(validator_lib STATIC ${validator_SRCS})
set_target_properties(validator_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(validator_lib Qt::Core Qt::Gui base32_lib)