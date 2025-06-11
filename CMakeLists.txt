cmake_minimum_required(VERSION 3.16)

project(deadlinelist VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
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

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia Network LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Multimedia Network LinguistTools)
if(WIN32)
    if(MSVC)
        set(OPENSSL_ROOT_DIR "C:\\Program Files\\OpenSSL-Win64")
    else()
        set(OPENSSL_ROOT_DIR "C:\\msys64\\mingw64")
    endif()
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()
find_package(OpenSSL REQUIRED)

qt_add_resources(QRCS resource/resource.qrc)
qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES en ru)


set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/mainwindow.ui
        src/tasks/edittask.cpp
        src/tasks/edittask.h
        src/tasks/edittask.ui
        src/settings.cpp
        src/settings.h
        src/settings.ui
        src/tasks/taskform.cpp
        src/tasks/taskform.h
        src/tasks/taskform.ui
        src/config.h
        src/binarydatahandler.cpp
        src/binarydatahandler.h
        src/button/shadowbutton.cpp
        src/button/shadowbutton.h
        src/network/authorizationform.cpp
        src/network/authorizationform.h
        src/network/authorizationform.ui
        src/button/spinnerbutton.cpp
        src/button/spinnerbutton.h
        src/network/securestorage.h
        src/network/securestorage.cpp
        src/network/registrationform.cpp
        src/network/registrationform.h
        src/network/registrationform.ui
        src/network/socketclient.cpp
        src/network/socketclient.h
        src/network/changepasswordform.h
        src/network/changepasswordform.cpp
        src/network/changepasswordform.ui
        src/network/otpinputwidget.h
        src/network/otpinputwidget.cpp
        src/network/passwordvalidator.h
        src/network/passwordvalidator.cpp
        src/tasks/deletedtaskdata.h
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

target_link_libraries(deadlinelist PRIVATE OpenSSL::SSL OpenSSL::Crypto OpenSSL::applink Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia Qt${QT_VERSION_MAJOR}::Network)
qt6_add_translations(deadlinelist
    QM_FILES_OUTPUT_VARIABLE qm_files
)
install(FILES ${qm_files} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/translations)
add_custom_command(
    TARGET deadlinelist POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/translations
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${qm_files}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/translations
)
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
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR}/translations)
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(deadlinelist)
endif()
