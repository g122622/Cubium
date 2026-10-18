# 物品数据组件

## 目录结构

```text
component/
├── DataComponentType.hpp/cpp         # 组件资源名与注册 ID
├── DataComponentMap.hpp              # 异构载荷与补丁
├── DataComponentPatchWire.hpp/cpp    # 网络补丁及精确谓词
├── DataComponentPatchNbt.hpp/cpp     # 存档补丁
├── DataComponentPayloadCodec.hpp/cpp # 存档载荷转换
├── AttackRange.hpp/cpp               # 攻击范围值类型
├── FireworkExplosion.hpp/cpp         # 单次烟花爆炸值类型（形状/颜色/拖尾/闪烁）
└── Fireworks.hpp                     # 烟花火箭值类型（飞行时长 + 爆炸列表）
```

## 内部模块关系

类型表驱动载荷转换；补丁区分新增和移除。网络编码与存档编码共用载荷定义，分别执行字段编码。

新增一个组件须同时改四处，缺一即静默失效（NBT 往返丢字段 / wire 读入报未知 typeId）：
`DataComponentType` 枚举与 `_entries` 表、`DataComponentMap.hpp` 的 `DataComponentPayload` 备选项与
`ComponentPayloadIndex` 索引、`DataComponentPayloadCodec` 的 NBT 双分支、`DataComponentPatchWire` 的 wire 双分支；
再在 `ItemStack` 的 `ItemExtras`/`toComponentPatch`/`applyComponentPatch`/`toJson`/`fromJson`/`_componentOverridesEqual` 接线。

## 上下游外部依赖关系

ItemStack 导出、应用补丁，IR 物品桥接和存档调用编码器。本目录依赖文本、NBT、附魔与效果值类型，以及 Java 注册序映射。

## 容易踩的坑

- 组件 ID 必须严格等于 Java 1.21.11 `DataComponents.register` 的声明序号（0..103，全表已对齐）。
  新增/重排一个组件会影响其后所有 id（wire VarInt 与 NBT 键），必须整表核对，勿只改单个值。
- 网络补丁先写两个计数，再写新增项和移除项；空补丁仍占两个零字节。
- 组件没有统一长度前缀，未知载荷必须报错，不能跳过后继续解析。
- 商人 ItemCost 的精确谓词只有一个计数，与 ItemStack 补丁不同。
- **vanilla 的 `ByteBufCodecs.INT` 是定长 4 字节大端，不是 VarInt**。烟花的颜色列表
  （`COLOR_LIST_STREAM_CODEC = ByteBufCodecs.INT.apply(list())`）必须用 `writeI32`/`readI32`，
  用 VarInt 会在颜色为小正值时少写 3 字节，把后续字段吞进颜色里连锁错位。
- **NBT 里的 Bool 是 Byte**：`Codec.BOOL` 在 NBT 上落为 `byte_tag`（1b/0b），读侧不能只认
  `int_tag`，否则组件字段静默读成默认值。写侧用 `i8` 以命中 `byte_tag`。
- **容器类组件不进 `DataComponentPayload`**：`container`（ItemContainerContents）与
  `charged_projectiles` 的元素是 `ItemStack`，而 `DataComponentMap.hpp` 先于 `ItemStack` 定义被包含，
  variant 无法持有不完整类型。二者由 `ItemStack` 的独立字段（`shared_ptr<const ...>`）承载，
  定义在 `ItemStack.hpp` 的 ItemStack 之后；拷贝 ItemStack 时共享同一只读载荷，修改走 set* 替换。
- **数据包组件 JSON 解码走 `jsonToComponentPayload`**：对象/数组走 `jsonToNbtStrictInt` 再走
  `nbtToPayload`；标量（bool/数值/字符串）须单独取值后解码——`jsonToNbt` 只接受对象，
  直接传标量会得到空 compound 并被误判为"已解码的空值"。
- **`DataComponentPayload` 的构造必须显式写 `std::in_place_index<N>`**：`ComponentPayloadIndex`
  里的常量是 `usize`，直接 `DataComponentPayload{Index::X, value}` 会被编译器当成 `in_place_type`
  以外的匹配而报 "no matching constructor"。
