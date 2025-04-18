# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Read README.cmake before using this.

# CMAKE_MINIMUM_REQUIRED should be the first directive in the file:
# https://cmake.org/cmake/help/latest/command/cmake_minimum_required.html
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(APR C)

# Enable support for MSVC runtime library selection by abstraction
# if supported by CMake.
IF(POLICY CMP0091)
  CMAKE_POLICY(SET CMP0091 NEW)
ENDIF()

# Add simple support CMAKE_WARNING_AS_ERROR if CMake doesn't
# support it.
IF (CMAKE_VERSION VERSION_LESS 3.24)
  IF(CMAKE_WARNING_AS_ERROR AND MSVC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
  ENDIF()
ENDIF()

SET(apr_name    apr-1)
SET(apr_libname libapr-1)

OPTION(APR_INSTALL_PRIVATE_H  "Install selected private .h files (for httpd)"  OFF)
OPTION(APR_HAVE_IPV6        "IPv6 support"                 ON)
OPTION(INSTALL_PDB          "Install .pdb files (if generated)"  ON)
OPTION(APR_BUILD_TESTAPR    "Build the test suite"         OFF)
OPTION(APR_BUILD_SHARED     "Build shared libraries"       ON)
OPTION(APR_BUILD_STATIC     "Build static libraries"       ON)
OPTION(TEST_STATIC_LIBS     "Test programs use APR static libraries instead of shared libraries?" OFF)
SET(MIN_WINDOWS_VER             "Vista" 
    CACHE STRING "Minimum Windows version")

SET(APR_INSTALL_BIN_DIR      "bin"                       CACHE STRING "Install subdirectory for binary files")
SET(APR_INSTALL_LIB_DIR      "lib"                       CACHE STRING "Install subdirectory for library files")
SET(APR_INSTALL_INCLUDE_DIR  "include"                   CACHE STRING "Install subdirectory for include files")

IF(NOT APR_BUILD_SHARED AND NOT APR_BUILD_STATIC)
  MESSAGE(FATAL_ERROR "Both APR_BUILD_SHARED and APR_BUILD_STATIC have been disabled")
ENDIF()

IF(NOT APR_BUILD_STATIC AND TEST_STATIC_LIBS)
  MESSAGE(FATAL_ERROR "APR_BUILD_STATIC has been disabled, but TEST_STATIC_LIBS is enabled")
ENDIF()

# create 1-or-0 representation of feature tests for apr.h

SET(apr_have_ipv6_10 0)

IF(APR_HAVE_IPV6)
  SET(apr_have_ipv6_10 1)
ENDIF()

IF("${MIN_WINDOWS_VER}" STREQUAL "")
  SET(win32_winnt_str "0x0600")
ELSEIF(${MIN_WINDOWS_VER} STREQUAL "Vista")
  SET(win32_winnt_str "0x0600")
ELSEIF(${MIN_WINDOWS_VER} STREQUAL "Windows7")
  SET(win32_winnt_str "0x0601")
ELSE()
  SET(win32_winnt_str ${MIN_WINDOWS_VER})
ENDIF()

# Read current version.
FILE(STRINGS include/apr_version.h VERSION_STRINGS REGEX "#define (APR_MAJOR_VERSION|APR_MINOR_VERSION|APR_PATCH_VERSION)")

STRING(REGEX REPLACE ".*#define APR_MAJOR_VERSION[ \t]+([0-9]+).*" "\\1" APR_MAJOR_VERSION ${VERSION_STRINGS})
STRING(REGEX REPLACE ".*#define APR_MINOR_VERSION[ \t]+([0-9]+).*" "\\1" APR_MINOR_VERSION ${VERSION_STRINGS})
STRING(REGEX REPLACE ".*#define APR_PATCH_VERSION[ \t]+([0-9]+).*" "\\1" APR_PATCH_VERSION ${VERSION_STRINGS})

CONFIGURE_FILE(include/apr.hwc
               ${PROJECT_BINARY_DIR}/apr.h)

ADD_EXECUTABLE(gen_test_char tools/gen_test_char.c)

ADD_CUSTOM_COMMAND(
  COMMENT "Generating character tables, apr_escape_test_char.h, for current locale"
  DEPENDS gen_test_char
  COMMAND $<TARGET_FILE:gen_test_char> > ${PROJECT_BINARY_DIR}/apr_escape_test_char.h
  OUTPUT ${PROJECT_BINARY_DIR}/apr_escape_test_char.h
)
ADD_CUSTOM_TARGET(
  test_char_header ALL
  DEPENDS ${PROJECT_BINARY_DIR}/apr_escape_test_char.h
)

# Generated .h files are stored in PROJECT_BINARY_DIR, not the
# source tree.
#
# BROKEN: not searching PROJECT_BINARY_DIR first, so you have to
#         manually delete apr.h in PROJECT_SOURCE_DIR/include if
#         you've generated apr.h before using a different build

SET(APR_INCLUDE_DIRECTORIES
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/arch/win32>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/arch/unix>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/private>
)

SET(APR_SYSTEM_LIBS
  ws2_32
  rpcrt4
)

INCLUDE_DIRECTORIES(${APR_INCLUDE_DIRECTORIES})

SET(APR_PUBLIC_HEADERS_STATIC
  include/apr_allocator.h
  include/apr_atomic.h
  include/apr_cstr.h
  include/apr_dso.h
  include/apr_env.h
  include/apr_errno.h
  include/apr_escape.h
  include/apr_file_info.h
  include/apr_file_io.h
  include/apr_fnmatch.h
  include/apr_general.h
  include/apr_getopt.h
  include/apr_global_mutex.h
  include/apr_hash.h
  include/apr_inherit.h
  include/apr_lib.h
  include/apr_mmap.h
  include/apr_network_io.h
  include/apr_perms_set.h
  include/apr_poll.h
  include/apr_pools.h
  include/apr_portable.h
  include/apr_proc_mutex.h
  include/apr_random.h
  include/apr_ring.h
  include/apr_shm.h
  include/apr_signal.h
  include/apr_skiplist.h
  include/apr_strings.h
  include/apr_support.h
  include/apr_tables.h
  include/apr_thread_cond.h
  include/apr_thread_mutex.h
  include/apr_thread_proc.h
  include/apr_thread_rwlock.h
  include/apr_time.h
  include/apr_user.h
  include/apr_version.h
  include/apr_want.h
)
SET(APR_PUBLIC_HEADERS_GENERATED
  ${PROJECT_BINARY_DIR}/apr.h
)

SET(APR_SOURCES
  atomic/win32/apr_atomic.c
  atomic/win32/apr_atomic64.c
  dso/win32/dso.c
  encoding/apr_encode.c
  encoding/apr_escape.c
  file_io/unix/copy.c
  file_io/unix/fileacc.c
  file_io/unix/filepath_util.c
  file_io/unix/fullrw.c
  file_io/unix/mktemp.c
  file_io/unix/tempdir.c
  file_io/win32/buffer.c
  file_io/win32/dir.c
  file_io/win32/filedup.c
  file_io/win32/filepath.c
  file_io/win32/filestat.c
  file_io/win32/filesys.c
  file_io/win32/flock.c
  file_io/win32/open.c
  file_io/win32/pipe.c
  file_io/win32/readwrite.c
  file_io/win32/seek.c
  locks/win32/proc_mutex.c
  locks/win32/thread_cond.c
  locks/win32/thread_mutex.c
  locks/win32/thread_rwlock.c
  memory/unix/apr_pools.c
  misc/unix/errorcodes.c
  misc/unix/getopt.c
  misc/unix/otherchild.c
  misc/unix/version.c
  misc/win32/charset.c
  misc/win32/env.c
  misc/win32/internal.c
  misc/win32/misc.c
  misc/win32/rand.c
  misc/win32/start.c
  misc/win32/utf8.c
  mmap/unix/common.c
  mmap/win32/mmap.c
  network_io/unix/inet_ntop.c
  network_io/unix/inet_pton.c
  network_io/unix/multicast.c
  network_io/unix/sockaddr.c
  network_io/unix/socket_util.c
  network_io/win32/sendrecv.c
  network_io/win32/sockets.c
  network_io/win32/sockopt.c
  passwd/apr_getpass.c
  poll/unix/poll.c
  poll/unix/pollcb.c
  poll/unix/pollset.c
  poll/unix/select.c
  poll/unix/wakeup.c
  random/unix/apr_random.c
  random/unix/sha2.c
  random/unix/sha2_glue.c
  shmem/win32/shm.c
  strings/apr_cpystrn.c
  strings/apr_cstr.c
  strings/apr_fnmatch.c
  strings/apr_snprintf.c
  strings/apr_strings.c
  strings/apr_strnatcmp.c
  strings/apr_strtok.c
  tables/apr_hash.c
  tables/apr_skiplist.c
  tables/apr_tables.c
  threadproc/win32/proc.c
  threadproc/win32/signals.c
  threadproc/win32/thread.c
  threadproc/win32/threadpriv.c
  time/win32/time.c
  time/win32/timestr.c
  user/win32/groupinfo.c
  user/win32/userinfo.c
)

SET(APR_TEST_SUITES
  testargs
  testatomic
  testcond
  testdir
  testdso
  testdup
  testenv
  testencode
  testescape
  testfile
  testfilecopy
  testfileinfo
  testflock
  testfmt
  testfnmatch
  testglobalmutex
  testhash
  testipsub
  testlfs
  testlock
  testmmap
  testnames
  testoc
  testpath
  testpipe
  testpoll
  testpools
  testproc
  testprocmutex
  testrand
  testshm
  testskiplist
  testsleep
  testsock
  testsockets
  testsockopt
  teststr
  teststrnatcmp
  testtable
  testtemp
  testthread
  testtime
  testud
  testuser
  testvsn
)

SET(install_targets)
SET(install_bin_pdb)

IF (MSVC)
  # Ignore Microsoft's interpretation of secure development
  # and the POSIX string handling API
  ADD_COMPILE_DEFINITIONS(_CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_WARNINGS)
ENDIF()

# libapr-1 is shared, apr-1 is static
IF (APR_BUILD_SHARED)
  ADD_LIBRARY(${apr_libname} SHARED ${APR_SOURCES} ${APR_PUBLIC_HEADERS_GENERATED} libapr.rc)
  LIST(APPEND install_targets ${apr_libname})
  LIST(APPEND install_bin_pdb ${PROJECT_BINARY_DIR}/${apr_libname}.pdb)
  TARGET_LINK_LIBRARIES(${apr_libname}
                        PRIVATE ${APR_SYSTEM_LIBS})
  TARGET_COMPILE_DEFINITIONS(${apr_libname}
                             PRIVATE "APR_DECLARE_EXPORT;WINNT"
                             INTERFACE "APR_DECLARE_IMPORT;WINNT")
  TARGET_INCLUDE_DIRECTORIES(${apr_libname}
    PRIVATE ${APR_INCLUDE_DIRECTORIES}
    INTERFACE $<INSTALL_INTERFACE:${APR_INSTALL_INCLUDE_DIR}> ${APR_INCLUDE_DIRECTORIES}
  )
  ADD_DEPENDENCIES(${apr_libname} test_char_header)
ENDIF()

IF (APR_BUILD_STATIC)
  ADD_LIBRARY(${apr_name} STATIC ${APR_SOURCES} ${APR_PUBLIC_HEADERS_GENERATED})
  LIST(APPEND install_targets ${apr_name})
  TARGET_LINK_LIBRARIES(${apr_name}
                        PRIVATE ${APR_SYSTEM_LIBS})
  TARGET_COMPILE_DEFINITIONS(${apr_name}
                             PRIVATE "APR_DECLARE_STATIC;WINNT"
                             INTERFACE "APR_DECLARE_STATIC;WINNT")
  TARGET_INCLUDE_DIRECTORIES(${apr_name}
    PRIVATE ${APR_INCLUDE_DIRECTORIES}
    INTERFACE $<INSTALL_INTERFACE:${APR_INSTALL_INCLUDE_DIR}> ${APR_INCLUDE_DIRECTORIES}
  )
  ADD_DEPENDENCIES(${apr_name} test_char_header)
ENDIF()

# libaprapp-1 and aprapp-1 are static
IF (APR_BUILD_SHARED)
  ADD_LIBRARY(libaprapp-1 STATIC misc/win32/apr_app.c misc/win32/internal.c ${APR_PUBLIC_HEADERS_GENERATED})
  LIST(APPEND install_targets libaprapp-1)
  SET_TARGET_PROPERTIES(libaprapp-1 PROPERTIES COMPILE_DEFINITIONS "APR_APP;WINNT")
  TARGET_INCLUDE_DIRECTORIES(libaprapp-1
    PRIVATE ${APR_INCLUDE_DIRECTORIES}
  )
ENDIF()

IF (APR_BUILD_STATIC)
  ADD_LIBRARY(aprapp-1 STATIC misc/win32/apr_app.c misc/win32/internal.c ${APR_PUBLIC_HEADERS_GENERATED})
  LIST(APPEND install_targets aprapp-1)
  SET_TARGET_PROPERTIES(aprapp-1 PROPERTIES COMPILE_DEFINITIONS "APR_DECLARE_STATIC;APR_APP;WINNT")
  TARGET_INCLUDE_DIRECTORIES(aprapp-1
    PRIVATE ${APR_INCLUDE_DIRECTORIES}
  )
ENDIF()

IF(APR_BUILD_TESTAPR)
  ENABLE_TESTING()
  # Create a "check" target that displays test program output to the console.
  ADD_CUSTOM_TARGET(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

  # copy data files to build directory so that we can run programs from there
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E make_directory 
                  ${PROJECT_BINARY_DIR}/data)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                  ${PROJECT_SOURCE_DIR}/test/data/file_datafile.txt
                  ${PROJECT_BINARY_DIR}/data/file_datafile.txt)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  ${PROJECT_SOURCE_DIR}/test/data/mmap_datafile.txt
                  ${PROJECT_BINARY_DIR}/data/mmap_datafile.txt)

  IF(TEST_STATIC_LIBS)
    SET(whichapr    ${apr_name})
    SET(whichaprapp aprapp-1)
  ELSE()
    SET(whichapr    ${apr_libname})
    SET(whichaprapp libaprapp-1)
  ENDIF()

  ADD_EXECUTABLE(testapp test/testapp.c)
  TARGET_LINK_LIBRARIES(testapp ${whichapr} ${whichaprapp})
  SET_TARGET_PROPERTIES(testapp PROPERTIES LINK_FLAGS /entry:wmainCRTStartup)
  ADD_TEST(NAME testapp COMMAND testapp)

  SET (APR_TEST_SOURCES
    test/abts.c
    test/testutil.c
    )

  FOREACH(testsuite ${APR_TEST_SUITES})
    LIST(APPEND APR_TEST_SOURCES test/${testsuite}.c)
  ENDFOREACH()

  ADD_EXECUTABLE(testall ${APR_TEST_SOURCES})
  TARGET_LINK_LIBRARIES(testall ${whichapr} ws2_32.lib)
  SET_TARGET_PROPERTIES(testall PROPERTIES COMPILE_DEFINITIONS "BINPATH=$<TARGET_FILE_DIR:testall>")

  FOREACH(test ${APR_TEST_SUITES})
    ADD_TEST(NAME ${test} COMMAND testall -v ${test})
  ENDFOREACH()

  ADD_LIBRARY(mod_test MODULE test/mod_test.c)
  TARGET_LINK_LIBRARIES(mod_test ${whichapr})
  SET_PROPERTY(TARGET mod_test APPEND PROPERTY LINK_FLAGS /export:print_hello)
  # nasty work-around for difficulties adding more than one additional flag
  # (they get joined in a bad way behind the scenes)
  GET_PROPERTY(link_flags TARGET mod_test PROPERTY LINK_FLAGS)
  SET(link_flags "${link_flags} /export:count_reps")
  SET_TARGET_PROPERTIES(mod_test PROPERTIES LINK_FLAGS ${link_flags})

  # Build all the single-source executable files with no special build
  # requirements.
  SET(single_source_programs
    test/echod.c
    test/sendfile.c
    test/sockperf.c
    test/testlockperf.c
    test/testmutexscope.c
    test/globalmutexchild.c
    test/occhild.c
    test/proc_child.c
    test/readchild.c
    test/sockchild.c
    test/testshmproducer.c
    test/testshmconsumer.c
    test/tryread.c
    test/internal/testucs.c
  )

  FOREACH(sourcefile ${single_source_programs})
    STRING(REGEX REPLACE ".*/([^\\]+)\\.c" "\\1" proggie ${sourcefile})
    ADD_EXECUTABLE(${proggie} ${sourcefile})
    TARGET_LINK_LIBRARIES(${proggie} ${whichapr})
    SET_TARGET_PROPERTIES(${proggie} PROPERTIES COMPILE_DEFINITIONS "BINPATH=$<TARGET_FILE_DIR:${proggie}>")

    ADD_DEPENDENCIES(testall ${proggie})
  ENDFOREACH()

  # Add tests for programs that run by themselves with no arguments.
  SET(simple_tests
    testmutexscope
    testucs
  )

  FOREACH(simple ${simple_tests})
    ADD_TEST(NAME ${simple} COMMAND ${simple})
  ENDFOREACH()

  # testlockperf takes forever on Windows with default counter limit
  ADD_TEST(NAME testlockperf COMMAND testlockperf -c 50000)

  # sendfile runs multiple times with different parameters.
  FOREACH(sendfile_mode blocking nonblocking timeout)
    ADD_TEST(NAME sendfile-${sendfile_mode} COMMAND sendfile client ${sendfile_mode} startserver)
  ENDFOREACH()

  # No test is added for echod+sockperf.  Those will have to be run manually.

