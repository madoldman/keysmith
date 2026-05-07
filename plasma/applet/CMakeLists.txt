qt_add_qml_module(plasma_applet_keysmith
    URI org.kde.plasma.private.keysmith
    VERSION 1.0
    SOURCES contents/code/keysmithapplet.cpp
    PLUGIN_TARGET plasma_applet_keysmith
)

set_target_properties(plasma_applet_keysmith PROPERTIES PREFIX "lib")

target_link_libraries(plasma_applet_keysmith
    PUBLIC
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
    KF6::I18n
    keysmith_lib
)

target_include_directories(plasma_applet_keysmith PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/contents/code
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)

install(FILES metadata.json DESTINATION ${KDE_INSTALL_DATADIR}/plasma/plasmoids/org.kde.plasma.keysmith)
install(FILES contents/ui/main.qml DESTINATION ${KDE_INSTALL_DATADIR}/plasma/plasmoids/org.kde.plasma.keysmith/contents/ui)

set(QML_MODULE_INSTALL_DIR ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/keysmith)
install(TARGETS plasma_applet_keysmith DESTINATION ${QML_MODULE_INSTALL_DIR})
install(FILES ${CMAKE_BINARY_DIR}/bin/org/kde/plasma/private/keysmith/qmldir DESTINATION ${QML_MODULE_INSTALL_DIR} RENAME qmldir)
install(FILES ${CMAKE_BINARY_DIR}/bin/org/kde/plasma/private/keysmith/plasma_applet_keysmith.qmltypes DESTINATION ${QML_MODULE_INSTALL_DIR})
