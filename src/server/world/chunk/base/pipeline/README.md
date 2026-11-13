# 生成阶段、邻居要求与推进约束

## 目录结构树

```text
pipeline/
├── ChunkDependencies.cpp  # 按邻居半径声明阶段要求
├── ChunkDependencies.hpp  # 按邻居半径声明阶段要求
├── ChunkPyramid.cpp  # 组合直接和累积阶段依赖
├── ChunkPyramid.hpp  # 组合直接和累积阶段依赖
├── ChunkStatus.cpp  # 生成阶段及顺序关系
├── ChunkStatus.hpp  # 生成阶段及顺序关系
└── ChunkStep.hpp  # 阶段访问范围和可写范围约束
```

## 内部模块关系

ChunkStatus 定义阶段，ChunkDependencies 与 ChunkStep 描述邻居和写入要求，ChunkPyramid 组合累积约束。

## 上下游外部依赖关系

生成算法、区块运行时、结构查询和网络发送共同消费；不执行任务或访问世界运行时。

## 容易踩的坑

- 直接依赖半径、累积访问半径和写半径含义不同；不能用写半径代替依赖图锁范围。
- 迁移源文件时同步更新全部消费目标的包含路径与唯一 CMake 编译归属。
