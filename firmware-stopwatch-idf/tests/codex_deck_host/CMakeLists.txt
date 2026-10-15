cmake_minimum_required(VERSION 3.20)
project(codex_deck_host LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
get_filename_component(FW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "" FORCE)
add_subdirectory("${FW_ROOT}/components/lvgl" lvgl EXCLUDE_FROM_ALL)
add_executable(codex_deck_host main.cpp
    "${FW_ROOT}/main/apps/app_codex/view/view.cpp"
    "${FW_ROOT}/main/assets/fonts/CommissionerMedium64.c"
    "${FW_ROOT}/main/assets/fonts/codex_deck_16.c")
target_include_directories(codex_deck_host BEFORE PRIVATE stubs
    "${FW_ROOT}/main" "${FW_ROOT}/components/smooth_ui_toolkit/src"
    "${FW_ROOT}/components/smooth_ui_toolkit/src/lvgl")
target_link_libraries(codex_deck_host PRIVATE lvgl)
