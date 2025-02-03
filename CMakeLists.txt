cmake_minimum_required(VERSION 3.3...3.5)
###### Author: Alexis Devillard
###### Date:   25/05/2022
###### Brief: This CmakeLists.txt file enable to build generic project with the following structure:
###### ┌─────────┐                        ┌────────────────────┐
###### │ project │                        │ CMakeList.txt      │
###### └─┬───────┘                     ┌──┤ CMakeConfig.txt    │
######   │                             │  │ CmakeFunctions.txt │
######   ├───────────┬───────┬──────┬──┘  └────────────────────┘
######   │           │       │      │
######   │      ┌────┴────┐  │   ┌──┴──┐     ┌─────────────┐
######   │      │ include │  │   │ lib ├──┬──┤ subproject1 │
######   │      └────┬────┘  │   └─────┘  │  └──────┬──────┘
######   │           │       │            │         │
######   │      ┌────┴────┐  │    modules │     ┌───┴──────────────┐
######   │      │ lib.hpp │  │    for the │     │                  │
######   │      └─────────┘  │    library │  ┌──┴──┐          ┌────┴────┐
######   │                   │    target  │  │ src │          │ include │
######  ┌┴────┐              │            │  └─┬───┘          └────┬────┘
######  │ src │              │            │    │                   │
######  └─┬───┘              │            │    │ ┌──────────┐ ┌────┴────┐
######    │                  │            │    ├─┤ main.cpp │ │ lib.hpp │
######    │ ┌──────────┐     │            │    │ └──────────┘ └─────────┘
######    ├─┤ main.cpp │     │            │    │
######    │ └──────────┘     │            │    │ ┌─────────┐
######    │                  │            │    └─┤ lib.cpp │
######    │ ┌─────────┐      │            │      └─────────┘
######    └─┤ lib.cpp │      │            │
######      └─────────┘      │            │  ┌───────────────┐
######                       │            └──┤ subproject ...│
######                       │               └───────────────┘
######                       │
######                      ┌┴─────────┐     ┌─────────────┐
######                      │ tool_lib ├──┬──┤ subproject1 │
######                      └──────────┘  │  └──────┬──────┘
######                                    │         │
######                          modules   │     ┌───┴──────────────┐
######                          for the   │     │                  │
######                          exe file  │  ┌──┴──┐          ┌────┴────┐
######                          target    │  │ src │          │ include │
######                                    │  └─┬───┘          └────┬────┘
######                                    │    │                   │
######                                    │    │ ┌──────────┐ ┌────┴────┐
######                                    │    ├─┤ main.cpp │ │ lib.hpp │
######                                    │    │ └──────────┘ └─────────┘
######                                    │    │
######                                    │    │ ┌─────────┐
######                                    │    └─┤ lib.cpp │
######                                    │      └─────────┘
######                                    │
######                                    │  ┌───────────────┐
######                                    └──┤ subproject ...│
######                                       └───────────────┘

########### PROJECT NAME AND VERSION ######
include(CMakeConfig.cmake)
project(${EXEC_NAME} VERSION ${PROJECT_VERSION})
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_PYTHON "Build python wrapper" OFF)

###### LIBRARY NAME ######
set(LIB_NAME ${EXEC_NAME}.${PROJECT_VERSION})


###### include helper functions ######
include(cmake/ColorLogs.cmake)
include(cmake/CMakeFunctions.cmake)


###### Check if the project is already built ######
if(TARGET ${EXEC_NAME}) #check if the target is already built
  message("${PREFIX_MSG}-- ${ColourBold}${Esc}[${color_code}m${EXEC_NAME}${ColourReset} is already built")
elseif(TARGET ${LIB_NAME})
  message("${PREFIX_MSG}-- ${ColourBold}${Esc}[${color_code}m${LIB_NAME}${ColourReset} is already built")
else() #if not, build the target

  #display the compiler user and its version
  message("${PREFIX_MSG}-- ${ColourBold}${Esc}[${color_code}m${EXEC_NAME}${ColourReset}: Compiler: ${ColourBold}${CMAKE_CXX_COMPILER_ID}${ColourReset} ${CMAKE_CXX_COMPILER_VERSION}")
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} --version OUTPUT_VARIABLE COMPILER_VERSION)


  set(CMAKE_PREFIX_PATH "/opt/6.2.4/gcc_64")
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core GUI Widgets PrintSupport)
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets PrintSupport)
  find_package(Qt6 COMPONENTS PrintSupport REQUIRED)
  set(QT_QPA_PLATFORM "wayland")
  if(Qt${QT_VERSION_MAJOR}_FOUND)
    list(APPEND EXTRA_LIBS Qt6::Widgets)
  else()
    message("Couldn't find Qt library")
  endif(Qt${QT_VERSION_MAJOR}_FOUND)

  find_package(Boost 1.56 REQUIRED COMPONENTS system filesystem iostreams)
  if(Boost_FOUND)
    list(APPEND EXTRA_LIBS ${Boost_IOSTREAMS_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})
  else()
    message("Couldn't find Boost library")
  endif(Boost_FOUND)

  list(APPEND EXTRA_LIBS "fftw3")


  ###### Get the sources and headers ######
  ###### Exec main src file ######
  file(GLOB MAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

  ###### Library src and headezr files (everything else) ######
  file(GLOB LIB_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "src/*.c")
  list(REMOVE_ITEM LIB_SRCS ${MAIN_FILE})
  file(GLOB LIB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "include/*.hpp")
  list(REMOVE_ITEM LIB_HEADERS ${EXTRA_HEADERS})


  ###### check for internet ######
  include(cmake/ConnectionTest.cmake)

  ###### Search for Subdirectories/Libraries ######
  include(cmake/PullingLibs.cmake)


  ###### Create the library ######
  add_library(${LIB_NAME} ${LIB_SRCS})
  target_link_libraries(${LIB_NAME}
    PUBLIC ${EXTRA_LIBS})
  target_include_directories(${LIB_NAME}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )



  ###### Create the executable ######
  add_executable(${EXEC_NAME} ${MAIN_FILE} ${LIB_HEADERS} ${APP_FORM})
  target_link_libraries(${EXEC_NAME}
    PUBLIC ${LIB_NAME}
    PUBLIC ${TOOL_LIBS}
    PRIVATE Qt6::PrintSupport)
  target_include_directories(${EXEC_NAME}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

  qt_finalize_executable(${EXEC_NAME})


  # copy the script folder to the build directory
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/script DESTINATION ${CMAKE_CURRENT_BINARY_DIR})



  ###### Create package ######
  set(CPACK_PACKAGE_NAME "adev-${PROJECT_NAME}"
    CACHE STRING "The resulting package name"
  )
  include(cmake/Packing.cmake)

endif()



