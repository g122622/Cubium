package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.level.block.Blocks;

/**
 * 岩浆与相邻流体/方块相遇产物的原版 oracle（{@code LiquidBlock#shouldSpreadLiquid}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.FluidLavaReactOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code lava_source:north:water}（自身 : 方位 : 邻居）。
 *
 * <pre>
 *   岩浆源   + 水             -> 黑曜石
 *   流动岩浆 + 水             -> 圆石
 *   岩浆     + 蓝冰 + 灵魂土  -> 玄武岩
 *   其它                      -> 不变
 * </pre>
 *
 * <p>两个易错点：
 *
 * <ol>
 *   <li><b>源与流动的产物不同</b>：写成一个，玩家浇岩浆造石头时得到黑曜石，或造黑曜石时只得到圆石
 *   <li><b>只看水平四个方向，不看正上方</b>：原版 {@code POSSIBLE_FLOW_DIRECTIONS} 是 DOWN + 四个水平方向。
 *       往岩浆正上方倒水不会当场凝固
 * </ol>
 *
 * <p>原版成功凝固时会调用 {@code fizz} 发 levelEvent 1501（嗤声与烟），失败则不发。
 */
public final class FluidLavaReactOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] SELF_KINDS = {"lava_source", "lava_flowing"};
    private static final String[] DIRECTIONS = {"north", "south", "east", "west"};
    private static final String[] NEIGHBOR_KINDS = {"water", "blue_ice", "air"};

    private FluidLavaReactOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String self : SELF_KINDS) {
                for (String dir : DIRECTIONS) {
                    for (String neighbor : NEIGHBOR_KINDS) {
                        units.add(self + ":" + dir + ":" + neighbor);
                    }
                }
            }
            java.util.Collections.sort(units);
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: FluidLavaReactOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length != 3) {
            out.addProperty("missing", true);
            return out;
        }
        String self = parts[0];
        String dir = parts[1];
        String neighbor = parts[2];

        boolean selfKnown = false;
        for (String s : SELF_KINDS) {
            if (s.equals(self)) {
                selfKnown = true;
                break;
            }
        }
        boolean dirKnown = false;
        for (String d : DIRECTIONS) {
            if (d.equals(dir)) {
                dirKnown = true;
                break;
            }
        }
        boolean neighborKnown = false;
        for (String n : NEIGHBOR_KINDS) {
            if (n.equals(neighbor)) {
                neighborKnown = true;
                break;
            }
        }
        if (!selfKnown || !dirKnown || !neighborKnown) {
            out.addProperty("missing", true);
            return out;
        }

        boolean isSource = self.equals("lava_source");

        // 按原版 shouldSpreadLiquid 的判定顺序：
        // 逐水平方向检查，遇到水 -> 源出黑曜石 / 流出圆石；否则若下方灵魂土且邻居蓝冰 -> 玄武岩
        String result = "minecraft:lava";
        JsonArray levelEvents = new JsonArray();

        if (neighbor.equals("water")) {
            result = isSource ? "minecraft:obsidian" : "minecraft:cobblestone";
            levelEvents.add(1501);
        } else if (neighbor.equals("blue_ice")) {
            // 探针场景恒在下方放灵魂土，所以岩浆 + 蓝冰 一律出玄武岩
            result = "minecraft:basalt";
            levelEvents.add(1501);
        }
        // neighbor == air 时不变

        // 用注册表校验名称确实存在（防止拼写错误制造假一致）
        out.addProperty("result", BuiltInRegistries.BLOCK.containsKey(
            net.minecraft.resources.Identifier.parse(result)) ? result : "UNKNOWN:" + result);
        out.add("levelEvents", levelEvents);
        return out;
    }
}
