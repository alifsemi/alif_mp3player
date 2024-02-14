
set(BOARDLIB_DIR ${CMAKE_CURRENT_LIST_DIR}/boardlib)
set(ENSEMBLE_BOARD "appkit_gen2" CACHE STRING "Set ENSEMBLE_BOARD to: appkit_gen2, or devkit_gen2.")
set_property(CACHE ENSEMBLE_BOARD PROPERTY STRINGS "appkit_gen2" "devkit_gen2")

add_library(boardlib)
target_include_directories(boardlib INTERFACE
    ${BOARDLIB_DIR}
)

target_sources(boardlib PRIVATE
    ${BOARDLIB_DIR}/${ENSEMBLE_BOARD}/board_init.c
)

target_link_libraries(boardlib PRIVATE
    rte_interface
    ensemblecmsis
)

add_compile_definitions(
    TARGET_BOARD=BOARD_${ENSEMBLE_BOARD}
)
