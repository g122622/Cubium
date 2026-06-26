#Container 模块

提供GUI容器（Container / Menu）的实现，用于客户端 -
        服务端同步玩家与方块实体的交互。

                所有容器类统一继承 `AbstractContainerMenu` 基类，提供一致的槽位管理、点击处理和同步机制。

            ##目录结构

``` container /
├── AnvilContainer.hpp /
            cpp #铁砧容器（修复 / 重命名 / 附魔合并）
├── BrewingStandContainer.hpp / cpp #酿造台容器（3药水槽 / 材料槽 / 燃料槽）
├── CartographyContainer.hpp / cpp #制图台容器（地图扩展 / 锁定 / 复制）
├── ChestContainer.hpp / cpp #箱子容器（单箱27格 / 双箱54格）
├── EnchantmentContainer.hpp / cpp #附魔台容器（物品槽 / 青金石槽）
├── FurnaceContainer.hpp / cpp #熔炉容器（输入 / 燃料 / 输出槽）
├── HopperContainer.hpp / cpp #漏斗容器（5格）
├── LoomContainer.hpp / cpp #织布机容器（旗帜图案染色）
├── MerchantContainer.hpp / cpp #交易容器（3格：支付1 / 支付2 / 结果）
├── MerchantContainerMenu.hpp / cpp #交易菜单（3交易槽 +
        玩家背包）
├── MerchantResultSlot.hpp /
            cpp #交易结果槽（执行交易扣除物品）
└── README.md
```

            ##内部模块关系

``` AbstractContainerMenu(基类，定义在 inventory / 目录)
        │
        ├── ChestContainer ──关联──
    > ChestEntity
        ├── FurnaceContainer ──关联── > AbstractFurnaceEntity
        ├── EnchantmentContainer ──关联── > EnchantingTableEntity（位置）
        ├── BrewingStandContainer ──关联── > BrewingStandEntity
        ├── AnvilContainer ──关联── > 铁砧方块位置
        ├── HopperContainer ──关联── > HopperEntity
        ├── CartographyContainer ──关联── > 制图台方块位置
        ├── LoomContainer ──关联── > 织布机方块位置
        └── MerchantContainerMenu ──关联── > IMerchant（村民 /
            流浪商人）
                │
                ├── MerchantContainer（3格交易库存）
                └── MerchantResultSlot（交易结果槽）
```

            所有容器都通过 `PlayerInventory` 同步玩家背包状态。

            ##上下游外部依赖关系

            ## #上游依赖（谁依赖了这个目录）

        - `client / ui / screen /` - 客户端 Screen 类（ChestScreen、FurnaceScreen、CartographyScreen、LoomScreen 等）
        - 服务端方块交互逻辑 -
        创建容器实例并绑定到玩家

        ## #下游依赖（这个目录依赖了谁）

        - `entity / inventory / AbstractContainerMenu.hpp` - 容器菜单基类
        - `entity / inventory / IInventory.hpp` - 背包接口 - `entity / inventory / Slot.hpp` - 槽位类
        - `entity / inventory / PlayerInventory.hpp` - 玩家背包
        - `world / blockentity /` - 方块实体（ChestEntity、AbstractFurnaceEntity、BrewingStandEntity、HopperEntity）
        - `world / village / trade / Merchant.hpp` - 商人接口和交易列表（MerchantContainerMenu 关联）
        - `world / village / trade / MerchantOffer.hpp` - 交易报价（MerchantContainerMenu、MerchantResultSlot 关联）
        - `item / enchantment / EnchantmentHelper.hpp` - 附魔工具类 - `item / potion / PotionBrewing.hpp` - 酿造配方管理
        - `world / block / BlockPos.hpp` - 方块位置 - `<memory>` -
        智能指针

        ##容易踩的坑

        ## #1. 双箱槽位映射

        双箱容器需要正确映射槽位到两个箱子：

```cpp
        // 错误：直接访问槽位
        ItemStack item = chestA->getItem(slot);

// 正确：根据槽位选择箱子
if (slot < 27) {
    return chestA->getItem(slot);
} else {
    return chestB->getItem(slot - 27);
}
```

    ## #2. 容器ID管理

        每个容器需要唯一的ID用于网络同步：

```cpp
            // 服务端分配ID
            u32 containerId = nextContainerId++;

// 客户端接收时验证
if (containerId != expectedId) {
    // ID不匹配，忽略
}
```

    ## #3. stillValid 距离检查

        所有容器都必须实现 `stillValid()` 方法，检查玩家是否仍在方块附近（8格范围内）：

    - **方块实体关联的容器 **（如熔炉、酿造台）：通过 `BlockEntity::getPos()` 获取位置并计算距离 -
    **位置关联的容器 **（如附魔台、铁砧、制图台、织布机）：通过存储的位置计算距离 -
    **距离检查规则 **：有效距离 8 格（距离平方 ≤ 64），检查点为方块中心（x + 0.5,
    y + 0.5,
    z +
    0.5）

    ## #4. 铁砧修复成本限制

    修复成本超过40级时操作不可用（显示 "太贵"提示）。创造模式玩家绕过此限制。

附魔兼容性检查使用 `Enchantment::isCompatibleWith()` 方法（对应 MC 原版
`Enchantment.isCompatibleWith`），由 `_updateRepairOutput()` 内联调用。
已移除历史遗留的 `_areEnchantmentsCompatible` 私有方法（从未被调用）。

    ## #5. 附魔台书架力量计算

    书架必须在附魔台周围2格范围内，且中间必须有空气。最大书架力量为15。

    ## #6. 容器关闭处理

    关闭容器时需要正确处理物品返回，确保无法放入玩家背包的物品被丢弃到世界中。

    ## #7. 创造模式特殊权限

    铁砧和附魔台容器有创造模式玩家特殊权限：
    - 铁砧：无视经验等级要求、绕过40级费用上限、可给任何物品应用任何附魔、铁砧永不损坏 -
    附魔台：不消耗经验（但仍消耗青金石）

    ## #8. 熔炉输出槽经验发放

`FurnaceResultSlot` 在取出物品时自动发放熔炼累积的经验。经验值向下取整后发放。
