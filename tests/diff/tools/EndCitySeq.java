import net.minecraft.SharedConstants;
import net.minecraft.server.Bootstrap;
import net.minecraft.world.level.levelgen.WorldgenRandom;
import net.minecraft.world.level.levelgen.LegacyRandomSource;
import net.minecraft.util.RandomSource;
import net.minecraft.world.level.block.Rotation;

/**
 * 只记录原版末端城生成过程中的随机数抽取，
 * 不实际加载模板（模板加载不消耗随机数）。
 */
public final class EndCitySeq {
    private static int counter = 0;

    static final class Tracing extends WorldgenRandom {
        Tracing(RandomSource inner) {
            super(inner);
        }

        @Override
        public int next(int bits) {
            int v = super.next(bits);
            System.out.println("D " + (counter++) + " bits" + bits + " " + v);
            return v;
        }
    }

    public static void main(String[] args) {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();

        long seed = Long.parseLong(args[0]);
        int chunkX = Integer.parseInt(args[1]);
        int chunkZ = Integer.parseInt(args[2]);

        Tracing rng = new Tracing(new LegacyRandomSource(0L));
        rng.setLargeFeatureSeed(seed, chunkX, chunkZ);

        Rotation rotation = Rotation.getRandom(rng);
        System.out.println("ROT " + rotation);
        System.out.println("DONE");
    }
}
