add_library(lyr_std MODULE
        "${CMAKE_CURRENT_LIST_DIR}/bevel.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/booleancurve.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/clamp.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/curvewarp.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/freetime.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/import.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/insideout.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/julia.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/mandelbrot.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/rotate.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/shade.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/sphere_distort.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/stretch.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/stroboscope.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/supersample.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/timeloop.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/translate.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/twirl.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/perspective.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/xorpattern.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/zoom.cpp"
)

target_link_libraries(lyr_std synfig ${GLIBMM_LINK_LIBRARIES})

install (
    TARGETS lyr_std
    DESTINATION lib/synfig/modules
)
