/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import net.minecraft.util.Mth;

/**
 * 护甲减伤公式的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DamageReductionOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.DamageReductionOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/damage_reduction.cpp 同构：单元是「伤害 × 护甲 × 韧性」的组合，输出该组合下
 * 的最终伤害，以及破甲附魔 1..4 级的版本。
 *
 * <p>{@code CombatRules.getDamageAfterAbsorb} 的签名要 {@code LivingEntity} 与 {@code DamageSource}，
 * 构造不出「任意护甲值 / 韧性值」的实体，因此这里按它的无武器路径逐行展开；破甲修正
 * （{@code EnchantmentHelper.modifyArmorEffectiveness}）同样展开成每级 -0.15 的加法。
 */
public final class DamageReductionOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 伤害取值：覆盖「小于护甲」「卡在 clamp 边界」「远大于护甲」三种情形。 */
    private static final float[] DAMAGES = {
        0.0F, 0.5F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 8.0F, 10.0F, 12.0F, 15.0F, 20.0F, 30.0F, 50.0F
    };

    /** 护甲取值：0 到 30（原版上限）逐点取值。 */
    private static final float[] ARMORS = {
        0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F, 10.0F, 11.0F,
        12.0F, 13.0F, 14.0F, 15.0F, 16.0F, 17.0F, 18.0F, 19.0F, 20.0F, 24.0F, 30.0F
    };

    /** 韧性取值：0（无护甲）、2（钻石全套）、3（下界合金全套）、4/8/20（附魔与极限情况）。 */
    private static final float[] TOUGHNESSES = {0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 8.0F, 20.0F};

    /** 原版 {@code BreachEnchantment} 每级对护甲有效率的修正（`perLevel(-0.15)`）。 */
    private static final float BREACH_STEP = -0.15F;

    private DamageReductionOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (float damage : DAMAGES) {
                for (float armor : ARMORS) {
                    for (float toughness : TOUGHNESSES) {
                        text.append(GSON.toJson(unit(damage, armor, toughness))).append('\n');
                    }
                }
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: DamageReductionOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static JsonObject unit(float damage, float armor, float toughness) {
        JsonObject unit = new JsonObject();
        // 比对键："damage/armor/toughness" 三位一体，单看伤害会让不同护甲的组合撞键。
        unit.addProperty("unit", damage + "/" + armor + "/" + toughness);
        unit.addProperty("damage", damage);
        unit.addProperty("armor", armor);
        unit.addProperty("toughness", toughness);
        return unit;
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            float damage = request.get("damage").getAsFloat();
            float armor = request.get("armor").getAsFloat();
            float toughness = request.get("toughness").getAsFloat();
            text.append(GSON.toJson(summarise(damage, armor, toughness))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(float damage, float armor, float toughness) {
        JsonObject result = unit(damage, armor, toughness);
        result.addProperty("absorb", absorb(damage, armor, toughness, 0.0F));
        for (int level = 1; level <= 4; ++level) {
            result.addProperty("absorbBreach" + level, absorb(damage, armor, toughness, BREACH_STEP * level));
        }
        return result;
    }

    /**
     * 与 {@code CombatRules.getDamageAfterAbsorb} 的无武器路径逐行等价。
     *
     * <p>原文（1.21.11）：
     * <pre>
     *   float f = 2.0F + toughness / 4.0F;
     *   float f1 = Mth.clamp(armor - damage / f, armor * 0.2F, 20.0F);
     *   float f2 = f1 / 25.0F;
     *   float f3 = weaponItem == null ? f2 : clamp(modifyArmorEffectiveness(...), 0, 1);
     *   return damage * (1.0F - f3);
     * </pre>
     */
    private static float absorb(float damage, float armor, float toughness, float armorEffectivenessModifier) {
        float f = 2.0F + toughness / 4.0F;
        float f1 = Mth.clamp(armor - damage / f, armor * 0.2F, 20.0F);
        float f2 = f1 / 25.0F;
        float f3 = armorEffectivenessModifier == 0.0F
                ? f2
                : Mth.clamp(f2 + armorEffectivenessModifier, 0.0F, 1.0F);
        return damage * (1.0F - f3);
    }
}