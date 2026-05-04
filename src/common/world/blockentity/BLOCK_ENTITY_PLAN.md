# 方块实体补全计划

## 概述

本文档为minecraft-reborn项目设计完整的方块实体补全方案，基于对MC Java 1.16.5源码的研究，遵循项目现有的C++20架构风格。

---

## 一、目录结构设计

```
src/common/world/blockentity/
├── core/                           # 核心基础设施
│   ├── BlockEntity.hpp             # 方块实体基类 (已存在)
│   ├── BlockEntity.cpp
│   ├── BlockEntityType.hpp         # 类型枚举 (已存在)
│   ├── BlockEntityType.cpp
│   ├── ContainerBlockEntity.hpp    # 容器基类 (已存在)
│   ├── BlockEntityRegistry.hpp     # 方块实体注册表 (新增)
│   ├── BlockEntityRegistry.cpp
│   ├── LockableBlockEntity.hpp     # 可锁定容器基类 (新增)
│   ├── LockableBlockEntity.cpp
│   └── README.md
│
├── storage/                        # 存储类方块实体
│   ├── ChestEntity.hpp             # 箱子
│   ├── ChestEntity.cpp
│   ├── TrappedChestEntity.hpp      # 陷阱箱
│   ├── TrappedChestEntity.cpp
│   ├── EnderChestEntity.hpp        # 末影箱
│   ├── EnderChestEntity.cpp
│   ├── DoubleSidedInventory.hpp    # 双箱合并容器
│   ├── DoubleSidedInventory.cpp
│   └── README.md
│
├── transport/                      # 传输类方块实体
│   ├── HopperEntity.hpp            # 漏斗
│   ├── HopperEntity.cpp
│   ├── HopperBlock.hpp             # 漏斗方块
│   ├── HopperBlock.cpp
│   ├── IHopper.hpp                 # 漏斗接口
│   └── README.md
│
├── processing/                     # 加工类方块实体
│   ├── AbstractFurnaceEntity.hpp   # 熔炉基类
│   ├── AbstractFurnaceEntity.cpp
│   ├── AbstractFurnaceBlock.hpp    # 熔炉方块基类
│   ├── AbstractFurnaceBlock.cpp
│   ├── FurnaceEntity.hpp           # 普通熔炉
│   ├── FurnaceEntity.cpp
│   ├── FurnaceBlock.hpp
│   ├── FurnaceBlock.cpp
│   ├── BlastFurnaceEntity.hpp      # 高炉
│   ├── BlastFurnaceEntity.cpp
│   ├── BlastFurnaceBlock.hpp
│   ├── BlastFurnaceBlock.cpp
│   ├── SmokerEntity.hpp            # 烟熏炉
│   ├── SmokerEntity.cpp
│   ├── SmokerBlock.hpp
│   ├── SmokerBlock.cpp
│   ├── FurnaceInventory.hpp        # 熔炉背包
│   ├── FurnaceInventory.cpp
│   └── README.md
│
├── interactive/                    # 交互类方块实体
│   ├── EnchantingTableEntity.hpp   # 附魔台
│   ├── EnchantingTableEntity.cpp
│   ├── EnchantingTableBlock.hpp
│   ├── EnchantingTableBlock.cpp
│   └── README.md
│
├── doors/                          # 门类方块实体
│   ├── DoorBlock.hpp               # 门方块
│   ├── DoorBlock.cpp
│   ├── FenceGateBlock.hpp          # 栅栏门方块
│   ├── FenceGateBlock.cpp
│   └── README.md
│
├── cauldron/                       # 炼药锅系统
│   ├── CauldronBlock.hpp           # 炼药锅方块
│   ├── CauldronBlock.cpp
│   ├── CauldronFluidLevel.hpp      # 水位枚举
│   └── README.md
│
├── container/                      # 容器GUI (与blockentity配合)
│   ├── ChestContainer.hpp          # 箱子容器
│   ├── ChestContainer.cpp
│   ├── FurnaceContainer.hpp        # 熔炉容器
│   ├── FurnaceContainer.cpp
│   ├── HopperContainer.hpp         # 漏斗容器
│   ├── HopperContainer.cpp
│   ├── EnchantmentContainer.hpp    # 附魔台容器
│   ├── EnchantmentContainer.cpp
│   └── README.md
│
└── CraftingTableEntity.hpp         # 工作台 (已存在)
└── CraftingTableEntity.cpp

src/common/world/block/blocks/       # 方块实现 (扩展现有目录)
├── ChestBlock.hpp
├── ChestBlock.cpp
├── TrappedChestBlock.hpp
├── TrappedChestBlock.cpp
├── EnderChestBlock.hpp
├── EnderChestBlock.cpp
├── DoorBlock.hpp
├── DoorBlock.cpp
├── FenceGateBlock.hpp
├── FenceGateBlock.cpp
├── CauldronBlock.hpp
├── CauldronBlock.cpp
└── README.md
```

---

## 二、类继承层次设计

### 2.1 方块实体继承层次

```
BlockEntity (基类)
│
├── ContainerBlockEntity (容器基类) [已存在]
│   │
│   ├── LockableBlockEntity (可锁定容器基类) [新增]
│   │   │
│   │   ├── ChestEntity (箱子)
│   │   ├── TrappedChestEntity (陷阱箱)
│   │   ├── HopperEntity (漏斗)
│   │   └── ... 其他可锁定容器
│   │
│   ├── AbstractFurnaceEntity (熔炉基类) [新增]
│   │   │
│   │   ├── FurnaceEntity (普通熔炉)
│   │   ├── BlastFurnaceEntity (高炉)
│   │   └── SmokerEntity (烟熏炉)
│   │
│   └── CraftingTableEntity (工作台) [已存在]
│
├── EnchantingTableEntity (附魔台) - 无容器
│
└── 无实体方块 (Door, FenceGate, Cauldron) - 仅方块状态
```

### 2.2 方块继承层次

```
Block (基类) [已存在]
│
├── SimpleBlock (简单方块) [已存在]
│
├── AbstractFurnaceBlock (熔炉方块基类) [新增]
│   ├── FurnaceBlock (普通熔炉)
│   ├── BlastFurnaceBlock (高炉)
│   └── SmokerBlock (烟熏炉)
│
├── ChestBlock (箱子方块) [新增]
│   └── TrappedChestBlock (陷阱箱)
│
├── HopperBlock (漏斗方块) [新增]
│
├── DoorBlock (门方块) [新增]
│
├── FenceGateBlock (栅栏门方块) [新增]
│
├── CauldronBlock (炼药锅方块) [新增]
│
└── EnchantingTableBlock (附魔台方块) [新增]
```

