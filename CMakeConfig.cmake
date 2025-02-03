set(EXEC_NAME lslsub_plotter)
set(PROJECT_VERSION 1.12)

###### Special libraries ######
set(EXTRA_LIBS "")
set(TOOL_LIBS "")
if(0)
  set(EXTRA_LIBS "pthread" "util")
endif()

###### Boost libraries ######
if(0)
  find_package(Boost 1.56 REQUIRED COMPONENTS system filesystem iostreams)
  if(Boost_FOUND)
    list(APPEND EXTRA_LIBS ${Boost_IOSTREAMS_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})
  else()
    message("Couldn't find Boost library")
  endif(Boost_FOUND)
endif()

###### SFML libraries ######
if(0)
  find_package(SFML REQUIRED COMPONENTS graphics window system)
  if(SFML_FOUND)
    list(APPEND EXTRA_LIBS "sfml-graphics" "sfml-window" "sfml-system")
  else()
    message("Couldn't find SFML library")
  endif(SFML_FOUND)
endif()

###### OpenCV libraries ######
if(0)
  find_package(OpenCV REQUIRED)
  if(OpenCV_FOUND)
    list(APPEND EXTRA_LIBS ${OpenCV_LIBS})
    list(APPEND EXTRA_INCLUDES ${OpenCV_INCLUDE_DIRS})
    message("-- Opencv library Found")
  else()
    message("Couldn't find Opencv library")
  endif(OpenCV_FOUND)
endif()

###### QT libraries ######
if(0)
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
    message("Couldn't find Boost library")
  endif(Qt${QT_VERSION_MAJOR}_FOUND)
endif()


