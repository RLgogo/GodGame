#ifndef GODGAME_BUFFS_H
#define GODGAME_BUFFS_H

#include <algorithm>

#include "core/Damage.h"
#include "entity/actor/Actor.h"
#include "entity/actor/Buff.h"

// 通用伤害免疫。
// "爆炸免疫"只是换个 mask 参数 —— 加一种免疫永远不用改实体类。
class DamageImmunityBuff : public Buff {
public:
    explicit DamageImmunityBuff(DamageMask mask, float duration = -1.0f) : mask_(mask) {
        setDuration(duration);
    }
    BuffId id() const override { return BuffId::DamageImmunity; }
    int priority() const override { return 0; }                    // 最先处理
    StackPolicy stackPolicy() const override { return StackPolicy::Ignore; }

    void onIncomingDamage(DamageInfo& damage, Actor& /*owner*/) override {
        if (damage.types & mask_) damage.cancelled = true;
    }

private:
    DamageMask mask_;
};

// 无敌帧。渲染层靠 status().has(BuffId::Invincible) 决定闪不闪 ——
// 逻辑和渲染共享同一份状态，不用两边各维护一个计时器。
class InvincibleBuff : public Buff {
public:
    explicit InvincibleBuff(float duration) { setDuration(duration); }
    BuffId id() const override { return BuffId::Invincible; }
    int priority() const override { return 10; }

    void onIncomingDamage(DamageInfo& damage, Actor& /*owner*/) override {
        if (!(damage.types & damageBit(DamageType::True))) damage.cancelled = true;  // 真伤打穿
    }
};

// 霸体：免疫控制类状态。这是"状态免疫"那个入口的示例，
// 走的是 StatusList::add 里的 onIncomingStatus，和伤害管线完全无关。
class SuperArmorBuff : public Buff {
public:
    explicit SuperArmorBuff(float duration) { setDuration(duration); }
    BuffId id() const override { return BuffId::SuperArmor; }
    StackPolicy stackPolicy() const override { return StackPolicy::Ignore; }

    bool onIncomingStatus(BuffId incoming) override {
        return incoming != BuffId::Chilled;   // 减速被挡住，其余照常
    }
};

// 数值护盾：先吃伤害，吃够了就碎。gauge() 就是渲染层画的那条附加白条。
class ShieldBuff : public Buff {
public:
    ShieldBuff(float amount, float duration = -1.0f) : amount_(amount) {
        setDuration(duration);
    }
    BuffId id() const override { return BuffId::Shield; }
    int priority() const override { return 100; }                  // 吸收阶段
    StackPolicy stackPolicy() const override { return StackPolicy::Ignore; }

    void onAbsorbDamage(float& remaining, DamageResult& result, Actor& /*owner*/) override {
        const float take = std::min(amount_, remaining);
        amount_    -= take;
        remaining  -= take;
        result.absorbed += take;
        if (amount_ <= 0.0f) setDuration(0.0f);   // 碎了，交给下一轮 update 清理
    }

    float gauge() const override { return amount_; }

private:
    float amount_ = 0.0f;
};

// 白条：挨打时先把伤害存起来，延迟一段时间后按固定速度转成真血损失。
//
// 这是"延迟结算"而不是"减伤"：伤害一点没少，只是晚一点到账。所以它套不了
// ShieldBuff 那套（吃够就碎、账当场结清），得自己记一本账。
//
// 老版 Boss 用的就是这套，当年是三个字段（bossWhiteHp / bossWhiteDelay /
// 结算分支）散在结构体和 update 里；现在它就是这一个类，而且任何单位都能挂。
class WhiteShieldBuff : public Buff {
public:
    struct Tuning {
        float delay = 0.6f;      // 挨打之后先冻住这么久
        float rate  = 25.0f;     // 冻结期过后，每秒转多少成真血
        float maxSettle = 1.5f;  // 账再大，也得在这个时间内结完
    };

    explicit WhiteShieldBuff(const Tuning& tuning) : tuning_(tuning) {}

    BuffId      id() const override { return BuffId::WhiteShield; }
    int         priority() const override { return 150; }   // 排在护盾后面：护盾先吃
    StackPolicy stackPolicy() const override { return StackPolicy::Ignore; }

    void onAbsorbDamage(float& remaining, DamageResult& result, Actor& /*owner*/) override {
        if (remaining <= 0.0f) return;

        white_ += remaining;          // 不设上限：设了就等于悄悄吞掉多出来的伤害
        result.absorbed += remaining;
        remaining = 0.0f;             // 全吃下
        settleTimer_ = tuning_.delay; // 每挨一次打，冻结期重新开始
        rate_ = 0.0f;                 // 转账速度要等冻完了按新的账重新算
    }

    void onUpdate(float dt, Actor& owner) override {
        if (white_ <= 0.0f) return;

        if (settleTimer_ > 0.0f) {
            settleTimer_ -= dt;
            if (settleTimer_ > 0.0f) return;

            // 冻完了，这时候才把转账速度定下来：账越大转得越快，
            // 但再大也拖不过 maxSettle 秒。用定额 rate 的话，
            // 一发 999 的暴击会让 BOSS 揣着必死的账继续追杀你五秒
            rate_ = std::max(tuning_.rate, white_ / std::max(0.01f, tuning_.maxSettle));
        }

        const float take = std::min(white_, rate_ * dt);
        white_ -= take;
        owner.rawDamageAndCheckDeath(take);   // 账转成真血
    }

    // 到期 = 账必须当场结清，不能揣着一身伤害跑掉
    void onExpire(Actor& owner) override {
        if (white_ <= 0.0f) return;

        const float take = white_;
        white_ = 0.0f;
        owner.rawDamageAndCheckDeath(take);
    }

    float gauge() const override { return white_; }

private:
    Tuning tuning_;
    float  white_       = 0.0f;
    float  settleTimer_ = 0.0f;
    float  rate_        = 0.0f;   // 本次结算锁定下来的转账速度，见 onUpdate
};

// 减速。它不碰实体的任何字段，只是把速度系数挂出去，
// 移动的代码统一乘 Actor::speedMultiplier() ——
// 于是"减速从哪来"（冰面、迟缓、泥地）和"谁在移动"彻底解耦
class ChilledBuff : public Buff {
public:
    ChilledBuff(float multiplier, float duration) : multiplier_(multiplier) {
        setDuration(duration);
    }

    BuffId id() const override { return BuffId::Chilled; }
    float  speedMultiplier() const override { return multiplier_; }

private:
    float multiplier_ = 0.6f;
};

#endif //GODGAME_BUFFS_H
