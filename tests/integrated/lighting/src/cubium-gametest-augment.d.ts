// Cubium GameTest 脚本绑定扩展声明（module augmentation）。
//
// 背景：Cubium 的 C++ 绑定在官方 @minecraft/server-gametest 的 RegistrationBuilder 之上扩展了
// skyAccess 链式方法（对齐 Java GameTest TestData.skyAccess）；同时在 @minecraft/server 的 Block
// 之上扩展了光照查询只读属性（blockLight/skyLight/brightness/canSeeSky，官方 Block 无光照 API）。
// 这些扩展用于光照集成测试直接读取光照数值。官方类型定义（node_modules）未声明这些扩展，
// 故在此用 module augmentation 补充类型，避免 TS 编译报 TS2339。
//
// module augmentation 要点：文件须是"模块"（含顶级 export）而非"脚本"，否则
// `declare module` 会被当成该模块的全新声明而遮蔽原有导出。末尾 `export {}` 即为此目的。

export {};

declare module "@minecraft/server-gametest" {
    import type { Vector3 } from "@minecraft/server";
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
         * 按 typeId + 属性字符串设带 block state 的方块。Cubium 专有方法（官方基岩 BDS Test 无）。
         *
         * 绑定：ScriptTestHelper.cpp:264 注册，转调 GameTestHelper::setBlockWithStates
         * （GameTestHelper.cpp:346-380）。statesStr 解析为 unordered_map<string,string>，
         * 格式 "prop=value" 或 "p1=v1,p2=v2"（如 "age=3"、"lit=true,candles=4"）。
         * 未知属性名静默忽略（容错）；非法值抛 GameTestError。updateFlags 默认 3
         * （NOTIFY | NEIGHBOR，与 setBlockType flags=3 一致，触发 onBlockAdded + 邻居更新）。
         *
         * 用于放置带特定 state 的方块（点燃熔炉、满能量重生锚、4根蜡烛、下半砖、含水树叶等），
         * 弥补 setBlockType 只能放默认 state 的不足。
         *
         * @param blockType 方块 typeId（如 "minecraft:furnace"）
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

declare module "@minecraft/server" {
    interface Block {
        /**
         * 方块光等级 (0-15)。Cubium 专有扩展（官方 @minecraft/server Block 无光照 API）。
         * 读 IWorld::getBlockLight，原始方块光等级，不含天气/时间衰减。
         * 主线程 visible 侧无锁安全（GameTest 回调在服务端主线程 post-tick 执行）。
         */
        readonly blockLight: number;
        /**
         * 天空光等级 (0-15)。Cubium 专有扩展。原始天空光等级，不含天气/时间衰减。
         * 需 skyAccess(true) 结构露天才能达 15，否则被 worldgen 遮挡。
         */
        readonly skyLight: number;
        /**
         * 综合亮度等级 (0-15)，含天空减暗。对齐 Java Level.getLight(BlockPos)。Cubium 专有扩展。
         * 白天露天处为 15，夜间/阴影处降低，用于作物生长等判定。
         */
        readonly brightness: number;
        /**
         * 是否可见天空（skyLight >= 15，无天空光维度恒 false）。Cubium 专有扩展。
         */
        readonly canSeeSky: boolean;
    }
}
