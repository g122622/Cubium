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
import net.minecraft.world.level.gameevent.GameEvent;

/**
 * 重生锚充能/放电时的层数变化与游戏事件的原版 oracle（{@code RespawnAnchorBlock#charge}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.AnchorChargeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code 2:charge}（当前 CHARGES : 动作）。
 *
 * <p>规则：
 *
 * <ul>
 *   <li>充能：层数 +1，最多到 4（MAX_CHARGES）；已满时不变、不发事件
 *   <li>放电：层数 -1，最少到 0；已空时不变、不发事件
 * </ul>
 *
 * <p>两种变化都发 {@code block_change}。
 *
 * <p>已达上限还继续加会让层数溢出到 5，光照等级（charges*3.75）算出 18，超出亮度上限。
 * 已满还发事件则每次往满锚上点萤石都会惊动侦测器。
 */
public final class AnchorChargeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 原版 RespawnAnchorBlock.MAX_CHARGES */
    private static final int MAX_CHARGES = 4;

    private AnchorChargeOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (int charges = 0; charges <= MAX_CHARGES; charges++) {
                units.add(charges + ":charge");
                units.add(charges + ":discharge");
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: AnchorChargeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        int sep = unit.indexOf(':');
        if (sep < 0) {
            out.addProperty("missing", true);
            return out;
        }
        int charges;
        try {
            charges = Integer.parseInt(unit.substring(0, sep));
        } catch (NumberFormatException e) {
            out.addProperty("missing", true);
            return out;
        }
        String action = unit.substring(sep + 1);
        if (charges < 0 || charges > MAX_CHARGES || (!action.equals("charge") && !action.equals("discharge"))) {
            out.addProperty("missing", true);
            return out;
        }

        boolean changes;
        int after;
        if (action.equals("charge")) {
            changes = charges < MAX_CHARGES;
            after = changes ? charges + 1 : charges;
        } else {
            changes = charges > 0;
            after = changes ? charges - 1 : charges;
        }
        out.addProperty("chargesAfter", after);

        JsonArray events = new JsonArray();
        if (changes) {
            events.add(BuiltInRegistries.GAME_EVENT.getKey(GameEvent.BLOCK_CHANGE.value()).toString());
        }
        out.add("gameEvents", events);
        return out;
    }
}
