#include "entity/Player.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "core/Bullet.h"
#include "core/Lerp.h"
#include "core/World.h"
#include "entity/actor/Buffs.h"
#include "utils/simpleIni/config.h"

namespace {
constexpr double kEpsilon = 1e-6;
}

Player::Player(const QPointF& pos)
    : Actor(pos, QSize(config().playerWidth, config().playerHeight))
    , ammo_(config().playerAmmo)
    , maxAmmo_(config().playerMaxAmmo)
    , stamina_(config().staminaMax)
    , maxStamina_(config().staminaMax)
    , staminaRegen_(config().staminaRegen)
    , staminaDrain_(config().staminaDrain) {
    health_.reset(static_cast<float>(config().playerHp));

    if (config().playerWhiteShield) {
        // 挨打时伤害先记在白条上，冻结 playerWhiteShieldDelay 秒后按每秒
        // playerWhiteShieldRate 点转成真血。伤害一点没少，只是晚到账 ——
        // 换来一小段"看见自己快死了"的时间。关掉它就是旧版那种立刻掉血
        WhiteShieldBuff::Tuning tuning;
        tuning.delay     = config().playerWhiteShieldDelay;
        tuning.rate      = config().playerWhiteShieldRate;
        tuning.maxSettle = config().whiteShieldMaxSettle;
        status_.add(std::make_unique<WhiteShieldBuff>(tuning), *this);
    }
}

void Player::addAmmo(int amount) {
    // 旧版这里是 ammo += 20 而不 clamp，多捡几个箱子就能超过 360 显示上限。
    // 上限存在的意义就是上限，所以这里夹住
    ammo_ = std::clamp(ammo_ + amount, 0, maxAmmo_);
}

void Player::applyLevelUp(const LevelUpReward& reward) {
    if (reward.healAmount > 0.0f) health_.heal(reward.healAmount);

    maxStamina_ = std::max(1.0f, maxStamina_ + reward.maxStaminaBonus);
    staminaRegen_ += reward.staminaRegenDelta;
    staminaDrain_ = std::max(0.0f, staminaDrain_ + reward.staminaDrainDelta);
    maxAmmo_ += reward.maxAmmoBonus;

    stamina_ = std::min(stamina_, maxStamina_);
    addAmmo(reward.ammoBonus);
}

void Player::tick(float dt, World& world) {
    updateAim(world);
    updateStamina(dt, world);
    move(dt, world);
    shoot(dt, world);
}

void Player::updateAim(World& world) {
    const InputState& in = world.input();
    if (!in.aimValid) return;

    const QPointF center = precisePos_ + QPointF(rect_.width() * 0.5, rect_.height() * 0.5);
    const double  dx = static_cast<double>(in.aimX) - center.x();
    const double  dy = static_cast<double>(in.aimY) - center.y();
    const double  len = std::sqrt(dx * dx + dy * dy);
    if (len < kEpsilon) return;   // 准星压在身上：保持上一个朝向，不要产生 (0,0) 方向

    aimDir_ = QPointF(dx / len, dy / len);
    angle_  = static_cast<float>(std::atan2(dy, dx));
}

void Player::updateStamina(float dt, World& world) {
    const InputState& in = world.input();
    const bool moving     = in.up || in.down || in.left || in.right;
    const bool wantSprint = in.sprint && moving;

    if (wantSprint && !exhausted_) {
        stamina_ = std::max(0.0f, stamina_ - staminaDrain_ * dt);
        if (stamina_ <= 0.0f) exhausted_ = true;   // 力竭：这一趟跑完必须缓过来
        return;
    }

    stamina_ = std::min(maxStamina_, stamina_ + staminaRegen_ * dt);
    if (exhausted_ && stamina_ >= maxStamina_ * config().staminaRecoverRatio) {
        exhausted_ = false;
    }
}

