# Network Buffer 模块

协议无关字节缓冲抽象，是 codec 的草稿区，传输无关。

## 目录结构

```
src/common/network/buffer/
├── Endian.hpp/cpp          # 大端序工具（hostToNetwork/networkToHost 的 16/32/64 位）
├── ByteBuf.hpp/cpp         # 可增长字节缓冲+读写游标+VarInt/VarLong/Utf8/String（大端，移植自旧 FriendlyByteBuf）
├── RegistryByteBuf.hpp/cpp # 派生 ByteBuf，持有 const RegistryAccess&，物品/方块/实体类型按 VarInt ID 查表
└── NbtIo.hpp/cpp           # ByteBuf ↔ nbt 库桥接（sstream 中转，Java 大端二进制上下文）
```

## 内部模块关系

- `Endian` 被 `ByteBuf` 内部定长原语读写使用。
- `ByteBuf` 是核心，提供所有定长/VarInt/String 读写；`RegistryByteBuf` 继承它，叠加注册表 holder 读写。
- `NbtIo` 不修改 `ByteBuf` 的字节布局，仅在游标处中转 `nbt` 库的流式读写。

## 上下游外部依赖关系

- **上游依赖**：`common/core/Result`、`common/core/Types`、`common/registry/RegistryAccess`（仅 RegistryByteBuf）、`common/util/nbt/Nbt.hpp`（仅 NbtIo）。
- **下游**：`codec/` 的 `StreamCodec` 实现以 `RegistryByteBuf` 为缓冲类型；`backend/java/codecs/` 全部基于本模块。

## 容易踩的坑

1. **大端是 Java 协议强约束**：所有定长原语经 `Endian` 大端化；基岩版（小端）有专属 ByteBuf 变体，**不要**复用本 `ByteBuf` 直接给基岩后端。
2. **读操作返回 Result，不抛异常**：越界返回 `OutOfBounds`，非法 VarInt 返回 `InvalidData`；codec 层用 `MC_TRY`/`MC_TRY_ASSIGN` 传播，**不要**用 try/catch。
3. **readBytesView 零拷贝视图生命周期**：返回的 `string_view` 指向 `ByteBuf` 内部存储，缓冲一旦 `clear()`/`takeBytes()`/重分配即失效；调用方须在缓冲结构稳定期内使用。
4. **VarInt 最多 5 字节、VarLong 最多 10 字节**：超长按 `InvalidData` 处理；写负数 i32 会占满 5 字节。
5. **RegistryByteBuf 默认构造无注册表**：默认构造的实例 `hasRegistry()==false`，调注册表相关方法返回 `InvalidState`；反序列化路径必须先 `bindRegistry`。
6. **NbtIo 读取消耗的字节数靠 tellg 差值推断**：若 nbt 库流状态异常（tellg 返回 -1）会保守前进全部剩余字节，后续读取将读到空缓冲——只用于受信任的 Java 线协议输入。
7. **writeBlockStateHolder 当前是占位**：BlockState 的统一整数 stateId getter 待 Phase6 区块格式对齐时补全，现阶段写 0。
