cmake_minimum_required(VERSION 3.16)

project(deadlinelist_server LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql)

add_executable(deadlinelist_server
  main.cpp
  server.h server.cpp
  cryptdata.h cryptdata.cpp
  smtpcredentials.h smtpcredentials.cpp
  smtpclient.h smtpclient.cpp
)
target_link_libraries(deadlinelist_server Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql)

include(GNUInstallDirs)
install(TARGETS deadlinelist_server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