void Player::move(float dt, World& world) {
    const InputState& in = world.input();

    QPointF dir;
    if (in.left)  dir.setX(dir.x() - 1.0);
    if (in.right) dir.setX(dir.x() + 1.0);
    if (in.up)    dir.setY(dir.y() - 1.0);
    if (in.down)  dir.setY(dir.y() + 1.0);

    const double len = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
    if (len > kEpsilon) dir /= len;   // 斜着走不该比直着走快

    const bool  sprinting = in.sprint && !exhausted_;
    const float speed     = (sprinting ? config().playerSprintSpeed : config().playerSpeed)
                            * speedMultiplier();   // buff 的速度修正在这里生效

    velocity_ = dir * speed;
    precisePos_ += velocity_ * dt;

    // 不许出画面（旧版用的也是同一套 clamp）
    const double maxX = static_cast<double>(world.worldWidth()) - rect_.width();
    const double maxY = static_cast<double>(world.worldHeight()) - rect_.height();
    precisePos_.setX(std::clamp(precisePos_.x(), 0.0, std::max(0.0, maxX)));
    precisePos_.setY(std::clamp(precisePos_.y(), 0.0, std::max(0.0, maxY)));
    syncRect();
}

void Player::shoot(float dt, World& world) {
    fireTimer_ = std::max(0.0f, fireTimer_ - dt);

    const InputState& in = world.input();
    if (!in.fire || fireTimer_ > 0.0f) return;
    if (ammo_ <= 0) return;   // 没子弹就只是打不响，不做别的

    const QPointF center = precisePos_ + QPointF(rect_.width() * 0.5, rect_.height() * 0.5);

    Bullet bullet;
    // 从身体边缘出膛，免得子弹一开始就叠在玩家身上
    const float muzzle = static_cast<float>(rect_.width()) * 0.5f;
    bullet.x      = static_cast<float>(center.x()) + static_cast<float>(aimDir_.x()) * muzzle;
    bullet.y      = static_cast<float>(center.y()) + static_cast<float>(aimDir_.y()) * muzzle;
    // 旧版这里是 int(dx/dist*10)：整数截断之后 45° 方向每轴只剩 7，斜射会明显偏慢。
    // 现在直接存浮点方向，速度恒定
    bullet.vx     = static_cast<float>(aimDir_.x()) * config().bulletSpeed;
    bullet.vy     = static_cast<float>(aimDir_.y()) * config().bulletSpeed;
    bullet.radius = config().bulletRadius;
    bullet.damage = config().bulletDamage;
    bullet.life   = config().bulletLife;
    bullet.type   = DamageType::Bullet;
    bullet.fromPlayer = true;
    bullet.owner  = id();

    world.spawnBullet(bullet);

    ammo_ -= 1;
    fireTimer_ = config().playerFireInterval;
}

void Player::onDamaged(const DamageResult& result, const DamageInfo& damage) {
    (void)damage;

    // 只有"被完全免疫掉"的伤害不算挨打。被护盾/白条吸收的仍然是实打实的一击：
    // 白条尤其如此 —— 那笔账早晚要结，不给无敌帧的话 Boss 贴着站就能一帧记一笔，
    // 一秒堆出一千多点待结算伤害，白条直接变成秒杀计时器
    if (result.cancelled) return;

    // 挨打之后一段无敌帧。它是个 buff，不是玩家身上的一个计时器 ——
    // 于是"敌人追着你打"只是不停刷新它的时长，不会叠出一堆计时器
    status_.add(std::make_unique<InvincibleBuff>(config().playerInvincibleTime), *this);
}

RenderItem Player::renderItem() const {
    // 从基类版本起手：id / kinds / 位置 / 尺寸都由它填好
    RenderItem item = Entity::renderItem();
    item.style  = RenderStyle::Player;
    item.facing = aimDir_.x() < 0.0 ? -1.0f : 1.0f;
    item.angle  = angle_;

    fillCommon(item);   // 血条 / 附加条 / 无敌闪烁
    return item;
}

void Player::savePrevState() {
    Entity::savePrevState();
    prevAngle_ = angle_;
}

void Player::interpolate(RenderItem& item, float alpha) const {
    Entity::interpolate(item, alpha);   // 位置
    item.angle = lerpAngle(prevAngle_, item.angle, alpha);
}
