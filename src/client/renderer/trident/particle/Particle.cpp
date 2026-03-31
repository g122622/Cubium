#include "Particle.hpp"
#include "ParticleTextureAtlas.hpp"
#include "../../../../common/util/math/MathUtils.hpp"
#include <algorithm>

// 前置声明 - 避免包含 ClientWorld.hpp
namespace mc::client::world {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle {

// 默认纹理位置
static const ResourceLocation DEFAULT_TEXTURE("minecraft:particle/generic");

Particle::Particle(const glm::vec3& pos, const glm::vec3& velocity)
    : m_position(pos)
    , m_prevPosition(pos)
    , m_velocity(velocity)
    , m_bboxMin(pos - glm::vec3(0.01f))
    , m_bboxMax(pos + glm::vec3(0.01f))
{
}

void Particle::tick(ClientWorld* world) {
    // 保存上一帧位置和旋转
    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    // 年龄增加
    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力（MC 的重力系数约为 0.04 blocks/tick²）
    m_velocity.y -= m_gravity * 0.04f;

    // 移动并碰撞检测
    if (m_hasPhysics && world != nullptr) {
        move(world, m_velocity);
    } else {
        m_position += m_velocity;
        m_onGround = false;
    }

    // 应用空气阻力
    m_velocity *= m_friction;

    // 地面摩擦
    if (m_onGround) {
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 根据年龄淡出
    if (m_age > m_maxAge * 0.5f) {
        f32 fadeProgress = (m_age - m_maxAge * 0.5f) / (m_maxAge * 0.5f);
        m_color.a = 1.0f - fadeProgress;
    }
}

void Particle::buildVertices(
    const glm::vec3& cameraPos,
    f32 partialTick,
    const ParticleTextureAtlas& atlas,
    std::vector<ParticleVertex>& outVertices) const
{
    // 插值位置
    glm::vec3 interpPos = m_prevPosition + (m_position - m_prevPosition) * partialTick;

    // 插值旋转
    f32 interpRoll = m_prevRoll + (m_roll - m_prevRoll) * partialTick;

    // 计算 billboard 基向量
    glm::vec3 toCamera = cameraPos - interpPos;
    f32 dist = glm::length(toCamera);
    if (dist < 0.001f) {
        return;  // 太近了，跳过
    }
    toCamera = glm::normalize(toCamera);

    // 计算右向量和上向量（camera-facing billboard）
    glm::vec3 right = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), toCamera);
    if (glm::length(right) < 0.001f) {
        // 相机在正上方或正下方
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        right = glm::normalize(right);
    }
    glm::vec3 up = glm::cross(toCamera, right);

    // 应用旋转
    if (std::abs(interpRoll) > 0.001f) {
        f32 cosR = std::cos(interpRoll);
        f32 sinR = std::sin(interpRoll);
        glm::vec3 newRight = right * cosR + up * sinR;
        glm::vec3 newUp = -right * sinR + up * cosR;
        right = newRight;
        up = newUp;
    }

    // 获取缩放
    f32 scale = getScale(partialTick);
    f32 halfSize = m_size * scale * 0.5f;

    // 获取 UV 坐标
    glm::vec4 uv(0.0f, 0.0f, 1.0f, 1.0f);  // 默认 UV
    const SpriteInfo* sprite = atlas.getSprite(getTextureLocation());
    if (sprite != nullptr) {
        // 对于动画精灵，基于年龄选择帧
        if (sprite->isAnimated()) {
            uv = atlas.getAnimatedFrameUV(getTextureLocation(), m_age, m_maxAge);
        } else {
            uv = glm::vec4(sprite->uvMin.x, sprite->uvMin.y, sprite->uvMax.x, sprite->uvMax.y);
        }
    }

    // 四个顶点（quad）
    // 左下
    outVertices.push_back({
        interpPos - right * halfSize - up * halfSize,
        glm::vec2(uv.x, uv.w),  // UV: 左下
        m_color,
        m_size * scale,
        m_color.a
    });
    // 右下
    outVertices.push_back({
        interpPos + right * halfSize - up * halfSize,
        glm::vec2(uv.z, uv.w),  // UV: 右下
        m_color,
        m_size * scale,
        m_color.a
    });
    // 右上
    outVertices.push_back({
        interpPos + right * halfSize + up * halfSize,
        glm::vec2(uv.z, uv.y),  // UV: 右上
        m_color,
        m_size * scale,
        m_color.a
    });
    // 左上
    outVertices.push_back({
        interpPos - right * halfSize + up * halfSize,
        glm::vec2(uv.x, uv.y),  // UV: 左上
        m_color,
        m_size * scale,
        m_color.a
    });
}

ResourceLocation Particle::getTextureLocation() const {
    return DEFAULT_TEXTURE;
}

u32 Particle::getLightColor(ClientWorld* world) const {
    // 默认实现：从世界采样光照
    // 如果 world 为 nullptr，返回最大亮度
    if (world == nullptr) {
        return 0xF0;  // 最大天空光和方块光
    }

    // TODO: 从 ClientWorld 采样光照值
    // 需要 ClientWorld 提供 getCombinedLight(BlockPos) 方法
    // 当前返回默认亮度
    return 0xF0;
}

f32 Particle::getScale(f32 partialTick) const {
    // 默认实现：返回 1.0（无缩放）
    // 子类可以重写以实现缩放动画
    return 1.0f;
}

void Particle::move(ClientWorld* world, const glm::vec3& delta) {
    // 简化版碰撞检测
    // 完整实现需要与世界进行 AABB 碰撞检测

    // 当前只进行简单的地面检测
    // TODO: 实现完整的碰撞检测

    glm::vec3 actualDelta = delta;

    // 如果 world 不为空，进行简单的地面检测
    if (world != nullptr) {
        // TODO: 从世界获取方块高度
        // 当前简单检测：如果 Y 低于某个值，认为在地面
        if (m_position.y + delta.y < 0.0f) {
            actualDelta.y = -m_position.y;
            m_onGround = true;
            m_velocity.y = 0.0f;
        } else {
            m_onGround = false;
        }
    } else {
        m_onGround = false;
    }

    // 应用移动
    m_position += actualDelta;

    // 更新碰撞盒
    m_bboxMin = m_position - glm::vec3(m_bboxWidth * 0.5f, 0.0f, m_bboxWidth * 0.5f);
    m_bboxMax = m_position + glm::vec3(m_bboxWidth * 0.5f, m_bboxHeight, m_bboxHeight * 0.5f);
}

void Particle::setBoundingBox(f32 width, f32 height) {
    m_bboxWidth = width;
    m_bboxHeight = height;
    m_bboxMin = m_position - glm::vec3(width * 0.5f, 0.0f, width * 0.5f);
    m_bboxMax = m_position + glm::vec3(width * 0.5f, height, width * 0.5f);
}

} // namespace mc::client::renderer::trident::particle