### 2.3 容器继承层次

```
Container (基类) [已存在]
│
├── PlayerContainer (玩家背包) [已存在]
│
├── ChestContainer (箱子容器) [新增]
│
├── FurnaceContainer (熔炉容器) [新增]
│   ├── BlastFurnaceContainer (高炉容器)
│   └── SmokerContainer (烟熏炉容器)
│
├── HopperContainer (漏斗容器) [新增]
│
└── EnchantmentContainer (附魔台容器) [新增]
```

---

## 三、核心接口定义

### 3.1 LockableBlockEntity - 可锁定容器基类

```cpp
// src/common/world/blockentity/core/LockableBlockEntity.hpp
#pragma once

#include "world/blockentity/ContainerBlockEntity.hpp"
#include <string>

namespace mc {

/**
 * @brief 可锁定容器方块实体基类
 * 
 * 提供锁定和自定义名称功能，适用于箱子、漏斗、熔炉等。
 * 参考: net.minecraft.tileentity.LockableTileEntity
 */
class LockableBlockEntity : public ContainerBlockEntity {
public:
    /**
     * @brief 检查容器是否被锁定
     * @return 如果容器被锁定返回true
     */
    [[nodiscard]] bool isLocked() const { return m_locked; }
    
    /**
     * @brief 设置锁定状态
     * @param locked 锁定状态
     */
    void setLocked(bool locked) { 
        if (m_locked != locked) {
            m_locked = locked; 
            setChanged(); 
        }
    }
    
    /**
     * @brief 获取锁定钥匙名称
     * @return 钥匙名称（物品显示名）
     */
    [[nodiscard]] const String& getLockKey() const { return m_lockKey; }
    
    /**
     * @brief 设置锁定钥匙名称
     * @param key 钥匙名称
     */
    void setLockKey(const String& key) { 
        m_lockKey = key; 
        setChanged(); 
    }
    
    /**
     * @brief 检查玩家是否可以打开容器
     * @param playerName 玩家名称
     * @param heldItem 手持物品（可能是钥匙）
     * @return 如果可以打开返回true
     */
    [[nodiscard]] bool canOpen(const String& playerName, 
                                const ItemStack& heldItem) const;
    
    // 重写自定义名称方法
    [[nodiscard]] String getCustomName() const override { return m_customName; }
    void setCustomName(const String& name) override { 
        m_customName = name; 
        setChanged(); 
    }
    
    /**
     * @brief 获取显示名称
     * @return 如果有自定义名称返回自定义名称，否则返回默认名称
     */
    [[nodiscard]] virtual String getDisplayName() const;
    
    // 序列化
    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

protected:
    LockableBlockEntity(BlockEntityType type, const BlockPos& pos)
        : ContainerBlockEntity(type, pos) {}
    
    /**
     * @brief 获取默认显示名称（子类重写）
     * @return 默认名称（如"箱子"、"熔炉"等）
     */
    [[nodiscard]] virtual String getDefaultName() const = 0;
    
private:
    bool m_locked = false;
    String m_lockKey;
    String m_customName;
};

} // namespace mc
```

### 3.2 AbstractFurnaceEntity - 熔炉基类

```cpp
// src/common/world/blockentity/processing/AbstractFurnaceEntity.hpp
#pragma once

#include "world/blockentity/core/LockableBlockEntity.hpp"
#include "world/blockentity/processing/FurnaceInventory.hpp"
#include <functional>

namespace mc {

// 前向声明
class AbstractFurnaceBlock;
class IRecipe;
namespace crafting { class SmeltingRecipe; }

/**
 * @brief 熔炉方块实体基类
 * 
 * 提供熔炼逻辑的通用实现，支持：
 * - 燃料燃烧
 * - 物品熔炼
 * - 配方匹配
 * 
 * 参考: net.minecraft.tileentity.AbstractFurnaceTileEntity
 */
class AbstractFurnaceEntity : public LockableBlockEntity {
public:
    // ========== 常量 ==========
    
    /// 默认熔炼时间 (ticks)
    static constexpr i32 DEFAULT_SMELT_TIME = 200;
    
    /// 默认燃料消耗间隔 (ticks)
    static constexpr i32 DEFAULT_BURN_TIME = 1600; // 煤炭
    
    // ========== 构造 ==========
    
    explicit AbstractFurnaceEntity(BlockEntityType type, const BlockPos& pos);
    
    // ========== IInventory 实现 ==========
    
    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return 3; }
    
    // ========== 熔炉特有接口 ==========
    
    /**
     * @brief 获取输入槽物品
     */
    [[nodiscard]] ItemStack getInputItem() const { return m_inventory.getItem(0); }
    
    /**
     * @brief 获取燃料槽物品
     */
    [[nodiscard]] ItemStack getFuelItem() const { return m_inventory.getItem(1); }
    
    /**
     * @brief 获取输出槽物品
     */
    [[nodiscard]] ItemStack getOutputItem() const { return m_inventory.getItem(2); }
    
    /**
     * @brief 获取当前燃烧时间
     * @return 剩余燃烧时间 (ticks)
     */
    [[nodiscard]] i32 getBurnTime() const { return m_burnTime; }
    
    /**
     * @brief 获取总燃烧时间
     * @return 当前燃料的总燃烧时间
     */
    [[nodiscard]] i32 getBurnTimeTotal() const { return m_burnTimeTotal; }
    
    /**
     * @brief 获取当前熔炼进度
     * @return 已熔炼时间 (ticks)
     */
    [[nodiscard]] i32 getCookProgress() const { return m_cookProgress; }
    
    /**
     * @brief 获取总熔炼时间
     * @return 完成熔炼需要的总时间
     */
    [[nodiscard]] i32 getCookTimeTotal() const { return m_cookTimeTotal; }
    
    /**
     * @brief 检查是否正在燃烧
     */
    [[nodiscard]] bool isBurning() const { return m_burnTime > 0; }
    
    /**
     * @brief 检查是否可以熔炼
     */
    [[nodiscard]] bool canSmelt() const;
    
    // ========== Tick 更新 ==========
    
    /**
     * @brief 每tick更新
     */
    void tick(World& world) override;
    
    /**
     * @brief 是否需要tick
     */
    [[nodiscard]] bool needsTick() const override { return true; }
    
    // ========== 序列化 ==========
    
    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    // ========== 模板方法（子类重写）==========
    
    /**
     * @brief 获取熔炼时间
     * @return 熔炼所需时间 (ticks)
     */
    [[nodiscard]] virtual i32 getSmeltTime() const { return DEFAULT_SMELT_TIME; }
    
    /**
     * @brief 检查配方是否适用于此熔炉
     * @param recipe 配方指针
     * @return 如果适用返回true
     */
    [[nodiscard]] virtual bool isRecipeValid(
        const crafting::SmeltingRecipe* recipe) const = 0;
    
    /**
     * @brief 获取经验倍率
     * @return 经验值倍率（默认1.0）
     */
    [[nodiscard]] virtual f32 getExperienceMultiplier() const { return 1.0f; }
    
    /**
     * @brief 获取熔炉方块类型
     */
    [[nodiscard]] virtual const AbstractFurnaceBlock* getFurnaceBlock() const = 0;

private:
    // ========== 内部方法 ==========
    
    /**
     * @brief 处理燃烧逻辑
     */
    void tickBurn();
    
    /**
     * @brief 处理熔炼逻辑
     * @param world 世界引用
     */
    void tickSmelt(World& world);
    
    /**
     * @brief 消耗燃料
     * @return 燃烧时间，如果无法燃烧返回0
     */
    i32 consumeFuel();
    
    /**
     * @brief 获取当前匹配的配方
     * @return 配方指针，没有匹配返回nullptr
     */
    [[nodiscard]] const crafting::SmeltingRecipe* getCurrentRecipe() const;
    
    // ========== 成员变量 ==========
    
    FurnaceInventory m_inventory;
    
    i32 m_burnTime = 0;         ///< 当前剩余燃烧时间
    i32 m_burnTimeTotal = 0;    ///< 当前燃料总燃烧时间
    i32 m_cookProgress = 0;     ///< 当前熔炼进度
    i32 m_cookTimeTotal = 0;    ///< 当前配方总熔炼时间
    
    const crafting::SmeltingRecipe* m_currentRecipe = nullptr;
};

} // namespace mc
```

