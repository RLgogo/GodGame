#include "core/Game.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "entity/Boss.h"
#include "entity/Enemy.h"
#include "entity/Entity.h"
#include "entity/Pickup.h"
#include "entity/Player.h"
#include "entity/actor/Buffs.h"
#include "utils/simpleIni/config.h"

namespace {

using Clock = std::chrono::steady_clock;

// 一次最多补多少逻辑时间：断点回来/系统卡顿时不要爆追
constexpr double kMaxFrameTime = 0.25;

// 伤害连锁最多几轮（死亡爆炸引发死亡爆炸的链）
constexpr int kMaxChainDepth = 8;

// 敌人剩这么多就算"快放完了"，开始挂 Boss 警告
constexpr int kBossWarnRemainder = 50;

// 旧版的出生距离要求
constexpr double kEnemyMinSpawnDistance = 300.0;
constexpr double kBossMinSpawnDistance  = 650.0;

// 待结算伤害占总血量的比例（血条上那一截白条）。
// 渲染层只认这个数字，具体是"白条"还是别的什么延迟结算机制它不管
float pendingRatioOf(const Actor& actor) {
    if (actor.health().maxHp() <= 0.0f) return 0.0f;

    return std::min(1.0f, actor.pendingDamage() / actor.health().maxHp());
}

Clock::duration toClockDuration(double seconds) {
    return std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(seconds));
}

// 只有一位的掩码返回它的位号，否则 -1（多于一位就得回退到全表扫描）
int bitIndexOf(KindMask mask) {
    for (int i = 0; i < kKindSlots; ++i) {
        if (mask & (KindMask{1} << i)) {
            return (mask & (mask - 1)) == 0 ? i : -1;
        }
    }
    return -1;
}

}

Game::Game() : rng_(std::random_device{}()) {
    expToNext_ = config().expBase;
}

Game::~Game() {
    stop();
}

// =================================== 生命周期 ===================================

void Game::start() {
    stop();        // 先保证上一个线程真的退了（join 完再重来）
    reset();
    running_ = true;
    worker_ = std::thread(&Game::run, this);
}

void Game::stop() {
    running_ = false;
    if (worker_.joinable()) {
        worker_.join();
    }
}

void Game::reset() {
    slots_.clear();
    freeSlots_.clear();
    bullets_.clear();

    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingSpawn_.clear();
        pendingHits_.clear();
        pendingCommands_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(inputMutex_);
        input_ = InputState{};
    }
    stepInput_ = InputState{};

    indexDirty_ = true;

    accumulator_ = 0.0;
    simulated_   = 0.0;
    tickCount_   = 0;
    gameOver_    = false;
    paused_      = false;

    score_        = 0;
    level_        = 1;
    exp_          = 0;
    expToNext_    = config().expBase;
    levelUpFlash_ = 0;

    // 旧版 resetGame() 直接从第 1 回合开打，没有开场等待
    round_            = 1;
    roundPhase_       = RoundPhase::Spawning;
    remainingToSpawn_ = config().enemyPerRound * round_;
    spawnTimer_       = 0.0f;
    phaseTimer_       = 0.0f;
    bossWarnTimer_    = 0.0f;
    bossActive_       = false;

    playerId_ = EntityId{};

    pendingSpawn_.push_back(std::make_unique<Player>(QPointF(450.0, 350.0)));
    commitSpawn();

    {
        std::lock_guard<std::mutex> lock(inputMutex_);
        stepInput_ = input_;
    }

    publishFrame();
}

// ================================== UI 线程接口 ==================================

Game::FramePtr Game::frame() const {
    std::lock_guard<std::mutex> lock(frameMutex_);
    return frame_;
}

void Game::setInput(const InputState& input) {
    std::lock_guard<std::mutex> lock(inputMutex_);
    input_ = input;
}

void Game::setPaused(bool paused) {
    paused_ = paused;
}

void Game::postCommand(Command command) {
    std::lock_guard<std::mutex> lock(pendingMutex_);
    pendingCommands_.push_back(command);
}

void Game::advance(float dt) {
    if (paused_ || gameOver_) {
        return;
    }
    step(dt);
    publishFrame(false);   // 手动驱动：不插值，发布的就是刚算完这一步的状态
}

