// Cubium GameTest 脚本绑定扩展声明（module augmentation）。
//
// 背景：Cubium 的 C++ 绑定（ScriptRegistrationBuilderBinding.cpp）在官方
// @minecraft/server-gametest 的 RegistrationBuilder 之上扩展了若干链式方法（如 skyAccess），
// 用于对齐 Java GameTest 的 TestData 字段。同时在 Test 之上扩展了 setBlockWithStates（按 typeId
// + 属性字符串设带 block state 的方块，弥补 setBlockType 只能放默认 state 的不足）。官方类型定义
// （node_modules）未声明这些扩展方法，故在此用 module augmentation 补充类型，避免 TS 编译报 TS2339。
//
// module augmentation 要点：文件须是"模块"（含顶级 export）而非"脚本"，否则
// `declare module` 会被当成该模块的全新声明而遮蔽原有导出（Test/register 等）。
// 末尾 `export {}` 即为此目的。

export {};

declare module "@minecraft/server-gametest" {
    import type { Vector3 } from "@minecraft/server";
    interface RegistrationBuilder {
        /**
         * 声明测试结构需要露天/天空光照进入（对齐 Java GameTest TestData.skyAccess）。
         *
         * Cubium 行为：MinecraftStructurePlacer 在 skyAccess=true 时清空结构 footprint
         * 正上方至世界顶部的方块（gridStartY=-59 把结构埋在地下 worldgen 中，需主动
         * 制造露天列使 canSeeSky=true）。skyAccess 默认 false（封顶隔离光照），不影响现有测试。
         *
         * @param skyAccess 是否露天
         * @returns RegistrationBuilder（链式）
         */
        skyAccess(skyAccess: boolean): RegistrationBuilder;
    }

    interface Test {
        /**
         * 按 typeId + 属性字符串设带 block state 的方块。Cubium 专有方法（官方基岩 BDS Test 无）。
         *
         * 绑定：ScriptTestHelper.cpp 注册，转调 GameTestHelper::setBlockWithStates
         * （GameTestHelper.cpp:346-392）。statesStr 解析为 unordered_map<string,string>，
         * 格式 "prop=value" 或 "p1=v1,p2=v2"（如 "age=3"、"lit=true,candles=4"、"level=1"）。
         * 未知属性名静默忽略（容错）；非法值抛 GameTestError。updateFlags 默认 3
         * （NOTIFY | NEIGHBOR，与 setBlockType flags=3 一致，触发 onBlockAdded + 邻居更新）。
         *
         * 用于放置带特定 state 的方块（点燃熔炉、流动熔岩 level=1、指定 age 作物等），弥补
         * setBlockType 只能放默认 state 的不足。
         *
         * @param blockType 方块 typeId（如 "minecraft:lava"）
         * @param blockLocation 结构相对坐标 {x,y,z}
         * @param statesStr 属性字符串 "p1=v1,p2=v2"
         * @param updateFlags 更新标志位（默认 3 = NOTIFY | NEIGHBOR）
         */
        setBlockWithStates(
            blockType: string,
            blockLocation: Vector3,
            statesStr: string,
            updateFlags?: number,
        ): void;
    }
}
