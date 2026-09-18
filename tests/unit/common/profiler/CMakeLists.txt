# tests/common/profiler/CMakeLists.txt
# Perfetto 模块测试
#
# 注意：Perfetto 测试已集成到主测试套件 (mc_tests) 中。
# 当 MC_ENABLE_TRACING=ON 时，运行完整的追踪功能测试。
# 当 MC_ENABLE_TRACING=OFF 时，运行禁用模式下的存根测试。
#
# 有关 Perfetto SDK 的链接说明：
# 由于 Perfetto SDK 的模板实例化要求，追踪功能需要将 perfetto.cc
# 包含在 TraceCategories.cpp 中编译。独立测试可执行文件会遇到
# DataSourceHelper<TrackEventDataSource>::type() 符号找不到的问题。
# 将测试集成到主测试套件中可以避免此问题。

# 测试源文件已添加到 tests/CMakeLists.txt 的 mc_tests 目标中
