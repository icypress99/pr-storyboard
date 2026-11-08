cmake_minimum_required(VERSION 3.16)
project(PrStoryboardPlugin VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)

if(NOT DEFINED GITEASE_SDK_DIR OR GITEASE_SDK_DIR STREQUAL "")
    get_filename_component(GITEASE_SDK_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../../GitEase/Src/Plugins"
        ABSOLUTE)
endif()

if(NOT EXISTS "${GITEASE_SDK_DIR}/IPlugin.h")
    message(FATAL_ERROR
        "GitEase SDK not found at: ${GITEASE_SDK_DIR}\n"
        "Pass -DGITEASE_SDK_DIR=<absolute/path/to/GitEase/Src/Plugins>.")
endif()

message(STATUS "GitEase SDK: ${GITEASE_SDK_DIR}")

add_library(pr-storyboard-plugin SHARED
    PrStoryboardPlugin.h
    PrStoryboardPlugin.cpp
    StoryboardEngine.h
    StoryboardEngine.cpp
)

set_target_properties(pr-storyboard-plugin PROPERTIES
    PREFIX ""
)

qt_add_resources(pr-storyboard-plugin "pr_storyboard_qml"
    PREFIX "/com.gitease.pr-storyboard"
    FILES
        qml/StoryboardPage.qml
        qml/StoryStrip.qml
        qml/StoryFrame.qml
        qml/FrameDetail.qml
        qml/StudioFlatButton.qml
)

target_include_directories(pr-storyboard-plugin PRIVATE
    ${GITEASE_SDK_DIR}
)

target_link_libraries(pr-storyboard-plugin PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

set(_SOURCE_PLUGIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_command(TARGET pr-storyboard-plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_SOURCE_PLUGIN_DIR}/lib"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:pr-storyboard-plugin>"
        "${_SOURCE_PLUGIN_DIR}/lib/"
    COMMENT "Copying pr-storyboard DLL into plugin lib/"
    VERBATIM
)

if(NOT GITEASE_PLUGINS_DIR)
    if(WIN32)
        set(GITEASE_PLUGINS_DIR "$ENV{APPDATA}/GitEase/plugins")
    else()
        set(GITEASE_PLUGINS_DIR "$ENV{HOME}/.local/share/GitEase/plugins")
    endif()
endif()

if(GITEASE_PLUGINS_DIR)
    set(_PLUGIN_DEPLOY_DIR "${GITEASE_PLUGINS_DIR}/pr-storyboard")
    message(STATUS "Auto-deploy -> ${_PLUGIN_DEPLOY_DIR}")

    add_custom_command(TARGET pr-storyboard-plugin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_PLUGIN_DEPLOY_DIR}/lib"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:pr-storyboard-plugin>"
            "${_PLUGIN_DEPLOY_DIR}/lib/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/plugin.json"
            "${_PLUGIN_DEPLOY_DIR}/"
        COMMENT "Deploying pr-storyboard -> ${_PLUGIN_DEPLOY_DIR}"
        VERBATIM
    )
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageGep.cmake")
gitease_package_gep(pr-storyboard-plugin)
