#   Copyright 2016 Comcast Cable Communications Management, LLC
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -g -fprofile-arcs -ftest-coverage -O0")
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE=600")
endif()
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W  -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -O0")

#-------------------------------------------------------------------------------
#   test nano-msg centric code
#-------------------------------------------------------------------------------
add_test(NAME NMsgTest COMMAND nmsg)
add_executable (nmsg nmsg.c dbg_err.c)

target_link_libraries (nmsg cunit -lnng -lm -lpthread)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (nmsg gcov)
endif()

#-------------------------------------------------------------------------------
#   test libparodus
#-------------------------------------------------------------------------------
add_test(NAME LibPDTest COMMAND libpd)
add_executable (libpd
                libpd_test.c
                libparodus_test_timing.c
                ../src/libparodus.c
                ../src/libparodus_time.c
                ../src/libparodus_queues.c)

target_link_libraries (libpd
                       cunit
                       -lwrp-c
                       -lmsgpackc
                       -ltrower-base64
                       -lnng
                       -lcimplog
                       -lm
                       -lpthread)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (libpd gcov)
target_link_libraries (libpd rt)
endif()

#-------------------------------------------------------------------------------
#   mock code
#-------------------------------------------------------------------------------
add_executable(mock_parodus mock_parodus.c dbg_err.c)

target_link_libraries (mock_parodus
 -lwrp-c
 -lmsgpackc
 -ltrower-base64
 -lnng
 -lcimplog
 -lm
 -lpthread
)

#-------------------------------------------------------------------------------
#   coverage
#-------------------------------------------------------------------------------
add_custom_target(coverage
                  COMMAND lcov -q --capture --directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/libpd.dir/__/src --output-file coverage.info
                  COMMAND genhtml coverage.info
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
