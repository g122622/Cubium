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
import net.minecraft.world.level.ChunkPos;
import net.minecraft.world.level.StructureManager;
import net.minecraft.world.level.chunk.PalettedContainerFactory;
import net.minecraft.world.level.chunk.ProtoChunk;
import net.minecraft.world.level.chunk.UpgradeData;
import net.minecraft.world.level.levelgen.structure.BoundingBox;
import net.minecraft.world.level.levelgen.structure.StructurePiece;
import net.minecraft.world.level.levelgen.structure.StructureStart;
import net.minecraft.world.level.levelgen.structure.structures.StrongholdStructure;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.levelgen.structure.templatesystem.StructureTemplateManager;

/**
 * 要塞构件序列的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.StrongholdPiecesOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 与 {@link MineshaftPiecesOracle} 同构：只输出 {@code minecraft:stronghold} 的构件序列
 * （顺序、类型、包围盒、朝向、深度），把「整座要塞对不上」定位到「第几个构件开始分歧」。
 * 要塞的构件数是各结构里最大的（可达三百以上），整座比对几乎必然只剩一条差异，
 * 拆到构件级才能看出分歧点。
 */
public final class StrongholdPiecesOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private StrongholdPiecesOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: StrongholdPiecesOracle run <input.jsonl> <output.jsonl>");
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
        ProtoChunk chunk = new ProtoChunk(new ChunkPos(chunkX, chunkZ), UpgradeData.EMPTY,
                dimension.height(), PalettedContainerFactory.create(registries.access), null);
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
            if (!(start.getStructure() instanceof StrongholdStructure)) {
                continue;
            }
            found = true;
            List<StructurePiece> list = start.getPieces();
            for (StructurePiece piece : list) {
                JsonObject object = new JsonObject();
                object.addProperty("type", typeName(piece));
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
                pieces.add(object);
            }
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", input.has("unit") ? input.get("unit").getAsString() : "");
        result.addProperty("present", found);
        result.add("pieces", pieces);
        return result;
    }

    /** 构件类型名（与 {@code StructurePieceType} 的注册名同口径，去掉命名空间）。 */
    private static String typeName(StructurePiece piece) {
        // 类型名取自注册表：StructurePieceType 注册时用小写 id（如 "shcc"），
        // 探针侧使用同名的全大写下划线常量，here 统一成那一形态。
        Identifier id = BuiltInRegistries.STRUCTURE_PIECE.getKey(piece.getType());
        return id == null ? "unknown" : id.getPath().toUpperCase(java.util.Locale.ROOT);
    }

}
