# Install script for directory: /home/tanay/ns-3-aqdc/src/core

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
  include("/home/tanay/ns-3-aqdc/cmake-cache/src/core/examples/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-core-default.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-core-default.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-core-default.so"
         RPATH "/usr/local/lib:\$ORIGIN/:\$ORIGIN/../lib:/usr/local/lib64:\$ORIGIN/:\$ORIGIN/../lib64")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/tanay/ns-3-aqdc/build/lib/libns3.46.1-core-default.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-core-default.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-core-default.so")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-core-default.so"
         OLD_RPATH "/home/tanay/ns-3-aqdc/build/lib::::::::::::::::::::::::::::::::::::::::::::::::::"
         NEW_RPATH "/usr/local/lib:\$ORIGIN/:\$ORIGIN/../lib:/usr/local/lib64:\$ORIGIN/:\$ORIGIN/../lib64")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.46.1-core-default.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ns3" TYPE FILE FILES
    "/home/tanay/ns-3-aqdc/build/include/ns3/core-config.h"
    "/home/tanay/ns-3-aqdc/src/core/model/int64x64-128.h"
    "/home/tanay/ns-3-aqdc/src/core/model/example-as-test.h"
    "/home/tanay/ns-3-aqdc/src/core/helper/csv-reader.h"
    "/home/tanay/ns-3-aqdc/src/core/helper/event-garbage-collector.h"
    "/home/tanay/ns-3-aqdc/src/core/helper/random-variable-stream-helper.h"
    "/home/tanay/ns-3-aqdc/src/core/model/abort.h"
    "/home/tanay/ns-3-aqdc/src/core/model/ascii-file.h"
    "/home/tanay/ns-3-aqdc/src/core/model/ascii-test.h"
    "/home/tanay/ns-3-aqdc/src/core/model/assert.h"
    "/home/tanay/ns-3-aqdc/src/core/model/attribute-accessor-helper.h"
    "/home/tanay/ns-3-aqdc/src/core/model/attribute-construction-list.h"
    "/home/tanay/ns-3-aqdc/src/core/model/attribute-container.h"
    "/home/tanay/ns-3-aqdc/src/core/model/attribute-helper.h"
    "/home/tanay/ns-3-aqdc/src/core/model/attribute.h"
    "/home/tanay/ns-3-aqdc/src/core/model/boolean.h"
    "/home/tanay/ns-3-aqdc/src/core/model/breakpoint.h"
    "/home/tanay/ns-3-aqdc/src/core/model/build-profile.h"
    "/home/tanay/ns-3-aqdc/src/core/model/calendar-scheduler.h"
    "/home/tanay/ns-3-aqdc/src/core/model/callback.h"
    "/home/tanay/ns-3-aqdc/src/core/model/command-line.h"
    "/home/tanay/ns-3-aqdc/src/core/model/config.h"
    "/home/tanay/ns-3-aqdc/src/core/model/default-deleter.h"
    "/home/tanay/ns-3-aqdc/src/core/model/default-simulator-impl.h"
    "/home/tanay/ns-3-aqdc/src/core/model/demangle.h"
    "/home/tanay/ns-3-aqdc/src/core/model/deprecated.h"
    "/home/tanay/ns-3-aqdc/src/core/model/des-metrics.h"
    "/home/tanay/ns-3-aqdc/src/core/model/double.h"
    "/home/tanay/ns-3-aqdc/src/core/model/enum.h"
    "/home/tanay/ns-3-aqdc/src/core/model/event-id.h"
    "/home/tanay/ns-3-aqdc/src/core/model/event-impl.h"
    "/home/tanay/ns-3-aqdc/src/core/model/fatal-error.h"
    "/home/tanay/ns-3-aqdc/src/core/model/fatal-impl.h"
    "/home/tanay/ns-3-aqdc/src/core/model/fd-reader.h"
    "/home/tanay/ns-3-aqdc/src/core/model/environment-variable.h"
    "/home/tanay/ns-3-aqdc/src/core/model/global-value.h"
    "/home/tanay/ns-3-aqdc/src/core/model/hash-fnv.h"
    "/home/tanay/ns-3-aqdc/src/core/model/hash-function.h"
    "/home/tanay/ns-3-aqdc/src/core/model/hash-murmur3.h"
    "/home/tanay/ns-3-aqdc/src/core/model/hash.h"
    "/home/tanay/ns-3-aqdc/src/core/model/heap-scheduler.h"
    "/home/tanay/ns-3-aqdc/src/core/model/int64x64-double.h"
    "/home/tanay/ns-3-aqdc/src/core/model/int64x64.h"
    "/home/tanay/ns-3-aqdc/src/core/model/integer.h"
    "/home/tanay/ns-3-aqdc/src/core/model/length.h"
    "/home/tanay/ns-3-aqdc/src/core/model/list-scheduler.h"
    "/home/tanay/ns-3-aqdc/src/core/model/log-macros-disabled.h"
    "/home/tanay/ns-3-aqdc/src/core/model/log-macros-enabled.h"
    "/home/tanay/ns-3-aqdc/src/core/model/log.h"
    "/home/tanay/ns-3-aqdc/src/core/model/make-event.h"
    "/home/tanay/ns-3-aqdc/src/core/model/map-scheduler.h"
    "/home/tanay/ns-3-aqdc/src/core/model/math.h"
    "/home/tanay/ns-3-aqdc/src/core/model/names.h"
    "/home/tanay/ns-3-aqdc/src/core/model/node-printer.h"
    "/home/tanay/ns-3-aqdc/src/core/model/nstime.h"
    "/home/tanay/ns-3-aqdc/src/core/model/object-base.h"
    "/home/tanay/ns-3-aqdc/src/core/model/object-factory.h"
    "/home/tanay/ns-3-aqdc/src/core/model/object-map.h"
    "/home/tanay/ns-3-aqdc/src/core/model/object-ptr-container.h"
    "/home/tanay/ns-3-aqdc/src/core/model/object-vector.h"
    "/home/tanay/ns-3-aqdc/src/core/model/object.h"
    "/home/tanay/ns-3-aqdc/src/core/model/pair.h"
    "/home/tanay/ns-3-aqdc/src/core/model/pointer.h"
    "/home/tanay/ns-3-aqdc/src/core/model/priority-queue-scheduler.h"
    "/home/tanay/ns-3-aqdc/src/core/model/ptr.h"
    "/home/tanay/ns-3-aqdc/src/core/model/random-variable-stream.h"
    "/home/tanay/ns-3-aqdc/src/core/model/rng-seed-manager.h"
    "/home/tanay/ns-3-aqdc/src/core/model/rng-stream.h"
    "/home/tanay/ns-3-aqdc/src/core/model/scheduler.h"
    "/home/tanay/ns-3-aqdc/src/core/model/show-progress.h"
    "/home/tanay/ns-3-aqdc/src/core/model/shuffle.h"
    "/home/tanay/ns-3-aqdc/src/core/model/simple-ref-count.h"
    "/home/tanay/ns-3-aqdc/src/core/model/simulation-singleton.h"
    "/home/tanay/ns-3-aqdc/src/core/model/simulator-impl.h"
    "/home/tanay/ns-3-aqdc/src/core/model/simulator.h"
    "/home/tanay/ns-3-aqdc/src/core/model/singleton.h"
    "/home/tanay/ns-3-aqdc/src/core/model/string.h"
    "/home/tanay/ns-3-aqdc/src/core/model/synchronizer.h"
    "/home/tanay/ns-3-aqdc/src/core/model/system-path.h"
    "/home/tanay/ns-3-aqdc/src/core/model/system-wall-clock-ms.h"
    "/home/tanay/ns-3-aqdc/src/core/model/system-wall-clock-timestamp.h"
    "/home/tanay/ns-3-aqdc/src/core/model/test.h"
    "/home/tanay/ns-3-aqdc/src/core/model/time-printer.h"
    "/home/tanay/ns-3-aqdc/src/core/model/timer-impl.h"
    "/home/tanay/ns-3-aqdc/src/core/model/timer.h"
    "/home/tanay/ns-3-aqdc/src/core/model/trace-source-accessor.h"
    "/home/tanay/ns-3-aqdc/src/core/model/traced-callback.h"
    "/home/tanay/ns-3-aqdc/src/core/model/traced-value.h"
    "/home/tanay/ns-3-aqdc/src/core/model/trickle-timer.h"
    "/home/tanay/ns-3-aqdc/src/core/model/tuple.h"
    "/home/tanay/ns-3-aqdc/src/core/model/type-id.h"
    "/home/tanay/ns-3-aqdc/src/core/model/type-name.h"
    "/home/tanay/ns-3-aqdc/src/core/model/type-traits.h"
    "/home/tanay/ns-3-aqdc/src/core/model/uinteger.h"
    "/home/tanay/ns-3-aqdc/src/core/model/uniform-random-bit-generator.h"
    "/home/tanay/ns-3-aqdc/src/core/model/valgrind.h"
    "/home/tanay/ns-3-aqdc/src/core/model/vector.h"
    "/home/tanay/ns-3-aqdc/src/core/model/warnings.h"
    "/home/tanay/ns-3-aqdc/src/core/model/watchdog.h"
    "/home/tanay/ns-3-aqdc/src/core/model/realtime-simulator-impl.h"
    "/home/tanay/ns-3-aqdc/src/core/model/wall-clock-synchronizer.h"
    "/home/tanay/ns-3-aqdc/src/core/model/val-array.h"
    "/home/tanay/ns-3-aqdc/src/core/model/matrix-array.h"
    "/home/tanay/ns-3-aqdc/build/include/ns3/core-module.h"
    "/home/tanay/ns-3-aqdc/build/include/ns3/core-export.h"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/tanay/ns-3-aqdc/cmake-cache/src/core/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
