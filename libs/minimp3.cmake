
set(MINIMP3_DIR ${CMAKE_CURRENT_LIST_DIR}/minimp3)

add_library(minimp3 INTERFACE)
target_include_directories(minimp3 INTERFACE
    ${MINIMP3_DIR}/
)
