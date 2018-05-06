# -*- mode: CMAKE; -*-

cmake_minimum_required(VERSION 2.6)

project(rpn)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE string "Choose the type of build, options are: None Debug Release" FORCE)
endif()

message(STATUS "Build mode: ${CMAKE_BUILD_TYPE}")

# INFO
set(RPN_VERSION "2.3")
set(RPN_DISPLAY_NAME "rpn")
set(RPN_URL_INFO_ABOUT "https://github.com/louisrubet/rpn")
set(RPN_CONTACT "Louis Rubet <louis@rubet.fr>")
set(RPN_FRIENDLY_STRING "rpn - Reverse Polish Notation cli calculator and language")
set(RPN_LICENSE "LGPLv3")
set(RPN_LICENSE_FILE "${PROJECT_SOURCE_DIR}/LICENSE")
set(RPN_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")

# compiler options
if(CMAKE_COMPILER_IS_GNUCXX)
  message(STATUS "Compiler type GNU: ${CMAKE_CXX_COMPILER}")
  set(BASE_COMPILER_OPTIONS "-std=c++0x -Wl,--no-as-needed")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${BASE_COMPILER_OPTIONS}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${BASE_COMPILER_OPTIONS} -O0 -g")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${BASE_COMPILER_OPTIONS} -O3 -fomit-frame-pointer -s")
endif()

# submodule linenoise-ng
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/linenoise-ng/.git")
    execute_process(COMMAND git submodule init ${PROJECT_SOURCE_DIR}/linenoise-ng)
    execute_process(COMMAND git submodule update ${PROJECT_SOURCE_DIR}/linenoise-ng)
    execute_process(COMMAND git checkout v1.0.1-rpn WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/linenoise-ng)
endif()
add_subdirectory(linenoise-ng)

# includes
include_directories(${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/linenoise-ng/include)

# build
add_executable(
  rpn
  src/main.cpp
  src/object.cpp
  src/program.cpp
)
add_dependencies(rpn linenoise)
target_link_libraries(rpn mpfr linenoise)

# man
add_custom_target(man ALL)
add_custom_command(
    TARGET man
    COMMAND gzip -f -k ${CMAKE_CURRENT_SOURCE_DIR}/rpn.1
    OUTPUTS ${CMAKE_CURRENT_SOURCE_DIR}/rpn.1.gz
)

# install
install(TARGETS rpn DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/rpn.1.gz DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1)

# packages (common variables for all packages)
SET(CPACK_PACKAGE_NAME ${RPN_DISPLAY_NAME})
set(CPACK_PACKAGE_VENDOR ${RPN_CONTACT})
set(CPACK_PACKAGE_DESCRIPTION_FILE ${RPN_DESCRIPTION_FILE})
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY, ${RPN_FRIENDLY_STRING})
set(CPACK_RESOURCE_FILE_LICENSE ${RPN_LICENSE_FILE})
set(CPACK_RESOURCE_FILE_README ${RPN_DESCRIPTION_FILE})

SET(CPACK_PACKAGE_FILE_NAME "${RPN_DISPLAY_NAME}-${RPN_VERSION}-amd64")
SET(CPACK_PACKAGE_VERSION ${RPN_VERSION})
set(CPACK_PACKAGE_RELEASE 0)
SET(CPACK_PACKAGE_CONTACT ${RPN_CONTACT})
set(CPACK_PACKAGE_LICENSE ${RPN_LICENSE})
SET(CPACK_PACKAGE_DESCRIPTION, ${RPN_FRIENDLY_STRING})
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

# DEB package
SET(CPACK_GENERATOR "DEB;RPM;TGZ")
SET(CPACK_DEBIAN_PACKAGE_NAME ${RPN_DISPLAY_NAME})
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION, ${RPN_FRIENDLY_STRING})
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER ${RPN_CONTACT})
SET(CPACK_DEBIAN_PACKAGE_VERSION ${RPN_VERSION})
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${SYSTEM_PROCESSOR})
SET(CPACK_DEBIAN_PACKAGE_SECTION "math")
SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
include(CPack)

# RPM package
set(CPACK_RPM_PACKAGE_REQUIRES "mpfr")
