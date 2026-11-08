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
import java.util.ArrayList;
import java.util.List;
import net.minecraft.core.BlockPos;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.block.Rotation;
import net.minecraft.world.level.levelgen.structure.templatesystem.StructureTemplate;
import net.minecraft.world.level.levelgen.structure.templatesystem.StructureTemplateManager;

/**
 * 拼图模板连接点洗牌顺序的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.JigsawJointOrderOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 对每个样本读取模板、做一次 {@code Util.shuffle} 加按 selectionPriority 降序的稳定排序，
 * 输出连接点的最终顺序。与 {@code jigsaw_joint_order} 探针逐条比对，即可把装配期
 * 「相邻构件旋转互换」这类错位定位到具体的一次洗牌。
 */
public final class JigsawJointOrderOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private JigsawJointOrderOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: JigsawJointOrderOracle run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        DataPackRegistries registries = DataPackRegistries.load();
        // 模板管理器复用 StructureStartsOracle 的构造路径：模板只从原版数据包读，
        // 不落盘任何世界目录（生成模板目录给一个临时空目录即可）。
        StructureTemplateManager templateManager = StructureStartsOracle.templateManager(registries);

        try (BufferedWriter writer = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
                if (line.isBlank()) {
                    continue;
                }
                JsonObject sample = JsonParser.parseString(line).getAsJsonObject();
                String unit = sample.has("unit") ? sample.get("unit").getAsString() : "";
                String templateId = sample.get("template").getAsString();
                Rotation rotation = Rotation.valueOf(sample.get("rotation").getAsString());
                long seed = sample.get("seed").getAsLong();

                JsonObject result = new JsonObject();
                result.addProperty("unit", unit);
                result.addProperty("rotation", rotation.name());

                StructureTemplate template = templateManager
                        .get(Identifier.parse(templateId))
                        .orElse(null);
                if (template == null) {
                    result.add("joints", new JsonArray());
                    writer.write(GSON.toJson(result));
                    writer.newLine();
                    continue;
                }

                // 与 SinglePoolElement.getShuffledJigsawBlocks 相同：取变换后的连接点 → 洗牌 →
                // 按 selectionPriority 降序稳定排序。这里只做洗牌与排序，不依赖任何装配状态。
                List<StructureTemplate.JigsawBlockInfo> joints = new ArrayList<>(
                        template.getJigsaws(BlockPos.ZERO, rotation));
                net.minecraft.util.RandomSource random = net.minecraft.util.RandomSource.create(seed);
                net.minecraft.util.Util.shuffle(joints, random);
                joints.sort(java.util.Comparator.comparingInt(
                                StructureTemplate.JigsawBlockInfo::selectionPriority)
                        .reversed());

                JsonArray array = new JsonArray();
                for (StructureTemplate.JigsawBlockInfo joint : joints) {
                    JsonObject entry = new JsonObject();
                    entry.addProperty("x", joint.info().pos().getX());
                    entry.addProperty("y", joint.info().pos().getY());
                    entry.addProperty("z", joint.info().pos().getZ());
                    entry.addProperty("name", joint.name().toString());
                    entry.addProperty("target", joint.pool().identifier().toString());
                    entry.addProperty("selectionPriority", joint.selectionPriority());
                    entry.addProperty("placementPriority", joint.placementPriority());
                    // 朝向与连接类型同样是匹配输入：正面相反、aligned 时顶面一致。
                    // 只比坐标时，朝向旋转错一格会表现为"同一模板、不同旋转"，无从定位。
                    entry.addProperty("front", net.minecraft.world.level.block.JigsawBlock
                            .getFrontFacing(joint.info().state()).getName());
                    entry.addProperty("top", net.minecraft.world.level.block.JigsawBlock
                            .getTopFacing(joint.info().state()).getName());
                    entry.addProperty("joint", joint.jointType().getSerializedName());
                    array.add(entry);
                }
                result.add("joints", array);

                writer.write(GSON.toJson(result));
                writer.newLine();
            }
        }
    }
}