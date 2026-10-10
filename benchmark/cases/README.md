# 基准用例

## 目录结构

```text
cases/
├── ChunkGenerationBenchmark.cpp   # 生产级区块生成吞吐
├── LightingBenchmark.cpp          # 光照更新吞吐
├── PalettedContainerBenchmark.cpp # 调色板容器读写
└── ServerInitializeBenchmark.cpp  # 同配置服务端进程的启动耗时
```

## 内部关系与外部依赖

用例注册到 google/benchmark，由 main 统一输出结果和内存指标。
前三组依赖项目世界与数据结构；启动用例依赖 CMake 注入的 minecraft-server 目标路径。

## 容易踩的坑

- 启动基准必须使用同一配置的服务端；Release benchmark 不得启动 sanitizer 版本。
- 构建产物跨 job 复用时，保留路径布局，使构建期注入的目标路径有效。
- 异步区块生成需要主线程泵送 tick，不能只等待 future。
