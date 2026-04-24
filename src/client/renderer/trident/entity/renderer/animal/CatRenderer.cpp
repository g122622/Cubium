#include "CatRenderer.hpp"
#include "common/entity/entities/passive/tamable/CatEntity.hpp"

namespace mc::client::renderer::entity::renderer::animal {

using mc::CatEntity;

namespace {
    // MC 1.16.5 猫类型纹理
    // 0: Tabby, 1: Black, 2: Red, 3: Siamese, 4: British Shorthair
    // 5: Calico, 6: Persian, 7: Ragdoll, 8: White, 9: Jellie, 10: Black (All Black)
    const char* CAT_TEXTURES[11] = {
        "textures/entity/cat/tabby.png",
        "textures/entity/cat/black.png",
        "textures/entity/cat/red.png",
        "textures/entity/cat/siamese.png",
        "textures/entity/cat/british_shorthair.png",
        "textures/entity/cat/calico.png",
        "textures/entity/cat/persian.png",
        "textures/entity/cat/ragdoll.png",
        "textures/entity/cat/white.png",
        "textures/entity/cat/jellie.png",
        "textures/entity/cat/all_black.png"
    };
}

CatRenderer::CatRenderer()
    : m_model(0.0f)
    , m_modelBaby(0.0f)
{
    setShadowSize(0.4f);
}

void CatRenderer::render(Entity& entity, f64 partialTicks) {
    auto& cat = static_cast<CatEntity&>(entity);

    // 选择模型（幼体或成体）
    bool isChild = cat.isChild();
    auto& model = isChild ? m_modelBaby : m_model;

    // 设置状态
    // TODO: 从实体获取躺下、放松、睡眠动画状态
    model.setCatAnimState(0.0f, 0.0f, 0.0f);
    model.setSitting(cat.isSitting());

    // 计算动画参数
    f64 limbSwing = 0.0;
    f64 limbSwingAmount = 0.0;
    f64 ageInTicks = static_cast<f64>(cat.ticksExisted());
    f64 headYaw = 0.0;
    f64 headPitch = 0.0;
    f64 scale = isChild ? 0.5 : 1.0;

    model.setAngles(limbSwing, limbSwingAmount, ageInTicks, headYaw, headPitch, scale);
    model.render(scale / 16.0);

    // 渲染阴影
    if (m_shadowSize > 0.0) {
        renderShadow(entity, partialTicks);
    }
}

ResourceLocation CatRenderer::getEntityTexture(CatEntity& entity) {
    // TODO: 从实体获取猫类型
    u32 catType = 0;
    (void)entity;
    return getCatTexture(catType);
}

ResourceLocation CatRenderer::getEntityTexture(const CatEntity& entity) const {
    u32 catType = 0;
    (void)entity;
    return getCatTexture(catType);
}

ResourceLocation CatRenderer::getCatTexture(u32 catType) {
    if (catType >= 11) {
        catType = 0;
    }
    return ResourceLocation("minecraft", CAT_TEXTURES[catType]);
}

void registerCatRenderer(EntityRendererManager& manager) {
    manager.registerRenderer("minecraft:cat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<CatRenderer>();
    });
}

} // namespace mc::client::renderer::entity::renderer::animal
