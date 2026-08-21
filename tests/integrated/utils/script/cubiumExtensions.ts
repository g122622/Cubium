// Cubium 脚本扩展方法的类型断言辅助。
//
// 背景：Cubium 在 @minecraft/server 的 Dimension/Entity（官方为 class）之上扩展了
// 世界状态读取方法（绑定于 MinecraftModuleFactory.cpp），用于解锁 TimeCommand/
// WeatherCommand/GameRuleCommand/TagCommand 的端到端 GameTest 断言。但官方类型定义里
// Dimension/Entity 是 class，TS 不允许 class 被 module-augmentation 的 interface 合并，
// 故无法用 augment.d.ts 声明这些扩展方法。改用本地接口 + as 断言绕过类型检查
// （运行时 Cubium opaque 绑定真实存在这些方法）。
//
// 各方法对应 C++ 绑定（MinecraftModuleFactory.cpp dimensionReg/entityReg）：
//   Dimension.getTimeOfDay()  -> IWorld::dayTimeOfDay() (% 24000)
//   Dimension.getDayTime()    -> IWorld::dayTime() (原始累计)
//   Dimension.isRaining()     -> IWorld::isRaining() (rainStrength > 0.2)
//   Dimension.isThundering()  -> IWorld::isThundering() (thunderStrength > 0.9)
//   Dimension.getGameRule(name) -> GameRules::getValueAsString (Cubium 专有)
//   Entity.getTags()/hasTag/addTag/removeTag -> Entity 基类 m_tags（对齐基岩）

export interface CubiumDimension {
    getTimeOfDay(): number;
    getDayTime(): number;
    isRaining(): boolean;
    isThundering(): boolean;
    getGameRule(ruleName: string): string;
}

export interface CubiumEntity {
    getTags(): string[];
    hasTag(tag: string): boolean;
    addTag(tag: string): boolean;
    removeTag(tag: string): boolean;
}

/** 将官方 Dimension 断言为含 Cubium 扩展方法的类型。 */
export function asDim(dim: unknown): CubiumDimension {
    return dim as CubiumDimension;
}

/** 将官方 Entity（含 Player/SimulatedPlayer）断言为含 Cubium 扩展方法的类型。 */
export function asEnt(ent: unknown): CubiumEntity {
    return ent as CubiumEntity;
}
