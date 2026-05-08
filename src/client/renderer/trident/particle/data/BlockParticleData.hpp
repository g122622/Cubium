#pragma once

#include "ParticleData.hpp"
#include "common/world/block/Block.hpp"

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 方块粒子数据
 *
 * 用于需要方块状态信息的粒子类型，如方块破坏、下落灰尘等。
 * 参考 MC 1.16.5 BlockParticleData
 *
 * 用法示例：
 * @code
 * BlockState state = BlockRegistry::get(BlockId::Stone).getDefaultState();
 * auto blockData = std::make_unique<BlockParticleData>(ParticleTypeId::Block, state);
 * @endcode
 */
class BlockParticleData : public ParticleData {
public:
    /**
     * @brief 构造方块粒子数据
     *
     * @param type 粒子类型 ID（必须是 Block, Breaking 或 FallingDust）
     * @param blockState 方块状态
     */
    BlockParticleData(ParticleTypeId type, BlockState blockState);

    ~BlockParticleData() override = default;

    // 允许拷贝
    BlockParticleData(const BlockParticleData&) = default;
    BlockParticleData& operator=(const BlockParticleData&) = default;

    // 允许移动
    BlockParticleData(BlockParticleData&&) noexcept = default;
    BlockParticleData& operator=(BlockParticleData&&) noexcept = default;

    // ========================================================================
    // ParticleData 接口实现
    // ========================================================================

    [[nodiscard]] ParticleTypeId getType() const override { return m_type; }
    [[nodiscard]] std::string getTypeName() const override;
    [[nodiscard]] std::string getParameters() const override;
    [[nodiscard]] std::unique_ptr<ParticleData> clone() const override;

    // ========================================================================
    // 方块特有方法
    // ========================================================================

    /**
     * @brief 获取方块状态
     *
     * @return 方块状态
     */
    [[nodiscard]] const BlockState& getBlockState() const { return m_blockState; }

private:
    ParticleTypeId m_type;
    BlockState m_blockState;
};

} // namespace mc::client::renderer::trident::particle::data
