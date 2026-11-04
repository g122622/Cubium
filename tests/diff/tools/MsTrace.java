/**
 * 废弃矿井随机序列的独立复算工具（不参与构建，手工编译运行）。
 *
 * 用途：差分排查时，把「原版在某个区块会依次抽哪些随机数」直接算出来，
 * 不必依赖源码推理。编译与运行见同目录 README。
 *
 * 本工具复现的是：setLargeFeatureSeed -> nextDouble -> 房间 nextInt(6)x3
 * -> 房间第 2 段 nextInt(5) -> 首个走廊的 nextInt(100) / nextInt(3) / nextInt(3) / nextInt(23)。
 */
import net.minecraft.world.level.levelgen.WorldgenRandom;

public final class MsTrace {
    public static void main(String[] args) {
        long worldSeed = Long.parseLong(args[0]);
        int chunkX = Integer.parseInt(args[1]);
        int chunkZ = Integer.parseInt(args[2]);

        WorldgenRandom rng = new WorldgenRandom(net.minecraft.world.level.levelgen.LegacyRandomSource.class.getName() == null ? null : new net.minecraft.world.level.levelgen.LegacyRandomSource(worldSeed));
        rng.setLargeFeatureSeed(worldSeed, chunkX, chunkZ);

        // findGenerationPoint 开头丢弃的一次 nextDouble
        rng.nextDouble();
        System.out.println("  after nextDouble");

        int spanX = rng.nextInt(6);
        int spanY = rng.nextInt(6);
        int spanZ = rng.nextInt(6);
        System.out.println("  room nextInt(6)x3 = " + spanX + "," + spanY + "," + spanZ);

        // 房间第 2 段：x 从 minX+3 起
        int pick = rng.nextInt(5);
        System.out.println("  room nextInt(5) = " + pick);

        if (pick == 0 || pick == 1) {
            int chance = rng.nextInt(100);
            System.out.println("  create nextInt(100) = " + chance);
            int attempt = rng.nextInt(3) + 2;
            System.out.println("  corridorSize nextInt(3)+2 = " + attempt);
            int rails = rng.nextInt(3);
            System.out.println("  corridorCtor nextInt(3) = " + rails + "  (rails=" + (rails == 0) + ")");
            if (rails != 0) {
                System.out.println("  corridorCtor nextInt(23) = " + rng.nextInt(23));
            }
        }
    }
}
