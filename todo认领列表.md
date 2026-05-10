# TODO 认领列表

本文件记录正在进行的 TODO 收敛工作，避免多人重复认领同一个 TODO。

---

## TODO #1: SetBlockCommand 支持方块状态属性

- **文件**: `src/server/command/commands/SetBlockCommand.cpp`
- **行号**: 28
- **内容**: TODO: 支持方块状态属性 [facing=north,half=bottom]
- **认领时间**: 2026-05-10
- **完成时间**: 2026-05-10
- **状态**: 已完成
- **描述**: 实现了完整的方块状态属性解析功能，包括：
  - 创建 `BlockStateArgument` 参数类型
  - 解析 `minecraft:stone[facing=north,half=bottom]` 格式
  - 支持带命名空间和简写的方块ID
  - 完整的错误处理（未知方块、未知属性、无效属性值、重复属性）
  - 更新 `SetBlockCommand` 和 `FillCommand` 使用新的解析器

---

## TODO #2: EnchantmentContainer 创建附魔书物品

- **文件**: `src/common/entity/inventory/container/EnchantmentContainer.cpp`
- **行号**: 273
- **内容**: TODO: 创建附魔书物品
- **认领时间**: 2026-05-10
- **完成时间**: 2026-05-10
- **状态**: 已完成
- **描述**: 实现了书 -> 附魔书转换功能，包括：
  - 检测物品是否为 `Items::BOOK`
  - 创建 `ENCHANTED_BOOK` 物品堆
  - 复制原有 NBT 标签和自定义名称
  - 替换物品槽中的书为附魔书
  - 对附魔书使用 `EnchantedBookItem::addEnchantment()` 存储附魔（StoredEnchantments）
  - 对普通物品继续使用 `ItemStack::addEnchantment()` 存储附魔（Enchantments）

---
