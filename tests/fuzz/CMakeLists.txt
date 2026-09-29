# tests/fuzz/CMakeLists.txt
# 协议模糊测试目标（libFuzzer harness）。
#
# 仅当 MC_BUILD_FUZZERS=ON 时由根 CMakeLists.txt 的 add_subdirectory(tests/fuzz) 进入。
# 全部目标（含插桩静态库 mc_fuzz_net）均标 EXCLUDE_FROM_ALL：不参与常规 `cmake --build`，
# 因此不会拖慢日常构建；须显式指定 --target 构建，例如：
#   cmake --build build --config RelWithDebInfo --target fuzz_java_codec
# 或经 build preset：
#   ./scripts/configure.sh --preset windows-clang-fuzz
#   cmake --build --preset windows-clang-fuzz
#
# ============================================================================
# 插桩策略：局部插桩
# ============================================================================
# 网络子系统的解码链路 TU 被本目录**再编译一份**为插桩静态库 mc_fuzz_net
# （SanitizerCoverage + ASan/UBSan），其余符号（方块/物品注册表、NBT、世界数据、
# 脚本系统等）从已编译的 mc_common 惰性拉取。
#
# 这样无需重建百万行代码即可获得解码路径的覆盖率反馈。代价是存在"同一 TU 两份副本"
# 的重复符号风险，详见 docs/test/FUZZING.md 的"容易踩的坑"。

option(MC_FUZZ_ASAN "Enable ASan/UBSan in fuzz targets" ON)

# ============================================================================
# libFuzzer 运行时：Windows 上的 CRT 冲突与源码自建
# ============================================================================
# 【背景】clang 自带的预编译 libFuzzer 运行时（clang_rt.fuzzer-x86_64.lib）是以
# **静态 CRT（/MT）**构建的，其 .drectve 段含
#     /FAILIFMISMATCH:"RuntimeLibrary=MT_StaticRelease" /DEFAULTLIB:libcmt.lib
# 而本项目（含 vcpkg x64-windows 依赖链）全量使用**动态 CRT（/MD）**，导致链接期硬错误：
#     lld-link: error: /failifmismatch: mismatch detected for 'RuntimeLibrary'
# 注意：ASan/UBSan 的运行时**没有**该指令，与 /MD 兼容；只有 libFuzzer 运行时受影响。
#
# 【解法】用同一编译器以项目一致的 CRT（/MD）把 libFuzzer 运行时从 compiler-rt 源码
# 重新编一遍。源文件约 25 个、编译约 1 分钟，远低于"整库切 /MT 重建"的代价。
# 官方配方见 compiler-rt/lib/fuzzer/CMakeLists.txt：
#   clang_rt.fuzzer = LIBFUZZER_SOURCES + FuzzerMain.cpp（FuzzerInterceptors 属独立运行时，不含）
# 各平台专属源文件自带 `#if LIBFUZZER_*` 守卫，可整体编译（与官方构建一致）。
#
# 【留空时的行为】回退到 `-fsanitize=fuzzer` 链接官方预编译运行时。若该运行时的 CRT 与
# 本项目不一致（Windows + /MD 即如此），链接会失败并给出上面的 /failifmismatch 信息。
set(MC_FUZZ_LIBFUZZER_SOURCE_DIR ""
    CACHE PATH
    "compiler-rt/lib/fuzzer 源码目录；非空时以本项目 CRT 自建 libFuzzer 运行时（Windows 上必需）")

set(MC_FUZZ_HAVE_SOURCE_LIBFUZZER FALSE)
if(MC_FUZZ_LIBFUZZER_SOURCE_DIR)
    if(EXISTS "${MC_FUZZ_LIBFUZZER_SOURCE_DIR}/FuzzerDriver.cpp"
        AND EXISTS "${MC_FUZZ_LIBFUZZER_SOURCE_DIR}/FuzzerMain.cpp")
        set(MC_FUZZ_HAVE_SOURCE_LIBFUZZER TRUE)
    else()
        message(FATAL_ERROR
            "MC_FUZZ_LIBFUZZER_SOURCE_DIR='${MC_FUZZ_LIBFUZZER_SOURCE_DIR}' 下找不到 "
            "FuzzerDriver.cpp / FuzzerMain.cpp，请指向 compiler-rt/lib/fuzzer 目录。")
    endif()
