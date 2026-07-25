# Item 核心模块

本目录包含物品系统的核心类型和接口。

## 目录结构

```
core/
├── AdventureModePredicate.hpp/cpp  # 冒险模式谓词（CanPlaceOn/CanDestroy方块匹配，支持属性过滤）
├── Item.hpp/cpp              # 物品基类，所有物品类型的父类（含 onCraftedBy/onCraftedPostProcess 合成回调）
├── ItemStack.hpp/cpp         # 物品堆，表示游戏中的一个物品实例（包含物品类型、数量、耐久、附魔、冒险模式谓词和结构化自定义标签，含 onCraftedBy 桥接方法）
├── ItemRegistry.hpp/cpp      # 物品注册表，管理所有物品的注册和查找
├── ItemGroup.hpp/cpp         # 创造模式物品组（标签页）
├── ProjectileItem.hpp/cpp    # 弹射物物品接口，提供 asProjectile()/getDispenseConfig()/shoot() 三个核心方法
├── UseAction.hpp             # 物品使用动作枚举（EAT、DRINK、BLOCK等）
├── ActionResult.hpp          # 物品使用结果枚举（ActionResultType）与模板类 ActionResult<T>（别名 ItemActionResult）
├── BlockActionResult.hpp     # 方块交互结果类，封装 ActionResultType + std::optional<ItemStack>，支持 heldItemTransformedTo 语义
└── README.md
```

## 内部模块关系

```
Item (抽象基类)
  │
  ├── ItemProperties (构建器模式配置Item属性)
  │
  ├── ItemStack (物品实例，持有Item引用)
  │     ├── 附魔数据 (EnchantmentContainer)
  │     ├── 冒险模式谓词 (AdventureModePredicate × 2: CanPlaceOn + CanDestroy)
  │     └── 自定义标签 (结构化NBT)
  │
  ├── AdventureModePredicate (冒险模式方块匹配谓词，支持精确ID、#标签引用和属性过滤)
  │
  ├── ItemRegistry (单例，管理Item注册)
  │
  ├── ItemGroup (创造模式物品分组)
  │
  └── ProjectileItem (弹射物物品接口)
        ├── ProjectileDispenseConfig (弹射物发射配置，提供 defaults()/potion()/arrow()/fireCharge()/windCharge()/fireworkRocket() 六种预设)
        ├── asProjectile() → 创建弹射物实体
        ├── getDispenseConfig() → 获取发射器行为参数
        └── shoot() → 发射弹射物（默认委托 ProjectileEntity::shoot()，风弹/火焰弹覆写为空操作）
```

## 上下游外部依赖关系

**本目录依赖：**
- `common/core/Types.hpp` - 基础类型定义
- `common/resource/ResourceLocation.hpp` - 资源位置标识
- `common/network/codec/PacketSerializer.hpp` - ItemStack 网络序列化（PacketSerializer/Deserializer 已迁至 codec/，ItemStack 仍保留 `serialize(PacketSerializer&)`/`deserialize(PacketDeserializer&)` 方法用于 wire 编码）
- `item/enchantment/EnchantmentContainer.hpp` - 附魔容器
- `common/util/math/Vector3.hpp` - 向量类型（ProjectileItem::asProjectile 参数）
- `entity/projectile/ProjectileEntity.hpp` - 弹射物实体（ProjectileItem::shoot 参数）

**依赖本目录的模块：**
- `item/items/` - 所有具体物品实现（FoodItem、ArmorItem、ToolItem等）
- `item/items/weapon/` - ThrowableItem 实现 ProjectileItem 接口
- `item/items/potion/` - ThrowablePotionItem 覆写 getDispenseConfig() 返回 potion() 配置
- `item/items/trial/` - WindChargeItem 实现 ProjectileItem 接口
- `item/food/` - 食物属性系统
- `item/armor/` - 盔甲材质系统
- `item/crafting/` - 合成系统（Ingredient匹配）
- `entity/` - 实体背包、物品掉落；OminousItemSpawnerEntity 通过 ProjectileItem 接口多态创建弹射物
- `world/` - 方块掉落、物品实体
- `network/` - 物品数据包同步
- `client/inventory/` - 客户端背包UI

