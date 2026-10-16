#ifndef GODGAME_PICKUP_H
#define GODGAME_PICKUP_H

#include "core/RenderItem.h"
#include "entity/actor/Actor.h"

// 补给：弹药箱和血包。
//
// 它们和敌人共用"实体 + 生命周期"这套东西，但没人会去打它们
// （kinds() 里没有 Damageable），所以子弹会直接穿过去。
class Pickup : public Actor {
public:
    enum class PickupType : std::uint8_t {
        Ammo = 0,
        Health,
    };

    Pickup(const QPointF& pos, PickupType type, int amount);

    KindMask kinds() const override { return kindBit(Kind::Pickup); }
    RenderItem renderItem() const override;

    PickupType pickupType() const { return type_; }
    int        amount() const     { return amount_; }

protected:
    void tick(float dt, World& world) override;

private:
    PickupType type_   = PickupType::Ammo;
    int        amount_ = 0;
    float      life_   = 0.0f;
};

#endif //GODGAME_PICKUP_H