### 3.3 ChestEntity - 箱子实体

```cpp
// src/common/world/blockentity/storage/ChestEntity.hpp
#pragma once

#include "world/blockentity/core/LockableBlockEntity.hpp"
#include <array>

namespace mc {

class ChestBlock;
class DoubleSidedInventory;

/**
 * @brief 箱子方块实体
 * 
 * 存储27格物品，支持：
 * - 单箱/双箱模式
 * - 打开计数和动画
 * - 红石比较器信号
 * 
 * 参考: net.minecraft.tileentity.ChestTileEntity
 */
class ChestEntity : public LockableBlockEntity {
public:
    /// 箱子容量
    static constexpr i32 CHEST_SIZE = 27;
    
    // ========== 构造 ==========
    
    explicit ChestEntity(const BlockPos& pos);
    explicit ChestEntity(BlockEntityType type, const BlockPos& pos);
    ~ChestEntity();
    
    // ========== IInventory 实现 ==========
    
    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return CHEST_SIZE; }
    
    // ========== 箱子特有接口 ==========
    
    /**
     * @brief 检查是否是双箱的一部分
     * @return 如果连接到另一个箱子返回true
     */
    [[nodiscard]] bool isDoubleChest() const;
    
    /**
     * @brief 获取相邻箱子（如果是双箱）
     * @return 相邻箱子实体指针，如果不是双箱返回nullptr
     */
    [[nodiscard]] ChestEntity* getConnectedChest(World& world) const;
    
    /**
     * @brief 获取合并后的双箱背包
     * @param world 世界引用
     * @return 双箱背包，如果是单箱返回nullptr
     */
    [[nodiscard]] std::unique_ptr<DoubleSidedInventory> getDoubleInventory(World& world);
    
    /**
     * @brief 获取打开计数
     * @return 当前打开的玩家数量
     */
    [[nodiscard]] i32 getOpenCount() const { return m_openCount; }
    
    /**
     * @brief 玩家打开箱子
     */
    void openContainer() override;
    
    /**
     * @brief 玩家关闭箱子
     */
    void closeContainer() override;
    
    /**
     * @brief 计算红石比较器信号
     * @param world 世界引用
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] i32 getComparatorSignal(World& world) const;
    
    // ========== 动画支持 ==========
    
    /**
     * @brief 获取盖子打开角度
     * @return 角度 (0.0 = 关闭, 1.0 = 完全打开)
     */
    [[nodiscard]] f32 getLidAngle() const { return m_lidAngle; }
    
    /**
     * @brief 更新盖子动画
     * @param partialTick 部分tick时间
     */
    void updateLidAnimation(f32 partialTick);
    
    // ========== Tick ==========
    
    void tick(World& world) override;
    [[nodiscard]] bool needsTick() const override { return true; }
    
    // ========== 序列化 ==========
    
    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    [[nodiscard]] String getDefaultName() const override { return "container.chest"; }
    
    /**
     * @brief 广播打开/关闭事件
     * @param open true=打开, false=关闭
     * @param world 世界引用
     */
    void broadcastChestState(World& world, bool open);

private:
    /// 简单背包实现
    class ChestInventory : public IInventory {
    public:
        explicit ChestInventory(i32 size);
        ~ChestInventory() override = default;
        
        [[nodiscard]] i32 getContainerSize() const override { return m_size; }
        [[nodiscard]] bool isEmpty() const override;
        [[nodiscard]] ItemStack getItem(i32 slot) const override;
        void setItem(i32 slot, const ItemStack& stack) override;
        ItemStack removeItem(i32 slot, i32 count) override;
        ItemStack removeItemNoUpdate(i32 slot) override;
        void clear() override;
        void setChanged() override;
        void serialize(network::PacketSerializer& ser) const override;
        
    private:
        std::vector<ItemStack> m_items;
        i32 m_size;
    };
    
    ChestInventory m_inventory;
    f32 m_lidAngle = 0.0f;
    f32 m_prevLidAngle = 0.0f;
};

/**
 * @brief 陷阱箱方块实体
 * 
 * 与普通箱子类似，但会输出红石信号。
 */
class TrappedChestEntity : public ChestEntity {
public:
    explicit TrappedChestEntity(const BlockPos& pos);
    
    /**
     * @brief 获取红石信号强度
     * @param world 世界引用
     * @return 信号强度 (打开玩家数 * 15 / 27)
     */
    [[nodiscard]] i32 getRedstoneSignal(World& world) const;
    
    void openContainer() override;
    void closeContainer() override;

protected:
    [[nodiscard]] String getDefaultName() const override { return "container.chestTrapped"; }
};

} // namespace mc
```

### 3.4 HopperEntity - 漏斗实体

