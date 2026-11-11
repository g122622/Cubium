# Clang 的 Windows GNU 驱动与 Unix 驱动均由 ccache 包装，不再按宿主平台跳过。
if(MC_ENABLE_CCACHE)
    find_program(CCACHE_PROGRAM ccache
        HINTS "$ENV{LOCALAPPDATA}/Cubium/tools/ccache")
    if(CCACHE_PROGRAM)
        # 必需选项随 launcher 传入，不经过 preset 的增量构建同样可以缓存 PCH。
        # 继续使用 ccache 的正常配置文件，保留调用方设置的目录、容量与远端前缀。
        set(_mc_cache_launcher "${CMAKE_COMMAND}" -E env
            "CCACHE_COMPILERCHECK=content"
            "CCACHE_SLOPPINESS=pch_defines,time_macros,include_file_mtime,include_file_ctime"
            "${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER ${_mc_cache_launcher})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${_mc_cache_launcher})
        message(STATUS "Compiler cache: ${CCACHE_PROGRAM}")
    else()
        message(WARNING "MC_ENABLE_CCACHE is ON but ccache was not found; see docs/BUILD.md for installation")
    endif()
endif()