endif()

# ---------------------------------------------------------------------------
# 被插桩的源文件：网络子系统解码链路
#
# 与 src/common/CMakeLists.txt 中 mc_common 的条目是同一批文件（此处重复编译以获得插桩）。
#
# 有意排除：
#   - transport/TcpTransport.cpp、transport/LocalTransport.cpp —— fuzz 目标不经 socket；
#     且 TcpTransport.hpp 是全项目唯一拖入 <asio.hpp> 的传输头；
#   - backend/bedrock/* —— stub，无实际解析逻辑；
#   - backend/java/generated/*.gen.cpp —— 仅静态查找表（无解析逻辑，插桩价值为零），
#     且它们是 src/common 目录作用域 add_custom_command 的产物，跨目录引用会引入
#     构建顺序问题；改由 mc_common 提供（见下方 add_dependencies）。
#
# backend/java/codecs/ 全部为 header-only，随 JavaProtocolTables.cpp 一并被插桩。
# ---------------------------------------------------------------------------
set(MC_FUZZ_INSTRUMENTED_SOURCES
    ${CMAKE_SOURCE_DIR}/src/common/network/buffer/Endian.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/buffer/ByteBuf.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/buffer/RegistryByteBuf.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/buffer/NbtIo.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/codec/EntityMetadataSerializer.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/codec/PacketSerializer.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/codec/PacketDeserializer.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/ir/ItemStackBridge.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/crypto/Crypt.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/crypto/AesCfb8.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/crypto/RsaHandshake.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/crypto/ZlibCodec.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/pipeline/VarintFraming.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/pipeline/CompressionHandlers.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/pipeline/CipherHandlers.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/pipeline/ProtocolSwapHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/backend/java/JavaBackend.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/backend/java/JavaProtocolTables.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/backend/java/mappings/JavaItemIdMap.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/backend/java/mappings/JavaBlockStateIdMap.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/backend/java/mappings/JavaBlockIdMap.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/backend/java/mappings/JavaMobEffectIdMap.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/backend/java/mappings/JavaAttributeIdMap.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/backend/java/mappings/JavaPotionIdMap.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/backend/java/mappings/JavaEnchantmentIdMap.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/backend/java/handshake/JavaLoginHandshaker.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/sync/ChunkSerializer.cpp
    ${CMAKE_SOURCE_DIR}/src/common/network/sync/VanillaChunkWire.cpp

    # ------------------------------------------------------------------------
    # 服务端网络层（握手/会话状态机）——C 阶段的状态机 harness 需要
    #
    # 这些文件不在 mc_common 里（它们直接编进 minecraft-server 可执行文件），
    # 故须在此单独编译以获得插桩。
    #
    # 只取**握手四阶段**真正需要的那几个：已核实 ServerClientConnection.cpp /
    # ServerNetwork.cpp / ServerHandshake.cpp 对 MinecraftServer / PlayerManager /
    # ServerWorld / IntegratedServer 的引用为 0 处；RegistryDataBuilder.cpp 与
    # EnchantmentNbtBuilder.cpp 中各自唯一的一处只出现在注释里。
    # LoginFlow.cpp（11 处服务端引用）与 ServerPlayHandler 及 6 个包族处理器属
    # Play 阶段，需要 MinecraftServer 与无头服务端骨架，不在本阶段范围内。
    #
    # 插桩这些文件同时带来 trace-cmp 值反馈：状态机的 phase/状态枚举比较会被
    # -fsanitize=fuzzer 的 trace-cmp 插桩捕获，配合 -use_value_profile=1 即可引导
    # 搜索走向新状态——这正是 AFLnet「状态反馈」思想的进程内等价物。
    # ------------------------------------------------------------------------
    ${CMAKE_SOURCE_DIR}/src/server/network/base/ServerClientConnection.cpp
    ${CMAKE_SOURCE_DIR}/src/server/network/session/ServerNetwork.cpp
    ${CMAKE_SOURCE_DIR}/src/server/network/handshake/ServerHandshake.cpp
    ${CMAKE_SOURCE_DIR}/src/server/network/handshake/RegistryDataBuilder.cpp
    ${CMAKE_SOURCE_DIR}/src/server/network/handshake/EnchantmentNbtBuilder.cpp
)

