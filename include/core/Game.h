#ifndef GODGAME_CORE_GAME_H
#define GODGAME_CORE_GAME_H

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <utility>
#include <vector>

#include "core/Bullet.h"
#include "core/Damage.h"
#include "core/EntityId.h"
#include "core/Hud.h"
#include "core/Input.h"
#include "core/Kind.h"
#include "core/RenderItem.h"
#include "core/World.h"

class Entity;
class Player;

// 一个逻辑线程 + 一份渲染快照的游戏本体。
//
// 线程模型：
//   * UI 线程只做四件事 —— setInput() / setPaused() / postCommand() / frame()；
//   * 逻辑线程按固定步长（tickRate）跑，每帧把结果整理成一份**不可变**的 Frame 发布出去；
//   * 两边共享的只有 Frame 的 shared_ptr 和几个 atomic，没有任何实体对象会被两个线程同时碰。
class Game : public World {
public:
    // 渲染层需要的一切，一次给全（items 已按画序排好）
    struct Frame {
        std::vector<RenderItem> items;
        HudState                hud;
    };
    using FramePtr = std::shared_ptr<const Frame>;

    // 控制台命令：UI 线程投递，逻辑线程执行（UI 线程不能直接动世界）
    enum class Command { GrantInvincible, ClearEnemies };

    Game();
    ~Game() override;

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // ---------------------------------- UI 线程 ----------------------------------
    void start();                        // 重置世界并起线程（重复调用安全）
    void stop();                         // 通知线程退出并 join
    bool running() const { return running_.load(); }
    FramePtr frame() const;
    void setInput(const InputState& input);
    void setPaused(bool paused);
    bool paused() const { return paused_.load(); }
    void postCommand(Command command);
    long long tickCount() const { return tickCount_.load(); }

    // ---------------------- 不开线程的手动驱动（自测/工具用）----------------------
    void reset();
    void advance(float dt);

    // ----------------------------------- World -----------------------------------
    std::vector<EntityId> query(KindMask mask) const override;
    Entity* get(EntityId id) override;
    void spawn(std::unique_ptr<Entity> entity) override;
    void queueDamage(EntityId target, const DamageInfo& damage) override;
    void spawnBullet(const Bullet& bullet) override;
    void addScore(int amount) override;
    int enemyCount() const override;
    float worldWidth() const override;
    float worldHeight() const override;
    const InputState& input() const override { return stepInput_; }
    EntityId playerId() const override { return playerId_; }
    float randomRange(float min, float max) override;
    int randomInt(int min, int max) override;
    double time() const override { return simulated_; }

private:
    struct Slot {
        std::unique_ptr<Entity> entity;
        std::uint32_t           gen = 0;   // 世代号：槽位被复用就 +1，老句柄自动失效
    };

    // 一局的推进阶段（对应旧版 roundState 0~4）
    enum class RoundPhase {
        Spawning,      // 正在把本回合的敌人放出来
        BossPending,   // 敌人放完了，倒数之后 Boss 登场
        BossFight,     // Boss 还活着
        Cleared,       // 打完了，歇一会儿再进下一回合
    };

    void run();
    void step(float dt);
    void commitSpawn();
    void commitDamage();
    void sweep();
    void rebuildIndex() const;
    void drainCommands();

    void updateRound(float dt);
    void updateBullets(float dt);
    void handleBulletHits();
    void spawnEnemy();
    void spawnBoss();
    void dropLoot(const QPointF& center, bool both);
    void awardKill(Entity& victim);
    void grantExp(int amount);
    void clearEnemiesNow();
    void grantInvincibleNow(float seconds);

    void publishFrame();
    HudState buildHud();

    Player*       player();
    const Entity* peek(EntityId id) const;
    Entity*       findBoss();

    float spawnIntervalForRound() const;

    // -------------------------- 世界存储（只有逻辑线程碰）--------------------------
    std::vector<Slot>          slots_;
    std::vector<std::uint32_t> freeSlots_;
    mutable std::array<std::vector<EntityId>, static_cast<std::size_t>(kKindSlots)> byKind_;
    mutable bool               indexDirty_ = true;
    BulletField                bullets_;

    // ------------------------------- 跨线程入队 -------------------------------
    std::vector<std::unique_ptr<Entity>>         pendingSpawn_;
    std::vector<std::pair<EntityId, DamageInfo>> pendingHits_;
    std::vector<Command>                         pendingCommands_;
    mutable std::mutex                           pendingMutex_;

    InputState         input_;
    InputState         stepInput_;
    mutable std::mutex inputMutex_;

    mutable std::mutex frameMutex_;
    FramePtr           frame_;

    std::atomic<bool>      running_{false};
    std::atomic<bool>      paused_{false};
    std::atomic<long long> tickCount_{0};
    std::thread            worker_;

    double accumulator_ = 0.0;
    double simulated_   = 0.0;
    bool   gameOver_    = false;

    // --------------------------------- 这一局的账 ---------------------------------
    int score_        = 0;
    int level_        = 1;
    int exp_          = 0;
    int expToNext_    = 100;
    int levelUpFlash_ = 0;

    RoundPhase roundPhase_       = RoundPhase::Spawning;
    int        round_            = 1;
    int        remainingToSpawn_ = 0;
    float      spawnTimer_       = 0.0f;
    float      phaseTimer_       = 0.0f;
    float      bossWarnTimer_    = 0.0f;
    bool       bossActive_       = false;

    EntityId playerId_;

    // HUD 缓存：玩家阵亡后实体被回收，界面上还得显示最后一刻的数字
    int   hudHp_         = 0;
    int   hudMaxHp_      = 0;
    int   hudAmmo_       = 0;
    int   hudMaxAmmo_    = 0;
    float hudStamina_    = 0.0f;
    float hudMaxStamina_ = 0.0f;
    bool  hudExhausted_  = false;

    std::mt19937 rng_;
};

#endif //GODGAME_CORE_GAME_H
