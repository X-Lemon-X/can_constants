cmake_minimum_required(VERSION 3.20)

project(can_constants VERSION 1.0 LANGUAGES C)


set(can_files
  ${PROJECT_SOURCE_DIR}/can_messages/output/can_messages.c 
  ${PROJECT_SOURCE_DIR}/can_messages/output/can_messages.h
)

set_source_files_properties(${can_files} PROPERTIES GENERATED TRUE)


add_library(can_constants STATIC
  ${can_files}
)

include_directories(
  can_constants PUBLIC
  src
  can_messages/output
)

# Include directories
target_include_directories(can_constants PUBLIC
  ${PROJECT_SOURCE_DIR}/can_messages/output
  ${PROJECT_SOURCE_DIR}/src
)
