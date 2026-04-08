# CompilerWarnings.cmake
# 设置编译器警告选项

function(mc_set_compiler_warnings target)
    if(MSVC)
        # MSVC警告选项
        target_compile_options(${target} PRIVATE
            /W4                    # 高警告级别
            /WX-                   # 警告不作为错误（可选开启）

            # 禁用的警告（使用注释语法 /* param */ 替代）
            /wd4251                # 禁用：DLL导出类成员警告
            /wd4275                # 禁用：DLL导出基类警告
            /wd4100                # 禁用：未引用的形参警告
            /wd4189                # 禁用：未引用的局部变量警告
            /wd4834                # 禁用：放弃 [[nodiscard]] 返回值警告

            # 将关键警告视为错误
            /we4172                # 错误：返回局部变量的地址或临时（严重bug）
            /we4701                # 错误：使用了可能未初始化的局部变量
            /we4703                # 错误：使用了可能未初始化的局部指针变量
            /we4477                # 错误：格式字符串宽/窄不匹配
            /we4312                # 错误：从较小类型转换为较大指针类型
            /we4552                # 错误："*" 运算符不起作用；应使用 "&&" 吗?
        )

        # 可选：将所有警告视为错误
        option(MC_WARNINGS_AS_ERRORS "Treat all warnings as errors" OFF)
        if(MC_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # GCC/Clang警告选项
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wold-style-cast
            -Wnon-virtual-dtor

            # 禁用的警告
            -Wno-unused-parameter       # 禁用：未使用的参数警告
            -Wno-unused-variable        # 禁用：未使用的变量警告
            -Wno-unused-private-field   # 禁用：未使用的私有成员警告
            -Wno-unused-lambda-capture  # 禁用：未使用的lambda捕获警告
            -Wno-defaulted-function-deleted # 禁用：默认函数被删除警告
            -Wno-address-of-temporary   # 禁用：获取临时对象地址警告

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
