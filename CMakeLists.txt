cmake_minimum_required(VERSION 3.20.0)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(vitaosc)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "VitaOSC")
set(VITA_TITLEID  "VOSC00001")
set(VITA_VERSION  "00.01")
set(VITA_EMU_STORAGE_PATH "/Users/diffty/Library/Application Support/Vita3K/Vita3K/ux0/app")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -D TARGET_PSVITA")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -D TARGET_PSVITA")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/lib)
set(CMAKE_BINARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out)

set(PSVITA_ARTIFACTS_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/psvita)

include_directories(
  "${PROJECT_SOURCE_DIR}/deps/muglib/src"
)

add_executable(${PROJECT_NAME}
  src/main.c
  deps/osclib/src/osc_data.h
)

add_subdirectory("deps/muglib/" "build")

target_link_libraries(${PROJECT_NAME}
  muglib
)

vita_create_self(${PROJECT_NAME}.self ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
)

add_custom_target(
  ${PROJECT_NAME}.vpk-install
  ALL
  COMMAND unzip -o ${PROJECT_NAME}.vpk -d ${VITA_EMU_STORAGE_PATH}/${VITA_TITLEID}/
  COMMENT "Installing vpk to emulator storage"
)
