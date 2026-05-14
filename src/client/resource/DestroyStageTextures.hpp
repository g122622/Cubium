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
#include <array>
#include <memory>
#include <vector>

namespace mc {

// 前向声明
class ResourceManager;
struct DecodedTexture;

namespace client {
namespace renderer {

/**
 * @brief 破坏阶段纹理资源管理器
 *
 * 加载和管理10个破坏阶段纹理（destroy_stage_0 ~ destroy_stage_9）。
 * 这些纹理用于在方块挖掘过程中显示逐渐碎裂的效果。
 *
 * 纹理采用叠加混合模式（DST_COLOR * SRC_COLOR），
 * 与原方块颜色相乘形成破坏效果。
 *
 * 参考 MC 1.16.5 ModelBakery.DESTROY_STAGES
 */
class DestroyStageTextures {
public:
    /// 破坏阶段数量（0-9，共10个阶段）
    static constexpr size_t STAGE_COUNT = 10;

    /// 纹理尺寸（假设为正方形）
    static constexpr u32 TEXTURE_SIZE = 16;

    /**
     * @brief 获取单例实例
     */
    static DestroyStageTextures& instance();

    /**
     * @brief 初始化纹理资源
     *
     * 加载所有破坏阶段纹理。如果资源包中没有纹理，
     * 则使用内置的默认纹理（程序生成）。
     *
     * @return 成功返回 true，失败返回 false
     */
    [[nodiscard]] bool initialize();

    /**
     * @brief 初始化纹理资源（带 ResourceManager）
     *
     * 尝试从 ResourceManager 加载纹理，失败则使用程序生成纹理。
     *
     * @param resourceManager 资源管理器指针
     * @return 成功返回 true，失败返回 false
     */
    [[nodiscard]] bool initialize(ResourceManager* resourceManager);

    /**
     * @brief 清理资源
     */
    void cleanup();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取指定阶段的纹理数据
     *
     * @param stage 破坏阶段 (0-9)
     * @return 纹理RGBA数据指针，无效阶段返回 nullptr
     */
    [[nodiscard]] const u8* getTextureData(size_t stage) const;

    /**
     * @brief 获取指定阶段纹理的UV坐标
     *
     * 如果使用纹理数组，返回层索引；
     * 如果使用纹理图集，返回UV区域。
     *
     * @param stage 破坏阶段 (0-9)
     * @param u0 输出：左U坐标
     * @param v0 输出：上V坐标
     * @param u1 输出：右U坐标
     * @param v1 输出：下V坐标
     * @return 成功返回 true
     */
    [[nodiscard]] bool getTextureUV(size_t stage, f32& u0, f32& v0, f32& u1, f32& v1) const;

    /**
     * @brief 获取所有纹理合并后的图集数据
     *
     * 将10个阶段纹理合并到一个大纹理中，方便GPU上传。
     * 图集布局：2行5列，每个纹理16x16像素。
     *
     * @return 图集RGBA数据
     */
    [[nodiscard]] const std::vector<u8>& getAtlasData() const { return m_atlasData; }

    /**
     * @brief 获取图集宽度
     */
    [[nodiscard]] u32 atlasWidth() const { return TEXTURE_SIZE * 5; }

    /**
     * @brief 获取图集高度
     */
    [[nodiscard]] u32 atlasHeight() const { return TEXTURE_SIZE * 2; }

private:
    DestroyStageTextures() = default;
    ~DestroyStageTextures() = default;

    // 禁止拷贝
    DestroyStageTextures(const DestroyStageTextures&) = delete;
    DestroyStageTextures& operator=(const DestroyStageTextures&) = delete;

    /**
     * @brief 生成默认的破坏阶段纹理
     *
     * 当资源包中没有破坏纹理时，使用程序生成的纹理。
     * 生成的纹理模拟逐渐碎裂的效果。
     *
     * @param stage 破坏阶段 (0-9)
     * @param data 输出纹理数据（RGBA，16x16）
     */
    void generateDefaultTexture(size_t stage, std::vector<u8>& data);

    /**
     * @brief 从资源包加载纹理
     *
     * @param resourceManager 资源管理器指针
     * @param stage 破坏阶段 (0-9)
     * @param data 输出纹理数据
     * @return 成功返回 true
     */
    [[nodiscard]] bool loadTextureFromResourcePack(
        ResourceManager* resourceManager, size_t stage, std::vector<u8>& data);

    /**
     * @brief 构建纹理图集
     */
    void buildAtlas();

    /// 各阶段纹理数据（RGBA格式）
    std::array<std::vector<u8>, STAGE_COUNT> m_textures;

    /// 合并后的纹理图集数据
    std::vector<u8> m_atlasData;

    /// 是否已初始化
    bool m_initialized = false;
};

} // namespace renderer
} // namespace client
} // namespace mc