```cpp
// src/common/world/blockentity/transport/HopperEntity.hpp
#pragma once

#include "world/blockentity/core/LockableBlockEntity.hpp"
#include <array>

namespace mc {

/**
 * @brief 漏斗方块实体
 * 
 * 5格物品存储，支持：
 * - 从上方容器/物品实体吸取物品
 * - 向下方容器输出物品
 * - 红石控制（禁用/启用）
 * - 传输冷却机制
 * 
 * 参考: net.minecraft.tileentity.HopperTileEntity
 */
class HopperEntity : public LockableBlockEntity, public IHopper {
public:
    /// 漏斗容量
    static constexpr i32 HOPPER_SIZE = 5;
    
    /// 正常传输冷却时间 (ticks)
    static constexpr i32 TRANSFER_COOLDOWN = 8;
    
    /// 漏斗链优化冷却时间 (ticks)
    static constexpr i32 TRANSFER_COOLDOWN_CHAIN = 7;
    
    // ========== 构造 ==========
    
    explicit HopperEntity(const BlockPos& pos);
    
    // ========== IInventory 实现 ==========
    
    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return HOPPER_SIZE; }
    
    // ========== 漏斗特有接口 ==========
    
    /**
     * @brief 获取传输冷却时间
     * @return 剩余冷却 ticks
     */
    [[nodiscard]] i32 getTransferCooldown() const { return m_transferCooldown; }
    
    /**
     * @brief 设置传输冷却时间
     * @param cooldown 冷却 ticks
     */
    void setTransferCooldown(i32 cooldown) {
        m_transferCooldown = cooldown;
        setChanged();
    }
    
    /**
     * @brief 检查漏斗是否启用
     * @return 如果漏斗可以传输物品返回true
     */
    [[nodiscard]] bool isEnabled() const;
    
    /**
     * @brief 检查是否正在传输
     */
    [[nodiscard]] bool isOnTransferCooldown() const { return m_transferCooldown > 0; }
    
    // ========== IHopper 接口 ==========
    
    [[nodiscard]] BlockPos getHopperX() const override { return m_pos; }
    [[nodiscard]] BlockPos getHopperY() const override { return m_pos; }
    [[nodiscard]] BlockPos getHopperZ() const override { return m_pos; }
    
    // ========== Tick 更新 ==========
    
    void tick(World& world) override;
    [[nodiscard]] bool needsTick() const override { return true; }
    
    // ========== 序列化 ==========
    
    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    [[nodiscard]] String getDefaultName() const override { return "container.hopper"; }

private:
    // ========== 内部方法 ==========
    
    /**
     * @brief 尝试传输物品
     * @param world 世界引用
     * @return 如果有物品传输返回true
     */
    bool tryTransfer(World& world);
    
    /**
     * @brief 从上方吸取物品
     * @param world 世界引用
     * @return 如果吸取了物品返回true
     */
    bool pullItems(World& world);
    
    /**
     * @brief 向下方输出物品
     * @param world 世界引用
     * @return 如果输出了物品返回true
     */
    bool pushItems(World& world);
    
    /**
     * @brief 尝试从指定容器吸取物品
     * @param inventory 源容器
     * @return 如果吸取了物品返回true
     */
    bool pullFromInventory(IInventory* inventory);
    
    /**
     * @brief 尝试向指定容器输出物品
     * @param inventory 目标容器
     * @return 如果输出了物品返回true
     */
    bool pushToInventory(IInventory* inventory);
    
    /**
     * @brief 尝试从世界中的物品实体吸取
     * @param world 世界引用
     * @return 如果吸取了物品返回true
     */
    bool pullFromWorld(World& world);
    
    /**
     * @brief 查找上方的容器
     * @param world 世界引用
     * @return 容器指针，如果没有返回nullptr
     */
    [[nodiscard]] IInventory* findContainerAbove(World& world);
    
    /**
     * @brief 查找下方的容器
     * @param world 世界引用
     * @return 容器指针，如果没有返回nullptr
     */
    [[nodiscard]] IInventory* findContainerBelow(World& world);
    
    // ========== 成员变量 ==========
    
    class HopperInventory : public IInventory {
    public:
        HopperInventory();
        ~HopperInventory() override = default;
        
        [[nodiscard]] i32 getContainerSize() const override { return HOPPER_SIZE; }
        [[nodiscard]] bool isEmpty() const override;
        [[nodiscard]] ItemStack getItem(i32 slot) const override;
        void setItem(i32 slot, const ItemStack& stack) override;
        ItemStack removeItem(i32 slot, i32 count) override;
        ItemStack removeItemNoUpdate(i32 slot) override;
        void clear() override;
        void setChanged() override;
        void serialize(network::PacketSerializer& ser) const override;
        
    private:
        std::array<ItemStack, HOPPER_SIZE> m_items;
    };
    
    HopperInventory m_inventory;
    i32 m_transferCooldown = 0;
};

/**
 * @brief 漏斗接口
 * 
 * 用于统一处理漏斗和漏斗矿车。
 */
class IHopper {
public:
    virtual ~IHopper() = default;
    
    [[nodiscard]] virtual BlockPos getHopperX() const = 0;
    [[nodiscard]] virtual BlockPos getHopperY() const = 0;
    [[nodiscard]] virtual BlockPos getHopperZ() const = 0;
};

} // namespace mc
```

---

## 四、方块状态属性设计

### 4.1 新增方块状态属性

