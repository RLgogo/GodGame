#ifndef GODGAME_HEALTH_H
#define GODGAME_HEALTH_H

#include <algorithm>

#include "core/EntityId.h"

// 只管血量本身。
//
// 免疫、无敌帧、护盾、白条一个字段都没有 —— 那些是状态，有持续时间、
// 有叠层、有渲染表现、能被驱散，全部属于 buff。一旦把它们塞进来，
// 这个类就会长成 bossInvincible / bossInvincibleTimer / bossWhiteHp /
// bossWhiteDelay 那种字段堆。
class Health {
public:
    Health() = default;
    explicit Health(float maxHp) { reset(maxHp); }

    void reset(float maxHp) {
        maxHp_ = std::max(1.0f, maxHp);
        hp_    = maxHp_;
        lastHitBy_ = EntityId{};
    }

    float hp() const    { return hp_; }
    float maxHp() const { return maxHp_; }
    float ratio() const { return maxHp_ > 0.0f ? hp_ / maxHp_ : 0.0f; }

    // 用 <= 判断，永远别写 == 0：浮点误差会让"剩 1e-7 血"的单位永远不死
    bool dead() const { return hp_ <= 0.0f; }

    // 血量本身不带"不能治疗死人"这种规则：该不该给死人回血是 buff / 玩法的事。
    // （原来这里是 if (!dead()) —— 恰好把"复活甲"这种最需要回血的场景堵死了）
    void heal(float v)     { hp_ = std::clamp(hp_ + v, 0.0f, maxHp_); }
    void raiseMax(float v) { maxHp_ = std::max(1.0f, maxHp_ + v); hp_ = std::min(hp_ + v, maxHp_); }

    // 只在伤害管线内部调用（它是唯一该动血量的地方）。
    // 地板压到 0：否则超杀会欠出一大笔负血，复活时怎么加都加不回来
    void rawDamage(float v) { hp_ = std::max(0.0f, hp_ - v); }

    EntityId lastHitBy() const { return lastHitBy_; }
    void setLastHitBy(EntityId id) { lastHitBy_ = id; }

private:
    float    hp_    = 1.0f;
    float    maxHp_ = 1.0f;
    EntityId lastHitBy_;   // 击杀归属 / 仇恨
};

#endif //GODGAME_HEALTH_H
