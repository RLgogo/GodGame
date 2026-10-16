#ifndef GODGAME_WORLD_H
#define GODGAME_WORLD_H

#include <memory>
#include <vector>

#include "core/Bullet.h"
#include "core/Damage.h"
#include "core/EntityId.h"
#include "core/Input.h"
#include "core/Kind.h"

class Entity;

// 实体能看到的"世界"。
//
// 实体之间不许互相持指针，也不许自己 new 东西：想要什么就问 World。
// 这样一来 Game 不会长成上帝对象（所有实体行为都堆在它里面），
// 实体自己又拿得到"身边有谁"。
//
// 实现是 Game。
// ★ query / get / time 只在逻辑线程里调（它们要看容器，容器归逻辑线程独占）。
// ★ spawn / queueDamage 是入队，任何线程都能调，Game 内部自己加锁。
class World {
public:
    virtual ~World() = default;

    // 按类型批量查（单一位走分类索引）
    virtual std::vector<EntityId> query(KindMask mask) const = 0;

    // 句柄 -> 对象。句柄过期返回 nullptr。
    // 返回的指针只在本 tick 内有效：跨 tick 请继续用 EntityId。
    virtual Entity* get(EntityId id) = 0;

    // 生成实体。入队，tick 末尾统一提交 —— update 期间容器结构不变
    virtual void spawn(std::unique_ptr<Entity> entity) = 0;

    // 造成伤害。入队，tick 末尾统一结算 —— 连锁爆炸不会在遍历中改容器
    virtual void queueDamage(EntityId target, const DamageInfo& damage) = 0;

    // 开一枪。子弹不是实体（见 core/Bullet.h），所以"发子弹"必须是世界的能力：
    // 否则每个会开火的实体都得自己攥着一个子弹容器
    virtual void spawnBullet(const Bullet& bullet) = 0;

    // 加分。分数是世界的账，不是实体的账 —— 实体死了以后还得有人记这笔
    virtual void addScore(int amount) = 0;

    // 世界里还剩几个"活着的敌人"。波次推进、Boss 触发都要看它，
    // 而实体自己数不出来（它只看得见 query 的结果，不知道全局规则）
    virtual int enemyCount() const = 0;

    // 世界尺寸。边界限制（玩家不许出画面）得有个统一口径
    virtual float worldWidth() const  = 0;
    virtual float worldHeight() const = 0;

    // 这一 tick 的键鼠状态。玩家实体自己来读，Game 不替它转手
    virtual const InputState& input() const = 0;

    // 世界上唯一的那个玩家。敌人每 tick 都要找它，让每个敌人都自己去 query
    // 一遍纯属浪费 —— 而且"谁是玩家"本来就是世界级的事实
    virtual EntityId playerId() const = 0;

    // 随机数。由世界统一提供：种子的来源只有一处，
    // 于是"复现某一局"和"让测试可重复"都只需要控制这一个地方
    virtual float randomRange(float min, float max) = 0;
    virtual int   randomInt(int min, int max) = 0;   // 闭区间，和 rand()%n+min 对齐

    virtual double time() const = 0;  // 已模拟的秒数，buff 计时用
};

#endif //GODGAME_WORLD_H
