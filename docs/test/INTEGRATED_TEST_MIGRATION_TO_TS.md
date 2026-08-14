# 测试脚本迁移到TS

JS 脚本提供的类型信息有限，**必须一开始就使用或迁移到 TypeScript**，官方本身就推荐这么做。但有一个关键前提：

> "TypeScript must be compiled into JavaScript before Minecraft can use it."

也就是说，Minecraft 的脚本引擎只运行 **JavaScript**。TypeScript 不是 manifest 里写个 `language: "typescript"` 就能让游戏自动编译的——你必须用外部工具（`tsc` / `esbuild` / `just-scripts`）把 `.ts` 编译成 `.js`，再让 manifest 的 `entry` 指向编译产物。

**最直接的证据**：官方 GameTest 文档里 `register` 函数的示例代码文件名就是 `simpleMobGameTest.ts`、`simpleMobAsyncTest.ts`，且全程使用 TypeScript 语法（`test: Test` 类型标注）。说明 GameTest API 对 TS 是一等公民支持——`@minecraft/server-gametest` 这个 npm 包自带 `.d.ts` 类型定义，装上就有完整类型提示。

---

## 具体迁移方案

对于JS 文件 + GameTest API，迁移工作量很小。需要做三件事：

### 1. 调整 `manifest.json`

`language` 字段保持 `"javascript"` 不变（游戏只认 JS），`entry` 仍指向编译后的 `main.js`：

```json
{
  "type": "script",
  "language": "javascript",
  "entry": "scripts/main.js"
}
```

### 2. 新增构建配置

在项目根目录下加 `tsconfig.json` 和 `package.json` 的 devDependencies。官方推荐的入门方案是 [microsoft/minecraft-scripting-samples](https://github.com/microsoft/minecraft-scripting-samples) 的 `ts-starter`（用 `just-scripts` + `tsc`）。也可以用更轻量的 `esbuild`。

最小 `tsconfig.json` 大致是：
```json
{
  "compilerOptions": {
    "target": "ES2020",
    "module": "ES2020",
    "moduleResolution": "node",
    "strict": true,
    "outDir": "scripts",
    "rootDir": "src",
    "types": ["@minecraft/server", "@minecraft/server-gametest"]
  }
}
```

### 3. 改写脚本（机械性改动）

TS 化后大致是：

```typescript
import * as GameTest from "@minecraft/server-gametest";
import { Test } from "@minecraft/server-gametest";
import { Utilities } from "./Utilities.js";

function collapsing(test: Test) {
  const zoglinEntityType = "minecraft:zoglin";
  const shulkerEntityType = "minecraft:shulker";
  // ... 其余基本不变
}
```

几个注意点：
- `import { Utilities } from "Utilities.js"` 建议改成 `"./Utilities.js"` —— TS 编译时 `.js` 后缀指向 `.ts` 源，是 ES 模块规范写法，能正确编译。
- `test` 参数需标注 `Test` 类型（来自 `@minecraft/server-gametest`），否则 `test.spawn` 等方法没类型提示。
- 实体类型字符串（`"minecraft:shulker"`）可保持 `string`，也可以用 `@minecraft/vanilla-data` 的 `MinecraftEntityTypes` 枚举获得类型安全（官方示例就是这么做的）。
- `Utilities.js` 里的 `test` 参数同样要标 `Test` 类型，`blockType` 标 `string`，坐标参数标 `number`。

其余 `CommandTests.js`、`MobBehaviorTests.js`、`main.js` 的改法完全一致，都是加类型标注、修 import 路径。

---

**参考来源：**
- [minecraft/server-gametest Module - Microsoft Learn](https://learn.microsoft.com/en-us/minecraft/creator/scriptapi/minecraft/server-gametest/minecraft-server-gametest)（示例代码即为 `.ts`）
- [Next Steps: Scripting with TypeScript - Microsoft Learn](https://learn.microsoft.com/en-us/minecraft/creator/documents/scripting/next-steps)（明确说明 TS 需编译为 JS，官方推荐 just-scripts 工具链）
- [minecraft-scripting-samples (ts-starter)](https://github.com/microsoft/minecraft-scripting-samples/)（已经clone到本地 E:\dev\MC\minecraft-scripting-samples）
