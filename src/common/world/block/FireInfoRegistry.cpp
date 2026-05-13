#include "FireInfoRegistry.hpp"
#include "VanillaBlocks.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// FireInfoRegistry
// ============================================================================

FireInfoRegistry& FireInfoRegistry::instance() {
    static FireInfoRegistry s_instance;
    return s_instance;
}

void FireInfoRegistry::registerFireInfo(u32 blockId, i32 encouragement, i32 flammability) {
    m_fireInfos[blockId] = FireInfo(encouragement, flammability);
}

FireInfo FireInfoRegistry::getFireInfo(u32 blockId) const {
    auto it = m_fireInfos.find(blockId);
    if (it != m_fireInfos.end()) {
        return it->second;
    }
    return FireInfo(0, 0);
}

i32 FireInfoRegistry::getFlammability(u32 blockId) const {
    return getFireInfo(blockId).flammability;
}

i32 FireInfoRegistry::getEncouragement(u32 blockId) const {
    return getFireInfo(blockId).encouragement;
}

void FireInfoRegistry::clear() {
    m_fireInfos.clear();
}

void FireInfoRegistry::initializeVanillaFireInfos() {
    // 参考 MC 1.16.5: net.minecraft.block.FireBlock.init()
    // 参数: encouragement (蔓延速度), flammability (可燃性)
    // 可燃性范围: 0-300，值越高越容易被点燃和烧毁
    //
    // 注意：这里的注册使用占位 ID，实际的方块燃烧参数
    // 应该在 VanillaBlocks 初始化后通过 Block::registerFireInfo() 方法注册。
    // 此函数目前仅作为示例保留，实际使用时需要在方块初始化时调用：
    // if (VanillaBlocks::OAK_PLANKS != nullptr) {
    //     registerFireInfo(VanillaBlocks::OAK_PLANKS->blockId(), 5, 20);
    // }
    //
    // 目前方块燃烧参数在 Block 基类的 getFlammability/getFireSpreadSpeed 方法中
    // 通过 Material::isFlammable() 和其他属性进行判断，此注册表作为扩展预留。
}

} // namespace blocks
} // namespace mc
