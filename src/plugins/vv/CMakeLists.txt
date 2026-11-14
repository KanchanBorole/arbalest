SET(CMAKE_AUTOMOC ON)
FIND_PACKAGE(Qt6 COMPONENTS Widgets REQUIRED)

INCLUDE_DIRECTORIES(
    ${Qt6Widgets_INCLUDE_DIRS}
)

SET(VVPluginSources
    VVPlugin.cpp
    VVController.cpp
    core/VVBackendTests.cpp
    ui/VVDockWidget.cpp
    ui/VVTestSelectionDialog.cpp
    ui/VVWidget.cpp
)

IF(MSVC)
    ADD_DEFINITIONS("-DBRLCAD_MOOSE_EXPORT=__declspec(dllimport)")
ELSE(MSVC)
    ADD_DEFINITIONS("-DBRLCAD_MOOSE_EXPORT=")
ENDIF(MSVC)

ADD_LIBRARY(VVPlugin STATIC ${VVPluginSources})
