#include "entity/Enemy.h"

#include <algorithm>
#include <cmath>

#include "core/Damage.h"
#include "core/World.h"
#include "entity/Entity.h"
#include "utils/simpleIni/config.h"

namespace {
constexpr double kEpsilon = 1e-6;

// 旧版分档阈值：speed>=4 / >=2 / 其余（px/帧）换算成 px/秒
constexpr float kTierFast = 250.0f;
constexpr float kTierMid  = 125.0f;
}

Enemy::Enemy(const QPointF& pos, float speed, bool dasher, float hp)
    : Actor(pos, QSize(config().enemyWidth, config().enemyHeight))
    , speed_(speed)
    , dasher_(dasher)
    , blinkTimer_(config().enemyBlinkInterval) {
    health_.reset(std::max(1.0f, hp));
}

void Enemy::tick(float dt, World& world) {
    chase(dt, world);
    if (dasher_) blinkIfReady(dt, world);
    hitPlayerIfTouching(world);
}

void Enemy::chase(float dt, World& world) {
    Entity* target = world.get(world.playerId());
    if (!target) return;

    const QPointF center = precisePos_ + QPointF(rect_.width() * 0.5, rect_.height() * 0.5);
    const QPointF tc     = target->center();

    const double dx = tc.x() - center.x();
    const double dy = tc.y() - center.y();
    const double len = std::sqrt(dx * dx + dy * dy);
    if (len < kEpsilon) return;

    const double step = speed_ * speedMultiplier() * dt;
    precisePos_ += QPointF(dx / len * step, dy / len * step);
    syncRect();
}

void Enemy::blinkIfReady(float dt, World& world) {
    blinkTimer_ -= dt;
    if (blinkTimer_ > 0.0f) return;

    // 旧版计时到点必重置（不管有没有真的闪），这里保持同样的节拍
    blinkTimer_ = config().enemyBlinkInterval;

    Entity* target = world.get(world.playerId());
    if (!target) return;

    const QPointF center = precisePos_ + QPointF(rect_.width() * 0.5, rect_.height() * 0.5);
    const QPointF tc     = target->center();

    const double dx = tc.x() - center.x();
    const double dy = tc.y() - center.y();
    const double len = std::sqrt(dx * dx + dy * dy);

    // 离得够远才闪：否则贴脸瞬移等于直接穿模
    if (len < config().enemyBlinkMinDistance) return;

    const double step = config().enemyBlinkDistance;
    precisePos_ += QPointF(dx / len * step, dy / len * step);

    // 旧版没有边界 clamp，闪完可能落在画面外（然后只能傻乎乎往回走）。
    // 这里夹一下，行为上更接近"闪现到玩家附近"
    const double maxX = static_cast<double>(world.worldWidth()) - rect_.width();
    const double maxY = static_cast<double>(world.worldHeight()) - rect_.height();
    precisePos_.setX(std::clamp(precisePos_.x(), 0.0, std::max(0.0, maxX)));
    precisePos_.setY(std::clamp(precisePos_.y(), 0.0, std::max(0.0, maxY)));
    syncRect();
}

void Enemy::hitPlayerIfTouching(World& world) {
    const EntityId pid = world.playerId();
    Entity* target = world.get(pid);
    if (!target || !target->alive()) return;

    if (!rect_.intersects(target->bounds())) return;

    // 撞上就同归于尽：自己死，玩家挨一下。
    // 伤害走队列，所以"玩家正在无敌"这件事由玩家的 buff 决定，敌人不用管
    DamageInfo damage;
    damage.amount = config().enemyContactDamage;
    damage.types  = damageBit(DamageType::Contact);
    damage.source = id();
    world.queueDamage(pid, damage);

    kill();   // 撞击致死：不计分、不掉落（旧版就是这样，而且那个分支会 return 掉整帧逻辑）
}

std::uint8_t Enemy::speedTier() const {
    if (speed_ >= kTierFast) return 0;
    if (speed_ >= kTierMid)  return 1;
    return 2;
}

RenderItem Enemy::renderItem() const {
    RenderItem item = Entity::renderItem();
    item.style   = dasher_ ? RenderStyle::EnemyDasher : RenderStyle::Enemy;
    item.variant = speedTier();
    fillCommon(item);
    return item;
}
