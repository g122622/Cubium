# 生成依赖调度、单步执行与保活

## 目录结构树

```text
generation/
├── base/  # 跨能力共享的值类型与交接协议
├── ChunkGenerationScheduler.cpp  # 维护生成依赖图和任务执行身份
├── ChunkGenerationScheduler.hpp  # 维护生成依赖图和任务执行身份
├── ChunkLightingProvider.cpp  # 生成 LIGHT 阶段的区域光照访问
├── ChunkLightingProvider.hpp  # 生成 LIGHT 阶段的区域光照访问
├── ChunkProgressionTask.cpp  # 保活中心及邻居并执行单个生成阶段
├── ChunkProgressionTask.hpp  # 保活中心及邻居并执行单个生成阶段
├── ChunkStepExecutor.cpp  # 调用生成算法并接入上下文和光照步骤
└── ChunkStepExecutor.hpp  # 调用生成算法并接入上下文和光照步骤
```

## 内部模块关系

调度器判断阶段和邻居资格，任务保活全部邻居，步骤执行器调用算法；FULL 与来源需求通过明确回调交接。

## 上下游外部依赖关系

依赖注册表、阶段模型、IChunkGenerator、世界光照接入和 AreaTaskScheduler；不依赖区块门面。

## 容易踩的坑

- 邻居从首次扫描开始保活，包含已经就绪的邻居；按实际登记集合释放。依赖图区域锁与执行区域互斥不同。种子始终来自生成器。
- 迁移源文件时同步更新全部消费目标的包含路径与唯一 CMake 编译归属。
