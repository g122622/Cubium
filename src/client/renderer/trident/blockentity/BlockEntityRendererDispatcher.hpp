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

#include "IBlockEntityRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

namespace mc {

class IWorld;
class BlockEntity;
class MatrixStack;

namespace client::renderer::trident {

class TridentContext;
class TridentTextureAtlas;

namespace resource {
class BlockModelCache;
}

namespace blockentity {

/**
 * @brief 方块实体渲染器调度器
 *
 * 管理所有方块实体渲染器，根据方块实体类型分派渲染。
 *
 * 使用方式：
 * 1. 在客户端初始化时注册渲染器
 * 2. 在渲染循环中调用 render() 渲染方块实体
 */
class BlockEntityRendererDispatcher {
public:
    using RendererFactory = std::function<std::unique_ptr<BlockEntityRendererBase>()>;

    BlockEntityRendererDispatcher();
    ~BlockEntityRendererDispatcher();

    // 禁止拷贝
    BlockEntityRendererDispatcher(const BlockEntityRendererDispatcher&) = delete;
    BlockEntityRendererDispatcher& operator=(const BlockEntityRendererDispatcher&) = delete;

    // 允许移动
    BlockEntityRendererDispatcher(BlockEntityRendererDispatcher&&) noexcept;
    BlockEntityRendererDispatcher& operator=(BlockEntityRendererDispatcher&&) noexcept;

    // ========== 渲染器注册 ==========

    /**
     * @brief 注册方块实体渲染器
     *
     * @tparam TEntity 方块实体类型
     * @tparam TRenderer 渲染器类型
     */
    template <typename TEntity, typename TRenderer>
    void registerRenderer()
    {
        static_assert(std::is_base_of_v<BlockEntityRenderer<TEntity>, TRenderer>,
            "TRenderer must inherit from BlockEntityRenderer<TEntity>");

        auto renderer = std::make_unique<TRenderer>();
        m_renderers[TEntity::getType()] = std::move(renderer);
    }

    /**
     * @brief 注册方块实体渲染器工厂
     *
     * @param type 方块实体类型
     * @param factory 渲染器工厂函数
     */
    void registerRenderer(BlockEntityType type, RendererFactory factory);

    // ========== 渲染 ==========

    /**
     * @brief 渲染方块实体
     *
     * @param entity 方块实体引用
     * @param partialTick 部分tick（用于插值）
     * @param light 组合光照
     * @param gameTime 游戏时间（总 tick 数），用于驱动动画
     * @return 是否成功渲染
     *
     * TODO: 当前 BlockEntityRendererDispatcher 尚未集成到主渲染循环（TridentEngine::render()），
     * 调用方需在集成时将 TridentEngine::m_gameTime 或 ClientWorld::gameTime() 作为 gameTime
     * 参数传入，以确保旗帜飘动、信标光束旋转等动画正常运行。
     * 集成点建议：在 TridentEngine::render() 中区块渲染之后、实体渲染回调之前，遍历可见
     * 区块中的方块实体并调用此方法。
     */
    bool render(BlockEntity& entity, f32 partialTick, u32 light, i64 gameTime);

    /**
     * @brief 渲染所有全局方块实体
     *
     * 全局方块实体（如信标光束）可以在远距离看到。
     *
     * @param world 世界引用
     * @param partialTick 部分tick
     */
    void renderGlobalBlockEntities(IWorld& world, f32 partialTick);

    // ========== 资源设置 ==========

    /**
     * @brief 设置Trident上下文
     */
    void setContext(TridentContext* context) { m_context = context; }

    /**
     * @brief 设置纹理图集
     */
    void setTextureAtlas(TridentTextureAtlas* atlas) { m_textureAtlas = atlas; }

    /**
     * @brief 设置模型缓存
     */
    void setModelCache(resource::BlockModelCache* cache) { m_modelCache = cache; }

    // ========== 初始化 ==========

    /**
     * @brief 初始化默认渲染器
     *
     * 注册所有原版方块实体渲染器。
     */
    void initializeDefaults();

    // ========== 查询 ==========

    /**
     * @brief 获取渲染器
     * @param type 方块实体类型
     * @return 渲染器指针，如果未注册返回nullptr
     */
    [[nodiscard]] BlockEntityRendererBase* getRenderer(BlockEntityType type);

    /**
     * @brief 检查是否有渲染器
     */
    [[nodiscard]] bool hasRenderer(BlockEntityType type) const;

    /**
     * @brief 清除所有渲染器
     */
    void clear();

private:
    std::unordered_map<BlockEntityType, std::unique_ptr<BlockEntityRendererBase>> m_renderers;

    TridentContext* m_context = nullptr;
    TridentTextureAtlas* m_textureAtlas = nullptr;
    resource::BlockModelCache* m_modelCache = nullptr;
};

} // namespace blockentity
} // namespace client::renderer::trident
} // namespace mc