```cpp
// 添加到 src/common/util/property/Properties.hpp

// ========== 箱子类型属性 ==========
/**
 * @brief 箱子类型枚举
 */
enum class ChestType : u8 {
    Single = 0,   ///< 单箱
    Left = 1,     ///< 双箱左半
    Right = 2     ///< 双箱右半
};

/**
 * @brief 箱子类型属性
 */
static const EnumProperty<ChestType>& CHEST_TYPE() {
    static auto prop = EnumProperty<ChestType>::create("type", {
        ChestType::Single, ChestType::Left, ChestType::Right
    });
    return *prop;
}

// ========== 门属性 ==========
/**
 * @brief 门半部分枚举
 */
enum class DoubleBlockHalf : u8 {
    Upper = 0,  ///< 上半部分
    Lower = 1   ///< 下半部分
};

/**
 * @brief 双方块半部分属性
 */
static const EnumProperty<DoubleBlockHalf>& HALF() {
    static auto prop = EnumProperty<DoubleBlockHalf>::create("half", {
        DoubleBlockHalf::Upper, DoubleBlockHalf::Lower
    });
    return *prop;
}

/**
 * @brief 门铰链位置枚举
 */
enum class DoorHinge : u8 {
    Left = 0,   ///< 左铰链
    Right = 1   ///< 右铰链
};

/**
 * @brief 门铰链属性
 */
static const EnumProperty<DoorHinge>& HINGE() {
    static auto prop = EnumProperty<DoorHinge>::create("hinge", {
        DoorHinge::Left, DoorHinge::Right
    });
    return *prop;
}

// ========== 熔炉属性 ==========
/**
 * @brief 熔炼进度属性 (0-15，用于方块状态显示)
 */
static const IntegerProperty& COOK_TIME_0_15() {
    static auto prop = IntegerProperty::create("cook_time", 0, 15);
    return *prop;
}

// ========== 漏斗属性 ==========
/**
 * @brief 是否启用（漏斗）
 * 注：ENABLED已在布尔属性中定义

// ========== 栅栏门属性 ==========
/**
 * @brief 是否在墙内（栅栏门）
 */
static const BooleanProperty& IN_WALL() {
    static auto prop = BooleanProperty::create("in_wall");
    return *prop;
}

// ========== 炼药锅属性 ==========
/**
 * @brief 炼药锅水位 (0-3)
 */
static const IntegerProperty& LEVEL_0_3() {
    static auto prop = IntegerProperty::create("level", 0, 3);
    return *prop;
}
```

---

## 五、实现优先级排序

### 第一阶段：基础设施 (估计时间：3-4天)

| 优先级 | 任务 | 文件 | 依赖 |
|-------|------|------|------|
| P0 | BlockEntityRegistry 注册表 | core/BlockEntityRegistry.hpp/cpp | BlockEntity |
| P0 | LockableBlockEntity 可锁定基类 | core/LockableBlockEntity.hpp/cpp | ContainerBlockEntity |
| P0 | SimpleInventory 简单背包实现 | core/SimpleInventory.hpp/cpp | IInventory |
| P0 | FurnaceInventory 熔炉背包 | processing/FurnaceInventory.hpp/cpp | IInventory |
| P1 | DoubleSidedInventory 双箱容器 | storage/DoubleSidedInventory.hpp/cpp | IInventory |

### 第二阶段：存储系统 (估计时间：4-5天)

| 优先级 | 任务 | 文件 | 依赖 |
|-------|------|------|------|
| P0 | ChestEntity 箱子实体 | storage/ChestEntity.hpp/cpp | LockableBlockEntity |
| P0 | ChestBlock 箱子方块 | blocks/ChestBlock.hpp/cpp | Block |
| P1 | DoubleSidedInventory 实现 | storage/DoubleSidedInventory.cpp | ChestEntity |
| P1 | ChestContainer 箱子GUI容器 | container/ChestContainer.hpp/cpp | Container |
| P2 | TrappedChestEntity 陷阱箱 | storage/TrappedChestEntity.hpp/cpp | ChestEntity |
| P2 | TrappedChestBlock 陷阱箱方块 | blocks/TrappedChestBlock.hpp/cpp | ChestBlock |
| P3 | EnderChestEntity 末影箱 | storage/EnderChestEntity.hpp/cpp | BlockEntity |

### 第三阶段：传输系统 (估计时间：3-4天)

| 优先级 | 任务 | 文件 | 依赖 |
|-------|------|------|------|
| P0 | HopperEntity 漏斗实体 | transport/HopperEntity.hpp/cpp | LockableBlockEntity |
| P0 | HopperBlock 漏斗方块 | transport/HopperBlock.hpp/cpp | Block |
| P1 | IHopper 漏斗接口 | transport/IHopper.hpp | - |
| P1 | HopperContainer 漏斗GUI | container/HopperContainer.hpp/cpp | Container |

### 第四阶段：加工系统 (估计时间：5-6天)

| 优先级 | 任务 | 文件 | 依赖 |
|-------|------|------|------|
| P0 | AbstractFurnaceEntity 熔炉基类 | processing/AbstractFurnaceEntity.hpp/cpp | LockableBlockEntity |
| P0 | AbstractFurnaceBlock 熔炉方块基类 | processing/AbstractFurnaceBlock.hpp/cpp | Block |
| P0 | FurnaceEntity 普通熔炉 | processing/FurnaceEntity.hpp/cpp | AbstractFurnaceEntity |
| P0 | FurnaceBlock 熔炉方块 | processing/FurnaceBlock.hpp/cpp | AbstractFurnaceBlock |
| P1 | SmeltingRecipe 熔炼配方 | crafting/SmeltingRecipe.hpp/cpp | IRecipe |
| P1 | FurnaceContainer 熔炉GUI | container/FurnaceContainer.hpp/cpp | Container |
| P2 | BlastFurnaceEntity 高炉 | processing/BlastFurnaceEntity.hpp/cpp | AbstractFurnaceEntity |
| P2 | BlastFurnaceBlock 高炉方块 | processing/BlastFurnaceBlock.hpp/cpp | AbstractFurnaceBlock |
| P2 | SmokerEntity 烟熏炉 | processing/SmokerEntity.hpp/cpp | AbstractFurnaceEntity |
| P2 | SmokerBlock 烟熏炉方块 | processing/SmokerBlock.hpp/cpp | AbstractFurnaceBlock |

### 第五阶段：交互系统 (估计时间：4-5天)

| 优先级 | 任务 | 文件 | 依赖 |
|-------|------|------|------|
| P0 | DoorBlock 门方块 | doors/DoorBlock.hpp/cpp | Block |
| P1 | FenceGateBlock 栅栏门方块 | doors/FenceGateBlock.hpp/cpp | Block |
| P1 | CauldronBlock 炼药锅方块 | cauldron/CauldronBlock.hpp/cpp | Block |
| P2 | EnchantingTableEntity 附魔台实体 | interactive/EnchantingTableEntity.hpp/cpp | BlockEntity |
| P2 | EnchantingTableBlock 附魔台方块 | interactive/EnchantingTableBlock.hpp/cpp | Block |
| P3 | EnchantmentContainer 附魔GUI | container/EnchantmentContainer.hpp/cpp | Container |

---

## 六、测试计划

### 6.1 单元测试框架

每个方块实体类都应包含以下测试：

