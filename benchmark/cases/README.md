# 基准用例

## 目录结构

```text
cases/
├── ChunkGenerationBenchmark.cpp   # 生产级区块生成吞吐
├── LightingBenchmark.cpp          # 光照更新吞吐
├── PalettedContainerBenchmark.cpp # 调色板容器读写
├── ServerExecutablePath.hpp       # 跨平台定位基准同目录的服务端
└── ServerInitializeBenchmark.cpp  # 同配置服务端进程的启动耗时
```

## 内部关系与外部依赖

用例注册到 google/benchmark，由 main 统一输出结果和内存指标。
前三组依赖项目世界与数据结构；启动用例用 CMake 注入的服务端文件名，结合运行时基准目录定位目标。

## 容易踩的坑

- 启动基准必须使用同一配置的服务端；Release benchmark 不得启动 sanitizer 版本。
- benchmark 与同配置服务端须放在同一目录；可整体移动到其他 runner，不依赖构建机器绝对路径或 CWD。
- 异步区块生成需要主线程泵送 tick，不能只等待 future。
