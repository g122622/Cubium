#pragma once

#include "../Block.hpp"
#include "../Material.hpp"
#include "../BlockPos.hpp"
#include "../../blockentity/BlockEntityType.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include <memory>

namespace mc {

class World;
class BlockItemUseContext;
class Player;
class BlockRaycastResult;
class BlockEntity;

namespace blocks {

/**
 * @brief 附魔台方块
 *
 * 提供附魔功能的方块。周围的书架可以增加附魔力量。
 *
 * 附魔力量计算：
 * - 有效书架：距离附魔台水平2格，垂直0-1格
 * - 书架与附魔台之间必须是空气
 * - 每个有效书架增加1点附魔力量（最大15）
 *
 * 参考: net.minecraft.block.EnchantingTableBlock
 */
class EnchantingTableBlock : public Block {
public:
    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit EnchantingTableBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~EnchantingTableBlock() override = default;

    // ========== 方块实体 ==========

    /**
     * @brief 检查是否有方块实体
     */
    [[nodiscard]] bool hasBlockEntity() const override { return true; }

    /**
     * @brief 创建方块实体
     * @param pos 方块位置
     * @return 方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    /**
     * @brief 获取方块实体类型
     */
    [[nodiscard]] BlockEntityType getBlockEntityType() const {
        return BlockEntityType::EnchantingTable;
    }

    // ========== 交互 ==========

    /**
     * @brief 玩家右键点击
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     * @param hand 手
     * @param hit 射线检测结果
     * @return 交互结果
     */
    [[nodiscard]] ActionResultType onBlockActivated(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit
    ) override;

    // ========== 形状 ==========

    /**
     * @brief 获取渲染形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取遮挡形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] const CollisionShape& getOcclusionShape(const BlockState& state) const override;

    // ========== 放置和更新 ==========

    /**
     * @brief 方块被放置后的处理
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 推动反应 ==========

    /**
     * @brief 获取推动反应
     * @param state 方块状态
     * @return 推动反应类型（附魔台不能被推动）
     */
    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override {
        MC_UNUSED(state);
        return Material::PushReaction::Block;
    }

private:
    /// 方块形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
