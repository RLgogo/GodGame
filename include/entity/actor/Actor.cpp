#include "entity/actor/Actor.h"

#include <algorithm>

#include "core/World.h"

namespace {
constexpr int kMaxDrainDepth = 8;   // 队列排空的硬上限，防"A 触发 B、B 触发 A"

bool listed(const std::vector<BuffId>& list, BuffId id) {
    return std::find(list.begin(), list.end(), id) != list.end();
}
} // namespace

// ============================== StatusList ==============================

bool StatusList::add(std::unique_ptr<Buff> buff, Actor& owner) {
    if (!buff) return false;

    if (busy_) {                        // 推进中/结算中：只排队，绝不改结构
        orderDirty_ = true;
        pendingAdded_.push_back(std::move(buff));
        return true;
    }
    return addNow(buff, owner);
}

bool StatusList::addNow(std::unique_ptr<Buff>& buff, Actor& owner) {
    // 1) 已有的 buff 先否决（霸体挡眩晕走这里，和伤害免疫对称）
    for (auto& existing : buffs_) {
        if (!existing->onIncomingStatus(buff->id())) return false;
    }

    // 2) 同类已存在？按它自己的叠层策略处理
    auto it = std::find_if(buffs_.begin(), buffs_.end(),
                           [&](const std::unique_ptr<Buff>& e) { return e->id() == buff->id(); });

    if (it == buffs_.end()) {
        Buff* raw = buff.get();
        buffs_.push_back(std::move(buff));
        orderDirty_ = true;
        DeferScope scope(*this);        // onApply 里再加 buff -> 进队列，不许递归
        raw->onApply(owner);
        return true;
    }

    Buff& existing = **it;
    switch (existing.stackPolicy()) {
        case StackPolicy::Ignore:
            return false;               // 丢弃新的一次

        case StackPolicy::Replace: {
            DeferScope scope(*this);
            existing.onExpire(owner);
            *it = std::move(buff);      // unique_ptr 赋值：旧的在这里被销毁。
                                        // 此处没有别人在遍历 buffs_，所以是安全的
            (*it)->onApply(owner);
            orderDirty_ = true;
            return true;
        }

        case StackPolicy::Refresh:
            existing.setDuration(buff->duration());   // 时长以新的为准，层数不动
            existing.onRefresh(owner);
            return true;

        case StackPolicy::Stack:
            existing.setDuration(buff->duration());
            if (existing.stacks() < existing.maxStacks()) {
                existing.setStacks(existing.stacks() + 1);
                existing.onStack(existing.stacks(), owner);
            }
            return true;
    }
    return false;
}

void StatusList::remove(BuffId id) {
    pendingRemoved_.push_back(id);
}

void StatusList::clear() {
    clearRequested_ = true;
}

bool StatusList::has(BuffId id) const {
    return find(id) != nullptr;
}

int StatusList::stacks(BuffId id) const {
    const Buff* b = find(id);
    return b ? b->stacks() : 0;
}

const Buff* StatusList::find(BuffId id) const {
    for (const auto& b : buffs_) {
        if (b->id() == id) return b.get();
    }
    return nullptr;
}

void StatusList::update(float dt, Actor& owner) {
    // ① 推进已有 buff。这一步 buffs_ 只读
    {
        DeferScope scope(*this);
        for (auto& b : buffs_) {
            b->advance(dt);
            b->onUpdate(dt, owner);
        }
    }

    // ② 排空请求队列。Replace/净化/onApply 可能再产生新请求，所以循环到排空为止。
    //    注意 expired() 也要当成"有活干"：否则一个有时长的 buff 在没人请求增删时
    //    永远等不到 onExpire，会一直挂在身上（这是这套延迟提交最容易踩的坑）。
    for (int depth = 0; depth < kMaxDrainDepth; ++depth) {
        bool hasExpired = false;
        for (const auto& b : buffs_) {
            if (b->expired()) { hasExpired = true; break; }
        }

        if (!clearRequested_ && pendingRemoved_.empty() && pendingAdded_.empty() && !hasExpired) break;

        if (clearRequested_) {                      // 全清（死亡 / 复活）
            for (auto& b : buffs_) b->onExpire(owner);
            buffs_.clear();
            pendingAdded_.clear();
            pendingRemoved_.clear();
            clearRequested_ = false;
            orderDirty_ = true;
            continue;
        }

        // 被请求移除的 + 自然过期的，一起发通知再一起摘掉
        for (auto& b : buffs_) {
            if (listed(pendingRemoved_, b->id()) || b->expired()) b->onExpire(owner);
        }
        const std::size_t before = buffs_.size();
        buffs_.erase(std::remove_if(buffs_.begin(), buffs_.end(),
                         [&](const std::unique_ptr<Buff>& b) {
                             return b->expired() || listed(pendingRemoved_, b->id());
                         }),
                     buffs_.end());
        if (buffs_.size() != before) orderDirty_ = true;
        pendingRemoved_.clear();

        // 提交当前这批新增；期间新产生的请求留给下一轮
        std::vector<std::unique_ptr<Buff>> batch = std::move(pendingAdded_);
        pendingAdded_.clear();
        for (auto& b : batch) addNow(b, owner);
    }

    // 深度用满还没排空：丢掉剩下的请求，别滚雪球
    pendingAdded_.clear();
    pendingRemoved_.clear();
    clearRequested_ = false;

    if (orderDirty_) rebuildOrder();
}

