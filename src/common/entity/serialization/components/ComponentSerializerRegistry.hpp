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

#include "common/core/Result.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <vector>
#include <entt/entt.hpp>

namespace mc {
class Entity;

namespace entity::serialization::components {

/**
 * @brief 组件序列化器注册表
 *
 * 把已 ECS 组件化的实体字段的 NBT 序列化逻辑从 OOP 虚函数链
 * （writeToNBT/addAdditionalSaveData 逐层 super）搬到按组件注册的序列化器。
 * 对齐基岩版 InternalComponentRegistry（mc/world/actor/InternalComponentRegistry.h）：
 * unordered_map<组件名, {save, load, legacy-convert}> 静态注册表，与
 * addAdditionalSaveData 虚函数并存（迁移期形态）。
 *
 * 与基岩版的取舍：
 * - 键用 entt::type_id<T>().hash()（编译期类型安全）而非 HashedString 字符串。
 *   项目单二进制无需跨进程稳定字符串标识，type_info 类型错配编译期即报错。
 * - 去掉 legacy-convert（基岩版处理旧版存档格式转换，项目对齐 Java 版平铺格式无此需求）。
 * - 序列化器是裸函数指针（无状态闭包，零分配）而非 std::function。
 *
 * 注册时机：VanillaEntities::doRegisterAll() 末尾调 registerAll()（已有集中注册点，
 * 避免静态初始化器跨 TU 顺序问题）。registerAll 幂等：m_registered 标志 + 内部 clear
 * 重注册，同 typeId 覆盖而非追加。测试 EntityRegistry::clear() 后重跑 doRegisterAll
 * 会顺带重注册序列化器。
 *
 * 存档格式：保持现状 Java 版平铺格式（Pos/Motion/Health 等直接在根 tag），不走基岩版
 * internalComponents 命名空间隔离。零迁移成本旧存档兼容。
 *
 * 序列化器签名选 Entity& 非 EntityContext&：序列化器必须调 setter（C 类字段 DataParameter
 * 同步副作用是硬约束，绕过 setter 直写组件会丢网络同步）。setter 是 Entity 继承体系成员，
 * 只能经 Entity& 调。经 EntityContext 无法调 setter（不持 Entity 指针）。
 *
 * load 顺序依赖：本批 18 字段间无依赖（Health/Absorption 的 maxHealth 依赖构造期
 * registerAttributes 已就位，非 NBT load 顺序）。Entry 保留 priority 字段为未来扩展
 * （Attributes priority=100 / ActiveEffects priority=200 保证顺序）。loadAll 按 priority
 * 升序遍历，saveAll 无序。
 *
 * 批次6 子目标1（序列化按组件注册）。
 */
class ComponentSerializerRegistry {
public:
    /** 存盘函数：把实体的该组件字段写到 tag */
    using SaveFn = void (*)(const Entity& entity, nbt::tags::compound_tag& tag);

    /** 读盘函数：从 tag 读该组件字段写回实体（经 setter 走 DataParameter 同步） */
    using LoadFn = Result<void> (*)(Entity& entity, const nbt::tags::compound_tag& tag);

    /**
     * @brief 注册组件序列化器
     * @tparam ComponentT 组件类型（仅用其 entt::type_id 作键，不实际访问组件）
     * @param save 存盘函数（nullptr 表示该组件不参与存盘，如 ArrowStateComponent 运行时同步值）
     * @param load 读盘函数（nullptr 表示不参与读盘）
     * @param priority load 顺序优先级（升序遍历，本批全 0；未来 Attributes=100/ActiveEffects=200）
     */
    template <class ComponentT>
    void registerSerializer(SaveFn save, LoadFn load, int priority = 0)
    {
        registerSerializerRaw(entt::type_id<ComponentT>().hash(), save, load, priority);
    }

    /** 进程单例 */
    static ComponentSerializerRegistry& instance();

    /**
     * @brief 注册全部组件序列化器（幂等）
     *
     * 在 VanillaEntities::doRegisterAll() 末尾调用。内部 m_registered 标志保证幂等，
     * 重复调用先 clear 再重注册。同 typeId 覆盖而非追加。
     */
    void registerAll();

    /** 存盘：遍历所有已注册序列化器，把组件字段写到 tag（顺序无关） */
    void saveAll(const Entity& entity, nbt::tags::compound_tag& tag) const;

    /** 读盘：按 priority 升序遍历所有已注册序列化器，从 tag 读组件字段写回实体 */
    Result<void> loadAll(Entity& entity, const nbt::tags::compound_tag& tag) const;

private:
    struct Entry {
        entt::id_type typeId{0};
        SaveFn save{nullptr};
        LoadFn load{nullptr};
        int priority{0};
    };

    void registerSerializerRaw(entt::id_type typeId, SaveFn save, LoadFn load, int priority);

    std::vector<Entry> m_entries;
    bool m_registered{false};
};

} // namespace entity::serialization::components
} // namespace mc
