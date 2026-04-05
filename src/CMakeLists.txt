add_executable(raccoon
        main.cpp
)

target_link_libraries(raccoon PRIVATE arc::arc)

target_include_directories(raccoon PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(raccoon PRIVATE cxx_std_23)

target_compile_options(raccoon PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
)