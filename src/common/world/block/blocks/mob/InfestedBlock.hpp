#pragma once

#include "../../Block.hpp"

namespace mc {

class IWorld;

namespace blocks {

/**
 * @brief 被感染方块基类
 *
 * 外观与普通方块相同，但被破坏时会生成蠹虫。
 *
 * 参考: net.minecraft.block.InfestedBlock
 */
class InfestedBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param hostBlock 被感染的方块ID
     * @param properties 方块属性
     */
    InfestedBlock(u32 hostBlock, const BlockProperties& properties);
    ~InfestedBlock() override = default;

    // ========== 破坏 ==========

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 掉落 ==========

    [[nodiscard]] u32 getHostBlock() const { return m_hostBlock; }

private:
    /// 被感染的方块ID
    u32 m_hostBlock;
};

} // namespace blocks
} // namespace mc
