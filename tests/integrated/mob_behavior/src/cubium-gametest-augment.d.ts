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
    }
}
