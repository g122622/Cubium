// Cubium GameTest 脚本绑定扩展声明（module augmentation）。
//
// 背景：Cubium 的 C++ 绑定（ScriptRegistrationBuilderBinding.cpp）在官方
// @minecraft/server-gametest 的 RegistrationBuilder 之上扩展了若干链式方法（如 skyAccess），
// 用于对齐 Java GameTest 的 TestData 字段。官方类型定义（node_modules）未声明这些扩展方法，
// 故在此用 module augmentation 补充类型，避免 TS 编译报 TS2339。
//
// module augmentation 要点：文件须是"模块"（含顶级 export）而非"脚本"，否则
// `declare module` 会被当成该模块的全新声明而遮蔽原有导出（Test/register 等）。
// 末尾 `export {}` 即为此目的。

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

        /**
         * 声明测试需强制加载结构周围 SPAWN_DISTANCE_CHUNK(8) 区块（项目独有，非 Java/基岩字段）。
         *
         * Cubium 行为：GameTestServer 无头门面无玩家区块加载链路（SimulatedPlayer::spawn 不走
         * 真实玩家 chunk loading），NaturalSpawner._collectSpawnableChunks 仅数到结构 footprint 区块
         * （3 个），cap=maxInstances*3/289=0 致 activeCategories.empty() 早退，怪物/动物都不自然生成。
         * loadSpawnChunks=true 让 MinecraftStructurePlacer force 结构中心周围 8 区块（满载 289），
         * 使 spawnableChunkCount 对齐原版 DistanceManager.getNaturalSpawnChunkCount。
         *
         * 仅 NaturalSpawner 类测试启用。副作用：force 的区块 worldgen 出真实地形，NaturalSpawner 会在
         * 结构外黑暗洞穴自然生成怪物残留世界——故须独立 batch（避免与全维度 getEntities 查询测试并行
         * 污染）+ 区域限定计数隔离。loadSpawnChunks 默认 false，不影响现有测试。
         *
         * @param loadSpawnChunks 是否强制加载结构周围刷怪区块
         * @returns RegistrationBuilder（链式）
         */
        loadSpawnChunks(loadSpawnChunks: boolean): RegistrationBuilder;
    }

    interface Test {
        /**
         * 对单个精确坐标执行一次 NaturalSpawner 自然生成判定（项目独有，非基岩/Java GameTest API）。
         *
         * 对齐 Java @VisibleForDebug NaturalSpawner.spawnCategoryForPosition(MobCategory, ServerLevel,
         * BlockPos)（vanilla /debugmobspawning 命令背后的调试单点入口）。绕过 tick 的随机区块/区块内
         * 选址，使 GameTest 能对精确坐标做一次完整条件检查 + 生成——消除小结构 footprint 命中率极低
         * 的随机性，确定性验证光照门槛/距离门控/玩家门控等 NaturalSpawner 核心条件。
         *
         * 语义：不做 cap/SpawnCosts 检查（对齐 vanilla 3 参版恒真 predicate），仅测条件判定 + 生成。
         * 内部对 pos 做 ±5 抖动（对齐 vanilla 外层 3 轮），y 用传入坐标（不重算 heightmap），故 pos
         * 须为合法 air 生成位（调用方用结构内 air 腔坐标）。
         *
         * 前置条件（任一不满足返回 0）：世界中存在玩家、抖动位距最近玩家 24-128 格、biome SpawnEntry
         * 池非空且抽中、通过光照/放置/碰撞检查。
         *
         * @param category vanilla MobCategory 名："monster"/"creature"/"ambient" 等
         * @param pos 种子位（须为合法 air 生成位，结构内 air 腔坐标）
         * @param biome 可选，强制用该 biome 取 SpawnEntry（绕过 pos 真实 biome）。测试专用——
         *   GameTest 结构固定放世界原点，原点 biome 由世界种子决定不可控（默认 seed=0 原点是
         *   ColdOcean，无陆地动物 SpawnEntry），注入 biome 让测试稳定验证"plains 能生成动物"等条件。
         *   取 vanilla biome 名："plains"/"forest"/"desert"/"swamp"/"river"/"ocean"/"cold_ocean" 等。
         *   省略则用 pos 所在 chunk 真实 biome（对齐 vanilla）。
         * @returns 实际生成实体数（0 表示本次未生成）
         */
        spawnNaturalAt(category: string, pos: import("@minecraft/server").Vector3, biome?: string): number;

        /**
         * 注册测试结束回调（项目独有，非基岩/Java GameTest API）。
         *
         * Cubium 行为：回调在测试进入终态（PASSED/FAILED/TIMEOUT）时触发（见 BaseGameTestInstance.cpp
         * 147-151,163-167 的 finish 路径），早于 GameTestHelper 析构（批次结束才 clear 实例），此时
         * SimulatedPlayer 等实体仍存活，回调内可调 player.chat 等恢复世界级状态（如 gamerule）。
         *
         * 用途：GameTest 共享单一 ServerWorld，世界级状态（gamerule doMobSpawning 等）跨测试/跨批次
         * 持久化、框架不自动重置。测试临时改 gamerule 后须用 runOnFinish 恢复，避免污染后续依赖该
         * 规则的测试。回调可注册多个，按注册顺序执行。
         *
         * @param callback 测试结束时执行的回调（无参无返回值）
         */
        runOnFinish(callback: () => void): void;

        /**
         * 按 typeId + 属性字符串设带 block state 的方块。Cubium 专有方法（官方基岩 BDS Test 无）。
         *
         * 绑定：ScriptTestHelper.cpp 注册，转调 GameTestHelper::setBlockWithStates
         * （GameTestHelper.cpp:346-392）。statesStr 解析为 unordered_map<string,string>，
         * 格式 "prop=value" 或 "p1=v1,p2=v2"（如 "part=head,facing=north"、"age=3"）。
         * 未知属性名静默忽略（容错）；非法值抛 GameTestError。updateFlags 默认 3
         * （NOTIFY | NEIGHBOR，与 setBlockType flags=3 一致，触发 onBlockAdded + 邻居更新）。
         *
         * 用于放置带特定 state 的方块（床的 head/foot 半、指定 facing 朝向等），弥补
         * setBlockType 只能放默认 state 的不足。
         *
         * @param blockType 方块 typeId（如 "minecraft:red_bed"）
         * @param blockLocation 结构相对坐标 {x,y,z}
         * @param statesStr 属性字符串 "p1=v1,p2=v2"
         * @param updateFlags 更新标志位（默认 3 = NOTIFY | NEIGHBOR）
         */
        setBlockWithStates(
            blockType: string,
            blockLocation: import("@minecraft/server").Vector3,
            statesStr: string,
            updateFlags?: number,
        ): void;
    }
}
