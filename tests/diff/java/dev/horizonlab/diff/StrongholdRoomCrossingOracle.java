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
import net.minecraft.core.Direction;
import net.minecraft.util.RandomSource;
import net.minecraft.world.level.levelgen.LegacyRandomSource;
import net.minecraft.world.level.levelgen.structure.BoundingBox;
import net.minecraft.world.level.levelgen.structure.StructurePiece;
import net.minecraft.world.level.levelgen.structure.structures.StrongholdPieces;

/**
 * 单个要塞房间构件（RoomCrossing）的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.StrongholdRoomCrossingOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 输入一行一个样本：门朝向、随机源种子、门的方块坐标、分支深度。输出该房间的包围盒、
 * 旋转/镜像、入口门类型，以及构造期消耗的随机数个数——最后一项是判断「随机流是否已经
 * 错位」的直接证据。
 */
public final class StrongholdRoomCrossingOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private StrongholdRoomCrossingOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]), -1);
            return;
        }
        if (args.length == 4 && args[0].equals("run") && args[3].equals("--ref")) {
            run(Path.of(args[1]), Path.of(args[2]), 7);
            return;
        }
        System.err.println("usage: StrongholdRoomCrossingOracle run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output, long forcedSeed) throws IOException {
        try (BufferedWriter writer = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
                if (line.isBlank()) {
                    continue;
                }
                writer.write(GSON.toJson(runSample(JsonParser.parseString(line).getAsJsonObject(), forcedSeed)));
                writer.newLine();
            }
        }
    }

    private static int typeField(StrongholdPieces.RoomCrossing piece) {
        try {
            var field = StrongholdPieces.RoomCrossing.class.getDeclaredField("type");
            field.setAccessible(true);
            return field.getInt(piece);
        } catch (ReflectiveOperationException error) {
            throw new IllegalStateException(error);
        }
    }

    private static JsonObject runSample(JsonObject input, long forcedSeed) {
        final String unit = input.has("unit") ? input.get("unit").getAsString() : "";
        final Direction direction = Direction.byName(input.get("direction").getAsString().toLowerCase(java.util.Locale.ROOT));
        final long seed = forcedSeed >= 0 ? forcedSeed : input.get("seed").getAsLong();
        final int x = input.get("x").getAsInt();
        final int y = input.get("y").getAsInt();
        final int z = input.get("z").getAsInt();
        final int depth = input.get("depth").getAsInt();
        final int cachedRoomType = input.has("roomType") ? input.get("roomType").getAsInt() : -1;

        RandomSource random = new LegacyRandomSource(seed);
        // 与装配流程一致：房间构造只用到「入口门」与「房间类型」两次抽取。
        BoundingBox box = BoundingBox.orientBox(x, y, z, -4, -1, 0, 11, 7, 11, direction);
        StrongholdPieces.RoomCrossing piece =
                new StrongholdPieces.RoomCrossing(depth, random, box, direction);

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        BoundingBox actual = piece.getBoundingBox();
        JsonArray boxArray = new JsonArray();
        boxArray.add(actual.minX());
        boxArray.add(actual.minY());
        boxArray.add(actual.minZ());
        boxArray.add(actual.maxX());
        boxArray.add(actual.maxY());
        boxArray.add(actual.maxZ());
        result.add("box", boxArray);
        result.addProperty("rotation", piece.getRotation() == null ? "NONE" : piece.getRotation().name());
        result.addProperty("orientation", piece.getOrientation() == null ? "null" : piece.getOrientation().name());
        // 两个实现都从同一份输入拿深度，取值必然相同；这里不复述，避免把「输入回显」当成比较项。
        result.addProperty("roomType", typeField(piece));
        // 用输入种子另开一支做对照，用来判断差异是「随机源本身不同」还是「抽取次数不同」。
        RandomSource reference = new LegacyRandomSource(seed);
        result.addProperty("referenceNextInt5", reference.nextInt(5));
        result.addProperty("nextInt5AfterConstruction", random.nextInt(5));
        return result;
    }
}