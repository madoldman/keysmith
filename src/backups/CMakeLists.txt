#
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2020 Johan Ouwerkerk <jm.ouwerkerk@gmail.com>
#
set(backup_SRCS
    backups.cpp
)

add_library(backups_lib STATIC ${backup_SRCS})
set_target_properties(backups_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(backups_lib Qt::Core OpenSSL::Crypto)