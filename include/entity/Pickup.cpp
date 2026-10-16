#include "entity/Pickup.h"

#include "core/World.h"
#include "entity/Player.h"
#include "utils/simpleIni/config.h"

namespace {
// 旧版快消失时每 6 tick 翻转一次显隐（即 0.096 秒一格）
constexpr float kBlinkCellSeconds = 6.0f / 62.5f;
constexpr float kBlinkWindowRatio = 0.25f;   // 剩余寿命后 25% 开始闪（旧版是最后 300/1200 帧）
}

Pickup::Pickup(const QPointF& pos, PickupType type, int amount)
    : Actor(pos, QSize(config().pickupSize, config().pickupSize))
    , type_(type)
    , amount_(amount)
    , life_(config().pickupLifetime) {
}

void Pickup::tick(float dt, World& world) {
    life_ -= dt;
    if (life_ <= 0.0f) {
        kill();
        return;
    }

    const EntityId pid = world.playerId();
    Entity* target = world.get(pid);
    if (!target || !target->alive()) return;
    if (!rect_.intersects(target->bounds())) return;

    // 血量和弹药都是玩家自己的字段，补给只负责"把这份东西交过去"。
    // 这里需要认识 Player —— 想彻底不认识，就得给 Actor 开一堆 setter，
    // 那等于把 Player 的接口搬进基类，不值
    if (Player* player = dynamic_cast<Player*>(target)) {
        if (type_ == PickupType::Ammo) player->addAmmo(amount_);
        else                           player->health().heal(static_cast<float>(amount_));
        kill();
    }
}

RenderItem Pickup::renderItem() const {
    RenderItem item = Entity::renderItem();
    item.style = (type_ == PickupType::Ammo) ? RenderStyle::AmmoBox : RenderStyle::HealthPack;

    // 快消失时闪。闪烁这件事由逻辑决定、渲染层照做 —— 两边各算一遍相位
    // 迟早会对不上
    const float window = config().pickupLifetime * kBlinkWindowRatio;
    if (life_ < window) {
        const int phase = static_cast<int>(life_ / kBlinkCellSeconds);
        item.flash = (phase % 2) == 0;
    }

    fillCommon(item);
    return item;
}