# ---------------------------------------------------------------------------
# 插桩编译选项
#
# fuzzer-no-link：打开 SanitizerCoverage，但不引入 libFuzzer 的 main
# （main 由链接期的 libFuzzer 运行时提供，见 mc_fuzz_libfuzzer）。
# ---------------------------------------------------------------------------
function(mc_fuzz_apply_instrumentation targetName)
    if(MC_FUZZ_ASAN)
        # -fno-sanitize-recover=all：UBSan 检出即终止，便于 fuzz 直接拿到崩溃现场。
        target_compile_options(${targetName} PRIVATE
            -fsanitize=fuzzer-no-link,address,undefined
            -fno-sanitize-recover=all
        )
    else()
        target_compile_options(${targetName} PRIVATE
            -fsanitize=fuzzer-no-link
        )
    endif()

    # 【Windows + ASan 必需】关闭 MSVC STL 的 ASan 容器注解。
    #
    # MSVC STL 的 <string>/<vector>/<optional> 会按"当前 TU 是否启用 ASan"发出
    # /FAILIFMISMATCH:"annotate_{string,vector,optional}=0|1" 指令。本方案只插桩网络
    # 子系统，项目其余 TU（如 mc_server_storage）未启用 ASan 值为 0，插桩 TU 为 1，
    # 链接期逐个报错：
    #   lld-link: error: /failifmismatch: mismatch detected for 'annotate_string'
    #   lld-link: error: /failifmismatch: mismatch detected for 'annotate_optional'
    #
    # _DISABLE_STL_ANNOTATION 是总开关，会自动定义 _DISABLE_STRING_ANNOTATION /
    # _DISABLE_VECTOR_ANNOTATION / _DISABLE_OPTIONAL_ANNOTATION，使插桩 TU 不再发出
    # 这些指令，冲突消失。上游 libFuzzer 测试对此有同样处理，见
    # llvm-project/compiler-rt/test/fuzzer/lit.cfg.py:88-95（那里只禁了 string+vector，
    # 新版 STL 增加 optional 后需用总开关）。宏定义见 MSVC STL 内部头
    # <__msvc_sanitizer_annotate_container.hpp>。
    #
    # 代价：失去 MSVC STL 容器在插桩 TU 内的 ASan container-overflow 注解检查。
    if(WIN32 AND MC_FUZZ_ASAN)
        target_compile_definitions(${targetName} PRIVATE _DISABLE_STL_ANNOTATION)
    endif()

    # 全局 add_compile_options 里的 -O2 出现在命令行更早位置，故此处 -O1 生效；
    # fuzz 构建用 -O1 换取更快的迭代与更可靠的 ASan 诊断。
    target_compile_options(${targetName} PRIVATE
        -O1
        -g
        -fno-omit-frame-pointer
    )
endfunction()

