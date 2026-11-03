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
import net.minecraft.core.component.DataComponents;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;

/**
 * 唱片机交互判定的原版 oracle（{@code JukeboxPlayable#tryInsertIntoJukebox} 的分支表）。
 *
 * <pre>
 *   java dev.horizonlab.diff.JukeboxInsertOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code music_disc_cat:false}（手持物标签:HAS_RECORD）。
 *
 * <p>判定直接取决于原版的两条事实：
 * <ul>
 *   <li>物品是否带 {@code DataComponents.JUKEBOX_PLAYABLE}
 *   <li>方块状态的 {@code HAS_RECORD}
 * </ul>
 * 「机器里有唱片」优先于「手里拿什么」—— 拿着另一张唱片点也是先弹旧的。
 */
public final class JukeboxInsertOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] FIXED_LABELS = {"empty", "stick"};
    private static final String[] FIXED_IDS = {"", "minecraft:stick"};

    /** 注册表里全部带 {@code jukebox_playable} 组件的物品，取路径部分作标签，按 id 排序。 */
    private static List<String> musicDiscLabels() {
        List<String> labels = new ArrayList<>();
        for (Item item : BuiltInRegistries.ITEM) {
            if (new ItemStack(item).get(DataComponents.JUKEBOX_PLAYABLE) != null) {
                Identifier id = BuiltInRegistries.ITEM.getKey(item);
                labels.add(id.getPath());
            }
        }
        labels.sort(String::compareTo);
        return labels;
    }

    /** 由标签反查物品 id：先查固定样本，再查唱片物品。找不到返回 null。 */
    private static String resolveItemId(String label) {
        for (int i = 0; i < FIXED_LABELS.length; i++) {
            if (FIXED_LABELS[i].equals(label)) {
                return FIXED_IDS[i];
            }
        }
        for (Item item : BuiltInRegistries.ITEM) {
            if (new ItemStack(item).get(DataComponents.JUKEBOX_PLAYABLE) != null) {
                Identifier id = BuiltInRegistries.ITEM.getKey(item);
                if (id.getPath().equals(label)) {
                    return id.toString();
                }
            }
        }
        return null;
    }

    private JukeboxInsertOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            // 固定样本
            for (String label : FIXED_LABELS) {
                units.add(label + ":false");
                units.add(label + ":true");
            }
            // 全部唱片物品（按 id 排序），标签取路径部分
            for (String label : musicDiscLabels()) {
                units.add(label + ":false");
                units.add(label + ":true");
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: JukeboxInsertOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        int colon = unit.lastIndexOf(':');
        if (colon < 0) {
            out.addProperty("missing", true);
            return out;
        }
        String heldLabel = unit.substring(0, colon);
        boolean hasRecord = unit.substring(colon + 1).equals("true");

        String heldId = resolveItemId(heldLabel);
        if (heldId == null) {
            out.addProperty("missing", true);
            return out;
        }

        // 原版 JukeboxBlock#useItemOn 的分支顺序：
        //   1. HAS_RECORD 为真 -> TRY_WITH_EMPTY_HAND（交给 useWithoutItem 弹唱片）
        //   2. 否则看手持物是否有 jukebox_playable 组件 -> 塞入 / TRY_WITH_EMPTY_HAND
        String action;
        if (hasRecord) {
            action = "eject";
        } else if (hasJukeboxPlayable(heldId)) {
            action = "insert";
        } else {
            action = "try_empty_hand";
        }

        out.addProperty("action", action);
        return out;
    }

    /** 物品是否带 {@code jukebox_playable} 组件（空手一律 false）。 */
    private static boolean hasJukeboxPlayable(String itemId) {
        if (itemId.isEmpty()) {
            return false;
        }
        Identifier id = Identifier.tryParse(itemId);
        if (id == null) {
            return false;
        }
        Item item = BuiltInRegistries.ITEM.get(id).map(h -> h.value()).orElse(null);
        if (item == null) {
            return false;
        }
        ItemStack stack = new ItemStack(item);
        return stack.get(DataComponents.JUKEBOX_PLAYABLE) != null;
    }
}