ENDIF (APR_BUILD_TESTAPR)

# Installation
INCLUDE(CMakePackageConfigHelpers)

string(TOLOWER "${PROJECT_NAME}" project_name_lower)

write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/${project_name_lower}-config-version.cmake"
  VERSION ${APR_MAJOR_VERSION}.${APR_MINOR_VERSION}.${APR_PATCH_VERSION}
  COMPATIBILITY SameMajorVersion
)

INSTALL(TARGETS ${install_targets} EXPORT apr-config
        RUNTIME DESTINATION ${APR_INSTALL_BIN_DIR}
        LIBRARY DESTINATION ${APR_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${APR_INSTALL_LIB_DIR}
)

INSTALL(EXPORT apr-config
        NAMESPACE apr::
        DESTINATION "lib/cmake/${project_name_lower}"
        FILE "${project_name_lower}-config.cmake"
)

INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/${project_name_lower}-config-version.cmake"
        DESTINATION "lib/cmake/${project_name_lower}"
)

IF(INSTALL_PDB)
  INSTALL(FILES ${install_bin_pdb}
          DESTINATION ${APR_INSTALL_BIN_DIR}
          CONFIGURATIONS RelWithDebInfo Debug)
ENDIF()

INSTALL(FILES ${APR_PUBLIC_HEADERS_STATIC} ${APR_PUBLIC_HEADERS_GENERATED} DESTINATION ${APR_INSTALL_INCLUDE_DIR})
IF(APR_INSTALL_PRIVATE_H)
  # Kludges for unexpected dependencies of httpd 2.x, not installed by default
  SET(APR_PRIVATE_H_FOR_HTTPD
    include/arch/win32/apr_arch_file_io.h
    include/arch/win32/apr_arch_misc.h
    include/arch/win32/apr_arch_utf8.h
    include/arch/win32/apr_private.h
    )
  INSTALL(FILES ${APR_PRIVATE_H_FOR_HTTPD} DESTINATION ${APR_INSTALL_INCLUDE_DIR}/arch/win32)
  INSTALL(FILES include/arch/apr_private_common.h DESTINATION ${APR_INSTALL_INCLUDE_DIR}/arch)
