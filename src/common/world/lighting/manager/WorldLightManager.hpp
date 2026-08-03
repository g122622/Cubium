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

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/world/lighting/engine/BaseLightEngine.hpp"
#include "common/world/lighting/engine/BlockLightEngine.hpp"
#include "common/world/lighting/engine/SkyLightEngine.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include <string>

namespace mc {

/**
 * @brief 世界光照管理器
 *
 * 协调方块光照和天空光照引擎，提供主线程无锁读路径与 TLS 引擎池。
 * 根据维度配置，可能只有方块光照（如下界）或两者都有（如主世界）。
 *
 * 引擎调用模型（对齐 Moonrise StarLightInterface）：
 * 引擎实例无跨 operation 持久状态——nibble/emptiness map 全挂 IChunk，
 * 引擎只有 per-op 缓存（setupCaches 建/destroyCaches 清）+ 构造后不变的
 * 配置常量。故引擎实例存 thread_local 池，每个 worker 线程独占一套，
 * acquire/release 配对使用，无引擎级锁。所有光照写操作（区块加载光照、
 * 运行时方块变更、LIGHT 生成阶段）统一经 UniversalWorkerPool 区域互斥池
 * （writeRadius=2）串行——重叠 5×5 区域的 nibble 写必被区域锁串行，
 * 满足 SWMRNibbleArray 更新侧非原子单写者语义。
 */
class WorldLightManager {
public:
    /**
     * @brief 构造函数
     *
     * @param provider 区块光照提供者
     * @param hasBlockLight 是否有方块光照
     * @param hasSkyLight 是否有天空光照
     */
    WorldLightManager(StarLightLightingProvider* provider, bool hasBlockLight, bool hasSkyLight);

    // ========================================================================
    // TLS 引擎池
    // ========================================================================

    /**
     * @brief 获取当前线程的天空光引擎（TLS）
     *
     * 首次调用惰性构造，之后该线程复用同一实例。对齐 Moonrise
     * StarLightInterface 的 thread_local 引擎池——每个 worker 线程独占
     * 一套引擎，无引擎级锁，区域锁保证 nibble 写串行。
     * 仅当 hasSkyLight() 为 true 时调用方才应获取。
     */
    [[nodiscard]] static SkyStarLightEngine* acquireSkyLightEngine();

    /**
     * @brief 释放天空光引擎（TLS，no-op）
     *
     * 对齐 Moonrise try/finally 签名，引擎留在线程局部存储复用，
     * 线程退出时 unique_ptr 析构释放。调用方须在 acquire 后用 try/finally
     * 风格配对调用，确保异常路径也不遗漏（当前无异常，仍保留对称性）。
     */
    static void releaseSkyLightEngine(SkyStarLightEngine* engine) noexcept;

    /**
     * @brief 获取当前线程的方块光引擎（TLS）
     * @see acquireSkyLightEngine
     */
    [[nodiscard]] static BlockStarLightEngine* acquireBlockLightEngine();

    /**
     * @brief 释放方块光引擎（TLS，no-op）
     * @see releaseSkyLightEngine
     */
    static void releaseBlockLightEngine(BlockStarLightEngine* engine) noexcept;

    // ========================================================================
    // 维度配置访问
    // ========================================================================

    [[nodiscard]] bool hasBlockLight() const noexcept { return m_hasBlockLight; }
    [[nodiscard]] bool hasSkyLight() const noexcept { return m_hasSkyLight; }

    // ========================================================================
    // 主线程读路径（visible 侧，无锁）
    // ========================================================================

    /**
     * @brief 获取指定位置的实际亮度
     *
     * 主线程读路径：直接从区块 nibble 数组的 visible 侧（atomic acquire）读取，
     * 不经光照引擎、不持锁。worker 传播时写 updating 侧经区域锁串行，主线程读
     * visible 侧原子安全（SWMRNibbleArray 双缓冲语义）。
     *
     * @param pos 方块位置
     * @param skyDarkening 天空减暗因子（0-15）
     * @return 亮度值 (0-15)
     */
    [[nodiscard]] i32 getLightSubtracted(const BlockPos& pos, i32 skyDarkening) const;

    /**
     * @brief 获取方块光照等级（主线程读 visible 侧）
     *
     * 经 provider 取已加载区块，直接读区块 blockNibbles 的 visible 侧。
     * 区块未加载或段无数据时返回 0。
     */
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取天空光照等级（主线程读 visible 侧）
     *
     * 经 provider 取已加载区块，直接读区块 skyNibbles 的 visible 侧。
     * 区块未加载或段无数据时返回 15（天空光默认满亮，对齐引擎 getLightLevel 默认值）。
     */
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取光照数据
     *
     * 主线程读路径：经 provider 取已加载区块，直接返回区块上对应段的
     * SWMRNibbleArray 指针（指向 visible 侧数据，toByteArray 读 atomic）。
     * 不经引擎缓存、不持锁。
     *
     * @param type 光照类型
     * @param pos 区块段位置
     * @return 光照数组指针，如果区块未加载或段越界返回 nullptr
     */
    [[nodiscard]] SWMRNibbleArray* getData(LightType type, const SectionPos& pos);

    /**
     * @brief 获取调试信息
     *
     * 主线程读 visible 侧状态查询（isNullVisible/isInitializedVisible 等），
     * 反映已发布到客户端的光照数据状态，而非引擎 updating 侧的中间态。
     *
     * @param type 光照类型
     * @param pos 区块段位置
     * @return 调试字符串
     */
    [[nodiscard]] std::string getDebugInfo(LightType type, const SectionPos& pos) const;

private:
    StarLightLightingProvider* m_provider;
    bool m_hasBlockLight;
    bool m_hasSkyLight;

    /// 最小光照段坐标（= m_minSection - 1 = (minBuildHeight >> SECTION_SHIFT) - 1），
    /// 用于主线程读 nibble 数组索引：nibbles[sectionY - m_minLightSection]。
    /// 与引擎 BaseLightEngine::m_minLightSection 同源，构造时从 provider 维度信息计算。
    i32 m_minLightSection;

    /// 最大光照段坐标（= m_maxSection + 1 = ((maxBuildHeight - 1) >> SECTION_SHIFT) + 1），
    /// 用于主线程读天空光时向上回溯非 null nibble 的上界。
    i32 m_maxLightSection;

    /// 最小/最大方块段坐标（建筑高度对应的段范围），用于天空光 null nibble 时
    /// 经 emptiness map 查找最低非空段。对齐 Moonrise minSection/maxSection。
    i32 m_minSection;
    i32 m_maxSection;

    /// 主线程读辅助：经 provider 取已加载区块上指定段的 nibble（visible 侧数据）。
    /// 不持锁——读 visible 侧 atomic 安全。区块未加载或段越界返回 nullptr。
    [[nodiscard]] SWMRNibbleArray* _getNibble(LightType type, const SectionPos& pos) const;

    /// 主线程读辅助：对齐 Moonrise StarLightInterface.getSkyLightValue。
    /// null nibble 时经 emptiness map 判断该段是否在最低非空段之上（之上天空光满亮 15），
    /// 否则向上回溯找到首个非 null nibble 取其该列天空光。区块未加载返回 15。
    [[nodiscard]] u8 _getSkyLightValue(i32 x, i32 y, i32 z) const;
};

} // namespace mc
