cmake_minimum_required(VERSION 3.10.2)

project("aupd")

add_library(
        aupd
        SHARED
        src/updater.cpp
)

find_library(
        log-lib
        log
)

target_link_libraries(
        aupd
        ${log-lib}
        android
)
