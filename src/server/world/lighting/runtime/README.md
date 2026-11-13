# 加载与运行时光照任务

## 目录结构树

```text
runtime/
├── ChunkLoadLightTask.cpp  # 区块加载后的异步光照任务
├── ChunkLoadLightTask.hpp  # 区块加载后的异步光照任务
├── RuntimeLightingProvider.cpp  # 为光照任务保活邻域并收集更新通知
├── RuntimeLightingProvider.hpp  # 为光照任务保活邻域并收集更新通知
├── RuntimeLightTask.cpp  # 运行时方块变化的光照传播任务
├── RuntimeLightTask.hpp  # 运行时方块变化的光照传播任务
├── ServerLightQueue.cpp  # 合并运行时方块变化并提交区域光照任务
└── ServerLightQueue.hpp  # 合并运行时方块变化并提交区域光照任务
```

## 内部模块关系

变更队列合并方块位置，加载任务和运行时任务共享区域执行器与光照提供者；worker 收集更新，主线程发布。

## 上下游外部依赖关系

由 ServerWorld 驱动；依赖世界区域调度器、光照引擎与公共区块数据。

## 容易踩的坑

- 同一世界所有光照任务和序列化必须共用区域调度器。主线程保持 flush、发送、提交新光照任务的顺序；提供者保活邻域直到回调结束。
- 迁移源文件时同步更新全部消费目标的包含路径与唯一 CMake 编译归属。
