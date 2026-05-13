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

    // ===== 木板类 =====
    // 所有木板、台阶、栅栏、楼梯等
    registerFireInfo(5, 20);   // PLANKS (各木板统一ID)
    // 注意：实际注册时会使用 VanillaBlocks::OAK_PLANKS->blockId() 等
    // 这里使用硬编码ID占位，实际需要根据 VanillaBlocks 动态注册

    // ===== 原木类 =====
    registerFireInfo(5, 5);    // LOG (原木)

    // ===== 煤炭块 =====
    registerFireInfo(5, 5);    // COAL_BLOCK

    // ===== 藤蔓 =====
    registerFireInfo(15, 100); // VINE

    // ===== TNT =====
    registerFireInfo(15, 100); // TNT

    // ===== 树叶 =====
    registerFireInfo(30, 60);  // LEAVES

    // ===== 书架 =====
    registerFireInfo(30, 20);  // BOOKSHELF

    // ===== 羊毛 =====
    registerFireInfo(30, 60);  // WOOL

    // ===== 地毯 =====
    registerFireInfo(60, 20);  // CARPET

    // ===== 干海带块 =====
    registerFireInfo(30, 60);  // DRIED_KELP_BLOCK

    // ===== 讲台 =====
    registerFireInfo(30, 20);  // LECTERN

    // ===== 蜂巢/蜂箱 =====
    registerFireInfo(30, 20);  // BEE_NEST, BEEHIVE

    // ===== 干草块 =====
    registerFireInfo(60, 20);  // HAY_BLOCK

    // ===== 花草类 =====
    // GRASS, FERN, TALL_GRASS, FLOWERS
    registerFireInfo(60, 100);

    // ===== 枯灌木 =====
    registerFireInfo(60, 100); // DEAD_BUSH

    // ===== 竹子 =====
    registerFireInfo(60, 60);  // BAMBOO

    // ===== 脚手架 =====
    registerFireInfo(60, 60);  // SCAFFOLDING

    // ===== 甜浆果丛 =====
    registerFireInfo(60, 100); // SWEET_BERRY_BUSH

    // ===== 标靶 =====
    registerFireInfo(15, 20);  // TARGET

    // ===== 堆肥桶 =====
    registerFireInfo(5, 20);   // COMPOSTER

    // 注意：实际的方块ID需要通过 VanillaBlocks 获取
    // 上述调用仅作为示例，实际初始化应使用：
    // if (VanillaBlocks::OAK_PLANKS != nullptr) {
    //     registerFireInfo(VanillaBlocks::OAK_PLANKS->blockId(), 5, 20);
    // }
}

} // namespace blocks
} // namespace mc