ENDIF()

STRING(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)
MESSAGE(STATUS "")
MESSAGE(STATUS "")
MESSAGE(STATUS "APR configuration summary:")
MESSAGE(STATUS "")

MESSAGE(STATUS "  Build type ...................... : ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "  Install .pdb (if available)...... : ${INSTALL_PDB}")
MESSAGE(STATUS "  Install prefix .................. : ${CMAKE_INSTALL_PREFIX}")
MESSAGE(STATUS "    Directory for binary files .... : PREFIX/${APR_INSTALL_BIN_DIR}")
MESSAGE(STATUS "    Directory for library files ... : PREFIX/${APR_INSTALL_LIB_DIR}")
MESSAGE(STATUS "    Directory for include files ... : PREFIX/${APR_INSTALL_INCLUDE_DIR}")
MESSAGE(STATUS "  C compiler ...................... : ${CMAKE_C_COMPILER}")
MESSAGE(STATUS "  IPv6 ............................ : ${APR_HAVE_IPV6}")
MESSAGE(STATUS "  Minimum Windows version ......... : ${MIN_WINDOWS_VER}")
MESSAGE(STATUS "  Build shared libs ............... : ${APR_BUILD_SHARED}")
MESSAGE(STATUS "  Build static libs ............... : ${APR_BUILD_STATIC}")
MESSAGE(STATUS "  Build test suite ................ : ${APR_BUILD_TESTAPR}")
IF(TEST_STATIC_LIBS)
MESSAGE(STATUS "    (testing static libraries)")
ELSE()
MESSAGE(STATUS "    (testing dynamic libraries)")
ENDIF()
MESSAGE(STATUS "  Install private .h for httpd .... : ${APR_INSTALL_PRIVATE_H}")