```cpp
// tests/common/world/blockentity/ChestEntityTest.cpp

#include <gtest/gtest.h>
#include "world/blockentity/storage/ChestEntity.hpp"
#include "world/block/BlockPos.hpp"

using namespace mc;

class ChestEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        entity = std::make_unique<ChestEntity>(BlockPos(0, 0, 0));
    }
    
    std::unique_ptr<ChestEntity> entity;
};

// ========== 基础功能测试 ==========

TEST_F(ChestEntityTest, Create_HasCorrectType) {
    EXPECT_EQ(entity->getType(), BlockEntityType::Chest);
}

TEST_F(ChestEntityTest, Create_HasCorrectPosition) {
    EXPECT_EQ(entity->getPos(), BlockPos(0, 0, 0));
}

TEST_F(ChestEntityTest, Create_InventoryIsEmpty) {
    EXPECT_TRUE(entity->isEmpty());
}

TEST_F(ChestEntityTest, Create_InventorySizeIs27) {
    EXPECT_EQ(entity->getContainerSize(), 27);
}

// ========== 物品操作测试 ==========

TEST_F(ChestEntityTest, SetItem_ChangedFlagSet) {
    entity->clearChanged();
    entity->getInventory()->setItem(0, ItemStack(/* some item */));
    EXPECT_TRUE(entity->isChanged());
}

TEST_F(ChestEntityTest, OpenContainer_IncrementsCount) {
    EXPECT_EQ(entity->getOpenCount(), 0);
    entity->openContainer();
    EXPECT_EQ(entity->getOpenCount(), 1);
    entity->openContainer();
    EXPECT_EQ(entity->getOpenCount(), 2);
}

TEST_F(ChestEntityTest, CloseContainer_De decrementsCount) {
    entity->openContainer();
    entity->openContainer();
    EXPECT_EQ(entity->getOpenCount(), 2);
    entity->closeContainer();
    EXPECT_EQ(entity->getOpenCount(), 1);
}

TEST_F(ChestEntityTest, CloseContainer_DoesNotGoNegative) {
    entity->closeContainer();
    EXPECT_EQ(entity->getOpenCount(), 0);
}

// ========== 序列化测试 ==========

TEST_F(ChestEntityTest, Save_ContainsBasicInfo) {
    nlohmann::json data;
    entity->save(data);
    EXPECT_EQ(data["id"], "minecraft:chest");
    EXPECT_EQ(data["x"], 0);
    EXPECT_EQ(data["y"], 0);
    EXPECT_EQ(data["z"], 0);
}

TEST_F(ChestEntityTest, Load_RestoresState) {
    // 设置状态
    entity->setCustomName("Test Chest");
    entity->getInventory()->setItem(0, ItemStack(/* item */));
    
    // 保存
    nlohmann::json data;
    entity->save(data);
    
    // 创建新实例并加载
    auto entity2 = std::make_unique<ChestEntity>(BlockPos(0, 0, 0));
    entity2->load(data);
    
    // 验证
    EXPECT_EQ(entity2->getCustomName(), "Test Chest");
    // ... 验证物品
}

// ========== 双箱测试 ==========

TEST_F(ChestEntityTest, SingleChest_NotDouble) {
    EXPECT_FALSE(entity->isDoubleChest());
}

// 需要Mock World来测试双箱逻辑
```

### 6.2 集成测试

```cpp
// tests/integration/blockentity/FurnaceIntegrationTest.cpp

class FurnaceIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试世界
        // 放置熔炉
        // 创建熔炉实体
    }
};

TEST_F(FurnaceIntegrationTest, SmeltIronOre_ProducesIronIngot) {
    // 放置铁矿石在输入槽
    // 放置煤炭在燃料槽
    // 等待熔炼完成
    // 验证输出槽有铁锭
}

TEST_F(FurnaceIntegrationTest, FuelConsumption_WorkingCorrectly) {
    // 放置燃料
    // 验证燃烧时间正确
    // 等待燃料消耗
    // 验证燃料减少
}

TEST_F(FurnaceIntegrationTest, RedstoneComparator_SignalBasedOnContents) {
    // 填充部分熔炉
    // 验证比较器信号正确
}
```

---

## 七、详细任务清单

### 第一阶段详细任务

#### 任务 1.1: BlockEntityRegistry 方块实体注册表

**文件**: `src/common/world/blockentity/core/BlockEntityRegistry.hpp/cpp`

**功能需求**:
- 工厂方法注册方块实体类型
- 从JSON创建方块实体
- 支持动态类型创建
- 线程安全的注册表

**关键代码**:
```cpp
class BlockEntityRegistry {
public:
    using Factory = std::function<std::unique_ptr<BlockEntity>(const BlockPos&)>;
    
    static BlockEntityRegistry& instance();
    
    void registerType(BlockEntityType type, Factory factory);
    std::unique_ptr<BlockEntity> create(BlockEntityType type, const BlockPos& pos);
    std::unique_ptr<BlockEntity> createFromJson(const nlohmann::json& data);
    
private:
    std::unordered_map<BlockEntityType, Factory> m_factories;
};
```

**测试用例**:
- 注册后能创建实体
- 未注册类型返回nullptr
- JSON反序列化正确

---

#### 任务 1.2: LockableBlockEntity 可锁定容器基类

**文件**: `src/common/world/blockentity/core/LockableBlockEntity.hpp/cpp`

**功能需求**:
- 锁定状态管理
- 自定义名称显示
- 钥匙匹配检测
- 序列化/反序列化

**依赖**: ContainerBlockEntity

---

#### 任务 1.3: SimpleInventory 简单背包实现

**文件**: `src/common/world/blockentity/core/SimpleInventory.hpp/cpp`

**功能需求**:
- IInventory接口完整实现
- 泛型尺寸支持
- 变更通知机制

---

### 第二阶段详细任务

#### 任务 2.1: ChestEntity 箱子实体

**文件**: `src/common/world/blockentity/storage/ChestEntity.hpp/cpp`

**功能需求**:
- 27格物品存储
- 单箱/双箱检测
- 打开计数管理
- 盖子动画状态
- 红石比较器信号
- 玩家打开/关闭事件广播

**关键实现**:
```cpp
void ChestEntity::tick(World& world) {
    // 更新盖子动画
    f32 targetAngle = (m_openCount > 0) ? 1.0f : 0.0f;
    m_prevLidAngle = m_lidAngle;
    m_lidAngle = std::lerp(m_lidAngle, targetAngle, 0.1f);
    
    // 广播打开状态变化
    // ...
}
```

---

#### 任务 2.2: ChestBlock 箱子方块

**文件**: `src/common/world/block/blocks/ChestBlock.hpp/cpp`

**功能需求**:
- 方块状态属性: FACING, TYPE (SINGLE/LEFT/RIGHT), WATERLOGGED
- 双箱检测和连接逻辑
- 放置时自动连接相邻箱子
- 红石信号输出
- 碰撞形状

