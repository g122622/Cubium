import dev.horizonlab.diff.DataPackRegistries;
import net.minecraft.SharedConstants;
import net.minecraft.core.BlockPos;
import net.minecraft.core.registries.Registries;
import net.minecraft.server.Bootstrap;
import net.minecraft.util.RandomSource;
import net.minecraft.util.datafix.DataFixers;
import net.minecraft.world.level.block.Rotation;
import net.minecraft.world.level.levelgen.LegacyRandomSource;
import net.minecraft.world.level.levelgen.WorldgenRandom;
import net.minecraft.world.level.levelgen.structure.StructurePiece;
import net.minecraft.world.level.levelgen.structure.structures.EndCityPieces;
import net.minecraft.world.level.levelgen.structure.templatesystem.StructureTemplateManager;
import net.minecraft.world.level.storage.LevelStorageSource;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

/**
 * 记录原版末地城生成过程中的每一次随机数抽取。
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

    static StructureTemplateManager templateManager(DataPackRegistries registries) throws Exception {
        Path emptyLevel = Files.createTempDirectory("diff-endcity");
        var source = LevelStorageSource.createDefault(emptyLevel);
        var storage = source.createAccess("diff");
        return new StructureTemplateManager(registries.resourceManager, storage,
            DataFixers.getDataFixer(), registries.lookup.lookupOrThrow(Registries.BLOCK));
    }

    public static void main(String[] args) throws Exception {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();

        long seed = Long.parseLong(args[0]);
        int chunkX = Integer.parseInt(args[1]);
        int chunkZ = Integer.parseInt(args[2]);

        DataPackRegistries registries = DataPackRegistries.load();
        StructureTemplateManager mgr = templateManager(registries);
        Tracing rng = new Tracing(new LegacyRandomSource(0L));
        rng.setLargeFeatureSeed(seed, chunkX, chunkZ);

        Rotation rotation = Rotation.getRandom(rng);
        List<StructurePiece> pieces = new ArrayList<>();
        EndCityPieces.startHouseTower(mgr, new BlockPos(0, 64, 0), rotation, pieces, rng);

        System.out.println("PIECES " + pieces.size());
    }
}
