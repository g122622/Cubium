#include "PlayerSkinInfo.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include <spdlog/spdlog.h>

namespace mc::skin {

PlayerSkinInfo::PlayerSkinInfo(const GameProfile& profile)
    : m_profile(profile)
{
    // 默认根据 UUID 设置皮肤类型
    m_textures.setSkinType(getDefaultSkinTypeForUUID(profile.uuid()));
}

ResourceLocation PlayerSkinInfo::getSkinLocation() const
{
    if (m_textures.hasSkin()) {
        return m_textures.getSkin().value();
    }
    return getDefaultSkinLocation();
}

std::optional<ResourceLocation> PlayerSkinInfo::getCapeLocation() const
{
    return m_textures.getCape();
}

std::optional<ResourceLocation> PlayerSkinInfo::getElytraLocation() const
{
    return m_textures.getElytra();
}

SkinType PlayerSkinInfo::getSkinType() const
{
    // 如果已加载皮肤，使用皮肤自带的类型
    if (m_textures.hasSkin()) {
        return m_textures.skinType();
    }
    // 否则根据 UUID 确定默认类型
    return getDefaultSkinTypeForUUID(m_profile.uuid());
}

void PlayerSkinInfo::setSkinTextures(const SkinTextures& textures)
{
    m_textures = textures;
}

void PlayerSkinInfo::setSkinLocation(const ResourceLocation& location)
{
    m_textures.setSkin(location);
}

void PlayerSkinInfo::setCapeLocation(const ResourceLocation& location)
{
    m_textures.setCape(location);
}

void PlayerSkinInfo::setElytraLocation(const ResourceLocation& location)
{
    m_textures.setElytra(location);
}

bool PlayerSkinInfo::isWearing(PlayerModelPart part) const
{
    return (m_modelParts & getPlayerModelPartMask(part)) != 0;
}

void PlayerSkinInfo::setModelPartEnabled(PlayerModelPart part, bool enabled)
{
    u8 mask = getPlayerModelPartMask(part);
    if (enabled) {
        m_modelParts |= mask;
    } else {
        m_modelParts &= ~mask;
    }
}

ResourceLocation PlayerSkinInfo::getDefaultSkinLocation() const
{
    SkinType type = getDefaultSkinTypeForUUID(m_profile.uuid());
    if (type == SkinType::Slim) {
        return ResourceLocation("minecraft:textures/entity/alex.png");
    }
    return ResourceLocation("minecraft:textures/entity/steve.png");
}

} // namespace mc::skin
