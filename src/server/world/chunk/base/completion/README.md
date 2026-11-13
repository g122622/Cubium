# 完成接收、预算与世界交接协议

## 目录结构树

```text
completion/
├── ChunkCompletionInbox.hpp  # 可独立保活并安全关闭的后台完成接收端
├── ChunkCompletionQueue.hpp  # 按预算消费入口快照并保持旧结果优先
├── ChunkGenerationCallbacks.hpp  # 来源需求和 FULL 生成结果的显式交接
├── ChunkRequestCompletion.hpp  # 在既定线程交付已经就绪的请求
└── IChunkWorldLifecycle.hpp  # 区块核心向世界提交激活与退场的窄接口
```

## 内部模块关系

请求交付与世界副作用分开；后台持有可独立存活的接收端，主线程消费入口快照并保持旧项优先。

## 上下游外部依赖关系

被来源加载、生成、卸载和世界接入共同消费；仅定义基础交接协议和队列规则。

## 容易踩的坑

- 关闭接收端后不能再访问宿主。预算按已处理项检查，至少处理一项；调用回调期间释放队列锁，来源接收另设重入守卫。
- 迁移源文件时同步更新全部消费目标的包含路径与唯一 CMake 编译归属。
