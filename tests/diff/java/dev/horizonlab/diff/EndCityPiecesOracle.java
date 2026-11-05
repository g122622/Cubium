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
import net.minecraft.world.level.StructureManager;
import net.minecraft.world.level.chunk.ProtoChunk;
import net.minecraft.world.level.levelgen.structure.BoundingBox;
import net.minecraft.world.level.levelgen.structure.StructurePiece;
import net.minecraft.world.level.levelgen.structure.StructureStart;
import net.minecraft.world.level.levelgen.structure.structures.EndCityPieces;
import net.minecraft.world.level.levelgen.structure.structures.EndCityStructure;
import net.minecraft.world.level.levelgen.structure.templatesystem.StructureTemplateManager;

/**
 * 末地城构件序列的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EndCityPiecesOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 与 {@link StructureStartsOracle} 共用维度构造与结构管理器，但只输出
 * {@code minecraft:end_city} 的构件序列（顺序、模板名、包围盒、旋转、生成深度）。
 *
 * 模板名取 {@code TemplateStructurePiece} 的受保护字段 {@code templateName}：构件名
 * （base_floor / tower_piece / bridge_end …）候选项的唯一标识，且不受旋转影响。
 * {@code genDepth} 是 1.21.11 决定邻近性豁免的关键标识——同一批提交的构件共用一个
 * 深度、桥段固定为 -1，两侧必须逐条一致。
 */
public final class EndCityPiecesOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EndCityPiecesOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: EndCityPiecesOracle run <input.jsonl> <output.jsonl>");
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
        ProtoChunk chunk = new ProtoChunk(new ChunkPos(chunkX, chunkZ),
                net.minecraft.world.level.chunk.UpgradeData.EMPTY, dimension.height(),
                net.minecraft.world.level.chunk.PalettedContainerFactory.create(registries.access), null);
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
            if (!(start.getStructure() instanceof EndCityStructure)) {
                continue;
            }
            found = true;
            List<StructurePiece> list = start.getPieces();
            for (StructurePiece piece : list) {
                JsonObject object = new JsonObject();
                object.addProperty("template", templateName(piece));
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
     * 构件的模板名。
     *
     * {@code templateName} 在 {@code TemplateStructurePiece} 里是 {@code protected final}，
     * 反射读取是唯一不依赖构件类型的取法；读取失败时输出空串（不会让整条比对崩掉，
     * 而是退化成"模板名缺失"这一条可见的差异）。
     */
    private static String templateName(StructurePiece piece) {
        try {
            java.lang.reflect.Field field =
                    net.minecraft.world.level.levelgen.structure.TemplateStructurePiece.class
                            .getDeclaredField("templateName");
            field.setAccessible(true);
            Object value = field.get(piece);
            return value == null ? "" : value.toString();
        } catch (ReflectiveOperationException error) {
            return "";
        }
    }

    /** 供其它 oracle 复用的构件名读取入口。 */
    static String pieceTemplateName(StructurePiece piece) {
        return templateName(piece);
    }

    /** 末地城构件的类别判断：{@code EndCityPieces.EndCityPiece} 是模板构件，其余是占位。 */
    static boolean isEndCityPiece(StructurePiece piece) {
        return piece instanceof EndCityPieces.EndCityPiece;
    }
}