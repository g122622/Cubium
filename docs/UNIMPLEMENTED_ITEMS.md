# 未实现物品清单（ItemTagLoader 警告来源）


> 本文档由脚本 `scripts/build_unimplemented_items_doc.py` 自动生成，列出 Cubium 当前注册物品集合与
> 原版数据包（`minecraft_reborn/datapacks/Vanilla`）物品标签（item tags）所引用物品之间的差集。
> 这些物品在标签加载阶段触发 `ItemTagLoader: unknown item 'X' (required), skipping (tag: Y)` 警告，
> 属于「数据包引用了尚未实现的物品」，并非注册顺序 bug（物品注册早于标签加载，见 
`src/server/application/MinecraftServer.cpp` 的 `initializeRegistries()`）。


## 摘要


- 数据包物品标签文件数：**198**
- 标签引用的去重叶子物品数：**882**
- Cubium 已注册物品数（源码扫描）：**1393**
- 标签引用但未实现的物品数：**0**
- 完全无法解析（标签内全部叶子物品均未实现）的标签数：**0**
- 部分缺失（至少一个叶子物品未实现）的标签数：**0**

## 注册物品来源


| 来源文件 | 注册方式 | 说明 |
| --- | --- | --- |
| `src/common/item/Items.cpp` | `registerItem(ResourceLocation("minecraft:X"), ...)` | 直接注册物品（含工具/食物/材料等） |
| `src/common/item/Items.cpp` | `registerBlockBackedItem(registry, VanillaBlocks::X, "name", ...)` | 以方块为底注册 BlockItem，物品 id 为 `minecraft:name` |
| `src/common/item/items/block/BlockItemRegistry.cpp` | `registerSimpleBlock(VanillaBlocks::X, "name")` | 注册简单方块物品，物品 id 取自方块的 `blockLocation()`（与 `name` 一致） |
| `src/common/item/items/block/BlockItemRegistry.cpp` | `registerItem<...>(ResourceLocation("minecraft:X"), ...)` | 显式注册特殊 BlockItem |
| `src/common/item/items/block/BlockItemRegistry.cpp` | `registerAnvilBlock(VanillaBlocks::X, "name")` | 铁砧专用本地 lambda，物品 id 取自 `blockLocation()`（与 `name` 一致），仅 maxStackSize 不同（=1） |

## 完全无法解析的标签（所有叶子物品均未实现）


这些标签会输出 `ItemTagLoader: tag 'X' resolved no valid items` 信息级日志，且其中每一项都触发一条警告。


| 标签 | 未实现的叶子物品数 |
| --- | --- |

## 部分缺失的标签（至少一个叶子物品未实现）


| 标签 | 未实现叶子物品数 |
| --- | --- |

## 未实现物品完整清单（按分类）


共 **0** 项。物品 id 形如 `minecraft:anvil`。


## 附录：完全无法解析标签的逐项明细


下表列出每个「完全无法解析」标签内的全部未实现物品，便于逐个实现时核对。


## 附录：部分缺失标签的逐项明细

