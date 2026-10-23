# tests/diff/CMakeLists.txt
# 差分测试探针（differential probes）。
#
# 定位：本目录是「同一份输入喂给 Cubium 与参照实现（原版数据 / Java oracle），
# 要求两侧输出逐字节相同」的第四类测试。它既不是 tests/unit（写死期望值），
# 也不是 tests/integrated（行为包 + GameTest），也不是 tests/e2e（真协议 + 真服务端）。
#
# 约定：
#   - 每个「能力领域」是一个 probe（probes/<domain>.cpp + probes/<domain>.hpp）。
#   - 所有 probe 共享同一入口 probes/runner_main.cpp，由编译期宏
#     MC_DIFF_PROBE_DOMAIN 选择能力，避免每个能力复制一份 main。
#   - Python 编排层在 harness/ 下：harness/runner.py 负责构建、生成输入、
#     运行 probe、比对与写报告；harness/test_<domain>_parity.py 是具体 harness。
#
# 全部目标标 EXCLUDE_FROM_ALL：不参与日常 `cmake --build`，须显式指定目标：
#   cmake --build build --config RelWithDebInfo --target diff_probes
#
# 为什么不是每能力一个独立 target：多个可执行文件都链同一个静态库 mc_common，
# 拆成多个 target 只是多几行 CMake，但对增量构建没有区别；合并到一个 target 里
# 用 foreach 生成，新增能力的成本最低。

# ---------------------------------------------------------------------------
# 能力清单：新增一个差分能力时，在三张平行列表的**同一位置**各追加一项，
# 然后在 probes/<domain>.cpp 实现它（runner_main.cpp 已共享，无需改动）。
#   后缀：可执行文件名 diff_<后缀>，也是测试脚本里引用的 domain
#   源文件：实现该能力的 .cpp
#   宏名：写入基类的 MC_DIFF_PROBE_DOMAIN，供 runner_main.cpp 选择能力
# ---------------------------------------------------------------------------
set(MC_DIFF_PROBE_SUFFIXES text_encode)
set(MC_DIFF_PROBE_SOURCES probes/text_encode.cpp)
set(MC_DIFF_PROBE_DOMAINS text_encode)

function(mc_add_diff_probe targetSuffix source domain)
    set(targetName "diff_${targetSuffix}")
    add_executable(${targetName} EXCLUDE_FROM_ALL
        probes/runner_main.cpp
        ${source}
    )

    target_include_directories(${targetName} PRIVATE
        # probes/ 为根：probe 源码以 #include "<domain>.hpp" 表达私有头。
        ${CMAKE_CURRENT_SOURCE_DIR}/probes
        ${CMAKE_SOURCE_DIR}/src
    )

    target_compile_definitions(${targetName} PRIVATE
        MC_DIFF_PROBE_DOMAIN=${domain}
        MC_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )

    # 依赖：mc_common 是本层唯一被测的公共库；mc_bedrock_addon 不是选择，而是方块/
# 物品注册链的链接期闭包——BaseBlocks 等触发器引用了基岩 addon 的组件注册表
# （BlockComponentRegistry 等），不链就一堆 undefined symbol。它也属 src/common。
    # 刻意不链 mc_gen / mc_server_storage：那两个是 OBJECT 库，会把世界生成与存档
    # 的全部目标文件无条件塞进链接行，而差分探针按定义不测它们。
    target_link_libraries(${targetName} PRIVATE
        mc_common
        mc_bedrock_addon
        mc_offset_allocator
        nlohmann_json::nlohmann_json
    )

    if(WIN32)
        target_link_libraries(${targetName} PRIVATE ws2_32)
    endif()

    # 顶层把 CMAKE_RUNTIME_OUTPUT_DIRECTORY 设为 bin/，多配置生成器（Ninja Multi-Config、
    # Visual Studio）会自动再追加 <Config>/ 一层。这里不覆盖它，让所有可执行文件
    # （服务端、mc_tests、probe）都按项目统一规则落在 bin/<Config>/；编排层按同一规则查找。
    # 注意 RUNTIME_OUTPUT_DIRECTORY_<Config> 是**整体覆盖**而非叠加，设成 bin/<Config>/diff
    # 反而会让 probe 与项目其它产物的布局不一致。

    mc_copy_runtime_dlls(${targetName})
endfunction()

list(LENGTH MC_DIFF_PROBE_SUFFIXES MC_DIFF_PROBE_COUNT)
list(LENGTH MC_DIFF_PROBE_SOURCES MC_DIFF_PROBE_SOURCE_COUNT)
list(LENGTH MC_DIFF_PROBE_DOMAINS MC_DIFF_PROBE_DOMAIN_COUNT)
if(NOT MC_DIFF_PROBE_COUNT EQUAL MC_DIFF_PROBE_SOURCE_COUNT
    OR NOT MC_DIFF_PROBE_COUNT EQUAL MC_DIFF_PROBE_DOMAIN_COUNT)
    message(FATAL_ERROR "tests/diff: 三张能力清单长度不一致（后缀 ${MC_DIFF_PROBE_COUNT}，"
        "源文件 ${MC_DIFF_PROBE_SOURCE_COUNT}，宏名 ${MC_DIFF_PROBE_DOMAIN_COUNT}）")
endif()

# 聚合目标：`--target diff_probes` 一次构建全部 probe。
add_custom_target(diff_probes)

math(EXPR MC_DIFF_PROBE_LAST "${MC_DIFF_PROBE_COUNT} - 1")
foreach(index RANGE ${MC_DIFF_PROBE_LAST})
    list(GET MC_DIFF_PROBE_SUFFIXES ${index} targetSuffix)
    list(GET MC_DIFF_PROBE_SOURCES ${index} source)
    list(GET MC_DIFF_PROBE_DOMAINS ${index} domain)
    mc_add_diff_probe(${targetSuffix} ${source} ${domain})
    add_dependencies(diff_probes diff_${targetSuffix})
endforeach()