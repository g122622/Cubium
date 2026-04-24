#include "ClientSkinManager.hpp"
#include "common/skin/core/GameProfile.hpp"
#include <spdlog/spdlog.h>

namespace mc::client::skin {

ClientSkinManager::ClientSkinManager()
    : m_skinManager(std::make_unique<::mc::skin::SkinManager>("./cache/skins"))
    , m_textureAtlas(std::make_unique<renderer::entity::pipeline::EntityTextureAtlas>()) {
}

ClientSkinManager::~ClientSkinManager() {
    shutdown();
}

Result<void> ClientSkinManager::initialize(VkDevice device,
                                           VkPhysicalDevice physicalDevice,
                                           VkCommandPool commandPool,
                                           VkQueue graphicsQueue,
                                           const String& cacheDir) {
    if (m_initialized) {
        return {};
    }

    m_device = device;

    // 初始化底层皮肤管理器
    m_skinManager = std::make_unique<::mc::skin::SkinManager>(cacheDir);
    auto skinResult = m_skinManager->initialize();
    if (!skinResult.success()) {
        return skinResult.error();
    }

    // 初始化纹理图集
    auto atlasResult = m_textureAtlas->initialize(
        device, physicalDevice, commandPool, graphicsQueue,
        256,  // 最大纹理数
        64    // 纹理尺寸（64x64）
    );
    if (!atlasResult.success()) {
        return atlasResult.error();
    }

    // 加载默认皮肤
    auto defaultResult = loadDefaultSkins();
    if (!defaultResult.success()) {
        spdlog::warn("ClientSkinManager: Failed to load default skins: {}",
                     defaultResult.error().toString());
        // 默认皮肤加载失败不是致命错误
    }

    // 构建图集
    auto buildResult = m_textureAtlas->build();
    if (!buildResult.success()) {
        return buildResult.error();
    }

    m_initialized = true;
    spdlog::info("ClientSkinManager initialized");
    return {};
}

void ClientSkinManager::shutdown() {
    if (!m_initialized) {
        return;
    }

    m_textureAtlas->destroy();

    {
        std::lock_guard<std::mutex> lock(m_regionMutex);
        m_skinRegions.clear();
        m_capeRegions.clear();
        m_elytraRegions.clear();
    }

    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        m_pendingSkins.clear();
    }

    m_skinManager->shutdown();

    m_initialized = false;
    spdlog::info("ClientSkinManager shutdown");
}

Result<ResourceLocation> ClientSkinManager::registerPlayerSkin(const ::mc::skin::GameProfile& profile) {
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "ClientSkinManager not initialized");
    }

    // 获取或创建玩家信息
    auto info = m_skinManager->getOrCreatePlayerInfo(profile);
    if (!info) {
        return Error(ErrorCode::InvalidArgument, "Failed to create player info");
    }

    String key = uuidToKey(profile.uuid());

    // 检查是否已有纹理区域
    {
        std::lock_guard<std::mutex> lock(m_regionMutex);
        auto it = m_skinRegions.find(key);
        if (it != m_skinRegions.end() && it->second) {
            return info->getSkinLocation();
        }
    }

    // 获取皮肤位置
    ResourceLocation location = info->getSkinLocation();

    // 检查是否为默认皮肤
    if (m_skinManager->defaultSkinProvider().isDefaultSkin(location)) {
        // 使用默认皮肤的纹理区域
        if (info->getSkinType() == ::mc::skin::SkinType::Slim) {
            return m_alexRegion ? ResourceLocation("minecraft:textures/entity/alex.png")
                               : location;
        } else {
            return m_steveRegion ? ResourceLocation("minecraft:textures/entity/steve.png")
                               : location;
        }
    }

    // TODO: 实现从缓存或下载加载皮肤并上传到 GPU
    // 当前简化实现：返回位置但不实际上传

    spdlog::debug("ClientSkinManager: Registered skin for {} at {}",
                  profile.name(), location.toString());

    return location;
}

