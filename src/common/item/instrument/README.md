# Item Instrument

乐器注册表与乐器标签，供山羊角等乐器物品按 `instrument` 数据组件解析音色。

## 目录结构

```
instrument/
├── InstrumentDefinition.hpp/cpp # 乐器定义（音效、吹奏时长、可听距离、显示名）
├── InstrumentRegistry.hpp/cpp   # 乐器注册表单例（内置八种山羊角 + 数据包覆盖）
├── InstrumentTag.hpp/cpp        # 单个乐器标签（有序资源名集合）
├── InstrumentTags.hpp/cpp       # 乐器标签注册表 + 内置标签占位初始化
└── README.md                    # 本文件
```

## 内部模块关系

`InstrumentRegistry` 持有 `InstrumentDefinition`；`InstrumentTag` 只存乐器资源名（不持有定义），
`InstrumentTags` 是标签的登记表。成员以资源名有序存储，使 `set_instrument` 之类随机抽取
只取决于数据包内容与随机种子，与文件遍历顺序无关。

## 上下游外部依赖关系

- **上游**：`ResourceLocation`、`SoundEvents`、文本组件（`TranslationTextComponent`）。
- **下游**：`GoatHornItem`（按 `ItemStack::getInstrument()` 解析并吹奏）、战利品表
  `set_instrument`（按标签随机抽取）。数据包定义与标签由服务端 `server/item/instrument/`
  的加载器写入注册表，`RegistryBootstrap` 负责启动期装配顺序。

## 容易踩的坑

- **装配顺序**：先 `InstrumentRegistry::initialize()`（内置定义）→ `initializeVanillaInstrumentTags()`
  （内置标签占位）→ 数据包定义 → 数据包标签。战利品表在其后加载，`set_instrument` 的
  `"options"` 标签必须已登记，否则解析报 unknown instrument tag。
- **标签成员只来自数据包**：内置只登记空占位，不硬编码成员列表，避免与数据包内容分叉。
- **无组件的山羊角不可吹奏**：创造模式物品栏取出的裸山羊角没有 `instrument` 组件，
  `getUseDuration` 返回 0、右键失败；需要的是经战利品表/`GoatHornItem::createStack` 产出的物品。
- **命名空间限定**：在 `mc::server::*` 内写 `item::instrument::X` 会先命中 `mc::server::item`，
  必须写全 `mc::item::instrument::X`。