# https://clang.llvm.org/extra/clang-tidy/
option (ENABLE_CLANG_TIDY "Use clang-tidy static analyzer" OFF)

if (ENABLE_CLANG_TIDY)

    find_program (CLANG_TIDY_CACHE_PATH NAMES "clang-tidy-cache")
    if (CLANG_TIDY_CACHE_PATH)
        find_program (_CLANG_TIDY_PATH NAMES "clang-tidy" "clang-tidy-15" "clang-tidy-14" "clang-tidy-13" "clang-tidy-12")

        set (CLANG_TIDY_PATH "${CLANG_TIDY_CACHE_PATH};${_CLANG_TIDY_PATH}" CACHE STRING "a doc")
    else ()
        find_program (CLANG_TIDY_PATH NAMES "clang-tidy" "clang-tidy-15" "clang-tidy-14" "clang-tidy-13" "clang-tidy-12")
    endif ()

    if (CLANG_TIDY_PATH)
        message (STATUS
            "Using clang-tidy: ${CLANG_TIDY_PATH}.
            The checks will be run during build process.
            See the .clang-tidy file at the root directory to configure the checks.")

        set (USE_CLANG_TIDY ON)

        # clang-tidy requires assertions to guide the analysis
        # Note that NDEBUG is set implicitly by CMake for non-debug builds
        set (COMPILER_FLAGS "${COMPILER_FLAGS} -UNDEBUG")

        # The variable CMAKE_CXX_CLANG_TIDY will be set inside the following directories with non third-party code.
        # - base
        # - programs
        # - src
        # - utils
        # set (CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_PATH}")
    else ()
        message (${RECONFIGURE_MESSAGE_LEVEL} "clang-tidy is not found")
    endif ()
endif ()
