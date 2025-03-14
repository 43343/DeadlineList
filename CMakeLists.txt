cmake_minimum_required(VERSION 3.16)

project(deadlinelist VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_WITH_QT6 ON)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "resource/app_icon.rc")
endif()

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Multimedia Network)
add_subdirectory(qtkeychain/)

qt_add_resources(QRCS resource/resource.qrc)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        edittask.cpp
        edittask.h
        edittask.ui
        settings.cpp
        settings.h
        settings.ui
        taskform.cpp
        taskform.h
        taskform.ui
        config.h
        binarydatahandler.cpp
        binarydatahandler.h
        shadowbutton.cpp
        shadowbutton.h
        authorizationform.cpp
        authorizationform.h
        authorizationform.ui
        spinnerbutton.cpp
        spinnerbutton.h
        registrationform.cpp
        registrationform.h
        registrationform.ui
        socketclient.cpp
        socketclient.h
        keychainclass.cpp
        keychainclass.h
        changepasswordform.h
        changepasswordform.cpp
        changepasswordform.ui
        otpinputwidget.h
        otpinputwidget.cpp
        passwordvalidator.h
        passwordvalidator.cpp
        ${QRCS}
)

if(WIN32)
    list(APPEND PROJECT_SOURCES ${APP_ICON_RESOURCE_WINDOWS})
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(deadlinelist
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET deadlinelist APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(deadlinelist SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(deadlinelist
            ${PROJECT_SOURCES}
        )
    endif()
endif()

message(STATUS "QTKeychain include directories: ${QTKEYCHAIN_INCLUDE_DIRS}")
message(STATUS "QTKeychain include libraries: " ${QTKEYCHAIN_LIBRARIES})


target_link_libraries(deadlinelist PRIVATE qt6keychain Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia Qt${QT_VERSION_MAJOR}::Network)
target_link_directories(deadlinelist PRIVATE ${QTKEYCHAIN_INCLUDE_DIRS})

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.deadlinelist)
endif()
set_target_properties(deadlinelist PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS deadlinelist
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(deadlinelist)
endif()