# ---------------------------------------------------------------------------
# ASan 运行时 DLL 部署（Windows）
#
# clang 在 Windows/MSVC ABI 下只用动态 ASan 运行时（clang_rt.asan_dynamic-x86_64.dll），
# 该 DLL 位于 LLVM 安装目录的 lib/clang/<ver>/lib/windows/ 下，不在 PATH 中，
# 因此必须随可执行文件一并部署，否则运行时报"找不到 DLL"。
# ---------------------------------------------------------------------------
function(mc_fuzz_deploy_asan_runtime targetName)
    if(NOT WIN32 OR NOT MC_FUZZ_ASAN)
        return()
    endif()

    get_filename_component(_clangBinDir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    file(GLOB _asanDlls
        "${_clangBinDir}/../lib/clang/*/lib/windows/clang_rt.asan_dynamic-x86_64.dll")

    if(NOT _asanDlls)
        message(WARNING
            "未找到 clang_rt.asan_dynamic-x86_64.dll（查找路径 ${_clangBinDir}/../lib/clang/*/lib/windows/）。"
            "运行 fuzz 目标时须自行保证该 DLL 在 PATH 中，否则启动即失败。")
        return()
    endif()

    list(GET _asanDlls 0 _asanDll)
    add_custom_command(TARGET ${targetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_asanDll}" "$<TARGET_FILE_DIR:${targetName}>"
        COMMENT "复制 ASan 运行时 DLL 到 ${targetName} 输出目录"
        VERBATIM
    )
endfunction()

# ---------------------------------------------------------------------------
# 源码自建 libFuzzer 运行时（仅当 MC_FUZZ_LIBFUZZER_SOURCE_DIR 非空）
#
# 关键点：
#   - 不加任何 -fsanitize 选项：libFuzzer 自身是"运行时"，不属于被插桩对象；
#     它定义 __sanitizer_cov_* 回调供插桩对象调用（见 FuzzerTracePC.cpp）。
#   - 与项目同样走 CMake 默认的 MultiThreadedDLL（/MD）CRT，这正是本方案的目的。
#   - 官方 MSVC 侧额外加 -D_HAS_EXCEPTIONS=0。
# ---------------------------------------------------------------------------
if(MC_FUZZ_HAVE_SOURCE_LIBFUZZER)
    set(MC_FUZZ_LIBFUZZER_SOURCES
        FuzzerCrossOver.cpp
        FuzzerDataFlowTrace.cpp
        FuzzerDriver.cpp
        FuzzerExtFunctionsDlsym.cpp
        FuzzerExtFunctionsWeak.cpp
        FuzzerExtFunctionsWindows.cpp
        FuzzerExtraCounters.cpp
        FuzzerExtraCountersDarwin.cpp
        FuzzerExtraCountersWindows.cpp
        FuzzerFork.cpp
        FuzzerIO.cpp
        FuzzerIOPosix.cpp
        FuzzerIOWindows.cpp
        FuzzerLoop.cpp
        FuzzerMerge.cpp
        FuzzerMutate.cpp
        FuzzerSHA1.cpp
        FuzzerTracePC.cpp
        FuzzerUtil.cpp
        FuzzerUtilDarwin.cpp
        FuzzerUtilFuchsia.cpp
        FuzzerUtilLinux.cpp
        FuzzerUtilPosix.cpp
        FuzzerUtilWindows.cpp
        # main 入口（官方 RTfuzzer_main）。FuzzerInterceptors.cpp 属独立运行时，不含。
        FuzzerMain.cpp
    )
    list(TRANSFORM MC_FUZZ_LIBFUZZER_SOURCES
        PREPEND "${MC_FUZZ_LIBFUZZER_SOURCE_DIR}/")

    add_library(mc_fuzz_libfuzzer STATIC ${MC_FUZZ_LIBFUZZER_SOURCES})
    set_target_properties(mc_fuzz_libfuzzer PROPERTIES EXCLUDE_FROM_ALL TRUE)

    target_include_directories(mc_fuzz_libfuzzer PRIVATE
        "${MC_FUZZ_LIBFUZZER_SOURCE_DIR}"
        # 官方 include_directories(../../include)：compiler-rt/include 下的
        # sanitizer/*.h（FuzzerTracePC.h 等引用）。
        "${MC_FUZZ_LIBFUZZER_SOURCE_DIR}/../../include"
    )

    target_compile_options(mc_fuzz_libfuzzer PRIVATE
        -O2
        -fno-omit-frame-pointer
        # 第三方运行时源码，不参与项目的 -Werror 规范（同 mc_offset_allocator 的处理）。
        -w
    )
    if(WIN32)
        target_compile_definitions(mc_fuzz_libfuzzer PRIVATE _HAS_EXCEPTIONS=0)
    endif()
endif()

# ---------------------------------------------------------------------------
# 插桩静态库：网络子系统解码链路
#
# 用 STATIC 而非 OBJECT 库：静态库成员按未解析符号**惰性拉取**，因此
#   1. 本库的符号优先于 mc_common 内的未插桩同名副本被选中（本库在链接行更靠前）；
#   2. 未被任何 harness 引用的 TU 不会进入链接，不产生无用开销。
# OBJECT 库则会把全部 .obj 无条件塞进链接，重复符号风险高得多。
# ---------------------------------------------------------------------------
add_library(mc_fuzz_net STATIC ${MC_FUZZ_INSTRUMENTED_SOURCES})
set_target_properties(mc_fuzz_net PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(mc_fuzz_net PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# PRIVATE 链接 mc_common 的目的不是链接，而是继承其 PUBLIC 使用要求：
# spdlog/nlohmann_json/simdjson/mc::profiler 的头路径，以及 MC_ENABLE_TRACING=1 等
# 编译定义——使插桩副本与 mc_common 内的同名 TU 编译配置一致，
# 否则 TraceEvents/perfetto 相关代码在两侧展开不一致（ChunkSerializer.cpp 用到）。
target_link_libraries(mc_fuzz_net PRIVATE mc_common)

# 显式顺序依赖：插桩副本含 network/backend/java/mappings/*.cpp，它们 include 由
# src/common 目录作用域 add_custom_command 生成的 generated/*.gen.hpp。
# 干净检出后须保证烘焙步骤先跑完，否则首个 fuzz 目标编译即因缺头文件失败。
add_dependencies(mc_fuzz_net mc_common)

mc_fuzz_apply_instrumentation(mc_fuzz_net)

# ---------------------------------------------------------------------------
# fuzz 目标定义函数
# ---------------------------------------------------------------------------
function(mc_add_fuzz_target targetName)
    cmake_parse_arguments(ARG "" "" "SOURCES;EXTRA_LIBS;DEFINES" ${ARGN})
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "mc_add_fuzz_target(${targetName}): SOURCES 不能为空")
    endif()

    add_executable(${targetName} EXCLUDE_FROM_ALL ${ARG_SOURCES})

    target_include_directories(${targetName} PRIVATE
        # fuzz/ 为根：harness 以 #include "support/FuzzSupport.hpp" 表达私有头，
        # 与 tests/unit 以自身为根的约定一致。
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
    )

    mc_fuzz_apply_instrumentation(${targetName})

    if(ARG_DEFINES)
        target_compile_definitions(${targetName} PRIVATE ${ARG_DEFINES})
    endif()

    # 链接期提供 libFuzzer 运行时与 main。
    if(MC_FUZZ_HAVE_SOURCE_LIBFUZZER)
        target_link_libraries(${targetName} PRIVATE mc_fuzz_libfuzzer)
        # 源码自建路径下不能再传 -fsanitize=fuzzer（那会拖入官方 /MT 运行时）。
        if(MC_FUZZ_ASAN)
            target_link_options(${targetName} PRIVATE -fsanitize=address,undefined)
        endif()
    else()
        if(MC_FUZZ_ASAN)
            target_link_options(${targetName} PRIVATE -fsanitize=fuzzer,address,undefined)
        else()
            target_link_options(${targetName} PRIVATE -fsanitize=fuzzer)
        endif()
    endif()

    # 顺序关键：mc_fuzz_net 必须排在 mc_common 之前，同名符号才优先取插桩副本。
    #
    # mc_common 是 STATIC 库，成员按未解析符号惰性拉取，因此默认链接集很轻。
    # 反之 mc_gen / mc_server_storage 都是 **OBJECT 库**——其全部 .obj 会被无条件
    # 塞进链接行，从而拖入世界生成、存档乃至服务端可执行文件里的符号（如
    # mc::scoreboard::ScoreboardDataManager）。因此这两者**只能按目标显式请求**
    # （EXTRA_LIBS），只在确实需要注册表装配的 harness 上使用。
    target_link_libraries(${targetName} PRIVATE
        mc_fuzz_net
        mc_common
        mc_offset_allocator
        ${ARG_EXTRA_LIBS}
        nlohmann_json::nlohmann_json
    )

    if(WIN32)
        target_link_libraries(${targetName} PRIVATE ws2_32)
    endif()

    set_target_properties(${targetName} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/fuzz"
    )

    # vcpkg 依赖 DLL（zlib1.dll 等）与 ASan 运行时 DLL 随产物部署。
    mc_copy_runtime_dlls(${targetName})
    mc_fuzz_deploy_asan_runtime(${targetName})
endfunction()

# ---------------------------------------------------------------------------
# 种子生成器（普通可执行文件，不是 fuzz 目标）
#
# 遍历 10 张包表的每个已登记包，用默认字段值编码成合法 wire 字节，产出结构化起始语料。
#
# 有意不链 mc_fuzz_net：后者带覆盖插桩，其 __sanitizer_cov_* 调用需要 libFuzzer 运行时
# 提供；本工具自带 main、也不需要覆盖率反馈，故直接链未插桩的 mc_common。
# ---------------------------------------------------------------------------
add_executable(fuzz_gen_seeds EXCLUDE_FROM_ALL
    tools/GenSeeds.cpp
    support/FuzzSupport.cpp)

target_include_directories(fuzz_gen_seeds PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

set(_genSeedsLinkLibraries mc_common mc_offset_allocator)
if(TARGET mc_bedrock_addon)
    list(APPEND _genSeedsLinkLibraries mc_bedrock_addon)
endif()

target_link_libraries(fuzz_gen_seeds PRIVATE
    ${_genSeedsLinkLibraries}
    nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(fuzz_gen_seeds PRIVATE ws2_32)
endif()

set_target_properties(fuzz_gen_seeds PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/fuzz"
)

mc_copy_runtime_dlls(fuzz_gen_seeds)

# ---------------------------------------------------------------------------
# 目标清单
# ---------------------------------------------------------------------------
# 帧层：VarInt21 长度前缀切帧（含半包/粘包/非法 VarInt）
mc_add_fuzz_target(fuzz_varint_framing
    SOURCES targets/FuzzVarintFraming.cpp)

# 压缩层：ZlibCodec + CompressionHandlers（阈值/声明长度/残留字节校验）
mc_add_fuzz_target(fuzz_compression
    SOURCES targets/FuzzCompression.cpp)

# 加密层：AES-CFB8 跨包流式状态
mc_add_fuzz_target(fuzz_cipher
    SOURCES targets/FuzzCipher.cpp)

# Java 全阶段 codec：packetID 分发 + payload 解析（NBT/Component/ItemStack/BlockState holder）
#
# EXTRA_LIBS 说明：本 harness 会拉入方块/物品注册表装配（VanillaBlocks::initialize /
# Items::initialize），其一路上溯到 mc_common 的 Entity.cpp —— 而 Entity 析构与
# doBlockCollisions 引用了基岩 Addon 的 ScriptHandleRegistry / BlockComponentRegistry。
# 因此需要 mc_bedrock_addon（STATIC 库，惰性拉取，不会像 OBJECT 库那样拖入全量对象）。
mc_add_fuzz_target(fuzz_java_codec
    SOURCES
        targets/FuzzJavaCodec.cpp
        support/FuzzSupport.cpp
    EXTRA_LIBS
        mc_bedrock_addon
    DEFINES
        MC_FUZZ_FLOW_MODE=2)

# 握手四阶段服务端状态机：消息**序列**变异（对应 AFLnet 的状态机 fuzz 价值）
mc_add_fuzz_target(fuzz_handshake_session
    SOURCES
        targets/FuzzHandshakeSession.cpp
        support/FuzzSupport.cpp
    EXTRA_LIBS
        mc_bedrock_addon)

# 整条 Wire 入站流水线：Connection + 伪传输（覆盖层间组合、TCP 切分、阶段切换）
mc_add_fuzz_target(fuzz_connection_wire
    SOURCES
        targets/FuzzConnectionWire.cpp
        support/FuzzSupport.cpp
    EXTRA_LIBS
        mc_bedrock_addon)

# 区块线格式：内部紧凑格式 + 单段 + vanilla LevelChunkWithLight → ChunkData 翻译
mc_add_fuzz_target(fuzz_chunk_wire
    SOURCES
        targets/FuzzChunkWire.cpp
        support/FuzzSupport.cpp
    EXTRA_LIBS
        mc_bedrock_addon)

# Serverbound 专用：只测服务端解码不可信客户端输入的方向（远程攻击面）。
mc_add_fuzz_target(fuzz_java_codec_sb
    SOURCES
        targets/FuzzJavaCodec.cpp
        support/FuzzSupport.cpp
    EXTRA_LIBS
        mc_bedrock_addon
    DEFINES
        MC_FUZZ_FLOW_MODE=0)

# Clientbound 专用：只测客户端解码服务端输入的方向。
mc_add_fuzz_target(fuzz_java_codec_cb
    SOURCES
        targets/FuzzJavaCodec.cpp
        support/FuzzSupport.cpp
    EXTRA_LIBS
        mc_bedrock_addon
    DEFINES
        MC_FUZZ_FLOW_MODE=1)
