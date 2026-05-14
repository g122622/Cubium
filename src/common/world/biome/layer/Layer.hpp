#pragma once

#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc {

/**
 * @brief 像素变换器函数类型
 *
 * 用于延迟计算区域的像素值计算。
 */
using PixelFunc = std::function<i32(i32 x, i32 z)>;

// Forward declarations
class IArea;
class IAreaContext;

// ============================================================================
// IArea - 区域接口
// ============================================================================

/**
 * @brief 区域接口
 *
 * 表示一个可以被采样的区域，返回指定位置的整数值。
 * 用于生物群系生成中的层叠处理。
 *
 * 参考 MC 1.16.5 IArea / LazyArea
 */
class IArea {
public:
    virtual ~IArea() = default;

    /**
     * @brief 采样指定位置的值
     * @param x X 坐标
     * @param z Z 坐标
     * @return 该位置的值
     */
    [[nodiscard]] virtual i32 getValue(i32 x, i32 z) const = 0;

    /**
     * @brief 批量采样多个位置的值
     *
     * 默认实现通过循环调用 getValue()，子类可以覆盖以优化性能。
     * 批量采样可以减少锁竞争和提高缓存命中率。
     *
     * @param startX 起始 X 坐标
     * @param startZ 起始 Z 坐标
     * @param width 宽度（X 方向）
     * @param height 高度（Z 方向）
     * @param output 输出数组（大小必须 >= width * height）
     */
    virtual void getValuesBatch(i32 startX, i32 startZ, i32 width, i32 height, i32* output) const
    {
        // 默认实现：循环调用 getValue
        if (output == nullptr || width <= 0 || height <= 0) {
            return;
        }
        size_t idx = 0;
        for (i32 z = 0; z < height; ++z) {
            for (i32 x = 0; x < width; ++x) {
                output[idx++] = getValue(startX + x, startZ + z);
            }
        }
    }
};

// ============================================================================
// IAreaContext - 区域上下文
// ============================================================================

/**
 * @brief 区域上下文接口
 *
 * 提供区域采样的上下文信息，包括位置感知的随机数生成器。
 * 参考 MC INoiseRandom
 */
class IAreaContext {
public:
    virtual ~IAreaContext() = default;

    /**
     * @brief 设置当前位置（必须在采样前调用）
     * @param x X 坐标
     * @param z Z 坐标
     */
    virtual void setPosition(i64 x, i64 z) = 0;

    /**
     * @brief 获取随机整数
     * @param bound 上界
     * @return [0, bound) 范围内的随机整数
     */
    [[nodiscard]] virtual i32 nextInt(i32 bound) = 0;

    /**
     * @brief 从两个值中随机选择
     * @param a 第一个值
     * @param b 第二个值
     * @return 随机选择的值
     */
    [[nodiscard]] virtual i32 pickRandom(i32 a, i32 b) = 0;

    /**
     * @brief 从四个值中随机选择
     * @param a 第一个值
     * @param b 第二个值
     * @param c 第三个值
     * @param d 第四个值
     * @return 随机选择的值
     */
    [[nodiscard]] virtual i32 pickRandom(i32 a, i32 b, i32 c, i32 d) = 0;

    /**
     * @brief 获取噪声生成器（用于 OceanLayer）
     * @return Perlin 噪声生成器的引用
     */
    [[nodiscard]] virtual class ImprovedNoiseGenerator* getNoiseGenerator() = 0;
};

// ============================================================================
// IExtendedAreaContext - 扩展区域上下文
// ============================================================================

/**
 * @brief 扩展区域上下文接口
 *
 * 扩展 IAreaContext，支持创建延迟计算区域。
 * 参考 MC IExtendedNoiseRandom
 */
class IExtendedAreaContext : public IAreaContext, public std::enable_shared_from_this<IExtendedAreaContext> {
public:
    /**
     * @brief 创建延迟计算区域
     * @param pixelFunc 像素计算函数
     * @return 延迟区域
     */
    [[nodiscard]] virtual std::unique_ptr<IArea> makeArea(PixelFunc pixelFunc) = 0;

    /**
     * @brief 创建延迟计算区域（单输入，持有所有权）
     * @param pixelFunc 像素计算函数
     * @param input 输入区域（将被持有）
     * @return 延迟区域
     */
    [[nodiscard]] virtual std::unique_ptr<IArea> makeArea(PixelFunc pixelFunc, std::unique_ptr<IArea> input) = 0;

