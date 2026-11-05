# CompilerWarnings.cmake
# 设置编译器警告选项

function(mc_set_compiler_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # GCC/Clang警告选项
        target_compile_options(${target} PRIVATE
            -Wall          # 启用一组常用警告
            -Wextra        # 启用额外的警告
            -pedantic      # 严格按照标准要求发出警告
            -Werror        # 将所有上述警告升级为错误

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
            # google/benchmark 的 BENCHMARK 宏用 __COUNTER__ 生成唯一名；Clang 20+
            # 把 __COUNTER__ 归入 C2y 扩展，-pedantic 下 -Werror 会把宏展开判为错误。
            -Wno-c2y-extensions           # 禁用：C2y 扩展警告（如 __COUNTER__）

            # 显式要求向量化的循环（`#pragma clang loop vectorize_width(N)`）偶尔会被优化器
            # 放弃（典型：循环体里出现跨 TU 调用、或依赖运行期 trip count）。-Wall 会顺带打开
            # -Wpass-failed=transform-warning，再经上面的 -Werror 升级成**编译错误**——于是
            # 一次"少了个向量化"的性能退化就能把整条构建链打断（实测 PerlinNoise.cpp 与
            # BlendedNoise.cpp 曾因此无法编译）。
            #
            # 这里关掉它：向量化与否是性能问题，不该阻断构建。若确实需要复核某个循环是否
            # 仍被向量化，可临时用 -Rpass=loop-vectorize 单独观察。
            -Wno-pass-failed             # 禁用：向量化等优化 pass 未执行时的诊断

            # 将关键警告视为错误。不用手动启用，因为 -Werror=all 已经包含了这些。
            # -Werror=return-local-addr    # 错误：返回局部变量地址
            # -Werror=uninitialized        # 错误：使用未初始化变量
            # -Werror=overloaded-virtual   # 错误：重载虚函数问题
            # -Werror=inconsistent-missing-override # 错误：不一致的缺失覆盖
            # -Werror=reorder-ctor       # 错误：构造函数成员初始化顺序与声明顺序不一致
            # -Werror=null-dereference   # 错误：空指针解引用
            # -Werror=non-virtual-dtor
            # -Werror=inconsistent-missing-destructor-override
            # -Werror=free-nonheap-object
            # -Werror=delete-non-virtual-dtor
            # -Werror=delete-incomplete
            # -Werror=return-stack-address
            # -Werror=unused-but-set-variable
            # -Werror=dangling-gsl
            # -Werror=unused-function
            # -Werror=nan-infinity-disabled
            # -Werror=sign-compare
            # -Werror=bitwise-op-parentheses
            # -Werror=shadow
            # -Werror=pessimizing-move
            # -Werror=switch
            # -Werror=old-style-cast
            # -Werror=unused-result
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
