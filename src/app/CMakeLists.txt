#
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2020-2021 Johan Ouwerkerk <jm.ouwerkerk@gmail.com>
#

set(keysmith_SRCS
    keysmith.cpp
    cli.cpp
    flows_p.cpp
    state_p.cpp
    vms.cpp
)

add_library(keysmith_lib STATIC ${keysmith_SRCS})
set_target_properties(keysmith_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)

kconfig_add_kcfg_files(keysmith_lib GENERATE_MOC ../stateconfig.kcfgc)

target_link_libraries(keysmith_lib
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Concurrent
        KF6::I18n
        KF6::ConfigCore
        KF6::ConfigGui
        model_lib
        account_lib
)

if (ENABLE_DBUS_INTERFACE)
    target_link_libraries(keysmith_lib PRIVATE KF6::DBusAddons KF6::WindowSystem)
endif()