    /**
     * @brief 创建延迟计算区域（双输入，持有所有权）
     * @param pixelFunc 像素计算函数
     * @param input1 第一个输入区域（将被持有）
     * @param input2 第二个输入区域（将被持有）
     * @return 延迟区域
     */
    [[nodiscard]] virtual std::unique_ptr<IArea> makeArea(
        PixelFunc pixelFunc, std::unique_ptr<IArea> input1, std::unique_ptr<IArea> input2) = 0;
};

// ============================================================================
// ITransformer0 - 零输入变换器
// ============================================================================

/**
 * @brief 零输入变换器接口
 *
 * 从无生成数据，用于初始层（如 IslandLayer, OceanLayer）。
 * 参考 MC IAreaTransformer0
 */
class ITransformer0 {
public:
    virtual ~ITransformer0() = default;

    /**
     * @brief 此变换器是否依赖位置随机数
     *
     * 返回 false 时，工厂将跳过 setPosition 调用，减少热路径开销。
     */
    [[nodiscard]] virtual bool usesRandom() const { return true; }

    /**
     * @brief 在指定位置生成值
     * @param ctx 区域上下文
     * @param x X 坐标
     * @param z Z 坐标
     * @return 生成的值
     */
    [[nodiscard]] virtual i32 apply(IAreaContext& ctx, i32 x, i32 z) = 0;

    /**
     * @brief 创建区域工厂
     * @param context 扩展上下文
     * @return 区域工厂
     */
    [[nodiscard]] virtual std::unique_ptr<class IAreaFactory> apply(IExtendedAreaContext& context) = 0;
};

// ============================================================================
// ITransformer1 - 单输入变换器
// ============================================================================

/**
 * @brief 单输入变换器接口
 *
 * 变换一个输入区域。
 * 参考 MC IAreaTransformer1
 */
class ITransformer1 {
public:
    virtual ~ITransformer1() = default;

    /**
     * @brief 此变换器是否依赖位置随机数
     *
     * 返回 false 时，工厂将跳过 setPosition 调用，减少热路径开销。
     */
    [[nodiscard]] virtual bool usesRandom() const { return true; }

    /**
     * @brief 应用变换
     * @param ctx 区域上下文
     * @param area 输入区域
     * @param x X 坐标
     * @param z Z 坐标
     * @return 变换后的值
     */
    [[nodiscard]] virtual i32 apply(IAreaContext& ctx, const IArea& area, i32 x, i32 z) = 0;

    /**
     * @brief 获取 X 偏移量（用于采样）
     * @param x 原始 X 坐标
     * @return 偏移后的 X 坐标
     */
    [[nodiscard]] virtual i32 getOffsetX(i32 x) const { return x; }

    /**
     * @brief 获取 Z 偏移量（用于采样）
     * @param z 原始 Z 坐标
     * @return 偏移后的 Z 坐标
     */
    [[nodiscard]] virtual i32 getOffsetZ(i32 z) const { return z; }

    /**
     * @brief 创建区域工厂
     * @param context 扩展上下文
     * @param input 输入工厂
     * @return 区域工厂
     */
    [[nodiscard]] virtual std::unique_ptr<class IAreaFactory> apply(
        IExtendedAreaContext& context, std::unique_ptr<class IAreaFactory> input) = 0;
};

// ============================================================================
// ITransformer2 - 双输入变换器
// ============================================================================

/**
 * @brief 双输入变换器接口
 *
 * 合并两个输入区域。
 * 参考 MC IAreaTransformer2
 */
class ITransformer2 {
public:
    virtual ~ITransformer2() = default;

    /**
     * @brief 此变换器是否依赖位置随机数
     *
     * 返回 false 时，工厂将跳过 setPosition 调用，减少热路径开销。
     */
    [[nodiscard]] virtual bool usesRandom() const { return true; }

    /**
     * @brief 应用变换
     * @param ctx 区域上下文
     * @param area1 第一个输入区域
     * @param area2 第二个输入区域
     * @param x X 坐标
     * @param z Z 坐标
     * @return 变换后的值
     */
    [[nodiscard]] virtual i32 apply(IAreaContext& ctx, const IArea& area1, const IArea& area2, i32 x, i32 z) = 0;

    /**
     * @brief 获取 X 偏移量（用于采样）
     * @param x 原始 X 坐标
     * @return 偏移后的 X 坐标
     */
    [[nodiscard]] virtual i32 getOffsetX(i32 x) const { return x; }

    /**
     * @brief 获取 Z 偏移量（用于采样）
     * @param z 原始 Z 坐标
     * @return 偏移后的 Z 坐标
     */
    [[nodiscard]] virtual i32 getOffsetZ(i32 z) const { return z; }

