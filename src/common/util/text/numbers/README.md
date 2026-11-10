# 数字格式（NumberFormat）

记分板分数的渲染格式：`blank`（不显示数字）/ `styled`（数字套样式）/ `fixed`（所有数字显示成同一个组件）。

## 目录结构树

```
src/common/util/text/numbers/
├── NumberFormat.hpp        # NumberFormatType 枚举与 NumberFormat 值类型（三变体、format(value)）
├── NumberFormat.cpp        # 三变体的渲染语义、深拷贝、注册名解析
├── NumberFormatCodec.hpp   # 存档 NBT 编码 + Java 线格式编码的对外接口
├── NumberFormatCodec.cpp   # 两者的实现（styled 载荷=Style NBT，fixed 载荷=Component NBT）
└── README.md
```

## 内部模块关系

- `NumberFormat` 只依赖 `TextStyle`（样式）与 `ITextComponent`（fixed 载荷）。
- `NumberFormatCodec` 依赖两个**共用**的基础编解码器，自己不重复实现字段清单：
  - `common/util/text/StyleNbt.hpp` —— Style ↔ NBT（被 Component 序列化与数字格式共用）
  - `common/util/text/ComponentNbtSerialization.hpp` —— Component ↔ NBT 标签/线格式字节
  - `common/network/buffer/NbtIo.hpp` —— 标签 → 根 NBT 线格式字节

## 上下游外部依赖关系

**上游（谁用本目录）**

- `common/scoreboard/core/ScoreObjective`（目标级默认格式）、`Score`（单条分数的覆盖格式）
- `server/scoreboard/ServerScoreboard` —— 填 `SetObjective` / `SetScore` 包的 `numberFormat` 字段
- `server/scoreboard/storage/ScoreboardSaveData` —— 存档 `format` 字段
- `server/command/commands/ScoreboardCommand` —— `/scoreboard ... numberformat` 命令

**下游（本目录依赖谁）**

- `common/util/text/*`（Style / ITextComponent / 两者的 NBT 编解码）
- `common/util/nbt`、`common/network/buffer/NbtIo`

## 容易踩的坑

- **枚举值就是网络上的注册表 id**（blank=0 / styled=1 / fixed=2），由原版注册顺序决定，不可调整顺序。
- **IR 里 `numberFormat` 字段用空 `std::vector<u8>` 表示 absent**。`blank` 是「存在但无载荷」，线格式至少
  1 字节（`VarInt(0)`）；把 blank 编码成空 vector 会被对端当成「没有格式」。
- 原版 `StyledFormat` / `FixedFormat` 的线格式载荷都是 **NBT 根标签**（`Style.Serializer` 与
  `ComponentSerialization` 都经 `NbtOps` 编码），不是自定义二进制。可折叠的 fixed 组件是
  StringTag（`0x08`），不要假定根一定是 `0x0A`。
- 存档 NBT 里变体字段与 `type` **平铺在同一层**（原版 dispatchMap 语义），不要嵌套成
  `{type:..., style:{...}}`。
- `NumberFormat` 的拷贝是深拷贝：`Fixed` 持有组件所有权，浅拷贝会导致两个格式共享同一组件。