add_executable(really
        really.cpp
        subscription.cpp
        testLogger.cpp)
target_link_libraries(really PRIVATE quicr  picotls-core picotls-openssl)

target_compile_options(really
        PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>: -Wpedantic -Wextra -Wall>
        $<$<CXX_COMPILER_ID:MSVC>: >)

set_target_properties(really
        PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS ON)


# --------------------------------
# Really test
# --------------------------------
add_executable( reallyTest reallyTest.cpp testLogger.cpp)

target_link_libraries( reallyTest LINK_PUBLIC quicr  picotls-core picotls-openssl)

target_compile_options(reallyTest
        PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>: -Wpedantic -Wextra -Wall>
        $<$<CXX_COMPILER_ID:MSVC>: >)

set_target_properties(reallyTest
        PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS OFF)