void Game::advanceFrame(double frameTime) {
    const double dt = 1.0 / static_cast<double>(std::max(1, config().tickRate));

    if (paused_ || gameOver_) {
        // 世界不动，欠账也别攒着 —— 否则一恢复就瞬移一大段
        accumulator_ = 0.0;
    } else {
        accumulator_ += std::min(frameTime, kMaxFrameTime);

        // 硬上限：卡顿之后最多补这么多步，追不上就把余数丢掉
        const int maxSteps = static_cast<int>(kMaxFrameTime / dt) + 1;
        int steps = 0;
        while (accumulator_ >= dt && steps < maxSteps) {
            step(static_cast<float>(dt));
            accumulator_ -= dt;
            ++steps;
        }
        if (steps >= maxSteps && accumulator_ >= dt) {
            accumulator_ = std::fmod(accumulator_, dt);
        }
    }

    publishFrame();
}

// =================================== 逻辑线程 ===================================

void Game::run() {
    const double frameGap   = 1.0 / static_cast<double>(std::max(1, config().frameRate));

    auto nextFrame = Clock::now();
    auto last      = Clock::now();

    while (running_) {
        const auto now = Clock::now();
        const double frameTime = std::chrono::duration<double>(now - last).count();
        last = now;

        advanceFrame(frameTime);

        nextFrame += toClockDuration(frameGap);
        const auto after = Clock::now();
        if (nextFrame > after) {
            std::this_thread::sleep_until(nextFrame);
        } else {
            nextFrame = after;   // 落后太多就重新对表，别一直追
        }
    }
}

void Game::step(float dt) {
    drainCommands();

    {
        std::lock_guard<std::mutex> lock(inputMutex_);
        stepInput_ = input_;
    }

    simulated_ += dt;
    ++tickCount_;
    if (levelUpFlash_ > 0) {
        --levelUpFlash_;
    }

    updateRound(dt);

    // 实体各自更新（走路、开火、撞人）—— 它们之间不互相 erase，只 kill()
    for (Slot& slot : slots_) {
        if (slot.entity && slot.entity->alive()) {
            slot.entity->savePrevState();   // 记下起点，渲染插值要用
            slot.entity->update(dt, *this);
        }
    }

    updateBullets(dt);      // 位移 + 命中入队
    commitDamage();         // 统一结算这一 tick 攒下的伤害（含死亡与奖励）
    commitSpawn();          // 提交新生成的实体并回填 EntityId
    sweep();                // 回收死掉的槽位（死亡的结算也在这里）
    rebuildIndex();
    bossActive_ = (findBoss() != nullptr);
}

// ------------------------------- 提交 / 回收 -------------------------------

void Game::commitSpawn() {
    std::vector<std::unique_ptr<Entity>> batch;
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        if (pendingSpawn_.empty()) {
            return;
        }
        batch = std::move(pendingSpawn_);   // 在锁外真正插入
    }

    for (auto& entity : batch) {
        if (!entity) {
            continue;
        }

        std::uint32_t index = 0;
        std::uint32_t gen   = 0;

        if (!freeSlots_.empty()) {
            index = freeSlots_.back();
            freeSlots_.pop_back();
            gen = ++slots_[index].gen;            // 复用槽位：老句柄就此失效
            slots_[index].entity = std::move(entity);
        } else {
            index = static_cast<std::uint32_t>(slots_.size());
            gen   = 1;                            // 世代号从 1 开始，0 表示"空句柄"
            Slot slot;
            slot.gen    = gen;
            slot.entity = std::move(entity);
            slots_.push_back(std::move(slot));
        }

        slots_[index].entity->id_ = EntityId{index, gen};
        indexDirty_ = true;

        // 玩家是"世界上唯一那个"：入列的时候顺手记下它的句柄，
        // 之后 Enemy/Boss 找玩家、HUD 取血量都靠它。
        if (dynamic_cast<Player*>(slots_[index].entity.get()) != nullptr) {
            playerId_ = slots_[index].entity->id();
        }
    }
}

