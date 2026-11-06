package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import net.minecraft.core.BlockPos;
import net.minecraft.world.level.ChunkPos;
import net.minecraft.world.level.levelgen.structure.BoundingBox;
import net.minecraft.world.level.levelgen.structure.StructurePiece;
import net.minecraft.world.level.levelgen.structure.StructureStart;
import net.minecraft.world.level.levelgen.structure.pieces.StructurePieceType;
import net.minecraft.world.level.levelgen.structure.structures.MineshaftStructure;
import net.minecraft.world.level.levelgen.structure.templatesystem.StructureTemplateManager;
import net.minecraft.world.level.chunk.ProtoChunk;
import net.minecraft.world.level.StructureManager;

/**
 * 废弃矿井构件序列的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MineshaftPiecesOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 与 {@link StructureStartsOracle} 共用维度构造与结构管理器，但只输出
 * {@code minecraft:mineshaft} 的构件序列（顺序、类型、包围盒、朝向、生成深度），
 * 便于把「整座矿井对不上」定位到「第几个构件开始分歧」。
 */
public final class MineshaftPiecesOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private MineshaftPiecesOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: MineshaftPiecesOracle run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        DataPackRegistries registries = DataPackRegistries.load();
        try (BufferedWriter writer = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
                if (line.isBlank()) {
                    continue;
                }
                JsonObject sample = JsonParser.parseString(line).getAsJsonObject();
                writer.write(GSON.toJson(runSample(sample, registries)));
                writer.newLine();
            }
        }
    }

    private static JsonObject runSample(JsonObject input, DataPackRegistries registries) {
        String dimensionName = input.get("dimension").getAsString();
        long seed = input.get("seed").getAsLong();
        int chunkX = input.get("chunkX").getAsInt();
        int chunkZ = input.get("chunkZ").getAsInt();

        StructureStartsOracle.Dimension dimension =
                StructureStartsOracle.makeDimension(dimensionName, seed, registries.lookup);
        ProtoChunk chunk = new ProtoChunk(new ChunkPos(chunkX, chunkZ), net.minecraft.world.level.chunk.UpgradeData.EMPTY,
                dimension.height(), net.minecraft.world.level.chunk.PalettedContainerFactory.create(registries.access), null);
        StructureTemplateManager templates = StructureStartsOracle.templateManager(registries);
        StructureManager structureManager =
                StructureStartsOracle.structureManager(registries, dimension, chunk, templates);
        dimension.generator()
                .createStructures(registries.worldGenAccess, dimension.placementState(), structureManager,
                        chunk, templates, dimension.levelKey());

        JsonArray pieces = new JsonArray();
        boolean found = false;
        for (var entry : chunk.getAllStarts().entrySet()) {
            StructureStart start = entry.getValue();
            if (!start.isValid()) {
                continue;
            }
            if (!(start.getStructure() instanceof MineshaftStructure)) {
                continue;
            }
            found = true;
            List<StructurePiece> list = start.getPieces();
            for (StructurePiece piece : list) {
                JsonObject object = new JsonObject();
                object.addProperty("type", typeName(piece.getType()));
                BoundingBox box = piece.getBoundingBox();
                JsonArray boxArray = new JsonArray();
                boxArray.add(box.minX());
                boxArray.add(box.minY());
                boxArray.add(box.minZ());
                boxArray.add(box.maxX());
                boxArray.add(box.maxY());
                boxArray.add(box.maxZ());
                object.add("box", boxArray);
                net.minecraft.world.level.block.Rotation rotation = piece.getRotation();
                object.addProperty("rotation", rotation == null ? "NONE" : rotation.name());
                object.addProperty("genDepth", piece.getGenDepth());
                net.minecraft.core.Direction orientation = piece.getOrientation();
                object.addProperty("orientation", orientation == null ? "null" : orientation.name());
                // 交叉点的方向字段：getOrientation() 对它是 null，但 addChildren 实际按
                // 构造时传入的方向选出口表，因此它决定下一批构件的坐标。
                object.addProperty("crossDirection", crossDirection(piece));
                pieces.add(object);
            }
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", input.has("unit") ? input.get("unit").getAsString() : "");
        result.addProperty("present", found);
        result.add("pieces", pieces);
        return result;
    }

    /**
     * 交叉点构件的方向字段。
     *
     * {@code MineShaftCrossing.direction} 是 {@code private final}，没有访问器，而
     * {@code addChildren} 完全依赖它选出口表——它是「下一个构件从哪出发」的唯一线索。
     * 只对交叉点读取，其余构件返回 {@code null}。
     */
    private static String crossDirection(StructurePiece piece) {
        if (!(piece instanceof net.minecraft.world.level.levelgen.structure.structures.MineshaftPieces
                .MineShaftCrossing)) {
            return "null";
        }
        try {
            java.lang.reflect.Field field = piece.getClass().getDeclaredField("direction");
            field.setAccessible(true);
            Object value = field.get(piece);
            return value == null ? "null" : ((net.minecraft.core.Direction) value).name();
        } catch (ReflectiveOperationException error) {
            return "null";
        }
    }

    private static String typeName(StructurePieceType type) {
        if (type == StructurePieceType.MINE_SHAFT_ROOM) {
            return "room";
        }
        if (type == StructurePieceType.MINE_SHAFT_CORRIDOR) {
            return "corridor";
        }
        if (type == StructurePieceType.MINE_SHAFT_CROSSING) {
            return "crossing";
        }
        if (type == StructurePieceType.MINE_SHAFT_STAIRS) {
            return "stairs";
        }
        return type.toString();
    }
}
