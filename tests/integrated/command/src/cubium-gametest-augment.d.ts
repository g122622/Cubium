// Cubium GameTest 脚本绑定扩展声明（module augmentation）。
//
// 背景：Cubium 的 C++ 绑定（ScriptRegistrationBuilderBinding.cpp）在官方
// @minecraft/server-gametest 的 RegistrationBuilder/Test 之上扩展了若干专有方法
// （如 skyAccess、runOnFinish），用于对齐 Java GameTest 的 TestData 字段或提供 Cubium
// 独有的世界状态操作。官方类型定义（node_modules）未声明这些扩展方法，故在此用 module
// augmentation 补充类型，避免 TS 编译报 TS2339。
//
// 注意：仅能 augmentation @minecraft/server-gametest 的 interface（RegistrationBuilder/Test
// 是 interface，可合并）。@minecraft/server 的 Dimension/Entity 是 class，TS 不允许 class
// 被 interface augmentation 合并，故 Cubium 在 Dimension/Entity 上扩展的 getTimeOfDay/
// isRaining/getGameRule/getTags 等方法改在测试文件内用类型断言（as any）调用，不在此声明。
//
// module augmentation 要点：文件须是"模块"（含顶级 export）而非"脚本"，否则
// `declare module` 会被当成该模块的全新声明而遮蔽原有导出（Test/register 等）。
// 末尾 `export {}` 即为此目的。
//
// 本声明与 mob_behavior 包的 cubium-gametest-augment.d.ts 内容一致，各包独立携带
// 以满足 TS 编译的模块解析（每包独立 tsconfig）。

export {};

declare module "@minecraft/server-gametest" {
    interface RegistrationBuilder {
        /**
         * 声明测试结构需要露天/天空光照进入（对齐 Java GameTest TestData.skyAccess）。
         *
         * Cubium 行为：MinecraftStructurePlacer 在 skyAccess=true 时清空结构 footprint
         * 正上方至世界顶部的所有方块（gridStartY=-59 把结构埋在地下 worldgen 中，需主动
         * 制造露天列使 canSeeSky=true）。skyAccess 默认 false（封顶隔离光照），不影响现有测试。
         *
         * @param skyAccess 是否露天
         * @returns RegistrationBuilder（链式）
         */
        skyAccess(skyAccess: boolean): RegistrationBuilder;
    }

    interface Test {
        /**
         * 注册测试结束回调（项目独有，非基岩/Java GameTest API）。
         *
         * Cubium 行为：回调在测试进入终态（PASSED/FAILED/TIMEOUT）时触发（见 BaseGameTestInstance.cpp
         * 的 finish 路径），早于 GameTestHelper 析构（批次结束才 clear 实例），此时
         * SimulatedPlayer 等实体仍存活，回调内可调 player.chat 等恢复世界级状态（如 gamerule）。
         *
         * 用途：GameTest 共享单一 ServerWorld，世界级状态（gamerule doMobSpawning / doDaylightCycle
         * / difficulty 等）跨测试/跨批次持久化、框架不自动重置。测试临时改世界级状态后须用
         * runOnFinish 恢复，避免污染后续依赖该状态的测试（全量跑共享同一 ServerWorld）。
         * 回调可注册多个，按注册顺序执行。
         *
         * @param callback 测试结束时执行的回调（无参无返回值）
         */
        runOnFinish(callback: () => void): void;
    }
}
