# CompilerWarnings.cmake
# 设置编译器警告选项

function(mc_set_compiler_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # GCC/Clang警告选项
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wold-style-cast
            -Wnon-virtual-dtor

            # 禁用的警告
            -Wno-unused-parameter       # 禁用：未使用的参数警告
            -Wno-unused-variable        # 禁用：未使用的变量警告
            -Wno-unused-private-field   # 禁用：未使用的私有成员警告
            -Wno-unused-lambda-capture  # 禁用：未使用的lambda捕获警告
            -Wno-defaulted-function-deleted # 禁用：默认函数被删除警告
            -Wno-sign-conversion        # 禁用：符号转换警告
            -Wno-implicit-int-float-conversion # 禁用：隐式整数到浮点转换警告
            -Wno-implicit-float-conversion # 禁用：隐式浮点转换警告
            -Wno-implicit-int-conversion # 禁用：隐式整数转换警告
            -Wno-float-conversion         # 禁用：浮点转换警告

            # 将关键警告视为错误
            -Werror=return-local-addr    # 错误：返回局部变量地址
            -Werror=uninitialized        # 错误：使用未初始化变量
            -Werror=format               # 错误：格式字符串问题
        )

        # 可选：将所有警告视为错误
        option(MC_WARNINGS_AS_ERRORS "Treat all warnings as errors" OFF)
        if(MC_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(mc_copy_runtime_dlls target)
    if(NOT WIN32)
        return()
    endif()

    if(NOT DEFINED VCPKG_TARGET_TRIPLET)
        return()
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DtargetBinary=$<TARGET_FILE:${target}>
            -DinstalledDir=${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin
            -DoutputDir=$<TARGET_FILE_DIR:${target}>
            -P "${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDlls.cmake"
        COMMENT "复制 ${target} 的运行时 DLL"
        VERBATIM
    )
endfunction()
