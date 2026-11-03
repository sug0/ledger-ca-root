cmake_minimum_required(VERSION 3.28)
project(UNIT_TESTS
    VERSION 0.1
    DESCRIPTION "Unit tests for Ledger Nano application"
    LANGUAGES C
)

#######################################################################
#                        Setup SDK & LedgerUT                         #
#######################################################################
if(NOT DEFINED ENV{BOLOS_SDK})
    message(FATAL_ERROR "BOLOS_SDK environment variable is not set. Please set it to the path of the Ledger Secure SDK.")
endif()

set(SDK_DIR $ENV{BOLOS_SDK})
list(APPEND CMAKE_MODULE_PATH "${SDK_DIR}/cmake")
include(LedgerUT)

#######################################################################
#             Setup app specific includes & compile defs              #
#######################################################################
set(UT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/apdu
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/handler
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/helper
                ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/action
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes/
)

set(UT_COMPILE_DEFS
    APPNAME="Boilerplate"
    APPVERSION="9.9.9"
    USE_ED25519
)

#######################################################################
#                           LedgerUT Setup                            #
#######################################################################
# init ledger_unit_tests environment
ledger_unit_tests_init()

# NB: LedgerUT appends -lgcov for gcov, but clang has no libgcov (it is a GCC
# runtime); --coverage links the profile runtime on both clang and gcc, so the
# explicit -lgcov only breaks macOS builds
string(REPLACE "-lgcov" "" CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
string(REPLACE "-lgcov" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")

# NB: macOS libc does not declare explicit_bzero, which the SDK's os_seed.h
# calls; force-include the shim on every host compilation
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${CMAKE_CURRENT_SOURCE_DIR}/fakes/explicit_bzero.h")

# build all app sources but app_main.c (coverage baseline; not linked by tests)
ledger_unit_tests_find_sources()

# build all app sources so all sources have a .gcno file to init coverage %
ledger_unit_tests_build_app_sources_as_lib(
    SOURCES      ${APP_SOURCES}
    INCLUDE_DIRS ${UT_INCLUDE_DIRS}
    COMPILE_DEFS ${UT_COMPILE_DEFS}
)

#######################################################################
#                        LedgerUT test suites                         #
#######################################################################
ledger_unit_tests_add_test(
    NAME          test_der
    SOURCES       ${CMAKE_CURRENT_SOURCE_DIR}/../src/der.c
    INCLUDE_DIRS  ${UT_INCLUDE_DIRS_SDK} ${UT_INCLUDE_DIRS}
    COMPILE_DEFS  ${UT_COMPILE_DEFS_SDK} ${UT_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME          test_tbs
    SOURCES       ${CMAKE_CURRENT_SOURCE_DIR}/../src/tbs.c ${CMAKE_CURRENT_SOURCE_DIR}/../src/der.c
    INCLUDE_DIRS  ${UT_INCLUDE_DIRS_SDK} ${UT_INCLUDE_DIRS}
    COMPILE_DEFS  ${UT_COMPILE_DEFS_SDK} ${UT_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME           test_send_response
    SOURCES        ${CMAKE_CURRENT_SOURCE_DIR}/../src/helper/send_response.c
    MOCK_HEADERS   ${SDK_DIR}/lib_standard_app/io.h
    INCLUDE_DIRS   ${UT_INCLUDE_DIRS_SDK} ${UT_INCLUDE_DIRS}
    COMPILE_DEFS   ${UT_COMPILE_DEFS_SDK} ${UT_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME           test_get_version
    SOURCES        ${CMAKE_CURRENT_SOURCE_DIR}/../src/handler/get_version.c
    MOCK_HEADERS   ${SDK_DIR}/lib_standard_app/io.h
    INCLUDE_DIRS   ${UT_INCLUDE_DIRS_SDK} ${UT_INCLUDE_DIRS}
    COMPILE_DEFS   ${UT_COMPILE_DEFS_SDK} ${UT_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME           test_get_app_name
    SOURCES        ${CMAKE_CURRENT_SOURCE_DIR}/../src/handler/get_app_name.c
    MOCK_HEADERS   ${SDK_DIR}/lib_standard_app/io.h
    INCLUDE_DIRS   ${UT_INCLUDE_DIRS_SDK} ${UT_INCLUDE_DIRS}
    COMPILE_DEFS   ${UT_COMPILE_DEFS_SDK} ${UT_COMPILE_DEFS}
)