## 容易踩的坑

### 1. Item类是抽象基类

不应直接实例化Item类，应通过ItemRegistry注册具体的物品子类。

### 2. ItemStack支持结构化自定义标签

ItemStack现在支持结构化自定义标签（如染色的`display.color`），JSON序列化会通过`Tag`字段保存这些数据。比较两个ItemStack是否可堆叠时，需要确保自定义数据一致。

### 3. ItemRegistry是单例

在游戏初始化时注册所有物品，注册顺序需注意依赖关系（如BlockItem需要Block先注册）。

### 4. 附魔物品堆叠逻辑

MC 1.16.5中，附魔物品堆叠是基于NBT标签完全相等判断的。如果两个物品有相同的附魔，它们可以堆叠。`canMergeWith()`方法会比较：物品类型、耐久度、修复成本、自定义名称、Lore描述、药水ID、附魔、自定义数据。

### 5. 属性修饰符与槽位

部分物品（如盔甲）的属性修饰符与装备槽位相关，调用`getAttributeModifiers(i32 slot)`时需要传入正确的槽位索引。

### 6. Item::inventoryTick与ItemStack::inventoryTick

`ItemStack::inventoryTick()`委托给`Item::inventoryTick()`，若需要实现物品在背包中的tick逻辑（如地图、时钟），应重写Item的虚方法而非ItemStack。

### 7. 合成回调 onCraftedBy/onCraftedPostProcess

物品合成后需要执行特殊后处理时，重写 `Item::onCraftedPostProcess(ItemStack&, IWorld&)`。`Item::onCraftedBy(ItemStack&, IWorld&, Player&)` 由 `ItemStack::onCraftedBy()` 调用，默认转发给 `onCraftedPostProcess`。调用链：`ServerPlayer::onItemCrafted()` → `ItemStack::onCraftedBy()` → `Item::onCraftedBy()` → `Item::onCraftedPostProcess()`。

对于地图物品，`FilledMapItem` 重写了 `onCraftedPostProcess` 处理 `map_scale_direction`（缩放）和 `map_lock`（锁定）NBT 标签，处理完毕后移除标签。

### 8. 冒险模式谓词 CanPlaceOn/CanDestroy

`AdventureModePredicate` 存储在 `ItemStack` 的 `m_canPlaceOn`/`m_canDestroy` 成员中，NBT 键名为 `CanPlaceOn`/`CanDestroy`（字符串列表）。冒险模式下：
- `Player::mayInteract()` 检查手持物品的 CanPlaceOn 标签，匹配目标方块时允许交互
- `BlockInteractionManager::handleBlockPlacement()` 检查 CanPlaceOn 标签
- `BlockInteractionManager::_canBreakBlock()` 检查 CanDestroy 标签
- 谓词条目支持四种格式：
  - 精确方块ID：`minecraft:stone`
  - 标签引用：`#minecraft:logs`
  - 带属性匹配的方块ID/标签：`minecraft:oak_log[axis=y]`、`#minecraft:logs[axis=y]`、`minecraft:oak_stairs[half=top,facing=east]`
  - 带NBT匹配的方块ID/标签：`minecraft:chest{Items:[...]}`、`minecraft:chest[waterlogged=false]{Items:[...]}`
- 属性匹配使用 AND 逻辑（同一条目内所有属性必须全部满足）
- 多条目之间使用 OR 逻辑（任一条目匹配即可）
- 属性值通过方块的 `StateContainer` 查找 `IProperty` 并调用 `parseValue()` 解析比较
- NBT 匹配语法（`minecraft:chest{Items:[...]}`）已支持，通过 NBTPredicate 进行子集匹配，需要使用带 BlockPos 参数的 AdventureModePredicate::test() 重载
- 空谓词列表不匹配任何方块，冒险模式下无 CanPlaceOn/CanDestroy 标签的物品不能放置/破坏方块

