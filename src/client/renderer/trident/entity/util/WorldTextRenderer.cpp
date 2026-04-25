#include "WorldTextRenderer.hpp"
#include "../pipeline/EntityPipeline.hpp"
#include "../model/core/ModelVertex.hpp"
#include "common/util/math/Matrix4.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::util {

// 静态成员初始化
bool WorldTextRenderer::s_initialized = false;
f32 WorldTextRenderer::s_maxDistance = DEFAULT_MAX_DISTANCE;
f32 WorldTextRenderer::s_scale = DEFAULT_SCALE;
bool WorldTextRenderer::s_showBackground = true;
u8 WorldTextRenderer::s_bgColorR = 0;
u8 WorldTextRenderer::s_bgColorG = 0;
u8 WorldTextRenderer::s_bgColorB = 0;
u8 WorldTextRenderer::s_bgColorA = 128;
Vector3d WorldTextRenderer::s_cameraPosition(0.0, 0.0, 0.0);
std::array<f64, 16> WorldTextRenderer::s_viewMatrix = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
};
std::unordered_map<char, GlyphMesh> WorldTextRenderer::s_glyphCache;
pipeline::EntityMesh* WorldTextRenderer::s_backgroundMesh = nullptr;

bool WorldTextRenderer::initialize(pipeline::EntityPipeline& pipeline) {
    if (s_initialized) {
        return true;
    }

    // 创建简单的 ASCII 字形网格
    // 实际实现需要字体纹理图集，这里创建简单的占位符
    for (char c = ' '; c <= '~'; ++c) {
        GlyphMesh mesh = createGlyphMesh(c, 1.0f);
        s_glyphCache[c] = std::move(mesh);
    }

    // 创建背景网格
    createBackgroundMesh(1.0f, 1.0f);

    s_initialized = true;
    spdlog::info("WorldTextRenderer: Initialized successfully with {} glyphs",
                 s_glyphCache.size());

    (void)pipeline;  // 暂时不使用
    return true;
}

void WorldTextRenderer::cleanup() {
    s_glyphCache.clear();
    s_backgroundMesh = nullptr;  // 网格由 EntityPipeline 管理
    s_initialized = false;
    spdlog::info("WorldTextRenderer: Cleaned up");
}

bool WorldTextRenderer::isInitialized() {
    return s_initialized;
}

void WorldTextRenderer::setCameraPosition(const Vector3d& position) {
    s_cameraPosition = position;
}

void WorldTextRenderer::setViewMatrix(const std::array<f64, 16>& viewMatrix) {
    s_viewMatrix = viewMatrix;
}

void WorldTextRenderer::renderText(
    VkCommandBuffer cmd,
    const String& text,
    const Vector3f& position,
    f32 scale,
    const Vector4f& color,
    bool showBackground,
    pipeline::EntityPipeline& pipeline)
{
    if (!s_initialized || text.empty()) {
        return;
    }

    // 计算到相机的距离
    Vector3f cameraPosF(
        static_cast<f32>(s_cameraPosition.x),
        static_cast<f32>(s_cameraPosition.y),
        static_cast<f32>(s_cameraPosition.z)
    );
    Vector3f toCamera = cameraPosF - position;
    f32 distance = toCamera.length();

    // 距离检查
    if (!shouldRenderText(position, distance)) {
        return;
    }

    // 计算缩放（距离越近越大）
    f32 effectiveScale = s_scale;
    if (distance < 10.0f) {
        effectiveScale *= 10.0f / distance;
    }

    // 应用用户指定的缩放
    effectiveScale *= scale;

    // 计算 billboard 矩阵
    std::array<f64, 16> billboardMatrix;
    computeBillboardMatrix(position, billboardMatrix);

    // 计算文本宽度
    f32 textWidth = calculateTextWidth(text, effectiveScale);
    f32 textHeight = CHAR_HEIGHT * effectiveScale;

    // 渲染背景
    if (showBackground) {
        f32 bgWidth = textWidth + BACKGROUND_PADDING * 2.0f * effectiveScale;
        f32 bgHeight = textHeight + BACKGROUND_PADDING * effectiveScale;
        Vector4f bgColor(
            static_cast<f32>(s_bgColorR) / 255.0f,
            static_cast<f32>(s_bgColorG) / 255.0f,
            static_cast<f32>(s_bgColorB) / 255.0f,
            static_cast<f32>(s_bgColorA) / 255.0f
        );
        // TODO: 渲染背景面板
        (void)bgWidth;
        (void)bgHeight;
        (void)bgColor;
    }

    // 渲染文本字符
    f32 cursorX = -textWidth * 0.5f;  // 居中起始位置

    for (char c : text) {
        auto it = s_glyphCache.find(c);
        if (it == s_glyphCache.end()) {
            // 未找到字形，跳过或使用空格
            cursorX += CHAR_WIDTH * effectiveScale;
            continue;
        }

        const GlyphMesh& glyph = it->second;

        // 创建变换矩阵（应用 billboard + 字符位置偏移）
        std::array<f64, 16> charMatrix = billboardMatrix;

        // 应用字符位置偏移
        charMatrix[3] += static_cast<f64>(cursorX);
        charMatrix[7] += static_cast<f64>(-textHeight * 0.5f);  // 垂直居中

        // TODO: 实际绘制字符网格
        // 当前作为占位符，需要字体纹理图集才能完整实现

        cursorX += glyph.advanceX * effectiveScale;

        (void)cmd;
        (void)pipeline;
        (void)color;
        (void)charMatrix;
    }

    spdlog::trace("WorldTextRenderer: Would render '{}' at ({}, {}, {})",
                  text, position.x, position.y, position.z);
}

