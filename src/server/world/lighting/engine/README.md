# 光照传播引擎

## 目录结构树

```text
engine/
├── BaseLightEngine.hpp/cpp   # 公共空间缓存、空段处理、队列传播及发布通知
├── BlockLightEngine.hpp/cpp  # 发光方块的增亮、减亮与 Hidden 段处理
└── SkyLightEngine.hpp/cpp    # 天空光初始化、遮挡传播与向下挤出
```

## 内部模块关系

`BlockStarLightEngine`、`SkyStarLightEngine` 继承 `StarLightEngine`。基类管理每次操作的
区块、段、nibble 和空映射缓存，子类决定初始化与传播规则。持久光照数据属于区块；
引擎临时创建的 nibble 在缓存销毁时释放。

## 上下游外部依赖关系

- 依赖 `IChunk`、`StarLightLightingProvider`、`SWMRNibbleArray` 与方块遮挡/发光规则。
- 由服务端生成、加载和运行时光照任务驱动，通过 `WorldLightManager` 获取 TLS 实例。
- 更新完成后发布区块可见数据，经 provider 通知主线程处理光照变化。

## 容易踩的坑

- `light()` 从临时 Null 数组开始；移入区块后，缓存必须重新指向区块数组，再发布可见侧。
- `destroyCaches()` 清除引用和临时所有者，不缩小传播队列的容量。
- 天空光去初始化使用 Null；方块光使用 Hidden 保留减亮所需数据。
- 天空光缓存重写只移除已经为 Null 的缓存项，不负责把有效数组转为 Null。
- FIFO 传播顺序、形状遮挡以及跨段边界处理属于算法语义，存储压缩不得改变这些规则。
- 发布使用的数组指针原子与数组字节的并发安全是两件事，参见光照存储中的 TODO。
