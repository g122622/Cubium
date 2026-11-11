# 通过目录的使用要求覆盖项目和 FetchContent 的 C/C++ 目标；关闭开关即移除插桩，
# 不把临时 sanitizer 参数留在 CMAKE_*_FLAGS 缓存里。
set(MC_SANITIZER_COMPILE_OPTIONS "" CACHE INTERNAL "Effective global sanitizer compile options" FORCE)
set(MC_SANITIZER_LINK_OPTIONS "" CACHE INTERNAL "Effective global sanitizer link options" FORCE)
if(MC_ENABLE_SANITIZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR WIN32)
        message(FATAL_ERROR "Global ASan/UBSan builds require Clang on Linux or macOS")
    endif()
    set(MC_SANITIZER_COMPILE_OPTIONS
        "-fsanitize=address,undefined;-fno-sanitize-recover=all;-fno-omit-frame-pointer"
        CACHE INTERNAL "Effective global sanitizer compile options" FORCE)
    set(MC_SANITIZER_LINK_OPTIONS "-fsanitize=address,undefined"
        CACHE INTERNAL "Effective global sanitizer link options" FORCE)
    add_compile_options(${MC_SANITIZER_COMPILE_OPTIONS})
    add_link_options(${MC_SANITIZER_LINK_OPTIONS})
endif()
