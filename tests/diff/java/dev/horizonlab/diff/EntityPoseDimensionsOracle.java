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
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.entity.EntityDimensions;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.entity.Pose;

/**
 * 实体在指定姿态下尺寸的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EntityPoseDimensionsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <h2>为什么对 SLEEPING 单独处理</h2>
 *
 * {@code LivingEntity.getDimensions(Pose)} 是 <b>public final</b>：
 *
 * <pre>
 *   return pose == Pose.SLEEPING
 *       ? SLEEPING_DIMENSIONS                                  // fixed(0.2, 0.2).withEyeHeight(0.2)
 *       : getDefaultDimensions(pose).scale(getScale());
 * </pre>
 *
 * {@code SLEEPING_DIMENSIONS} 是 {@code LivingEntity} 的静态常量，**与实体类型、年龄、
 * 缩放全都无关**。其余姿态下成年体的默认实现等价于
 * {@code getType().getDimensions().scale(getAgeScale())}，而成年体 age scale 为 1。
 *
 * 因此本能力对成年体只比对两件事：
 *
 * 1. {@code SLEEPING} 是否恒为 (0.2, 0.2, 0.2)；
 * 2. 其余姿态是否等于该类型的注册尺寸。
 *
 * 这样不需要构造实体实例（{@code EntityType.create} 必须传非空 Level），
 * 也不需要跑世界，保持纯查表。
 */
public final class EntityPoseDimensionsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** SLEEPING 姿态的固定尺寸，取自 LivingEntity.SLEEPING_DIMENSIONS。 */
    private static final float SLEEPING_SIZE = 0.2F;

    private static final String[] TYPES = {
        "minecraft:pig",
        "minecraft:player",
        "minecraft:zombie",
        "minecraft:creeper",
        "minecraft:sniffer",
        "minecraft:goat",
        "minecraft:polar_bear",
        "minecraft:phantom",
        "minecraft:pufferfish",
        "minecraft:slime",
        "minecraft:bat",
        "minecraft:axolotl",
        "minecraft:frog",
        "minecraft:warden",
        "minecraft:breeze",
        "minecraft:allay",
    };

    private EntityPoseDimensionsOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String type : TYPES) {
                for (Pose pose : Pose.values()) {
                    units.add(type + "|" + pose.getSerializedName());
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
            "usage: EntityPoseDimensionsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        String[] parts = unit.split("\\|", -1);
        boolean sleeping = parts[1].equals("sleeping");

        var id = net.minecraft.resources.Identifier.tryParse(parts[0]);
        EntityType<?> type = id == null
            ? null
            : BuiltInRegistries.ENTITY_TYPE.get(id).map(h -> h.value()).orElse(null);
        if (type == null) {
            out.addProperty("missing", true);
            return out;
        }

        EntityDimensions dims;
        if (sleeping) {
            // 所有生物：LivingEntity.getDimensions 对 SLEEPING 返回固定尺寸。
            dims = EntityDimensions.fixed(SLEEPING_SIZE, SLEEPING_SIZE).withEyeHeight(SLEEPING_SIZE);
        } else if (parts[0].equals("minecraft:player")) {
            // 玩家是 Avatar，覆写 getDefaultDimensions 用一张**姿态表**（Avatar.POSES），
            // 未列出的姿态回落到 STANDING_DIMENSIONS。逐条取自 Avatar.java:20-35。
            dims = switch (parts[1]) {
                case "fall_flying", "swimming", "spin_attack" ->
                    EntityDimensions.scalable(0.6F, 0.6F).withEyeHeight(0.4F);
                case "crouching" -> EntityDimensions.scalable(0.6F, 1.5F).withEyeHeight(1.27F);
                default -> EntityDimensions.scalable(0.6F, 1.8F).withEyeHeight(1.62F);
            };
        } else if (parts[0].equals("minecraft:goat") && parts[1].equals("long_jumping")) {
            // 山羊长跳：原版 Goat.LONG_JUMPING_DIMENSIONS = scalable(0.9, 1.3).scale(0.7)，
            // 即 0.63 x 0.91，eyeHeight 也乘 0.7。
            dims = EntityDimensions.scalable(0.9F, 1.3F).scale(0.7F);
        } else if (parts[0].equals("minecraft:pufferfish")) {
            // 河豚：getDefaultDimensions 对类型尺寸再 scale(getScale(PUFF_STATE))。
            // 新建实例是 STATE_SMALL=0，getScale(0)=0.5，故 0.7 x 0.7 -> 0.35 x 0.35，
            // eyeHeight 也乘 0.5（0.595 -> 0.2975）。逐条取自 Pufferfish.java:175-186。
            dims = EntityDimensions.scalable(0.7F, 0.7F).scale(0.5F);
        } else {
            // 其余实体：成年体的默认实现等价于类型注册尺寸。
            dims = type.getDimensions();
        }

        out.addProperty("width", bits(dims.width()));
        out.addProperty("height", bits(dims.height()));
        out.addProperty("eyeHeight", bits(dims.eyeHeight()));
        return out;
    }

    private static String bits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }
}
