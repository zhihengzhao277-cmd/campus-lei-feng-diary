cmake_minimum_required(VERSION 3.16)

project(CampusLeiFengDiary LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The current single-config generators place leifeng_qt.exe in the binary
# directory, so its runtime data directory is CMAKE_BINARY_DIR/data.
set(LEIFENG_RUNTIME_DATA_DIR "${CMAKE_BINARY_DIR}/data")
file(MAKE_DIRECTORY "${LEIFENG_RUNTIME_DATA_DIR}")

foreach(LEIFENG_DATA_FILE
    students.txt
    administrators.txt
    records.txt
    diaries.txt
)
    set(LEIFENG_RUNTIME_DATA_FILE
        "${LEIFENG_RUNTIME_DATA_DIR}/${LEIFENG_DATA_FILE}")
    if(NOT EXISTS "${LEIFENG_RUNTIME_DATA_FILE}")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/data/${LEIFENG_DATA_FILE}"
            "${LEIFENG_RUNTIME_DATA_FILE}"
            COPYONLY
        )
    endif()
endforeach()

set(CMAKE_AUTOMOC ON)

find_package(
    Qt6 REQUIRED COMPONENTS Widgets
)

set(CORE_SOURCES
    core/user.cpp
    core/student.cpp
    core/administrator.cpp
    core/volunteer_record.cpp
    core/volunteer_category.cpp
    core/diary_post.cpp
    core/data_manager.cpp
    core/student_volunteer_service.h
    core/student_volunteer_service.cpp
)

add_executable(
    leifeng_console
    console/main.cpp
    ${CORE_SOURCES}
)

target_include_directories(
    leifeng_console
    PRIVATE
    core
)

add_executable(
    leifeng_qt

    qt/main_qt.cpp

    qt/login_window.cpp
    qt/login_window.h

    qt/student_main_window.cpp
    qt/student_main_window.h

    qt/administrator_main_window.cpp
    qt/administrator_main_window.h

    qt/style_helper.cpp
    qt/style_helper.h

    qt/resources/badges.qrc

    ${CORE_SOURCES}
)
set_property(TARGET leifeng_qt PROPERTY AUTORCC ON)
target_include_directories(
    leifeng_qt
    PRIVATE
    core
    qt
)

target_link_libraries(
    leifeng_qt
    PRIVATE
    Qt6::Widgets
)

include(CTest)

if(BUILD_TESTING)
    add_executable(
        core_behavior_tests
        tests/core_behavior_tests.cpp
        ${CORE_SOURCES}
    )

    target_include_directories(
        core_behavior_tests
        PRIVATE
        core
    )

    add_test(
        NAME core_behavior_tests
        COMMAND core_behavior_tests
    )
endif()
