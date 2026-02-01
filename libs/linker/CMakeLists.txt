add_library(raccoon_linker STATIC
        src/Linker.cpp
)

target_include_directories(raccoon_linker
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raccoon_linker
        PUBLIC
        lldCommon
        lldCOFF
        lldELF
        lldMachO
        lldMinGW
        lldWasm
)

target_compile_features(raccoon_linker PUBLIC cxx_std_23)