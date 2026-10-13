// Cubium 已实现的 GameTest 扩展；声明与原生绑定保持一致。
import "@minecraft/server-gametest";
import type { Vector3 } from "@minecraft/server";

declare module "@minecraft/server-gametest" {
  interface RegistrationBuilder {
    /** 声明测试结构上方需要露天。 */
    skyAccess(skyAccess: boolean): RegistrationBuilder;
  }

  interface Test {
    /** 使用注册方块的字符串标识符设置结构内方块。 */
    setBlockType(blockType: string, pos: Vector3): void;
  }
}