**方块状态**:
```cpp
// 属性定义
auto FACING = BlockStateProperties::HORIZONTAL_FACING();
auto TYPE = BlockStateProperties::CHEST_TYPE();
auto WATERLOGGED = BlockStateProperties::WATERLOGGED();

// 默认状态
setDefaultState(stateContainer().getDefaultState()
    .with(FACING, Direction::North)
    .with(TYPE, ChestType::Single)
    .with(WATERLOGGED, false));
```

---

#### 任务 2.3: DoubleSidedInventory 双箱容器

**文件**: `src/common/world/blockentity/storage/DoubleSidedInventory.hpp/cpp`

**功能需求**:
- 合并两个27格箱子为54格
- 实现IInventory接口
- 代理所有操作到两个底层箱子
- 变更通知到两个箱子

**关键实现**:
```cpp
class DoubleSidedInventory : public IInventory {
public:
    DoubleSidedInventory(ChestEntity* left, ChestEntity* right);
    
    i32 getContainerSize() const override { return 54; }
    
    ItemStack getItem(i32 slot) const override {
        if (slot < 27) {
            return m_left->getInventory()->getItem(slot);
        } else {
            return m_right->getInventory()->getItem(slot - 27);
        }
    }
    
    // ... 其他方法类似
    
private:
    ChestEntity* m_left;
    ChestEntity* m_right;
};
```

---

### 第三阶段详细任务

#### 任务 3.1: HopperEntity 漏斗实体

**文件**: `src/common/world/blockentity/transport/HopperEntity.hpp/cpp`

**功能需求**:
- 5格物品存储
- 传输冷却机制 (8tick正常, 7tick漏斗链)
- 从上方容器吸取物品
- 向下方容器输出物品
- 从世界物品实体吸取
- 红石控制（禁用/启用）

**关键实现**:
```cpp
void HopperEntity::tick(World& world) {
    // 冷却递减
    if (m_transferCooldown > 0) {
        m_transferCooldown--;
        return;
    }
    
    // 检查是否启用
    if (!isEnabled()) {
        return;
    }
    
    // 尝试传输
    if (tryTransfer(world)) {
        m_transferCooldown = TRANSFER_COOLDOWN;
        setChanged();
    }
}

bool HopperEntity::tryTransfer(World& world) {
    // 先尝试输出
    if (pushItems(world)) {
        return true;
    }
    // 再尝试吸入
    return pullItems(world);
}
```

---

#### 任务 3.2: HopperBlock 漏斗方块

**文件**: `src/common/world/blockentity/transport/HopperBlock.hpp/cpp`

**功能需求**:
- 方块状态属性: FACING (输出方向), ENABLED
- 碰撞形状 (漏斗形状)
- 红石响应（禁用/启用）
- 放置方向检测

---

### 第四阶段详细任务

#### 任务 4.1: AbstractFurnaceEntity 熔炉基类

**文件**: `src/common/world/blockentity/processing/AbstractFurnaceEntity.hpp/cpp`

**功能需求**:
- 3槽背包（输入、燃料、输出）
- 燃烧时间/燃烧总时间
- 熔炼进度/熔炼总时间
- 燃料系统（煤炭、木板等）
- 配方匹配
- 光照等级（燃烧时）

**燃料系统**:
```cpp
namespace Fuels {
    // 燃料燃烧时间映射 (ticks)
    std::unordered_map<Item*, i32> getFuelValues();
    
    // 燃烧时间示例:
    // 煤炭: 1600
    // 木炭: 1600
    // 木板: 300
    // 木棍: 100
    // 岩浆桶: 20000
}
```

**熔炼逻辑**:
```cpp
void AbstractFurnaceEntity::tick(World& world) {
    bool wasBurning = isBurning();
    bool changed = false;
    
    // 1. 处理燃烧
    if (isBurning()) {
        m_burnTime--;
    }
    
    // 2. 检查是否需要新燃料
    if (canSmelt()) {
        if (!isBurning() && consumeFuel() > 0) {
            m_burnTimeTotal = m_burnTime;
            changed = true;
        }
        
        // 3. 处理熔炼
        if (isBurning()) {
            m_cookProgress++;
            if (m_cookProgress >= m_cookTimeTotal) {
                // 完成熔炼
                smeltItem();
                m_cookProgress = 0;
                changed = true;
            }
        }
    } else {
        m_cookProgress = 0;
    }
    
    // 4. 更新方块状态
    if (wasBurning != isBurning()) {
        updateBlockState(world);
    }
    
    if (changed) {
        setChanged();
    }
}
```

---

#### 任务 4.2: FurnaceBlock, BlastFurnaceBlock, SmokerBlock

**功能差异**:

| 特性 | 熔炉 | 高炉 | 烟熏炉 |
|------|------|------|--------|
| 熔炼时间 | 200 ticks | 100 ticks | 100 ticks |
| 熔炼物品 | 所有 | 仅矿石/金属 | 仅食物 |
| XP倍率 | 1.0 | 0.5 | 0.5 |
| 光照等级 | 13 | 13 | 13 |

---

### 第五阶段详细任务

#### 任务 5.1: DoorBlock 门方块

**文件**: `src/common/world/block/blocks/DoorBlock.hpp/cpp`

**功能需求**:
- 双方块结构（HALF = UPPER/LOWER）
- 方块状态: FACING, OPEN, HINGE, POWERED, HALF
- 红石响应
- 铁门/木门差异（手动/红石）
- 放置逻辑（自动创建上下两部分）
- 破坏逻辑（同时移除上下两部分）

**方块状态**:
```cpp
// 属性
auto FACING = BlockStateProperties::HORIZONTAL_FACING();
auto OPEN = BlockStateProperties::OPEN();
auto HINGE = BlockStateProperties::HINGE();
auto POWERED = BlockStateProperties::POWERED();
auto HALF = BlockStateProperties::HALF();

// 方块状态数量: 4 * 2 * 2 * 2 * 2 = 64
```

---

#### 任务 5.2: FenceGateBlock 栅栏门方块

**文件**: `src/common/world/block/blocks/FenceGateBlock.hpp/cpp`

**功能需求**:
- 方块状态: FACING, OPEN, POWERED, IN_WALL
- 碰撞箱动态变化（打开时无碰撞）
- 与墙的连接逻辑（IN_WALL状态）
- 红石响应

---

#### 任务 5.3: CauldronBlock 炼药锅方块

**文件**: `src/common/world/block/blocks/CauldronBlock.hpp/cpp`

**功能需求**:
- 水位状态（LEVEL = 0-3）
- 与水桶/玻璃瓶/水瓶的交互
- 洗皮甲/洗旗帜/洗潜影盒
- 雨水填充
- 红石比较器支持
- 燃烧实体熄灭

