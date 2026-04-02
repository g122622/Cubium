#pragma once

#include "../Block.hpp"
#include "../../world/IWorld.hpp"
#include "../../world/World.hpp"

namespace mc {

/**
 * @brief 冰方块
 *
 * 透明冰方块，在明亮环境中会融化成水。
 * 挖掘后会变成水源方块。
 *
 * MC ID: minecraft:ice
 *
 * 参考 MC 1.16.5 IceBlock
 */
class IceBlock : public Block {
public:
    /**
     * @brief 构造冰方块
     */
    explicit IceBlock(BlockProperties properties);

    // ========== 方块行为 ==========

    /**
     * @brief 是否透明
     * 冰是半透明的，会散射天空光照
     */
    [[nodiscard]] bool isTransparent(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 是否不透明
     * 冰不是完全不透明的
     */
    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 是否阻挡运动
     * 冰阻挡实体运动
     */
    [[nodiscard]] bool blocksMotion(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 获取摩擦力
     * 冰的摩擦力比普通方块低，使实体可以滑动
     */
    [[nodiscard]] f32 getFriction(const BlockState& state) const override {
        MC_UNUSED(state);
        return 0.98f; // 冰的摩擦力
    }

    /**
     * @brief 方块被破坏后
     * 冰在非寒冷生物群系会融化成水，在温暖光源附近也会融化
     */
    void onBlockDestroyed(
        IWorld& world,
        const BlockPos& pos,
        const BlockState& state
    ) const override;

    /**
     * @brief 随机刻
     * 在明亮环境中融化
     */
    void randomTick(
        IWorld& world,
        const BlockPos& pos,
        BlockState& state,
        math::IRandom& random
    ) const override;

    /**
     * @brief 挖掘工具
     * 冰需要镐子才能快速挖掘
     */
    [[nodiscard]] u8 getHarvestTool(const BlockState& state) const override {
        MC_UNUSED(state);
        return item::tool::TOOL_TYPE_PICKAXE;
    }

    /**
     * @brief 挖掘等级
     */
    [[nodiscard]] i32 getHarvestLevel(const BlockState& state) const override {
        MC_UNUSED(state);
        return 0;
    }
};

/**
 * @brief 浮冰方块
 *
 * 不透明的冰方块，不会融化。
 * 挖掘后会掉落自身（使用精准采集）或什么都不掉落。
 *
 * MC ID: minecraft:packed_ice
 *
 * 参考 MC 1.16.5 PackedIceBlock
 */
class PackedIceBlock : public Block {
public:
    /**
     * @brief 构造浮冰方块
     */
    explicit PackedIceBlock(BlockProperties properties);

    /**
     * @brief 是否透明
     * 浮冰是不透明的
     */
    [[nodiscard]] bool isTransparent(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 是否不透明
     */
    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 获取摩擦力
     * 浮冰比普通冰稍微滑一点
     */
    [[nodiscard]] f32 getFriction(const BlockState& state) const override {
        MC_UNUSED(state);
        return 0.98f;
    }

    /**
     * @brief 挖掘工具
     * 浮冰需要镐子
     */
    [[nodiscard]] u8 getHarvestTool(const BlockState& state) const override {
        MC_UNUSED(state);
        return item::tool::TOOL_TYPE_PICKAXE;
    }

    /**
     * @brief 挖掘等级
     */
    [[nodiscard]] i32 getHarvestLevel(const BlockState& state) const override {
        MC_UNUSED(state);
        return 0;
    }
};

/**
 * @brief 蓝冰方块
 *
 * 最光滑的冰方块，摩擦力极低。
 * 可以用浮冰合成，不会融化。
 *
 * MC ID: minecraft:blue_ice
 *
 * 参考 MC 1.16.5 BlueIceBlock
 */
class BlueIceBlock : public Block {
public:
    /**
     * @brief 构造蓝冰方块
     */
    explicit BlueIceBlock(BlockProperties properties);

    /**
     * @brief 是否透明
     * 蓝冰是不透明的
     */
    [[nodiscard]] bool isTransparent(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 是否不透明
     */
    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 获取摩擦力
     * 蓝冰是游戏中最滑的方块
     */
    [[nodiscard]] f32 getFriction(const BlockState& state) const override {
        MC_UNUSED(state);
        return 0.989f; // 蓝冰摩擦力，比冰更滑
    }

    /**
     * @brief 挖掘工具
     * 蓝冰需要镐子
     */
    [[nodiscard]] u8 getHarvestTool(const BlockState& state) const override {
        MC_UNUSED(state);
        return item::tool::TOOL_TYPE_PICKAXE;
    }

    /**
     * @brief 挖掘等级
     */
    [[nodiscard]] i32 getHarvestLevel(const BlockState& state) const override {
        MC_UNUSED(state);
        return 0;
    }
};

/**
 * @brief 霜冰方块
 *
 * 由冰霜行者附魔生成的临时冰方块。
 * 在光源附近会融化成水。
 *
 * MC ID: minecraft:frosted_ice
 *
 * 参考 MC 1.16.5 FrostedIceBlock
 */
class FrostedIceBlock : public Block {
public:
    /**
     * @brief 构造霜冰方块
     */
    explicit FrostedIceBlock(BlockProperties properties);

    /**
     * @brief 是否透明
     */
    [[nodiscard]] bool isTransparent(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 是否不透明
     */
    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 获取摩擦力
     */
    [[nodiscard]] f32 getFriction(const BlockState& state) const override {
        MC_UNUSED(state);
        return 0.98f;
    }

    /**
     * @brief 随机刻
     * 在光源附近融化
     */
    void randomTick(
        IWorld& world,
        const BlockPos& pos,
        BlockState& state,
        math::IRandom& random
    ) const override;

    /**
     * @brief 挖掘工具
     */
    [[nodiscard]] u8 getHarvestTool(const BlockState& state) const override {
        MC_UNUSED(state);
        return item::tool::TOOL_TYPE_PICKAXE;
    }

    /**
     * @brief 挖掘等级
     */
    [[nodiscard]] i32 getHarvestLevel(const BlockState& state) const override {
        MC_UNUSED(state);
        return 0;
    }
};

} // namespace mc
