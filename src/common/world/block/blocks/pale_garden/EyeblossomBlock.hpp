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

#include "../vegetation/FlowerBlock.hpp"
#include "EyeblossomEnvironment.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 眼眸花方块
 *
 * 苍白花园中的特殊花朵，有两种状态：开放（open_eyeblossom）和闭合（closed_eyeblossom）。
 * - 开放状态发光等级为 1，闭合状态不发光
 * - 响应随机刻：主世界昼夜节律下在开/合状态间切换
 * - 切换时生成 TrailParticle 转换粒子、播放长/短音效，并连锁触发周围 3×2×3 范围内同种眼眸花
 * - 蜜蜂接触开放眼眸花会获得 25 tick 中毒效果（仅非和平难度，闭合眼眸花不触发）
 *
 * 状态切换由 EnvironmentAttributes.EYEBLOSSOM_OPEN 环境属性驱动：
 * - 主世界夜晚（dayTimeOfDay ∈ [12600, 23401)）→ 应开放
 * - 主世界白天 → 应闭合
 * - 下界/末地 → DEFAULT（不切换，保持当前状态）
 *
 * 本项目尚未完整实现 EnvironmentAttributes 系统，使用 EyeblossomEnvironment
 * 工具函数近似查询。详见 EyeblossomEnvironment.hpp。
 *
 * MC ID: minecraft:open_eyeblossom, minecraft:closed_eyeblossom
 *
 * 参考: net.minecraft.world.level.block.EyeblossomBlock
 */
class EyeblossomBlock : public FlowerBlock {
public:
    /**
     * @brief 眼眸花类型枚举
     *
     * 对应 MC 1.21.11 EyeblossomBlock.Type，封装开/合状态的全部元数据：
     * - open：是否为开放状态
     * - suspiciousStewEffect：可疑炖菜效果 ID
     * - effectDuration：可疑炖菜效果持续时间（秒）
     * - longSwitchSound：随机刻触发的长音效
     * - shortSwitchSound：连锁 tick 触发的短音效
     * - particleColor：转换粒子的 ARGB 颜色
     */
    enum class Type : u8 {
        /// 开放状态：失明 11 秒，开放长/短音效，淡黄色粒子
        Open,
        /// 闭合状态：反胃 7 秒，闭合长/短音效，深紫色粒子
        Closed,
    };

    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param type 眼眸花类型（开/合）
     * @param suspiciousStewEffect 可疑炖汤效果 ID
     * @param effectDuration 效果持续时间（秒）
     */
    EyeblossomBlock(const BlockProperties& properties, Type type, u32 suspiciousStewEffect = 0, i32 effectDuration = 0);

    ~EyeblossomBlock() noexcept override = default;

    // ========== 光照 ==========

    /**
     * @brief 获取光照等级
     *
     * 开放状态返回 1，闭合状态返回 0。
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    // ========== 随机刻 / 计划刻 ==========

    /**
     * @brief 是否响应随机刻
     *
     * 眼眸花两种状态都响应随机刻，以便在昼夜节律下切换状态。
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    /**
     * @brief 随机刻处理
     *
     * 由服务端随机刻系统调用。检查 EYEBLOSSOM_OPEN 环境属性，
     * 若与当前状态不一致则切换为对应的眼眸花方块，并：
     * 1. 生成 TrailParticle 转换粒子
     * 2. 播放 longSwitchSound（长音效）
     * 3. 连锁触发周围 3×2×3 范围内同种眼眸花方块的延迟 tick
     *
     * 参考: net.minecraft.world.level.block.EyeblossomBlock#randomTick
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 计划刻处理
     *
     * 由连锁触发调度到期后调用。逻辑与 randomTick 相同，但使用
     * shortSwitchSound（短音效）。注意：连锁仅对相同状态的眼眸花方块触发，
     * 这样在原始方块切换后，邻居会在延迟 tick 后跟着切换。
     *
     * 参考: net.minecraft.world.level.block.EyeblossomBlock#tick
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 客户端动画 ==========

    /**
     * @brief 客户端动画 tick
     *
     * 仅在客户端调用，用于环境效果：
     * - 开放状态：1/700 概率在苍白苔藓方块上播放 EYEBLOSSOM_IDLE 环境音
     *
     * 参考: net.minecraft.world.level.block.EyeblossomBlock#animateTick
     */
    void animateTick(IBlockAnimateContext& context,
        const BlockPos& pos,
        const BlockState& state,
        math::IRandom& random) const override;

