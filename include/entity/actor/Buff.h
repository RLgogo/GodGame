#ifndef GODGAME_BUFF_H
#define GODGAME_BUFF_H

#include <cstdint>

#include "core/Damage.h"

class Actor;

enum class BuffId : std::uint32_t {
    DamageImmunity = 0,   // 通用免疫（爆炸免疫是它的一个实例）
    Invincible,           // 无敌帧
    Shield,               // 数值护盾（先吃伤害）
    WhiteShield,          // 白条（延迟结算，boss 用）
    Burning,              // DoT
    Chilled,              // 减速
    SuperArmor,           // 霸体：免疫控制
    Count_
};

// 重复施加同一种 buff 时怎么办。不写死这个策略，迟早会出现
// "点两次毒药叠了两层、点两次减速时长反而变短"这种怪事。
enum class StackPolicy {
    Refresh,   // 重置时长，层数不变（默认）
    Stack,     // 层数 +1，效果按层数放大
    Ignore,    // 已存在就丢弃新的一次
    Replace    // 旧的销毁，用新的（不同强度的同类 debuff）
};

// 优先级区间（管线只遍历一次，三个阶段靠 priority 分区）：
//   0~19    免疫 / 取消      DamageImmunityBuff(0)、InvincibleBuff(10)
//   20~49   标记 / 破甲
//   50~99   加减修正         火抗 50%、易伤 +30%
//   100~199 吸收             白条 / 护盾
//   200+    只读观察         仇恨、伤害统计、飘字
class Buff {
public:
    virtual ~Buff() = default;

    virtual BuffId      id() const = 0;
    virtual StackPolicy stackPolicy() const { return StackPolicy::Refresh; }
    virtual int         priority() const { return 50; }
    virtual int         maxStacks() const { return 1; }

    // ---- 生命周期 ----
    virtual void onApply(Actor& /*owner*/) {}
    virtual void onRefresh(Actor& /*owner*/) {}
    virtual void onStack(int /*newStacks*/, Actor& /*owner*/) {}
    virtual void onUpdate(float /*dt*/, Actor& /*owner*/) {}
    virtual void onExpire(Actor& /*owner*/) {}

    // ---- 伤害管线 ----
    // 第一段：只改 multiplier / flatReduce / cancelled。乘法可交换，所以顺序无关
    virtual void onIncomingDamage(DamageInfo& /*damage*/, Actor& /*owner*/) {}
    // 第二段：吸收已经算好的最终伤害（护盾/白条在这里吃）
    virtual void onAbsorbDamage(float& /*remaining*/, DamageResult& /*result*/, Actor& /*owner*/) {}
    // 事后通知：不管有没有真掉血都会发（记账、飘字、仇恨）
    virtual void onDamageApplied(const DamageResult& /*result*/, Actor& /*owner*/) {}

    // false = 否决这个状态（霸体挡眩晕就走这里，和伤害免疫对称）
    virtual bool onIncomingStatus(BuffId /*incoming*/) { return true; }
    // true = 阻止这次死亡（复活甲）
    virtual bool onDeath(Actor& /*owner*/) { return false; }

    // ---- 时长 / 层数 ----
    float duration() const { return duration_; }        // < 0 = 永久
    void  setDuration(float seconds) { duration_ = seconds; }
    bool  expired() const { return duration_ >= 0.0f && duration_ <= 0.0f; }

    int   stacks() const { return stacks_; }
    void  setStacks(int n) { stacks_ = n; }
    float elapsed() const { return elapsed_; }

    // 由 StatusList 推进（不放在 protected 里，是为了让容器能统一推时间）
    void advance(float dt) {
        elapsed_ += dt;
        if (duration_ > 0.0f) {
            duration_ -= dt;
            // 夹到 0。负数在这个类里是"永久"的编码，不夹的话：
            // 一个剩 0.01 秒的 buff 走过这一帧就变成 duration_ = -0.04（永久不灭）
            if (duration_ < 0.0f) duration_ = 0.0f;
        }
    }

    // 血条上的附加条（白条/护盾）。渲染层直接读它，不用自己算
    virtual float gauge() const { return 0.0f; }

    // 属性修正：移动速度系数。默认不参与修正，减速类 buff 覆盖它。
    // 聚合方式是全部相乘（和伤害的 multiplier 同一个道理：乘法可交换，
    // 于是"先踩冰还是先中迟缓"这种顺序问题根本不存在）
    virtual float speedMultiplier() const { return 1.0f; }

protected:
    float duration_ = -1.0f;
    float elapsed_  = 0.0f;
    int   stacks_   = 1;
};

#endif //GODGAME_BUFF_H
