# src/common/test/CMakeLists.txt
# GameTest 集成测试框架 - 引擎无关核心库（mc_test）
#
# 依赖边界（对齐 plan 1I + 架构 pivot）：
#   mc_test 仅含 base/ + framework/ + native/，只依赖 mc_common 基础类型，
#   不依赖 server 类型（MinecraftServer/ServerWorld 在 src/server/）。
#   facade/ + minecraft/ + runner/ + simulated/ + script/ 已迁至 src/server/test/，
#   编译进 minecraft-server 与 mc_tests 两个 exe（它们已 link mc_common + mc_bedrock_addon）。
#
# mc_test 不得依赖 mc_bedrock_addon（避免环：script/ 经 server exe 编译，include mc_test 头）。

add_library(mc_test STATIC
    # base/ - 末梢基础类型（依赖链最底，被所有层依赖）
    base/error/GameTestError.cpp
    base/data/TestData.cpp
    base/coords/TestTransform.cpp

    # framework/ - 引擎无关抽象核心
    framework/function/BaseGameTestFunction.cpp
    framework/helper/NullGameTestHelper.cpp
    framework/sequence/GameTestSequence.cpp
    framework/registry/GameTestRegistry.cpp
    framework/instance/BaseGameTestInstance.cpp
    framework/batch/GameTestBatch.cpp
    framework/batch/BaseGameTestBatchRunner.cpp
    framework/ticker/GameTestTicker.cpp
    framework/environment/AllOfEnvironment.cpp
    framework/environment/EnvironmentRegistry.cpp
    framework/environment/SetGameRulesEnvironment.cpp
    framework/environment/TimeOfDayEnvironment.cpp
    framework/environment/WeatherEnvironment.cpp
    framework/environment/FunctionsEnvironment.cpp

    # native/ - 原生测试函数族（被 facade/GameTestRegistrar 封装）
    native/NativeGameTestFunction.cpp
    native/NativeTestRegistrationBuilder.cpp
)

target_link_libraries(mc_test
    PUBLIC
        mc_common
    PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

mc_set_compiler_warnings(mc_test)

target_compile_features(mc_test PUBLIC cxx_std_20)

target_include_directories(mc_test
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
