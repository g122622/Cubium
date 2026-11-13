# 生命周期用例编排与内部能力

## 目录结构树

```text
lifecycle/
├── ChunkLifecycleController.cpp  # 编排需求、加载、生成、交付与退场
├── ChunkLifecycleController.hpp  # 编排需求、加载、生成、交付与退场
├── demand/  # 加载票据、距离传播和玩家追踪
├── generation/  # 生成依赖调度、单步执行与保活
├── loading/  # 异步来源解析和在途加载合并
├── state/  # 当前生命周期实例、区块发布与状态不变量
└── unloading/  # 候选回收、异步保存和最终退场复核
```

## 内部模块关系

控制器负责动作顺序；state、demand、loading、generation、unloading 分别拥有自己的状态和流程。

## 上下游外部依赖关系

上层为 ServerChunkManager；依赖世界接入协议、公共数据、存储及共享区域调度。

## 容易踩的坑

- 请求登记与最终移除共用调度区域锁；取消只置位令牌，任务身份与邻居保活必须等待执行器确认退出。
- 迁移源文件时同步更新全部消费目标的包含路径与唯一 CMake 编译归属。