**交互逻辑**:
```cpp
ActionResult CauldronBlock::onUse(
    World& world, const BlockPos& pos, BlockState& state,
    Player& player, const BlockRayTraceResult& hit) {
    
    i32 level = state.get(LEVEL_0_3());
    ItemStack heldItem = player.getHeldItem();
    
    // 水桶交互
    if (heldItem.getItem() == Items::WATER_BUCKET && level < 3) {
        // 填充炼药锅
        state = state.with(LEVEL_0_3(), 3);
        world.setBlockState(pos, state);
        player.setHeldItem(ItemStack(Items::BUCKET));
        return ActionResult::SUCCESS;
    }
    
    // 玻璃瓶交互
    if (heldItem.getItem() == Items::GLASS_BOTTLE && level > 0) {
        state = state.with(LEVEL_0_3(), level - 1);
        world.setBlockState(pos, state);
        player.setHeldItem(ItemStack(Items::POTION)); // 水瓶
        return ActionResult::SUCCESS;
    }
    
    // ... 其他交互
    
    return ActionResult::PASS;
}
```

---

## 八、依赖关系图

```
┌─────────────────────────────────────────────────────────────────┐
│                        基础设施层                                 │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐         │
│  │ BlockEntity │  │ IInventory   │  │ BlockStateProps │         │
│  │   (已有)    │  │    (已有)    │  │      (已有)     │         │
│  └──────┬──────┘  └──────┬───────┘  └────────┬────────┘         │
│         │                │                    │                  │
│         ▼                ▼                    ▼                  │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐         │
│  │ContainerBE  │  │SimpleInv     │  │ 新增属性        │         │
│  │   (已有)    │  │   (新增)     │  │ ChestType等     │         │
│  └──────┬──────┘  └──────────────┘  └─────────────────┘         │
│         │                                                        │
│         ▼                                                        │
│  ┌─────────────┐                                                │
│  │LockableBE   │  BlockEntityRegistry                           │
│  │   (新增)    │                                                │
│  └──────┬──────┘                                                │
└─────────┼───────────────────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────────────────┐
│                        存储层                                    │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐         │
│  │ ChestEntity │◄─┤DoubleInv    │  │ TrappedChest    │         │
│  └──────┬──────┘  └──────────────┘  └─────────────────┘         │
│         │                │                                       │
│         ▼                ▼                                       │
│  ┌─────────────┐  ┌──────────────┐                              │
│  │ ChestBlock  │  │ChestContainer│                              │
│  └─────────────┘  └──────────────┘                              │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                        传输层                                    │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐         │
│  │ HopperEntity│──► IHopper     │  │ HopperBlock     │         │
│  └──────┬──────┘  └──────────────┘  └─────────────────┘         │
│         │                                                       │
│         ▼                                                       │
│  ┌─────────────┐                                                │
│  │HopperCont.  │                                                │
│  └─────────────┘                                                │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                        加工层                                    │
│  ┌─────────────────────┐                                        │
│  │ AbstractFurnaceEntity│◄───────────────────┐                  │
│  └──────────┬──────────┘                    │                  │
│             │                               │                  │
│     ┌───────┴───────┬───────────────┐       │                  │
│     ▼               ▼               ▼       │                  │
│  ┌────────┐   ┌──────────┐   ┌──────────┐   │                  │
│  │Furnace │   │BlastFurn.│   │ Smoker   │   │                  │
│  │ Entity │   │  Entity  │   │  Entity  │   │                  │
│  └───┬────┘   └────┬─────┘   └────┬─────┘   │                  │
│      │             │              │         │                  │
│      ▼             ▼              ▼         │                  │
│  ┌────────┐   ┌──────────┐   ┌──────────┐   │                  │
│  │Furnace │   │BlastFurn.│   │ Smoker   │───┘                  │
│  │ Block  │   │  Block   │   │  Block   │                      │
│  └────────┘   └──────────┘   └──────────┘                      │
│                                                                 │
│  ┌─────────────────┐                                           │
│  │ SmeltingRecipe  │  (配方系统扩展)                           │
│  └─────────────────┘                                           │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                        交互层                                    │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐         │
│  │  DoorBlock  │  │FenceGateBlk │  │ CauldronBlock   │         │
│  └─────────────┘  └──────────────┘  └─────────────────┘         │
│                                                                 │
│  ┌─────────────────┐  ┌──────────────────┐                      │
│  │EnchantingTableBE│  │EnchantmentCont.  │                      │
│  └─────────────────┘  └──────────────────┘                      │
└─────────────────────────────────────────────────────────────────┘
```

---

## 九、文档注释规范

所有新增代码应遵循项目现有的文档注释风格：

```cpp
/**
 * @brief 简短描述
 * 
 * 详细描述，说明类的职责、用法、注意事项等。
 * 
 * 参考: net.minecraft.xxx.ClassName
 *
 * 用法示例:
 * @code
 * ChestEntity chest(pos);
 * chest.getInventory()->setItem(0, ItemStack(Items::DIAMOND, 64));
 * @endcode
 *
 * 注意事项:
 * - 线程安全说明
 * - 内存管理说明
 * - 其他重要信息
 */
class ClassName {
public:
    /**
     * @brief 方法描述
     * 
     * 详细说明方法的行为、参数、返回值。
     *
     * @param paramName 参数描述
     * @return 返回值描述
     *
     * @throws ExceptionType 异常说明
     *
     * @note 重要提示
     *
     * @see 相关方法或类
     */
    ReturnType methodName(ParamType paramName);
};
```

---

## 十、Critical Files for Implementation

基于本设计方案，以下是实现此计划最关键的文件：

### Critical Files for Implementation

1. **`src/common/world/blockentity/core/LockableBlockEntity.hpp`** - 可锁定容器基类，是箱子、漏斗、熔炉等大多数容器实体的直接父类

2. **`src/common/world/blockentity/processing/AbstractFurnaceEntity.hpp`** - 熔炉基类，实现了完整的燃料燃烧和熔炼逻辑，是三种熔炉的核心

3. **`src/common/world/blockentity/storage/ChestEntity.hpp`** - 箱子实体，是最常用的容器，实现双箱逻辑是技术难点

4. **`src/common/world/blockentity/transport/HopperEntity.hpp`** - 漏斗实体，物品传输逻辑复杂，涉及多个容器交互

5. **`src/common/world/blockentity/core/BlockEntityRegistry.hpp`** - 方块实体注册表，是动态创建方块实体的基础设施