void Game::commitDamage() {
    for (int depth = 0; depth < kMaxChainDepth; ++depth) {
        std::vector<std::pair<EntityId, DamageInfo>> batch;
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            if (pendingHits_.empty()) {
                return;
            }
            batch = std::move(pendingHits_);
        }

        for (auto& [target, damage] : batch) {
            Entity* entity = get(target);
            if (!entity || !entity->alive()) {
                continue;
            }
            if (Actor* actor = dynamic_cast<Actor*>(entity)) {
                const DamageResult result = actor->takeDamage(damage);
                if (result.killed) {
                    awardKill(*entity);
                }
            }
        }
    }
}

void Game::sweep() {
    bool removed = false;
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        Slot& slot = slots_[i];
        if (!slot.entity || slot.entity->alive()) {
            continue;
        }

        // 这里是"死亡"唯一的结算点。伤害管线里死掉的人在 commitDamage
        // 已经结过账了（rewarded_ 挡住重复），但有些死亡是绕开管线的 ——
        // 白条到点扣真血就是一条，那条路上没人记得发奖励、也没人宣布 GameOver
        awardKill(*slot.entity);

        slot.entity.reset();
        freeSlots_.push_back(static_cast<std::uint32_t>(i));
        removed = true;
    }
    if (removed) {
        indexDirty_ = true;
    }
}

void Game::rebuildIndex() const {
    for (auto& bucket : byKind_) {
        bucket.clear();
    }
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        const Slot& slot = slots_[i];
        if (!slot.entity || !slot.entity->alive()) {
            continue;
        }
        const KindMask kinds = slot.entity->kinds();
        for (int bit = 0; bit < kKindSlots; ++bit) {
            if (kinds & (KindMask{1} << bit)) {
                byKind_[static_cast<std::size_t>(bit)].push_back(
                    EntityId{static_cast<std::uint32_t>(i), slot.gen});
            }
        }
    }
    indexDirty_ = false;
}

// =================================== World ===================================

std::vector<EntityId> Game::query(KindMask mask) const {
    if (indexDirty_) {
        rebuildIndex();
    }

    std::vector<EntityId> result;
    if (mask == 0) {
        return result;
    }

    // 只要一位就查索引；多位的（比如"所有能挨打的东西"）回退全表扫描
    const int single = bitIndexOf(mask);
    if (single >= 0) {
        for (EntityId id : byKind_[static_cast<std::size_t>(single)]) {
            if (peek(id)) {
                result.push_back(id);
            }
        }
        return result;
    }

    for (std::size_t i = 0; i < slots_.size(); ++i) {
        const Slot& slot = slots_[i];
        if (!slot.entity || !slot.entity->alive()) {
            continue;
        }
        if ((slot.entity->kinds() & mask) == 0) {
            continue;
        }
        result.push_back(EntityId{static_cast<std::uint32_t>(i), slot.gen});
    }
    return result;
}

const Entity* Game::peek(EntityId id) const {
    if (!id.valid() || id.index >= slots_.size()) {
        return nullptr;
    }
    const Slot& slot = slots_[id.index];
    if (slot.gen != id.gen) {
        return nullptr;
    }
    return slot.entity.get();
}

Entity* Game::get(EntityId id) {
    return const_cast<Entity*>(peek(id));
}

Entity* Game::findBoss() {
    for (EntityId id : query(kindBit(Kind::Boss))) {
        Entity* entity = get(id);
        if (entity && entity->alive()) {
            return entity;
        }
    }
    return nullptr;
}

Player* Game::player() {
    // 阵亡后槽位会被 sweep 回收，这里就返回 nullptr，
    // 于是 buildHud 走 else 分支、界面显示最后一刻缓存下来的数字
    return dynamic_cast<Player*>(get(playerId_));
}

void Game::spawn(std::unique_ptr<Entity> entity) {
    if (!entity) {
        return;
    }
    std::lock_guard<std::mutex> lock(pendingMutex_);
    pendingSpawn_.push_back(std::move(entity));
}

void Game::queueDamage(EntityId target, const DamageInfo& damage) {
    std::lock_guard<std::mutex> lock(pendingMutex_);
    pendingHits_.emplace_back(target, damage);
}

void Game::spawnBullet(const Bullet& bullet) {
    bullets_.spawn(bullet);
}

void Game::addScore(int amount) {
    score_ += amount;
}