void WorldTextRenderer::renderNameTag(
    VkCommandBuffer cmd,
    const String& name,
    const Vector3f& entityPosition,
    f32 entityHeight,
    pipeline::EntityPipeline& pipeline)
{
    // 计算名称标签位置（实体头顶上方）
    Vector3f tagPos = entityPosition;
    tagPos.y += entityHeight + HEIGHT_OFFSET;

    // 渲染文本
    renderText(
        cmd,
        name,
        tagPos,
        1.0f,  // 使用默认缩放
        Vector4f(1.0f, 1.0f, 1.0f, 1.0f),  // 白色文本
        s_showBackground,
        pipeline
    );
}

void WorldTextRenderer::setMaxDistance(f32 distance) {
    s_maxDistance = distance;
}

f32 WorldTextRenderer::maxDistance() {
    return s_maxDistance;
}

void WorldTextRenderer::setBackgroundColor(u8 r, u8 g, u8 b, u8 a) {
    s_bgColorR = r;
    s_bgColorG = g;
    s_bgColorB = b;
    s_bgColorA = a;
}

GlyphMesh WorldTextRenderer::createGlyphMesh(char c, f32 size) {
    GlyphMesh mesh;

    // 创建简单的四边形表示字符
    // 实际实现需要从字体纹理图集获取 UV 坐标
    f32 halfSize = size * 0.5f;

    // ASCII 字符的简单 UV 映射（16x8 网格）
    i32 charIndex = static_cast<i32>(c);
    f32 u0 = static_cast<f32>((charIndex % 16) / 16.0f);
    f32 v0 = static_cast<f32>((charIndex / 16) / 8.0f);
    f32 u1 = u0 + 1.0f / 16.0f;
    f32 v1 = v0 + 1.0f / 8.0f;

    // 四个顶点（正面）
    mesh.vertices = {
        // 位置 xyz, 纹理 uv, 法线 xyz, 颜色 rgba
        -halfSize, -halfSize, 0.0f,  u0, v1,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,
         halfSize, -halfSize, 0.0f,  u1, v1,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,
         halfSize,  halfSize, 0.0f,  u1, v0,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,
        -halfSize,  halfSize, 0.0f,  u0, v0,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,
    };

    mesh.indices = {
        0, 1, 2,  0, 2, 3  // 两个三角形
    };

    mesh.advanceX = size * CHAR_WIDTH;
    mesh.width = size;
    mesh.height = size;

    return mesh;
}

void WorldTextRenderer::createBackgroundMesh(f32 width, f32 height) {
    // 创建背景四边形
    f32 halfWidth = width * 0.5f;
    f32 halfHeight = height * 0.5f;

    // 背景使用统一的 UV
    std::vector<f32> vertices = {
        // 位置 xyz, 纹理 uv, 法线 xyz, 颜色 rgba
        -halfWidth, -halfHeight, 0.0f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f, 0.5f,
         halfWidth, -halfHeight, 0.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f, 0.5f,
         halfWidth,  halfHeight, 0.0f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f, 0.5f,
        -halfWidth,  halfHeight, 0.0f,  0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f, 0.5f,
    };

    std::vector<u32> indices = {
        0, 1, 2,  0, 2, 3
    };

    // TODO: 创建 GPU 网格
    (void)vertices;
    (void)indices;
}

void WorldTextRenderer::computeBillboardMatrix(
    const Vector3f& position,
    std::array<f64, 16>& outMatrix)
{
    // 参考 MC 1.16.5: 名称标签始终面向相机（billboard）
    // 从视图矩阵中提取旋转部分，然后反转

    // 提取视图矩阵的上方向和右方向
    f64 view00 = s_viewMatrix[0];
    f64 view01 = s_viewMatrix[1];
    f64 view02 = s_viewMatrix[2];
    f64 view10 = s_viewMatrix[4];
    f64 view11 = s_viewMatrix[5];
    f64 view12 = s_viewMatrix[6];
    f64 view20 = s_viewMatrix[8];
    f64 view21 = s_viewMatrix[9];
    f64 view22 = s_viewMatrix[10];

    // billboard 矩阵是视图矩阵旋转部分的转置（即逆）
    outMatrix[0] = view00;
    outMatrix[1] = view10;
    outMatrix[2] = view20;
    outMatrix[3] = static_cast<f64>(position.x);
    outMatrix[4] = view01;
    outMatrix[5] = view11;
    outMatrix[6] = view21;
    outMatrix[7] = static_cast<f64>(position.y);
    outMatrix[8] = view02;
    outMatrix[9] = view12;
    outMatrix[10] = view22;
    outMatrix[11] = static_cast<f64>(position.z);
    outMatrix[12] = 0.0;
    outMatrix[13] = 0.0;
    outMatrix[14] = 0.0;
    outMatrix[15] = 1.0;
}

f32 WorldTextRenderer::calculateTextWidth(const String& text, f32 scale) {
    f32 width = 0.0f;
    for (char c : text) {
        auto it = s_glyphCache.find(c);
        if (it != s_glyphCache.end()) {
            width += it->second.advanceX * scale;
        } else {
            width += CHAR_WIDTH * scale;
        }
    }
    return width;
}

bool WorldTextRenderer::shouldRenderText(
    const Vector3f& position,
    f32 distance)
{
    (void)position;

    // 距离检查
    if (distance > s_maxDistance) {
        return false;
    }

    // TODO: 视锥体剔除
    // TODO: 背面剔除（如果玩家背对文本）

    return true;
}

} // namespace mc::client::renderer::entity::util
