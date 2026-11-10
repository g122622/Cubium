package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.BufferedWriter;
import java.io.IOException;
import java.lang.reflect.Field;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import net.minecraft.core.Direction;
import net.minecraft.util.RandomSource;
import net.minecraft.world.level.levelgen.LegacyRandomSource;
import net.minecraft.world.level.levelgen.structure.BoundingBox;
import net.minecraft.world.level.levelgen.structure.StructurePiece;
import net.minecraft.world.level.levelgen.structure.StructurePieceAccessor;
import net.minecraft.world.level.levelgen.structure.structures.StrongholdPieces;

/**
 * 要塞面向小门的三个落点公式的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.StrongholdSmallDoorOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 输入一行一个样本：父构件的构造原点、朝向、包围盒尺寸与 `orientBox` 局部偏移，以及
 * 两个门偏移量。输出三个方向（正向 / 左门 / 右门）各自落下的子构件包围盒——直接来自原版
 * `generateSmallDoorChildForward/Left/Right`，不是照着公式重写一遍。
 *
 * 场景里只放「起始楼梯 + 本构件」两件，子构件的包围盒只要不与这两件相撞就会真的落下；
 * 三个方向各自独占一条随机流（由两个不同种子派生），避免互相干扰。
 */
public final class StrongholdSmallDoorOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private StrongholdSmallDoorOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: StrongholdSmallDoorOracle run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        try (BufferedWriter writer = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
                if (line.isBlank()) {
                    continue;
                }
                writer.write(GSON.toJson(runSample(JsonParser.parseString(line).getAsJsonObject())));
                writer.newLine();
            }
        }
    }

    private static JsonObject runSample(JsonObject input) {
        final String unit = input.has("unit") ? input.get("unit").getAsString() : "";
        final Direction direction = Direction.byName(
                input.get("direction").getAsString().toLowerCase(java.util.Locale.ROOT));
        final long seed = input.get("seed").getAsLong();
        final int x = input.get("x").getAsInt();
        final int y = input.get("y").getAsInt();
        final int z = input.get("z").getAsInt();
        final int sizeX = input.get("sizeX").getAsInt();
        final int sizeY = input.get("sizeY").getAsInt();
        final int sizeZ = input.get("sizeZ").getAsInt();
        final int offsetX = input.get("offsetX").getAsInt();
        final int offsetY = input.get("offsetY").getAsInt();
        final int verticalOffset = input.get("verticalOffset").getAsInt();
        final int positionOffset = input.get("positionOffset").getAsInt();
        final int parentDepth = input.get("parentDepth").getAsInt();

        BoundingBox parentBox = BoundingBox.orientBox(
                x, y, z, offsetX, offsetY, 0, sizeX, sizeY, sizeZ, direction);

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        result.add("forward", probe(
                direction, seed, x, y, z, parentBox, parentDepth, "forward", verticalOffset, positionOffset));
        result.add("left", probe(
                direction, seed + 1, x, y, z, parentBox, parentDepth, "left", verticalOffset, positionOffset));
        result.add("right", probe(
                direction, seed + 2, x, y, z, parentBox, parentDepth, "right", verticalOffset, positionOffset));
        return result;
    }

    /** 用一条独立随机流重跑一次落点，返回子构件（未落下则为 null）。 */
    private static JsonElement probe(
            Direction direction,
            long seed,
            int x,
            int y,
            int z,
            BoundingBox parentBox,
            int parentDepth,
            String mode,
            int verticalOffset,
            int positionOffset) {
        // 落点会走抽签表（currentPieces 是静态状态，必须先 reset）。
        StrongholdPieces.resetPieces();
        RandomSource random = new LegacyRandomSource(seed);
        StrongholdPieces.StartPiece start = new StrongholdPieces.StartPiece(new LegacyRandomSource(0L), 0, 0);
        // 三个落点方法都是 protected：用一个派生自直廊的探针子类把它们暴露出来，
        // 落点逻辑仍是原版那一份实现。父构件的构造本身要消耗随机数，必须在改写过起点盒
        // 之后再构造，否则随机流比探针少走几步。

        // 起始楼梯与父构件共用同一个盒子（生产里父构件正是从起始楼梯那道门长出来的）：
        // 它既不会挡住落点，也不影响 112 格判据，两种实现都不必再额外搬迁它。
        setBox(start, parentBox);
        // 构件自身记录的朝向是它导入口的反向（生产里 addChildren 用的就是这个字段）。
        StraightAccess parent = new StraightAccess(parentDepth, random, parentBox, direction.getOpposite());

        ProbeBuilder builder = new ProbeBuilder(new java.util.ArrayList<>());
        builder.addPiece(start);
        builder.addPiece(parent);
        StructurePiece child = switch (mode) {
            // 三个方法的形参顺序与原版一致：正向是 (水平偏移, 垂直偏移)，侧门是 (垂直偏移, 水平偏移)。
            case "forward" -> parent.forward(start, builder, random, positionOffset, verticalOffset);
            case "left" -> parent.left(start, builder, random, verticalOffset, positionOffset);
            default -> parent.right(start, builder, random, verticalOffset, positionOffset);
        };
        if (child == null) {
            return JsonNull.INSTANCE;
        }
        // 原始输出：[minX,minY,minZ,maxX,maxY,maxZ,type,depth,orientation,rotation]，与探针逐项对应。
        BoundingBox box = child.getBoundingBox();
        JsonArray array = new JsonArray();
        array.add(box.minX());
        array.add(box.minY());
        array.add(box.minZ());
        array.add(box.maxX());
        array.add(box.maxY());
        array.add(box.maxZ());
        array.add(typeName(child));
        array.add(child.getGenDepth());
        array.add(child.getOrientation() == null ? "null" : child.getOrientation().name());
        array.add(child.getRotation() == null ? "NONE" : child.getRotation().name());
        return array;
    }

    /** 直接改写构件的包围盒（`boundingBox` 是 protected 字段）。 */
    private static void setBox(StructurePiece piece, BoundingBox box) {
        try {
            Field field = StructurePiece.class.getDeclaredField("boundingBox");
            field.setAccessible(true);
            field.set(piece, box);
        } catch (ReflectiveOperationException error) {
            throw new IllegalStateException(error);
        }
    }

    /** 把原版直廊的三个 protected 落点方法暴露给本 oracle。 */
    private static final class StraightAccess extends StrongholdPieces.Straight {
        StraightAccess(int depth, RandomSource random, BoundingBox box, Direction direction) {
            super(depth, random, box, direction);
        }

        StructurePiece forward(StrongholdPieces.StartPiece start, StructurePieceAccessor accessor, RandomSource random,
                int verticalOffset, int positionOffset) {
            return this.generateSmallDoorChildForward(start, accessor, random, verticalOffset, positionOffset);
        }

        StructurePiece left(StrongholdPieces.StartPiece start, StructurePieceAccessor accessor, RandomSource random,
                int verticalOffset, int positionOffset) {
            return this.generateSmallDoorChildLeft(start, accessor, random, verticalOffset, positionOffset);
        }

        StructurePiece right(StrongholdPieces.StartPiece start, StructurePieceAccessor accessor, RandomSource random,
                int verticalOffset, int positionOffset) {
            return this.generateSmallDoorChildRight(start, accessor, random, verticalOffset, positionOffset);
        }
    }

    /** 只收集、不做包围盒以外的任何登记：原版装配用 StructurePiecesBuilder 完成同样的角色。 */
    private static final class ProbeBuilder implements StructurePieceAccessor {
        private final List<StructurePiece> pieces;

        ProbeBuilder(List<StructurePiece> pieces) {
            this.pieces = pieces;
        }

        @Override
        public void addPiece(StructurePiece piece) {
            pieces.add(piece);
        }

        @Override
        public StructurePiece findCollisionPiece(BoundingBox box) {
            return StructurePiece.findCollisionPiece(pieces, box);
        }

    }

    /** 构件类型名：与 `StructurePieceType` 注册名同口径，去掉命名空间并转大写。 */
    private static String typeName(StructurePiece piece) {
        var id = net.minecraft.core.registries.BuiltInRegistries.STRUCTURE_PIECE.getKey(piece.getType());
        return id == null ? "unknown" : id.getPath().toUpperCase(java.util.Locale.ROOT);
    }
}