    // ========== 实体碰撞 ==========

    /**
     * @brief 实体与方块碰撞时调用
     *
     * 蜜蜂接触吸引蜜蜂的花朵（开放眼眸花等）时获得 25 tick 中毒效果（仅非和平难度）。
     * 闭合眼眸花不在 BlockTags::BEE_ATTRACTIVE 标签中，因此不会触发中毒。
     * 已中毒的蜜蜂不会被重复施加效果。
     *
     * 参考: net.minecraft.world.level.block.EyeblossomBlock#entityInside
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    // ========== 类型查询 ==========

    /**
     * @brief 获取眼眸花类型
     */
    [[nodiscard]] Type type() const noexcept { return m_type; }

    /**
     * @brief 是否为开放状态
     */
    [[nodiscard]] bool isOpen() const noexcept { return m_type == Type::Open; }

    /**
     * @brief 获取当前方块对应的"反状态"方块（Open <-> Closed）
     *
     * 用于状态切换：randomTick/tick 检测到环境偏好与当前状态不一致时，
     * 通过此方法获取目标方块的 defaultState 并替换。
     *
     * @return 反状态方块指针；若注册表中找不到则返回 nullptr
     */
    [[nodiscard]] const EyeblossomBlock* transform() const noexcept;

    /**
     * @brief 生成状态切换的转换粒子
     *
     * 在方块中心生成一个 TrailParticle，飞向附近随机偏移的目标位置，
     * 颜色为新状态对应的 particleColor，持续时间为 20 * d0 tick。
     *
     * 服务端通过 addTrailParticle 广播给附近玩家。
     *
     * 参考: net.minecraft.world.level.block.EyeblossomBlock.Type#spawnTransformParticle
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param random 随机数生成器
     * @param newType 切换后的新状态类型（决定粒子颜色）
     */
    static void spawnTransformParticle(IWorld& world, const BlockPos& pos, math::IRandom& random, Type newType);

    /**
     * @brief 获取指定类型对应的 ARGB 粒子颜色
     *
     * - Open：淡黄色 0xFFFCBE22（RGB 252, 190, 34）
     * - Closed：深紫色 0xFF5F498F（RGB 95, 73, 143）
     *
     * MC 1.21.11 中 Type.particleColor 是 int（RGB），ARGB 透明度由调用方控制。
     * 此处返回 ARGB 0xFFRRGGBB 形式（不透明）。
     */
    [[nodiscard]] static constexpr u32 particleColorOf(Type type) noexcept
    {
        switch (type) {
            case Type::Open:
                return 0xFFFCBE22u;
            case Type::Closed:
                return 0xFF5F498Fu;
        }
        return 0xFFFFFFFFu;
    }

    /**
     * @brief 获取指定类型对应的长切换音效（随机刻触发）
     */
    [[nodiscard]] static const ResourceLocation& longSwitchSoundOf(Type type) noexcept;

    /**
     * @brief 获取指定类型对应的短切换音效（连锁 tick 触发）
     */
    [[nodiscard]] static const ResourceLocation& shortSwitchSoundOf(Type type) noexcept;

private:
    /// 眼眸花类型
    Type m_type;

    /**
     * @brief 内部状态切换核心逻辑
     *
     * 检查 EYEBLOSSOM_OPEN 环境属性，若与当前状态不一致则：
     * 1. 切换为对应的眼眸花方块（setBlockState）
     * 2. 触发 BLOCK_CHANGE 游戏事件
     * 3. 生成 TrailParticle 转换粒子
     * 4. 连锁触发周围 3×2×3 范围内同种眼眸花方块的延迟 tick
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param random 随机数生成器
     * @return 是否实际切换了状态
     */
    bool tryChangingState(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random);
};

} // namespace blocks
} // namespace mc
