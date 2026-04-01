#pragma once

#include "EntityRenderer.hpp"
#include "../../../../common/core/Types.hpp"
#include <memory>

namespace mc {

class ItemStack;
struct TextureRegion;

namespace client {
class ItemTextureAtlas;
class ClientEntity;

namespace renderer::trident::gui {
class GuiRenderer;
}
}

namespace client::renderer {

/**
 * @brief ItemEntity 渲染器
 *
 * 渲染掉落在世界中的物品实体。
 * 物品以 3D 方式浮动渲染，具有上下浮动和旋转动画。
 *
 * 参考 MC 1.16.5 ItemEntityRenderer
 */
class ItemEntityRenderer : public EntityRenderer {
public:
    ItemEntityRenderer();
    ~ItemEntityRenderer() override = default;

    // 禁止拷贝
    ItemEntityRenderer(const ItemEntityRenderer&) = delete;
    ItemEntityRenderer& operator=(const ItemEntityRenderer&) = delete;

    /**
     * @brief 渲染 ItemEntity
     * @param entity 实体（必须是 ClientEntity）
     * @param partialTicks 部分 tick
     */
    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 渲染阴影（ItemEntity 通常没有阴影或阴影很小）
     * @param entity 实体
     * @param partialTicks 部分 tick
     */
    void renderShadow(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 设置物品纹理图集
     * @param atlas 物品纹理图集
     */
    void setItemTextureAtlas(ItemTextureAtlas* atlas) { m_itemTextureAtlas = atlas; }

    /**
     * @brief 设置 Gui 渲染器（用于绘制物品图标）
     * @param gui GUI 渲染器
     */
    void setGuiRenderer(trident::gui::GuiRenderer* gui) { m_guiRenderer = gui; }

private:
    /**
     * @brief 计算浮动偏移
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return Y 轴偏移
     */
    [[nodiscard]] f64 calculateBobOffset(u32 ticksExisted, f64 partialTick) const;

    /**
     * @brief 计算旋转角度
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return 旋转角度（度）
     */
    [[nodiscard]] f64 calculateRotation(u32 ticksExisted, f64 partialTick) const;

    /**
     * @brief 获取物品纹理区域
     * @param stack 物品堆
     * @return 纹理区域指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getItemTextureRegion(const ItemStack& stack) const;

    ItemTextureAtlas* m_itemTextureAtlas = nullptr;
    trident::gui::GuiRenderer* m_guiRenderer = nullptr;

    // ItemEntity 动画常量（参考 MC 1.16.5）
    static constexpr f64 BOB_AMPLITUDE = 0.1f;       // 浮动高度
    static constexpr f64 BOB_FREQUENCY = 0.1f;       // 浮动速度（弧度/tick）
    static constexpr f64 ROTATION_SPEED = 2.0f;      // 旋转速度（度/tick）
    static constexpr f64 GROUND_OFFSET = 0.25f;      // 地面高度偏移
    static constexpr f64 ITEM_SIZE = 0.25f;          // 渲染大小
};

} // namespace client::renderer
} // namespace mc