int Game::enemyCount() const {
    int count = 0;
    for (const Slot& slot : slots_) {
        if (slot.entity && slot.entity->alive() &&
            (slot.entity->kinds() & kindBit(Kind::Enemy))) {
            ++count;
        }
    }
    return count;
}

float Game::worldWidth() const {
    return static_cast<float>(config().worldWidth);
}

float Game::worldHeight() const {
    return static_cast<float>(config().worldHeight);
}

float Game::randomRange(float min, float max) {
    if (max <= min) {
        return min;
    }
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng_);
}

int Game::randomInt(int min, int max) {
    if (max <= min) {
        return min;
    }
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng_);
}

// ================================== 回合推进 ==================================

float Game::spawnIntervalForRound() const {
    // 旧版：3 + 19 / (1 + exp(round - 6) / 1.5)，round 越大间隔越短，下限 3 帧
    const float r        = static_cast<float>(round_);
    const float interval = config().enemySpawnBase
                         + config().enemySpawnAmp
                               / (1.0f + std::exp(r - config().enemySpawnCenter)
                                             / config().enemySpawnK);
    return std::max(config().enemySpawnMin, interval);
}

void Game::updateRound(float dt) {
    // "快放完了"和"等 Boss"这两段，警告一直亮着
    const bool warn = (roundPhase_ == RoundPhase::Spawning &&
                       remainingToSpawn_ <= kBossWarnRemainder) ||
                      roundPhase_ == RoundPhase::BossPending;
    bossWarnTimer_ = warn ? config().bossWarnTime : 0.0f;

    switch (roundPhase_) {
        case RoundPhase::Spawning: {
            spawnTimer_ -= dt;
            if (remainingToSpawn_ > 0 &&
                enemyCount() < config().enemyAtLeast &&
                spawnTimer_ <= 0.0f) {
                spawnEnemy();
                spawnTimer_ = spawnIntervalForRound();
            }
            if (remainingToSpawn_ <= 0) {
                roundPhase_ = RoundPhase::BossPending;
                phaseTimer_ = config().roundWaitTime;
            }
            break;
        }

        case RoundPhase::BossPending: {
            phaseTimer_ -= dt;
            if (phaseTimer_ <= 0.0f) {
                spawnBoss();
                roundPhase_ = RoundPhase::BossFight;
                spawnTimer_ = spawnIntervalForRound();
            }
            break;
        }

        case RoundPhase::BossFight: {
            // Boss 战期间敌人照样补到保底数量（旧版 state3 就是这样）
            spawnTimer_ -= dt;
            if (bossActive_ && enemyCount() < config().enemyAtLeast && spawnTimer_ <= 0.0f) {
                spawnEnemy();
                spawnTimer_ = spawnIntervalForRound();
            }
            if (!bossActive_ && enemyCount() == 0) {
                roundPhase_ = RoundPhase::Cleared;
                phaseTimer_ = config().roundWaitTime;
            }
            break;
        }

        case RoundPhase::Cleared: {
            phaseTimer_ -= dt;
            if (phaseTimer_ <= 0.0f) {
                // 新回合：清掉残余，重置出场配额
                // 注意这里顺手修掉了旧版的 bug —— 旧版清场只清显示列表，
                // 而且 spawnInterval 是另一处算的，重开一局会沿用上一局的残留值
                for (EntityId id : query(kindBit(Kind::Enemy))) {
                    if (Entity* entity = get(id)) {
                        entity->kill();
                    }
                }
                ++round_;
                remainingToSpawn_ = config().enemyPerRound * round_;
                spawnTimer_       = 0.0f;
                roundPhase_       = RoundPhase::Spawning;
            }
            break;
        }
    }
}

