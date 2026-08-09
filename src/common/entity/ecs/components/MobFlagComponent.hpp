#pragma once

namespace mc::ecs {

/**
 * @brief 敌对生物标记组件（tag component）
 *
 * 承载 IMob 接口的类型标记语义。对齐基岩版混合架构：IMob 接口保留作 OOP
 * 行为层（虚析构不删，MonsterEntity 仍 public IMob），本组件作 ECS 类型
 * 标记层。MonsterEntity 构造时 attach，所有怪物子类（Zombie/Skeleton/
 * Creeper/Shulker 等 20+ 叶子）经 MonsterEntity 间接获得此 tag。
 *
 * 空结构体，仅用于 hasComponent<MobFlagComponent>() 类型判定，无数据字段。
 * 生产代码 5 处 dynamic_cast<IMob*> 改 hasComponent<MobFlagComponent>()，
 * 消除 AI 热路径 RTTI 开销（entt all_of 编译期类型 id 比较替代 dynamic_cast
 * 字符串比较）。
 *
 * 批次5 子批 5.1 试点（IMob→tag component），验证「接口保留 + tag 并存」
 * 的混合架构模式。
 */
struct MobFlagComponent {};

} // namespace mc::ecs
