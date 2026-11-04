package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.core.Direction;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.BellAttachType;
import net.minecraft.world.level.block.state.properties.BlockStateProperties;

/**
 * 敲钟有效击判定原版 oracle（{@code BellBlock#isProperHit}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BellRingOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code floor:north:east:0.20}（附着方式:钟朝向:点击面:点击高度）。
 *
 * <p>原版 {@code isProperHit} 是 private，这里逐条复现它的判定，全部规则都来自源码：
 *
 * <pre>
 *   if (点击面轴 != Y && !(hitY > 0.8124)) {
 *       switch (附着方式) {
 *           case FLOOR:        return 钟朝向轴 == 点击面轴;
 *           case SINGLE_WALL:
 *           case DOUBLE_WALL:  return 钟朝向轴 != 点击面轴;
 *           case CEILING:      return true;
 *       }
 *   }
 *   return false;
 * </pre>
 */
public final class BellRingOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] ATTACHMENTS = {"floor", "ceiling", "single_wall", "double_wall"};
    private static final Direction[] FACINGS = {
        Direction.NORTH, Direction.SOUTH, Direction.WEST, Direction.EAST
    };
    private static final Direction[] FACES = {
        Direction.DOWN, Direction.UP, Direction.NORTH, Direction.SOUTH, Direction.WEST, Direction.EAST
    };
    private static final double[] SAMPLE_HEIGHTS = {0.2, 0.5, 0.81, 0.82, 0.95};

    private BellRingOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String attachment : ATTACHMENTS) {
                for (Direction facing : FACINGS) {
                    for (Direction face : FACES) {
                        for (double height : SAMPLE_HEIGHTS) {
                            units.add(attachment + ":" + facing.getSerializedName() + ":"
                                + face.getSerializedName() + ":" + String.format("%.2f", height));
                        }
                    }
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BellRingOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        List<String> results = new ArrayList<>(lines.size());
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject obj = JsonParser.parseString(line).getAsJsonObject();
            results.add(GSON.toJson(solve(obj.get("unit").getAsString())));
        }
        Files.write(output, results, StandardCharsets.UTF_8);
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String[] parts = unit.split(":");
        if (parts.length != 4) {
            out.addProperty("missing", true);
            return out;
        }
        BellAttachType attachment = switch (parts[0]) {
            case "floor" -> BellAttachType.FLOOR;
            case "ceiling" -> BellAttachType.CEILING;
            case "single_wall" -> BellAttachType.SINGLE_WALL;
            case "double_wall" -> BellAttachType.DOUBLE_WALL;
            default -> null;
        };
        Direction facing = byName(parts[1]);
        Direction face = byName(parts[2]);
        double height;
        try {
            height = Double.parseDouble(parts[3]);
        } catch (NumberFormatException e) {
            out.addProperty("missing", true);
            return out;
        }
        if (attachment == null || facing == null || face == null) {
            out.addProperty("missing", true);
            return out;
        }

        out.addProperty("proper", isProperHit(attachment, facing, face, height));
        return out;
    }

    /** 逐字复现原版 {@code BellBlock#isProperHit}。 */
    private static boolean isProperHit(BellAttachType attachment, Direction bellFacing, Direction face, double hitY) {
        if (face.getAxis() != Direction.Axis.Y && !(hitY > 0.8124F)) {
            return switch (attachment) {
                case FLOOR -> bellFacing.getAxis() == face.getAxis();
                case SINGLE_WALL, DOUBLE_WALL -> bellFacing.getAxis() != face.getAxis();
                case CEILING -> true;
            };
        }
        return false;
    }

    private static Direction byName(String value) {
        for (Direction direction : FACES) {
            if (direction.getSerializedName().equals(value)) {
                return direction;
            }
        }
        return null;
    }
}