void Game::spawnEnemy() {
    if (remainingToSpawn_ > 0) {
        --remainingToSpawn_;
    }

    const float width  = static_cast<float>(config().enemyWidth);
    const float height = static_cast<float>(config().enemyHeight);

    QPointF playerCenter(worldWidth() * 0.5f, worldHeight() * 0.5f);
    if (const Entity* p = peek(playerId_)) {
        playerCenter = QPointF(p->bounds().center());
    }

    // 旧版：随机取点，要求离玩家中心至少 300，最多试 50 次
    QPointF pos(0.0, 0.0);
    for (int attempt = 0; attempt < 50; ++attempt) {
        pos = QPointF(randomRange(0.0f, worldWidth() - width),
                      randomRange(0.0f, worldHeight() - height));
        const QPointF center(pos.x() + width * 0.5, pos.y() + height * 0.5);
        const double dx = center.x() - playerCenter.x();
        const double dy = center.y() - playerCenter.y();
        if (dx * dx + dy * dy >= kEnemyMinSpawnDistance * kEnemyMinSpawnDistance) {
            break;
        }
    }
    // 50 次都不满意就用最后一次采样（旧版是直接落到 (0,0)，那是个 bug）

    const float hpGrowth    = 1.0f + config().enemyHpGrowth * static_cast<float>(round_ - 1);
    const float speedGrowth = 1.0f + config().enemySpeedGrowth * static_cast<float>(round_ - 1);
    const float speed       = randomRange(config().enemySpeedMin, config().enemySpeedMax) * speedGrowth;
    const bool  dasher      = randomRange(0.0f, 1.0f) < config().enemyDashChance;

    spawn(std::make_unique<Enemy>(pos, speed, dasher, config().enemyHp * hpGrowth));
}

void Game::spawnBoss() {
    const float width  = static_cast<float>(config().bossWidth);
    const float height = static_cast<float>(config().bossHeight);

    QPointF playerCenter(worldWidth() * 0.5f, worldHeight() * 0.5f);
    if (const Entity* p = peek(playerId_)) {
        playerCenter = QPointF(p->bounds().center());
    }

    // 旧版：采样 100 次，取第一个离玩家 >= 650 的；都不满足就取最远的那个
    QPointF best(0.0, 0.0);
    double  bestDist = -1.0;
    for (int attempt = 0; attempt < 100; ++attempt) {
        const QPointF pos(randomRange(0.0f, worldWidth() - width),
                          randomRange(0.0f, worldHeight() - height));
        const QPointF center(pos.x() + width * 0.5, pos.y() + height * 0.5);
        const double dx = center.x() - playerCenter.x();
        const double dy = center.y() - playerCenter.y();
        const double distSq = dx * dx + dy * dy;
        if (distSq >= kBossMinSpawnDistance * kBossMinSpawnDistance) {
            best = pos;
            bestDist = distSq;
            break;
        }
        if (distSq > bestDist) {
            bestDist = distSq;
            best = pos;
        }
    }

    // 旧版 bossMaxHp 每出场一次 +10（起始 30）：这里换成 config 的 base + growth
    const float hp = config().bossHpBase + config().bossHpGrowth * static_cast<float>(round_ - 1);

    spawn(std::make_unique<Boss>(best, hp));
    bossActive_    = true;
    bossWarnTimer_ = 0.0f;
}

// ================================== 子弹与命中 ==================================

void Game::updateBullets(float dt) {
    bullets_.update(dt, worldWidth(), worldHeight());
    handleBulletHits();
}

void Game::handleBulletHits() {
    const std::vector<EntityId> targets = query(kindBit(Kind::Damageable));
    const EntityId              pid     = playerId_;

    for (std::size_t i = bullets_.all().size(); i-- > 0;) {
        const Bullet& bullet = bullets_.all()[i];

        for (EntityId targetId : targets) {
            Entity* target = get(targetId);
            if (!target || !target->alive()) {
                continue;
            }

            const bool isPlayer = (targetId == pid);
            if (bullet.fromPlayer == isPlayer) {
                continue;   // 自己的子弹不打自己，敌人的子弹只打玩家
            }

            // 圆 vs 矩形：把圆心夹进矩形取最近点，距离小于半径就算命中
            const QRect rect = target->bounds();
            const float cx = std::clamp(bullet.x, static_cast<float>(rect.left()),
                                        static_cast<float>(rect.right()));
            const float cy = std::clamp(bullet.y, static_cast<float>(rect.top()),
                                        static_cast<float>(rect.bottom()));
            const float dx = bullet.x - cx;
            const float dy = bullet.y - cy;
            if (dx * dx + dy * dy > bullet.radius * bullet.radius) {
                continue;
            }

            DamageInfo damage;
            damage.amount = bullet.damage;
            damage.types  = damageBit(bullet.type);
            damage.source = bullet.owner;
            damage.hitPos = QPointF(bullet.x, bullet.y);
            queueDamage(targetId, damage);

            bullets_.killAt(i);   // 一颗子弹只打一个目标
            break;
        }
    }
}

