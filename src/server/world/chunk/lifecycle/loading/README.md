# 异步来源解析和在途加载合并

## 目录结构树

```text
loading/
├── ChunkSourceLoader.cpp  # 合并来源读取并在主线程接收结果
└── ChunkSourceLoader.hpp  # 合并来源读取并在主线程接收结果
```

## 内部模块关系

在途表合并来源读取；后台回调仅持有完成接收端，主线程按预算安装区块并派发后续决策。

## 上下游外部依赖关系

消费 SingleLevelStorageManager、ChunkRegistry 和 IChunkWorldLifecycle；生成调度通过显式回调请求解析邻居来源。

## 容易踩的坑

- 读档错误不能当作存档缺失后重新生成。接收端关闭后安全丢弃迟到结果。处理期间新到达的完成项留给下一批，重入交给最外层消费。
- 迁移源文件时同步更新全部消费目标的包含路径与唯一 CMake 编译归属。
