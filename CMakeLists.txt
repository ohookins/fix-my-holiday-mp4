cmake_minimum_required(VERSION 3.0.0)
project(fix-my-holiday-mp4 VERSION 0.1.0)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Linking
set(SOURCE main.c parsefile.c)
add_executable(${PROJECT_NAME} ${SOURCE})