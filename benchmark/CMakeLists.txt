add_executable(mc_benchmark
    main.cpp
    MemoryProfiler.hpp
    MemoryProfiler.cpp
    PerfettoProfilerAdapter.hpp
    PerfettoProfilerAdapter.cpp
    cases/ChunkStorageBenchmark.cpp
    cases/ChunkGenerationBenchmark.cpp
    cases/EntityRangeSeekBenchmark.cpp
    cases/LightingBenchmark.cpp
    cases/PalettedContainerBenchmark.cpp
    cases/ServerInitializeBenchmark.cpp
)

target_include_directories(mc_benchmark
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mc_benchmark
    PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
        mc_server_shared
        mc_gen
        mc_server_storage
        mc_common
        mc_bedrock_addon
        nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(mc_benchmark PRIVATE ws2_32)
endif()

mc_set_compiler_warnings(mc_benchmark)
target_compile_features(mc_benchmark PRIVATE cxx_std_20)

# 启动基准必须测同一配置的服务端，不能硬编码为 RelWithDebInfo。
if(NOT TARGET minecraft-server)
    message(FATAL_ERROR "mc_benchmark requires MC_BUILD_SERVER=ON")
endif()
target_compile_definitions(mc_benchmark PRIVATE
    MC_BENCHMARK_SERVER_FILE_NAME="$<TARGET_FILE_NAME:minecraft-server>")
add_dependencies(mc_benchmark minecraft-server)
