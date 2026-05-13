#pragma once

#include "../../core/Types.hpp"
#include <unordered_map>

namespace mc {

class Block;
class BlockState;
class IWorld;
class BlockPos;
class Entity;

// 前向声明 Direction
enum class Direction : u8;

namespace blocks {

/**
 * @brief 火焰信息结构
 *
 * 存储方块的燃烧参数：
 * - encouragement: 火焰蔓延速度（影响火焰向此方块蔓延的概率）
 * - flammability: 可燃性 0-300（影响方块被点燃和烧毁的概率）
 *
 * 参考 MC 1.16.5: net.minecraft.block.FireBlock
 */
struct FireInfo {
    i32 encouragement = 0;  ///< 火焰蔓延速度
    i32 flammability = 0;   ///< 可燃性 (0-300)

    constexpr FireInfo() = default;
    constexpr FireInfo(i32 enc, i32 flam) : encouragement(enc), flammability(flam) {}
};

/**
 * @brief 火焰信息注册表
 *
 * 管理所有方块的燃烧参数。
 * 在方块注册时调用 registerFireInfo() 注册燃烧参数。
 *
 * 参考 MC 1.16.5: net.minecraft.block.FireBlock.init()
 */
class FireInfoRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static FireInfoRegistry& instance();

    /**
     * @brief 注册方块的燃烧参数
     *
     * @param blockId 方块ID
     * @param encouragement 火焰蔓延速度
     * @param flammability 可燃性 (0-300)
     */
    void registerFireInfo(u32 blockId, i32 encouragement, i32 flammability);

    /**
     * @brief 获取方块的燃烧参数
     *
     * @param blockId 方块ID
     * @return 燃烧参数，如果未注册返回默认值 (0, 0)
     */
    [[nodiscard]] FireInfo getFireInfo(u32 blockId) const;

    /**
     * @brief 获取方块的可燃性
     *
     * @param blockId 方块ID
     * @return 可燃性值 (0-300)
     */
    [[nodiscard]] i32 getFlammability(u32 blockId) const;

    /**
     * @brief 获取方块的火焰蔓延速度
     *
     * @param blockId 方块ID
     * @return 火焰蔓延速度
     */
    [[nodiscard]] i32 getEncouragement(u32 blockId) const;

    /**
     * @brief 初始化原版方块的燃烧参数
     *
     * 注册所有原版可燃方块的燃烧参数。
     * 应在 VanillaBlocks 初始化后调用。
     */
    void initializeVanillaFireInfos();

    /**
     * @brief 清空所有注册的燃烧参数
     */
    void clear();

private:
    FireInfoRegistry() = default;
    ~FireInfoRegistry() = default;

    // 禁止拷贝
    FireInfoRegistry(const FireInfoRegistry&) = delete;
    FireInfoRegistry& operator=(const FireInfoRegistry&) = delete;

    std::unordered_map<u32, FireInfo> m_fireInfos;
};

} // namespace blocks
} // namespace mc
