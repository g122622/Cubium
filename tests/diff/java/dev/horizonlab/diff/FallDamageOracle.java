package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.monster.zombie.Zombie;

/**
 * 摔落伤害的数值公式（原版 {@code LivingEntity.calculateFallDamage} / {@code calculateFallPower}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.FallDamageOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>{@code calculateFallDamage} 是 protected，且 {@code causeFallDamage} 会真的扣血、
 * 触发音效与伤害反馈，不适合当被测入口。这里用反射直接调 protected 方法，
 * 只比公式本身，两侧输入输出完全对称。
 *
 * <p>实体用 {@code EntityType.ZOMBIE} 构造、{@code Level} 传 {@code null}：
 * 属性表在构造函数里由 {@code DefaultAttributes} 建好，不依赖世界。
 */
public final class FallDamageOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private FallDamageOracle() {}

    /** 缓存反射到的 protected 方法。 */
    private static Method calculateFallDamageMethod;

    private static Method method() throws NoSuchMethodException {
        if (calculateFallDamageMethod == null) {
            Method found = LivingEntity.class.getDeclaredMethod("calculateFallDamage", double.class, float.class);
            found.setAccessible(true);
            calculateFallDamageMethod = found;
        }
        return calculateFallDamageMethod;
    }

    /**
     * 每个单元都新建实体，避免属性修改器在用例之间残留。
     *
     * <p>用 {@code Zombie} 而不是 {@code LivingEntity}：后者是抽象类无法实例化；而 Zombie
     * 没有覆写 {@code calculateFallDamage}，走的就是基类通用公式，正是这里要比的东西。
     *
     * <p>不写匿名子类是因为 {@code LivingEntity} 的构造签名是
     * {@code EntityType<? extends LivingEntity>}，{@code EntityType.ZOMBIE} 的类型参数却是
     * {@code Zombie}，通配符在这一处不接受隐式向上转换。
     */
    private static LivingEntity newEntity() {
        return new Zombie(EntityType.ZOMBIE, null);
    }

    private static String formatUnit(double distance, float multiplier) {
        return String.format(java.util.Locale.ROOT, "%.6f|%.3f", distance, (double) multiplier);
    }

    private static List<String> unitStrings() {
        List<String> list = new ArrayList<>();

        double[] nearZero = {0.0, 0.5, 1.0, 2.0, 2.999999, 3.0, 3.000001, 3.5, 4.0, 4.000001, 5.0};
        for (double distance : nearZero) {
            list.add(formatUnit(distance, 1.0f));
        }

        for (int whole = 1; whole <= 25; whole++) {
            list.add(formatUnit(whole, 1.0f));
            list.add(formatUnit(whole + 0.5, 1.0f));
            list.add(formatUnit(whole + 0.999, 1.0f));
        }

        float[] multipliers = {0.0f, 0.2f, 0.5f, 1.0f, 2.0f};
        double[] distances = {3.0, 4.0, 5.0, 8.0, 12.5, 20.0, 40.0};
        for (float multiplier : multipliers) {
            for (double distance : distances) {
                list.add(formatUnit(distance, multiplier));
            }
        }

        return list;
    }

    public static void main(String[] args) throws Exception {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : unitStrings()) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text.toString(), StandardCharsets.UTF_8);
            return;
        }

        if (args.length == 3 && args[0].equals("run")) {
            List<String> inputs = Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8);
            List<String> outputs = new ArrayList<>(inputs.size());
            for (String line : inputs) {
                if (line.isBlank()) {
                    continue;
                }
                outputs.add(runLine(line));
            }
            Files.write(Path.of(args[2]), outputs, StandardCharsets.UTF_8);
            return;
        }

        throw new IllegalArgumentException("usage: FallDamageOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) throws Exception {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int bar = requested.indexOf('|');
        if (bar < 0) {
            return missing(requested);
        }
        double distance = Double.parseDouble(requested.substring(0, bar));
        float multiplier = Float.parseFloat(requested.substring(bar + 1));

        LivingEntity entity = newEntity();
        int damage = (int) method().invoke(entity, distance, multiplier);

        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("damage", damage);
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}