// ==================================== 奖励 ====================================

void Game::dropLoot(const QPointF& center, bool both) {
    const float size = static_cast<float>(config().pickupSize);
    const QPointF pos(center.x() - size * 0.5, center.y() - size * 0.5);

    if (both || randomRange(0.0f, 1.0f) < config().pickupAmmoChance) {
        spawn(std::make_unique<Pickup>(pos, Pickup::PickupType::Ammo,
                                       randomInt(config().pickupAmmoMin, config().pickupAmmoMax)));
    }
    if (both || randomRange(0.0f, 1.0f) < config().pickupHealthChance) {
        spawn(std::make_unique<Pickup>(pos, Pickup::PickupType::Health,
                                       config().pickupHealthAmount));
    }
}

void Game::awardKill(Entity& victim) {
    // 一次死亡只结一次账。伤害管线里发现 killed 会调这里，
    // sweep 发现死实体也会调这里 —— 谁先看到都行，但只有第一个算数
    if (victim.rewarded_) return;
    victim.rewarded_ = true;

    const KindMask kinds = victim.kinds();

    if (kinds & kindBit(Kind::Player)) {
        gameOver_ = true;
        return;
    }

    if (kinds & kindBit(Kind::Boss)) {
        score_ += config().bossDeathScore;
        grantExp(config().bossDeathExp);
        dropLoot(QPointF(victim.bounds().center()), true);   // Boss 必掉两样
        bossActive_ = false;
        return;
    }

    if (kinds & kindBit(Kind::Enemy)) {
        score_ += config().enemyDeathScore;
        grantExp(config().enemyDeathExp);
        dropLoot(QPointF(victim.bounds().center()), false);
    }
}

void Game::grantExp(int amount) {
    exp_ += amount;

    Player* p = player();
    while (level_ < config().maxLevel && expToNext_ > 0 && exp_ >= expToNext_) {
        exp_ -= expToNext_;
        ++level_;
        expToNext_ += config().expStep;

        LevelUpReward reward;
        reward.ammoBonus = config().ammoPerLevel;
        if (p) {
            reward.healAmount = p->health().maxHp() * config().healOnLevelUp;
        }
        // 每 5 级额外奖励一次体力/弹匣上限（旧版是 level % 5 == 0）
        if (config().bonusEveryLevels > 0 && level_ % config().bonusEveryLevels == 0) {
            reward.maxStaminaBonus   = config().bonusMaxStamina;
            reward.staminaRegenDelta = config().bonusStaminaRegen;
            reward.staminaDrainDelta = config().bonusStaminaDrain;
            reward.maxAmmoBonus      = config().bonusMaxAmmo;
        }
        if (p) {
            p->applyLevelUp(reward);
        }
        levelUpFlash_ = static_cast<int>(config().levelUpFlashTime * static_cast<float>(config().tickRate));
    }
}

// =================================== 控制台 ===================================

void Game::drainCommands() {
    std::vector<Command> batch;
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        if (pendingCommands_.empty()) {
            return;
        }
        batch = std::move(pendingCommands_);
    }

    for (Command command : batch) {
        if (command == Command::GrantInvincible) {
            grantInvincibleNow(config().consoleInvincibleTime);
        } else if (command == Command::ClearEnemies) {
            clearEnemiesNow();
        }
    }
}

void Game::grantInvincibleNow(float seconds) {
    if (Player* p = player()) {
        p->tryAddStatus(std::make_unique<InvincibleBuff>(seconds));
    }
}

void Game::clearEnemiesNow() {
    // 旧版 boom：场上敌人全清 + 每个都给分给经验（旧版只加 expValue 不跑升级判定，
    // 于是能攒出"下一次击杀连升好几级"，这里改成走正常的 grantExp）
    for (EntityId id : query(kindBit(Kind::Enemy))) {
        Entity* entity = get(id);
        if (!entity || !entity->alive()) {
            continue;
        }
        entity->kill();
        score_ += config().enemyDeathScore;
        grantExp(config().enemyDeathExp);
    }
}

