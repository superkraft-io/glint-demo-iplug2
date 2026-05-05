cmake_minimum_required(VERSION 3.14)
project(GlintPlug VERSION 1.0.0)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/config.h" GP_BUNDLE_NAME_LINE REGEX "^#define BUNDLE_NAME[ \t]+\"[^\"]+\"")
if(GP_BUNDLE_NAME_LINE)
  string(REGEX REPLACE "^#define BUNDLE_NAME[ \t]+\"([^\"]+)\".*" "\\1" GP_BUNDLE_NAME "${GP_BUNDLE_NAME_LINE}")
else()
  set(GP_BUNDLE_NAME "${PROJECT_NAME}")
endif()

# Discover IPLUG2_DIR if not already set (for independent builds)
if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/iPlug2OOS/iPlug2" CACHE PATH "iPlug2 root directory")
endif()

if(APPLE)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E env
      IPLUG_SKIP_RESOURCE_COPY=1
      IPLUG_RESOURCE_BASENAME=${PROJECT_NAME}
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_resources-mac.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY
  )
endif()

# Include iPlug2 CMake configuration
include(${IPLUG2_DIR}/iPlug2.cmake)

find_package(iPlug2 REQUIRED)

option(GLINT_BUNDLE_SHALLOW "Enable shallow glint bundle mode" OFF)
option(GLINT_BUNDLE_DEEP "Enable deep glint bundle mode" OFF)

# Add glint as a subdirectory. Point it at glint's own Skia bundle in third_party/skia/,
# populated by running: node third_party/glint/scripts/init_skia.mjs --source --backend metal
# glint owns all Skia knowledge; GP just links glint::host_mac.
set(GLINT_DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/skia" CACHE PATH "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/glint glint-build)

set(GP_SOURCES
  GP.cpp
  GP.h
  resources/resource.h
)

if(APPLE)
  list(APPEND GP_SOURCES
    GPGlintEditorMac.mm
    ../third_party/glint/platform/mac/glint_view_mac.mm     # macOS embedded view host
    ../third_party/glint/platform/mac/glint_window_mac.mm   # macOS glint_window backend + inspector support
  )
endif()

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    ${GP_SOURCES}
  FORMATS APP VST3 AU
  UI NONE
  DEFINES
    NO_IGRAPHICS
    SAMPLE_TYPE_FLOAT
)

set(GP_TARGETS)

if(TARGET GlintPlug-app)
  list(APPEND GP_TARGETS GlintPlug-app)
endif()

if(TARGET GlintPlug-vst3)
  list(APPEND GP_TARGETS GlintPlug-vst3)
endif()

if(TARGET GlintPlug-au)
  list(APPEND GP_TARGETS GlintPlug-au)
endif()

foreach(target IN LISTS GP_TARGETS)
  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
  )

  if(APPLE)
    target_link_libraries(${target} PRIVATE glint::host_mac "-framework Accelerate")
  endif()

  if(GLINT_BUNDLE_SHALLOW)
    target_compile_definitions(${target} PRIVATE GLINT_BUNDLE_SHALLOW)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${target}>/bundle"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/glint_bundle/shallow/groups/data"
        "$<TARGET_FILE_DIR:${target}>/bundle"
      VERBATIM
    )
  elseif(GLINT_BUNDLE_DEEP)
    target_compile_definitions(${target} PRIVATE GLINT_BUNDLE_DEEP)
    file(GLOB GLINT_BUNDLE_DEEP_SOURCES
      "${CMAKE_CURRENT_SOURCE_DIR}/glint_bundle/deep/groups/*.cpp"
    )
    target_sources(${target} PRIVATE ${GLINT_BUNDLE_DEEP_SOURCES})
  endif()
endforeach()

if(APPLE AND GP_TARGETS)
  add_custom_target(gp-macos-deploy ALL
    DEPENDS ${GP_TARGETS}
    COMMAND ${CMAKE_COMMAND} -E echo "[GP] Refreshing deployed macOS bundles"
    VERBATIM
  )

  function(gp_add_macos_deploy_command target format bundle_suffix)
    iplug_get_default_deploy_path(${format})
    set(deploy_path "${IPLUG_DEPLOY_PATH_${format}}")

    if(TARGET ${target} AND NOT "${deploy_path}" STREQUAL "")
      add_custom_command(TARGET gp-macos-deploy
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${deploy_path}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${deploy_path}/${GP_BUNDLE_NAME}.${bundle_suffix}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:${target}>" "${deploy_path}/${GP_BUNDLE_NAME}.${bundle_suffix}"
        VERBATIM
      )

      if(NOT "${PROJECT_NAME}" STREQUAL "${GP_BUNDLE_NAME}")
        add_custom_command(TARGET gp-macos-deploy
          POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/out/${PROJECT_NAME}.${bundle_suffix}"
          COMMAND ${CMAKE_COMMAND} -E rm -rf "${deploy_path}/${PROJECT_NAME}.${bundle_suffix}"
          VERBATIM
        )
      endif()
    endif()
  endfunction()

  gp_add_macos_deploy_command(GlintPlug-app APP app)
  gp_add_macos_deploy_command(GlintPlug-vst3 VST3 vst3)
  gp_add_macos_deploy_command(GlintPlug-au AU component)
endif()