### 9. ProjectileItem 接口与多态创建弹射物

`ProjectileItem` 是一个纯虚接口（非继承自 Item），供需要创建弹射物实体的物品实现。当前实现类：
- **ThrowableItem**：投掷物品基类，实现 `asProjectile()` 调用子类的 `createProjectileEntity()` 创建弹射物实体，默认 `getDispenseConfig()` 返回 `defaults()`，默认 `shoot()` 委托给 `ProjectileEntity::shoot()`
- **WindChargeItem**：风弹物品，直接继承 Item 并实现 ProjectileItem，覆写 `shoot()` 为空操作（因为 `asProjectile()` 中已预设初速度），`getDispenseConfig()` 返回 `windCharge()` 配置

使用方式：通过 `dynamic_cast<const ProjectileItem*>(item)` 检测物品是否可创建弹射物，无需硬编码物品到弹射物的映射表。

### 10. ProjectileDispenseConfig 预设配置

三种预设配置对应不同弹射物的发射器行为：
- `defaults()`：power=1.1, uncertainty=6.0（雪球、鸡蛋、末影珍珠等普通投掷物）
- `potion()`：power=1.375, uncertainty=3.0（药水、附魔之瓶，散布减半力度增加25%）
- `windCharge()`：power=1.0, uncertainty=6.6666665（风弹）

ThrowablePotionItem 和 ExperienceBottleItem 覆写 `getDispenseConfig()` 返回 `potion()` 配置。WindChargeItem 覆写返回 `windCharge()` 配置。

### 11. BlockActionResult 与 heldItemTransformedTo 语义

`BlockActionResult` 是方块交互结果的专用类型，封装 `ActionResultType + std::optional<ItemStack>`，参考 MC 1.21.11 `InteractionResult.Success.heldItemTransformedTo(ItemStack)`。

**设计动机**：
- `ActionResultType` 仅表示结果类型（Success/Consume/Fail/Pass），无法携带"转换后的手持物品"
- MC 1.21.11 的 `InteractionResult.Success` 通过 `heldItemTransformedTo(ItemStack)` 工厂方法携带转换后的物品
- 消费方（如 `BlockInteractionManager`）据此决定是否同步物品栏到客户端

**核心 API**：
- 隐式转换构造：`BlockActionResult(ActionResultType)` —— 向后兼容已有 52 个 `Block::onBlockActivated` override 直接返回 `ActionResultType`
- 工厂方法：`success() / success(ItemStack) / consume() / consume(ItemStack) / fail() / pass()`
- 链式方法：`heldItemTransformedTo(ItemStack)` —— 返回携带物品的新结果
- 访问器：`getType() / heldItemTransformedTo() / hasHeldItemTransformed() / isSuccess() / isConsume() / ...`
- 比较运算符：`operator==/!=` 与 `ActionResultType` 比较（仅比较 type，便于测试 `EXPECT_EQ(result, ActionResultType::Success)`）

**使用场景**：
- 方块交互修改了玩家手持物品时（如 `ShelfBlock` 放入/取出/交换物品），应通过 `BlockActionResult::success(ItemStack)` 或 `success().heldItemTransformedTo(stack)` 携带转换后的物品
- 方块交互未修改手持物品时，返回 `ActionResultType` 即可（通过隐式转换构造包装为 `BlockActionResult`，`heldItemTransformedTo` 为 `std::nullopt`）

**与 ActionResult<T> 的区别**：
- `ActionResult<T>` 是通用模板（别名 `ItemActionResult = ActionResult<ItemStack>`），用于 `Item::use()` 等物品使用场景
- `BlockActionResult` 是方块交互专用类型，`heldItemTransformedTo` 使用 `std::optional<ItemStack>`（区分"未转换"与"转换为空"）
