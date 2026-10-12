# Install script for directory: /home/tanay/ns-3-aqdc/src/stats

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "default")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/tanay/ns-3-aqdc/cmake-cache/src/stats/examples/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-stats-default.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-stats-default.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-stats-default.so"
         RPATH "/usr/local/lib:\$ORIGIN/:\$ORIGIN/../lib:/usr/local/lib64:\$ORIGIN/:\$ORIGIN/../lib64")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/tanay/ns-3-aqdc/build/lib/libns3.46.1-stats-default.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-stats-default.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-stats-default.so")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-stats-default.so"
         OLD_RPATH "/home/tanay/ns-3-aqdc/build/lib::::::::::::::::::::::::::::::::::::::::::::::::::"
         NEW_RPATH "/usr/local/lib:\$ORIGIN/:\$ORIGIN/../lib:/usr/local/lib64:\$ORIGIN/:\$ORIGIN/../lib64")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-stats-default.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ns3" TYPE FILE FILES
    "/home/tanay/ns-3-aqdc/src/stats/helper/file-helper.h"
    "/home/tanay/ns-3-aqdc/src/stats/helper/gnuplot-helper.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/average.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/basic-data-calculators.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/boolean-probe.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/data-calculator.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/data-collection-object.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/data-collector.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/data-output-interface.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/double-probe.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/file-aggregator.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/get-wildcard-matches.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/gnuplot-aggregator.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/gnuplot.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/histogram.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/omnet-data-output.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/probe.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/stats.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/time-data-calculators.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/time-probe.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/time-series-adaptor.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/uinteger-16-probe.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/uinteger-32-probe.h"
    "/home/tanay/ns-3-aqdc/src/stats/model/uinteger-8-probe.h"
    "/home/tanay/ns-3-aqdc/build/include/ns3/stats-module.h"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/tanay/ns-3-aqdc/cmake-cache/src/stats/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
