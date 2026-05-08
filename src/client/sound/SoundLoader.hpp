#pragma once

#include "client/sound/backend/AudioBuffer.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <vector>

namespace mc {

// 前向声明
class ResourcePackList;

namespace client::sound {

/**
 * @brief OGG Vorbis 音频加载器
 *
 * 从资源包加载 OGG Vorbis 音频文件并解码为 PCM 数据。
 * 使用 stb_vorbis 库进行解码。
 *
 * 参考: net.minecraft.client.audio.SoundLoader
 *
 * 使用示例:
 * @code
 * SoundLoader loader(resourcePacks);
 *
 * // 加载音频文件
 * auto result = loader.load(ResourceLocation("minecraft:sounds/dig/stone1"));
 * if (result.success()) {
 *     AudioData& audio = result.value();
 *     // 使用音频数据
 * }
 * @endcode
 *
 * 支持的格式:
 * - OGG Vorbis (.ogg)
 * - 单声道和立体声
 * - 8-bit 和 16-bit 采样
 *
 * @see AudioData
 * @see AudioBufferCache
 */
class SoundLoader {
public:
    /**
     * @brief 构造音频加载器
     *
     * @param resourcePacks 资源包列表
     */
    explicit SoundLoader(ResourcePackList& resourcePacks);

    ~SoundLoader() = default;

    // 禁止拷贝
    SoundLoader(const SoundLoader&) = delete;
    SoundLoader& operator=(const SoundLoader&) = delete;

    // 允许移动
    SoundLoader(SoundLoader&&) noexcept = default;
    SoundLoader& operator=(SoundLoader&&) noexcept = default;

    // ========================================================================
    // 音频加载
    // ========================================================================

    /**
     * @brief 加载音频文件
     *
     * 从资源包加载音频文件并解码为 PCM 数据。
     * 自动添加 .ogg 后缀。
     *
     * @param location 音频文件资源位置（不含 .ogg 后缀）
     * @return 音频数据，或错误
     */
    [[nodiscard]] Result<AudioData> load(const ResourceLocation& location);

    /**
     * @brief 从原始数据解码音频
     *
     * @param data OGG Vorbis 编码的音频数据
     * @param size 数据大小
     * @return 解码后的音频数据，或错误
     */
    [[nodiscard]] static Result<AudioData> decode(const u8* data, size_t size);

    // ========================================================================
    // 辅助方法
    // ========================================================================

    /**
     * @brief 将资源位置转换为音频文件路径
     *
     * 例如: "minecraft:sounds/dig/stone1" -> "assets/minecraft/sounds/dig/stone1.ogg"
     *
     * @param location 资源位置
     * @return 音频文件路径
     */
    [[nodiscard]] static std::string toAudioPath(const ResourceLocation& location);

private:
    ResourcePackList& m_resourcePacks;
};

} // namespace mc::client::sound
} // namespace mc