    /**
     * @brief 创建区域工厂
     * @param context 扩展上下文
     * @param input1 第一个输入工厂
     * @param input2 第二个输入工厂
     * @return 区域工厂
     */
    [[nodiscard]] virtual std::unique_ptr<class IAreaFactory> apply(IExtendedAreaContext& context,
        std::unique_ptr<class IAreaFactory> input1,
        std::unique_ptr<class IAreaFactory> input2) = 0;
};

// ============================================================================
// IAreaFactory - 区域工厂接口
// ============================================================================

/**
 * @brief 区域工厂接口
 *
 * 创建区域对象的工厂，支持层的链式组合。
 */
class IAreaFactory {
public:
    virtual ~IAreaFactory() = default;

    /**
     * @brief 创建区域
     * @return 区域对象
     */
    [[nodiscard]] virtual std::unique_ptr<IArea> create() const = 0;
};

// ============================================================================
// SharedFactory - 共享工厂包装器
// ============================================================================

/**
 * @brief 共享工厂包装器
 *
 * 包装一个 IAreaFactory 在 shared_ptr 中，允许多个消费者共享同一个工厂。
 * 这对于河流生成分支架构是必需的，因为 MC 的层链需要将同一个工厂传递给多个下游分支。
 *
 * 用法示例：
 * @code
 * auto climateFactory = std::make_shared<SharedFactory>(std::move(factory));
 * // 河流分支
 * auto riverBranch = riverLayer.apply(ctx, climateFactory->clone());
 * // 生物群系分支
 * auto biomeBranch = biomeLayer.apply(ctx, climateFactory->clone());
 * @endcode
 */
class SharedFactory : public IAreaFactory {
public:
    /**
     * @brief 构造共享工厂
     * @param factory 要共享的工厂
     */
    explicit SharedFactory(std::unique_ptr<IAreaFactory> factory)
        : m_factory(std::move(factory))
    {}

    /**
     * @brief 创建区域（调用底层工厂的 create）
     */
    [[nodiscard]] std::unique_ptr<IArea> create() const override { return m_factory->create(); }

    /**
     * @brief 创建一个引用相同底层工厂的新 SharedFactory
     *
     * 这允许在需要 unique_ptr 的 API 中共享工厂。
     * 注意：返回的工厂与原始工厂共享相同的底层工厂实例。
     */
    [[nodiscard]] std::unique_ptr<SharedFactory> share() const
    {
        // 创建一个新的 SharedFactory，共享同一个底层工厂
        // 这里使用 const_cast 是安全的，因为我们只需要读取访问
        return std::unique_ptr<SharedFactory>(new SharedFactory(m_factory));
    }

private:
    // 私有构造函数，用于 share() 方法
    explicit SharedFactory(std::shared_ptr<IAreaFactory> factory)
        : m_factory(std::move(factory))
    {}

    std::shared_ptr<IAreaFactory> m_factory;
};

// ============================================================================
// IDimOffset0Transformer - 无偏移变换器特征
// ============================================================================

/**
 * @brief 无偏移变换器特征
 *
 * 采样坐标无偏移。
 * 参考 MC IDimOffset0Transformer
 */
class IDimOffset0Transformer {
public:
    [[nodiscard]] i32 getOffsetX(i32 x) const { return x; }
    [[nodiscard]] i32 getOffsetZ(i32 z) const { return z; }
};

// ============================================================================
// IDimOffset1Transformer - +1偏移变换器特征
// ============================================================================

/**
 * @brief +1偏移变换器特征
 *
 * 采样坐标偏移 +1。
 * 参考 MC IDimOffset1Transformer
 */
class IDimOffset1Transformer {
public:
    [[nodiscard]] i32 getOffsetX(i32 x) const { return x + 1; }
    [[nodiscard]] i32 getOffsetZ(i32 z) const { return z + 1; }
};

// ============================================================================
// IPixelTransformer - 像素变换器（遗留兼容）
// ============================================================================

/**
 * @brief 像素变换器接口
 *
 * 直接变换单个像素值。
 * 参考 MC IPixelTransformer
 */
class IPixelTransformer {
public:
    virtual ~IPixelTransformer() = default;

    /**
     * @brief 变换单个像素
     * @param x X 坐标
     * @param z Z 坐标
     * @return 变换后的值
     */
    [[nodiscard]] virtual i32 apply(i32 x, i32 z) = 0;
};

} // namespace mc