// ==================================== 快照 ====================================

void Game::publishFrame(bool interpolate) {
    auto frame = std::make_shared<Frame>();

    // alpha 是"离下一次 tick 还有多远"：accumulator_ 里剩下的零头除以一步的长度。
    // 逻辑 20 TPS、画面 60 Hz —— 用它在上一 tick 与这一 tick 的位置之间插值，
    // 画面上才是连续移动，而不是每秒跳 20 下。
    //
    // 注意这条插值天然让画面比逻辑晚 0~1 个 tick（平均 25ms）：插值必须同时握着
    // "起点和终点"两个状态，所以画的永远是已经发生过的区间。这是标准做法，
    // 20 TPS 下这点延迟比不上输入本身就有的 50ms。
    const double stepDt = 1.0 / static_cast<double>(std::max(1, config().tickRate));
    const float  alpha  = interpolate
                          ? std::clamp(static_cast<float>(accumulator_ / stepDt), 0.0f, 1.0f)
                          : 1.0f;

    // 画序沿用旧版：玩家 → 子弹 → Boss → 敌人 → 补给
    const Kind ordered[] = {Kind::Player, Kind::Boss, Kind::Enemy, Kind::Pickup};
    for (Kind kind : ordered) {
        for (EntityId id : query(kindBit(kind))) {
            Entity* entity = get(id);
            if (entity && entity->alive()) {
                RenderItem item = entity->renderItem();
                entity->interpolate(item, alpha);
                frame->items.push_back(item);
            }
        }
        if (kind == Kind::Player) {
            bullets_.appendItems(frame->items, alpha);   // 子弹画在玩家之后、Boss 之前
        }
    }

    frame->hud = buildHud();

    std::lock_guard<std::mutex> lock(frameMutex_);
    frame_ = std::move(frame);
}

HudState Game::buildHud() {
    HudState hud;
    hud.status = gameOver_ ? GameStatus::GameOver
                           : (paused_ ? GameStatus::Paused : GameStatus::Playing);

    if (Player* p = player()) {
        hudHp_         = static_cast<int>(std::ceil(p->health().hp()));
        hudMaxHp_      = static_cast<int>(p->health().maxHp());
        hudAmmo_       = p->ammo();
        hudMaxAmmo_    = p->maxAmmo();
        hudStamina_    = p->stamina();
        hudMaxStamina_ = p->maxStamina();
        hudExhausted_  = p->exhausted();
        hud.hpPending  = pendingRatioOf(*p);
    } else {
        hudHp_ = 0;   // 玩家不在场上只可能是阵亡
    }

    hud.hp         = hudHp_;
    hud.maxHp      = hudMaxHp_;
    hud.ammo       = hudAmmo_;
    hud.maxAmmo    = hudMaxAmmo_;
    hud.stamina    = hudStamina_;
    hud.maxStamina = hudMaxStamina_;
    hud.exhausted  = hudExhausted_;

    hud.level        = level_;
    hud.score        = score_;
    hud.exp          = exp_;
    hud.expNext      = expToNext_;
    hud.expRatio     = expToNext_ > 0 ? static_cast<float>(exp_) / static_cast<float>(expToNext_) : 0.0f;
    hud.levelUpFlash = levelUpFlash_;
    hud.round        = round_;
    hud.enemiesLeft  = remainingToSpawn_ + enemyCount() + (bossActive_ ? 1 : 0);
    hud.roundWait    = (roundPhase_ == RoundPhase::Cleared)
                           ? static_cast<int>(phaseTimer_ * static_cast<float>(config().tickRate))
                           : 0;
    hud.bossActive   = bossActive_;
    hud.bossWarning  = static_cast<int>(bossWarnTimer_ * static_cast<float>(config().tickRate));
    hud.elapsed      = simulated_;

    if (Entity* boss = findBoss()) {
        if (const Actor* actor = dynamic_cast<const Actor*>(boss)) {
            hud.bossHp      = static_cast<int>(std::ceil(actor->health().hp()));
            hud.bossMaxHp   = static_cast<int>(actor->health().maxHp());
            hud.bossHpRatio = actor->health().ratio();
            hud.bossPending = pendingRatioOf(*actor);
        }
    }

    return hud;
}
