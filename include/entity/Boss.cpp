#include "entity/Boss.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "core/Damage.h"
#include "core/World.h"
#include "entity/Enemy.h"
#include "entity/actor/Buffs.h"
#include "utils/simpleIni/config.h"

namespace {
constexpr double kEpsilon = 1e-6;
}

Boss::Boss(const QPointF& pos, float hp)
    : Actor(pos, QSize(config().bossWidth, config().bossHeight)) {
    health_.reset(std::max(1.0f, hp));

    if (config().bossWhiteShield) {
        // 冻结时长和追赶速度直接沿用旧版那两个常量：挨一次打冻住
        // bossWhiteFreezeTime，之后每秒把 max(1% 最大血量, 31.25) 点转成真血。
        // 旧版这条白条是纯画出来的（真血立刻掉、白条慢慢追），现在它真的记账
        WhiteShieldBuff::Tuning tuning;
        tuning.delay     = config().bossWhiteFreezeTime;
        tuning.rate      = std::max(health_.maxHp() * config().bossWhiteDropRatio,
                                    config().bossWhiteDropMin);
        tuning.maxSettle = config().whiteShieldMaxSettle;
        status_.add(std::make_unique<WhiteShieldBuff>(tuning), *this);
    }
}

void Boss::tick(float dt, World& world) {
    // 半血判定要算上白条：那笔账已经挨过了，只是还没转成真血。
    // 只看真血的话，玩家把白条打到六成、血还是满的，Boss 就不变身。
    // 待结算是"欠的债"，所以是减不是加
    const float effectiveHp = health_.hp() - pendingDamage();
    if (!enraged_ && effectiveHp <= health_.maxHp() * config().bossPhase2Ratio) {
        enterPhaseTwo(world);
    }

    // 无敌阶段原地不动（旧版行为：bossInvincible 期间停移动，但照样能撞人）
    if (status_.has(BuffId::Invincible)) return;

    chase(dt, world);
    hitPlayerIfTouching(world);
}

void Boss::enterPhaseTwo(World& world) {
    enraged_ = true;

    // 阶段切换的无敌窗口 —— 就是个带时长的 buff，不用再单独维护一个计时器
    status_.add(std::make_unique<InvincibleBuff>(config().bossInvincibleTime), *this);
    summonAdds(world);
}

void Boss::summonAdds(World& world) {
    const QPointF center = precisePos_ + QPointF(rect_.width() * 0.5, rect_.height() * 0.5);
    const float   spread = config().bossSummonSpread;

    for (int i = 0; i < config().bossSummonCount; ++i) {
        const float ox = world.randomRange(-spread, spread);
        const float oy = world.randomRange(-spread, spread);

        // 旧版召唤出来的是"speed=5 的非冲刺敌人"，换算成 px/秒
        world.spawn(std::make_unique<Enemy>(center + QPointF(ox, oy),
                                            config().bossSummonSpeed,
                                            false,
                                            config().enemyHp));
    }
}

void Boss::chase(float dt, World& world) {
    Entity* target = world.get(world.playerId());
    if (!target) return;

    const QPointF center = precisePos_ + QPointF(rect_.width() * 0.5, rect_.height() * 0.5);
    const QPointF tc     = target->center();

    const double dx = tc.x() - center.x();
    const double dy = tc.y() - center.y();
    const double len = std::sqrt(dx * dx + dy * dy);
    if (len < kEpsilon) return;

    const double step = config().bossSpeed * speedMultiplier() * dt;
    precisePos_ += QPointF(dx / len * step, dy / len * step);
    syncRect();
}

void Boss::hitPlayerIfTouching(World& world) {
    const EntityId pid = world.playerId();
    Entity* target = world.get(pid);
    if (!target || !target->alive()) return;
    if (!rect_.intersects(target->bounds())) return;

    // Boss 不会因为撞人而死（这点和杂兵不一样）
    DamageInfo damage;
    damage.amount = config().bossContactDamage;
    damage.types  = damageBit(DamageType::Contact);
    damage.source = id();
    world.queueDamage(pid, damage);
}

RenderItem Boss::renderItem() const {
    RenderItem item = Entity::renderItem();
    item.style = RenderStyle::Boss;
    fillCommon(item);   // 血条和白条（pending）都在这里面填
    return item;
}
