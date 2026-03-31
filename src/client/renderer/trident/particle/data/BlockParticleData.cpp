#include "BlockParticleData.hpp"
#include "../ParticleRegistry.hpp"
#include "../../../../../common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::data {

BlockParticleData::BlockParticleData(ParticleTypeId type, BlockState blockState)
    : m_type(type)
    , m_blockState(std::move(blockState)) {
    MC_ASSERT_MSG(requiresBlockState(type),
                  "BlockParticleData requires a block-type particle");
}

String BlockParticleData::getTypeName() const {
    return ParticleRegistry::instance().getTypeName(m_type);
}

String BlockParticleData::getParameters() const {
    // 方块粒子参数格式: block_state
    // 例如: minecraft:stone
    // TODO: 实现 BlockState 的字符串表示
    return "minecraft:stone"; // 占位符
}

std::unique_ptr<ParticleData> BlockParticleData::clone() const {
    return std::make_unique<BlockParticleData>(m_type, m_blockState);
}

} // namespace mc::client::renderer::trident::particle::data