const TextureRegion* ClientSkinManager::getSkinRegion(const std::array<u8, 16>& uuid) const {
    String key = uuidToKey(uuid);

    {
        std::lock_guard<std::mutex> lock(m_regionMutex);
        auto it = m_skinRegions.find(key);
        if (it != m_skinRegions.end() && it->second) {
            return it->second;
        }
    }

    // 返回默认皮肤
    auto info = m_skinManager->getPlayerInfo(uuid);
    if (info && info->getSkinType() == ::mc::skin::SkinType::Slim) {
        return m_alexRegion;
    }
    return m_steveRegion;
}

const TextureRegion* ClientSkinManager::getCapeRegion(const std::array<u8, 16>& uuid) const {
    String key = uuidToKey(uuid);

    std::lock_guard<std::mutex> lock(m_regionMutex);
    auto it = m_capeRegions.find(key);
    return it != m_capeRegions.end() ? it->second : nullptr;
}

const TextureRegion* ClientSkinManager::getElytraRegion(const std::array<u8, 16>& uuid) const {
    String key = uuidToKey(uuid);

    std::lock_guard<std::mutex> lock(m_regionMutex);
    auto it = m_elytraRegions.find(key);
    return it != m_elytraRegions.end() ? it->second : nullptr;
}

::mc::skin::SkinType ClientSkinManager::getSkinType(const std::array<u8, 16>& uuid) const {
    auto info = m_skinManager->getPlayerInfo(uuid);
    if (info) {
        return info->getSkinType();
    }
    return m_skinManager->getDefaultSkinType(uuid);
}

std::shared_ptr<::mc::skin::PlayerSkinInfo> ClientSkinManager::getPlayerInfo(
    const std::array<u8, 16>& uuid) const {
    return m_skinManager->getPlayerInfo(uuid);
}

Result<void> ClientSkinManager::rebuildAtlas() {
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "ClientSkinManager not initialized");
    }

    // 重新构建纹理图集
    // 注意：这会销毁现有的图集并重新构建

    auto buildResult = m_textureAtlas->build();
    if (!buildResult.success()) {
        return buildResult.error();
    }

    // 更新纹理区域引用
    // TODO: 重新获取所有玩家的纹理区域

    spdlog::info("ClientSkinManager: Rebuilt texture atlas");
    return {};
}

Result<void> ClientSkinManager::loadDefaultSkins() {
    // 加载 Steve 皮肤
    ResourceLocation steveLocation("minecraft:textures/entity/steve.png");

    // 从默认皮肤提供者获取数据
    const auto& steveData = m_skinManager->defaultSkinProvider().getSteveSkinData();
    if (!steveData.empty()) {
        auto result = m_textureAtlas->addTextureFromFile(
            "resources/assets/minecraft/textures/entity/steve.png",
            steveLocation
        );
        if (result.success()) {
            // 稍后在 build() 后获取区域
            spdlog::debug("ClientSkinManager: Added Steve skin to atlas");
        }
    }

    // 加载 Alex 皮肤
    ResourceLocation alexLocation("minecraft:textures/entity/alex.png");

    const auto& alexData = m_skinManager->defaultSkinProvider().getAlexSkinData();
    if (!alexData.empty()) {
        auto result = m_textureAtlas->addTextureFromFile(
            "resources/assets/minecraft/textures/entity/alex.png",
            alexLocation
        );
        if (result.success()) {
            spdlog::debug("ClientSkinManager: Added Alex skin to atlas");
        }
    }

    return {};
}

Result<ResourceLocation> ClientSkinManager::uploadSkinToAtlas(
    const std::vector<u8>& pngData,
    const ResourceLocation& preferredLocation) {

    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "ClientSkinManager not initialized");
    }

    // 添加纹理到图集
    // 注意：当前 EntityTextureAtlas 需要从文件加载
    // 这里需要扩展 API 支持从内存加载

    // TODO: 扩展 EntityTextureAtlas 支持从内存数据添加纹理
    spdlog::warn("ClientSkinManager: uploadSkinToAtlas not fully implemented");

    return preferredLocation;
}

String ClientSkinManager::uuidToKey(const std::array<u8, 16>& uuid) {
    return ::mc::skin::GameProfile(uuid, "").uuidToStringNoDashes();
}

} // namespace mc::client::skin
