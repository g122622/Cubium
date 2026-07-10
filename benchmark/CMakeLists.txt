add_executable(mc_benchmarks
    main.cpp
    BenchmarkCase.cpp
    BenchmarkConfig.cpp
    BenchmarkRegistry.cpp
    BenchmarkResultWriter.cpp
    BenchmarkRunner.cpp
    cases/ClientInitializeBenchmark.cpp
    cases/ChunkGenerationBenchmark.cpp
    # cases/ChunkMeshBenchmark.cpp - 暂时禁用，依赖客户端渲染模块
    cases/LightingBenchmark.cpp
)

target_include_directories(mc_benchmarks
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mc_benchmarks
    PRIVATE
        mc_common
        mc_bedrock_addon
        mc::profiler
        nlohmann_json::nlohmann_json
)

mc_set_compiler_warnings(mc_benchmarks)
target_compile_features(mc_benchmarks PRIVATE cxx_std_20)