const std::vector<Buff*>& StatusList::ordered() const {
    if (orderDirty_) rebuildOrder();
    return ordered_;
}

void StatusList::rebuildOrder() const {
    ordered_.clear();
    ordered_.reserve(buffs_.size());
    for (const auto& b : buffs_) ordered_.push_back(b.get());

    std::stable_sort(ordered_.begin(), ordered_.end(),
                     [](const Buff* a, const Buff* b) { return a->priority() < b->priority(); });
    orderDirty_ = false;
}

// ================================ Actor ================================

DamageResult Actor::takeDamage(const DamageInfo& in) {
    // 必须拷贝：AoE 会拿同一份 DamageInfo 打一圈，buff 改的必须是各自这一份。
    // 否则第一个"爆炸免疫"的敌人会把 cancelled 写在公共数据上，后面全都免疫。
    DamageInfo damage = in;
    DamageResult result;

    StatusList::DeferScope scope(status_);   // 结算期间状态表结构冻结

    // ① 免疫 / 修正。只改 multiplier / flatReduce / cancelled（乘法可交换，顺序无关）
    for (Buff* b : status_.ordered()) b->onIncomingDamage(damage, *this);
    if (damage.cancelled) {
        result.cancelled = true;
        return result;
    }

    // 只要不是免疫，就算"被打到了"：记仇、破隐。哪怕最终 0 伤害
    health_.setLastHitBy(damage.source);

    float final = (damage.types & damageBit(DamageType::True))
                      ? damage.amount
                      : damage.amount * damage.multiplier - damage.flatReduce;
    if (final < 0.0f) final = 0.0f;

    // ② 吸收：护盾 / 白条在这里吃已经算好的最终值
    for (Buff* b : status_.ordered()) {
        if (final <= 0.0f) break;
        b->onAbsorbDamage(final, result, *this);
    }

    // ③ 真扣血
    if (final > 0.0f) {
        health_.rawDamage(final);
        result.applied = final;
    }

    // ④ 事后通知：不管有没有真掉血都发（飘字、统计、飘仇恨）
    for (Buff* b : status_.ordered()) b->onDamageApplied(result, *this);
    onDamaged(result, damage);

    // ⑤ 死亡。buff 可以否决一次（复活甲）
    if (health_.dead() && !tryRevive()) {
        result.killed = true;
        onDeath();
        kill();          // 只标记，真正的移除由 Game::sweep() 统一做
    }
    return result;
}

bool Actor::tryAddStatus(std::unique_ptr<Buff> buff) {
    return status_.add(std::move(buff), *this);
}

bool Actor::tryRevive() {
    for (Buff* b : status_.ordered()) {
        if (b->onDeath(*this)) return true;
    }
    return false;
}

// 绕过伤害管线的直接扣血。白条（延迟结算）用它把"早就裁定过的伤害"补上账：
// 那份伤害在被打中的那一刻已经走过免疫和护盾了，现在再走一遍就等于收两次税。
//
// 但"死了"这件事必须走完流程 —— 否则白条打死人会越过 onDeath 和复活甲，
// 留下一个既不会死也不会消失的单位
void Actor::rawDamageAndCheckDeath(float amount) {
    if (amount <= 0.0f) return;

    health_.rawDamage(amount);
    if (health_.dead() && !tryRevive()) {
        onDeath();
        kill();
    }
}

// 所有 buff 的移动速度修正相乘。实体更新位置时乘一下就行
float Actor::speedMultiplier() const {
    float mul = 1.0f;
    for (const Buff* b : status_.ordered()) mul *= b->speedMultiplier();
    return mul;
}

float Actor::pendingDamage() const {
    const Buff* white = status_.find(BuffId::WhiteShield);
    return white != nullptr ? white->gauge() : 0.0f;
}

void Actor::fillCommon(RenderItem& item) const {
    item.hpRatio = health_.ratio();
    item.flash   = status_.has(BuffId::Invincible);

    // 附加条（护盾/白条）统一换算成"占总血量的比例"，渲染层直接照着画
    float gauge = 0.0f;
    for (const Buff* b : status_.ordered()) gauge = std::max(gauge, b->gauge());
    item.gauge = health_.maxHp() > 0.0f ? std::min(1.0f, gauge / health_.maxHp()) : 0.0f;

    // 白条得单独拎出来：它和护盾语义不同 —— 护盾是"白赚的一段"，白条是
    // "这段伤害已经记在账上、还没转成真血"。所以渲染层要把白条画在血条**里面**：
    // 血条顶端那一截是待结算，下面才是真正安全的血量
    item.pending = 0.0f;
    if (health_.maxHp() > 0.0f) {
        if (const Buff* white = status_.find(BuffId::WhiteShield)) {
            item.pending = std::min(1.0f, white->gauge() / health_.maxHp());
        }
    }
}
