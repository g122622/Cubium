/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

#pragma once

#include "../../core/Types.hpp"
#include "../../resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace mc {

class Block;
class BlockState;

/**
 * @brief 方块标签
 *
 * 用于将方块分组以便功能判断。
 * 参考: net.minecraft.tags.BlockTags
 *
 * 用法示例:
 * @code
 * // 检查方块状态是否在标签中
 * if (BlockTags::JUNGLE_LOGS.contains(state)) {
 *     // 方块是丛林原木
 * }
 * @endcode
 */
class BlockTag {
public:
    /**
     * @brief 构造方块标签
     * @param id 标签资源位置
     */
    explicit BlockTag(ResourceLocation id);

    /**
     * @brief 获取标签ID
     */
    [[nodiscard]] const ResourceLocation& getId() const { return m_id; }

    /**
     * @brief 添加方块到标签
     * @param blockId 方块资源位置
     */
    void add(const ResourceLocation& blockId);

    /**
     * @brief 批量添加方块
     * @param blockIds 方块资源位置列表
     */
    void addAll(const std::vector<ResourceLocation>& blockIds);

    /**
     * @brief 检查方块是否在标签中
     * @param blockId 方块资源位置
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const ResourceLocation& blockId) const;

    /**
     * @brief 检查方块是否在标签中
     * @param block 方块指针
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const Block* block) const;

    /**
     * @brief 检查方块是否在标签中
     * @param block 方块引用
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const Block& block) const;

    /**
     * @brief 检查方块状态是否在标签中
     * @param state 方块状态引用
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const BlockState& state) const;

    /**
     * @brief 获取标签中的所有方块ID
     */
    [[nodiscard]] const std::unordered_set<ResourceLocation>& getBlockIds() const { return m_blockIds; }

private:
    ResourceLocation m_id;
    std::unordered_set<ResourceLocation> m_blockIds;
};

/**
 * @brief 内置方块标签集合
 *
 * 参考 MC 1.16.5 BlockTags
 */
class BlockTags {
public:
    // ========== 原木标签 ==========

    /// 原木标签（所有原木）
    static BlockTag& LOGS();

    /// 丛林原木标签（丛林原木、丛林木、去皮丛林原木、去皮丛林木）
    static BlockTag& JUNGLE_LOGS();

    /// 橡木原木标签
    static BlockTag& OAK_LOGS();

    /// 云杉原木标签
    static BlockTag& SPRUCE_LOGS();

    /// 白桦原木标签
    static BlockTag& BIRCH_LOGS();

    /// 金合欢原木标签
    static BlockTag& ACACIA_LOGS();

    /// 深色橡木原木标签
    static BlockTag& DARK_OAK_LOGS();

    /// 绯红原木标签
    static BlockTag& CRIMSON_STEMS();

    /// 诡异原木标签
    static BlockTag& WARPED_STEMS();

    // ========== 其他常用标签 ==========

    /// 叶子标签
    static BlockTag& LEAVES();

    /// 木板标签
    static BlockTag& PLANKS();

    /// 土壤标签（可以种植的土地）
    static BlockTag& DIRT();

    /// 沙子标签
    static BlockTag& SAND();

    /// 石头标签
    static BlockTag& STONE();

    /// 火标签
    static BlockTag& FIRE();

    /// 灵魂火基座方块标签（灵魂沙、灵魂土）
    /// 在这些方块上可以点燃灵魂火
    static BlockTag& SOUL_FIRE_BASE_BLOCKS();

    /// 羊毛标签
    static BlockTag& WOOL();

    /// 竹子可种植标签（草、泥土、沙子、沙砾、竹林土）
    static BlockTag& BAMBOO_PLANTABLE_ON();

    /// 甜浆果丛可种植标签（草方块、泥土、粗糙泥土、灰化土、耕地）
    static BlockTag& VALID_SWEET_BERRY_BUSH_GROUND();

    // ========== 珊瑚和水下骨粉标签 ==========

    /// 墙珊瑚标签（所有墙珊瑚扇，包括死的和活的）
    /// 参考 MC 1.16.5: BlockTags.WALL_CORALS
    static BlockTag& WALL_CORALS();

    /// 水下骨粉标签（骨粉可以在水下催熟的方块）
    /// 参考 MC 1.16.5: BlockTags.UNDERWATER_BONEMEALS
    static BlockTag& UNDERWATER_BONEMEALS();

    // ========== 炽足兽标签 ==========

    /// 炽足兽温暖方块标签（熔岩方块）
    /// 炽足兽在这些方块上不会感到寒冷
    /// 参考 MC 1.16.5: BlockTags.STRIDER_WARM_BLOCKS
    static BlockTag& STRIDER_WARM_BLOCKS();

    // ========== 蜜蜂相关标签 ==========

    /// 小花朵标签（蒲公英、虞美人等）
    /// 蜜蜂可以采集这些花朵
    /// 参考 MC 1.16.5: BlockTags.SMALL_FLOWERS
    static BlockTag& SMALL_FLOWERS();

    /// 高花朵标签（向日葵、丁香等）
    /// 蜜蜂可以采集这些花朵
    /// 参考 MC 1.16.5: BlockTags.TALL_FLOWERS
    static BlockTag& TALL_FLOWERS();

    /// 蜂巢/蜂箱标签
    /// 蜜蜂可以进入的方块
    /// 参考 MC 1.16.5: BlockTags.BEEHIVES
    static BlockTag& BEEHIVES();

    /// 蜜蜂可授粉作物标签
    /// 小麦、胡萝卜、马铃薯、甜菜根、西瓜茎、南瓜茎、甜浆果丛
    /// 参考 MC 1.16.5: BlockTags.BEE_GROWABLES
    static BlockTag& BEE_GROWABLES();

    // ========== 末影人标签 ==========

    /// 末影人可拾取方块标签
    /// 草方块、泥土、沙子、沙砾、蘑菇、花、仙人掌、南瓜、西瓜、TNT等
    /// 参考 MC 1.16.5: BlockTags.ENDERMAN_HOLDABLE
    static BlockTag& ENDERMAN_HOLDABLE();

    /**
     * @brief 初始化所有内置标签
     *
     * 在 BlockRegistry::initialize() 之后调用
     */
    static void initialize();

    /**
     * @brief 根据ID获取标签
     *
     * @param id 标签资源位置
     * @return 标签指针，如果不存在返回 nullptr
     */
    [[nodiscard]] static BlockTag* getTag(const ResourceLocation& id);

    /**
     * @brief 遍历所有标签
     */
    static void forEachTag(std::function<void(BlockTag&)> callback);

private:
    BlockTags() = delete;

    static std::unordered_map<ResourceLocation, std::unique_ptr<BlockTag>>& getTags();
    static bool s_initialized;
};

} // namespace mc
