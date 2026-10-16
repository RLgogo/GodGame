#ifndef GODGAME_ACTOR_H
#define GODGAME_ACTOR_H

#include <memory>

#include "core/Damage.h"
#include "core/RenderItem.h"
#include "entity/Entity.h"
#include "entity/actor/Buff.h"
#include "entity/actor/Health.h"
#include "entity/actor/StatusList.h"

// 有血量、能被伤害、能挂状态的实体。
//
// 这里的字段只有 health_ + status_：血量之外的东西（免疫、无敌帧、护盾、
// 白条）一个都不在，全都是 buff。
class Actor : public Entity {
public:
    Actor() = default;
    explicit Actor(const QPointF& pos, const QSize& size = QSize())
        : Entity(pos, size) {}

    // 全游戏唯一的伤害入口。所有伤害都必须从这里进
    DamageResult takeDamage(const DamageInfo& damage);

    // 全游戏唯一的加状态入口
    bool tryAddStatus(std::unique_ptr<Buff> buff);

    // 绕过伤害管线的直接扣血，但死亡流程照走（白条延迟结算用）。
    // 见实现里的注释：这正是"延迟结算"和"绕过规则"的区别
    void rawDamageAndCheckDeath(float amount);

    // 所有 buff 聚合出的移动速度系数。实体移动时乘一下即可
    float speedMultiplier() const;

    // 已经记在账上、还没转成真血的伤害（白条那一截）。
    // 它虽然还没进 Health，但迟早要进 —— 所以"我还剩多少血"这类判断
    // 必须把它算进去，否则白条厚的单位会晚一截才变身/才认输
    float pendingDamage() const;

    // 框架层：先推进 buff，再跑具体实体的行为
    void update(float dt, World& world) override {
        status_.update(dt, *this);
        tick(dt, world);
    }

    Health&           health()       { return health_; }
    const Health&     health() const { return health_; }
    StatusList&       status()       { return status_; }
    const StatusList& status() const { return status_; }

protected:
    // 具体实体的行为。没有叫 update，是为了不让"实体行为"和"状态更新"混在一起
    virtual void tick(float /*dt*/, World& /*world*/) {}
    virtual void onDamaged(const DamageResult& /*result*/, const DamageInfo& /*damage*/) {}
    virtual void onDeath() {}

    bool tryRevive();                            // 给 buff 一次否决死亡的机会
    void fillCommon(RenderItem& item) const;     // 血量/白条/闪烁，派生类填外观时调一次

    Health     health_;
    StatusList status_;
};

#endif //GODGAME_ACTOR_H
