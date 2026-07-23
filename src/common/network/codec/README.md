# Network Codec 模块

协议无关 codec 框架，是"LLVM IR 指令集"——与具体后端解耦的编解码原语与组合器。

## 目录结构

```
src/common/network/codec/
├── StreamCodec.hpp      # 接口 StreamCodec<B,V>（encode/decode）+ LambdaCodec 适配器 + CodecFor 概念
├── StreamCodecs.hpp     # 原语+组合器（header-only）：U8/U16/I32/I64/F32/F64/Bool/VarInt/VarLong/String 原语，optional/collection/memberCodec/unit 组合器
└── IdDispatchCodec.hpp  # 注册顺序即 ID 的分发 codec（matches→写 id+payload / 读 id→解码 payload 成 variant）
```

## 内部模块关系

- `StreamCodec<B,V>` 是接口基类；`LambdaCodec` 把一对 lambda 包成接口实现，供顶层经 `StreamCodec&` 分发。
- `StreamCodecs` 里的原语/组合器是值类型结构体（鸭子类型，满足 `CodecFor`），按值层层组合，编译期单态化，无虚函数开销。
- `IdDispatchCodec` 持有 entries（每项含 matches/encodePayload/decode 的 std::function），用 `IdDispatchCodec::encode/decode` 做包 ID 分发；它本身不继承 `StreamCodec`，而是 ProtocolInfo 持有它做包表。

## 上下游外部依赖关系

- **上游依赖**：`common/core/Result`、`common/core/Types`、`common/network/buffer/ByteBuf`（原语转发其读写方法）。
- **下游**：`protocol/ProtocolInfo` 持有 `IdDispatchCodec` 做包表；`backend/java/codecs/` 用 `StreamCodecs` 原语 + `makeLambdaCodec` 拼每个 IR 包的 codec。

## 容易踩的坑

1. **包 ID 由注册顺序决定，绝不硬编码 case**：所有包分发走 `IdDispatchCodec`，addPacket 顺序 = packet id；改顺序或插包会导致 ID 漂移破坏网络兼容。Java 后端的 addPacket 顺序须严格对齐 `GameProtocols.java` 注册顺序。
2. **IdDispatchCodec::encode 是"先匹配再写"**：先用 `matches` 定位 variant 当前备选项，再写 id+payload，**不要**先写 id 再回滚（VarInt id 字节数可变，回滚易错）。
3. **组合器 decode 用 MC_TRY_ASSIGN 传播错误**：子 codec 失败冒泡到顶层，不抛异常；codec 层禁止 try/catch。
4. **optional/collection 的元素类型靠 decltype 推导**：`Inner::decode` 返回 `Result<T>`，组合器用 `decltype(... .value())` 取 T；原语 codec 的 decode 必须返回 `Result<具体类型>`，不能返回 `Result<auto>`。
5. **MemberCodec 是工具非分发器**：它绑定成员指针+成员 codec 供 IR 包 codec 复用逐字段读写，不参与 ID 分发；IR 包的顶层 codec 用 `makeLambdaCodec` 组合若干 MemberCodec。
