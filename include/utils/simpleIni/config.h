#ifndef CONFIG_H
#define CONFIG_H

#include <iostream>

#include "utils/simpleIni/SimpleIni.h"

// 全部可调参数。玩法代码里不许再出现魔法数字 —— 想要新参数就在这里加一个
// 字段、在 cfg.ini 里加一行，两边名字一致即可。
//
// 每个字段的初值都是"能跑的一套"：cfg.ini 缺项、少一节、甚至整个文件读不到，
// 游戏依然按这套默认值跑起来，不会有未初始化的成员。
//
// 单位统一成「秒」和「像素/秒」。旧版逻辑跑在 QTimer(16ms) 上，约 62.5 tick/秒，
// 那些按帧写的数值一律换算过：x 帧 → x * 62.5 /秒，x tick 时长 → x / 62.5 秒。
// 换算全部在括号里注明，方便对着旧代码核对。
struct Config {
    // ================================ 循环 ================================
    int   tickRate    = 20;       // 逻辑 TPS（与渲染帧率解耦）
    int   frameRate   = 60;       // 渲染 FPS
    float worldWidth  = 1400.0f;  // 逻辑世界尺寸
    float worldHeight = 1000.0f;

    // ================================ 玩家 ================================
    int   playerHp              = 100;
    float playerSpeed           = 125.0f;  // 旧 baseSpeed 2 px/帧
    float playerSprintSpeed     = 625.0f;  // 旧 sprintSpeed 10 px/帧
    int   playerWidth           = 35;
    int   playerHeight          = 35;
    int   playerAmmo            = 60;
    int   playerMaxAmmo         = 360;
    float playerFireInterval    = 0.128f;  // 旧 shootInterval 8 帧
    float playerInvincibleTime  = 1.92f;   // 旧 invincibleDuration 120 帧

    // 白条：挨打后伤害先记在账上，延迟一小段再按固定速度转成真血损失。
    // 关掉它就是旧版那种"挨打立刻掉血"
    bool  playerWhiteShield      = true;
    float playerWhiteShieldDelay = 0.6f;   // 挨打后先冻这么久
    float playerWhiteShieldRate  = 100.0f; // 冻完之后每秒转多少点成真血（= 一秒结清满血条）

    // 白条上那一笔账，无论多大都在这个时间内结完（全局规则，玩家和 BOSS 共用）。
    // 少了这条，一发 999 的暴击会让 BOSS 揣着必死的账继续追杀你五秒
    float whiteShieldMaxSettle   = 1.5f;

    float staminaMax            = 100.0f;
    float staminaDrain          = 50.0f;   // 旧 0.8/帧
    float staminaRegen          = 25.0f;   // 旧 0.4/帧
    // 力竭之后，体力回到 maxStamina * 这个比例才能再冲刺。
    // 旧版是硬编码的 0.15（15%），成员 exhaustThreshold 定义了却从没被用过
    float staminaRecoverRatio   = 0.15f;

    // 升级。旧版是等差曲线：每级所需经验 +50
    int   expBase               = 100;
    int   expStep               = 50;
    int   ammoPerLevel          = 20;      // 每次升级 +20 弹（旧版不 clamp）
    float healOnLevelUp         = 1.0f;    // 每次升级回满血（1.0 = 回满）
    int   maxLevel              = 100;
    int   bonusEveryLevels      = 5;       // 每 5 级加一档体力/弹药上限
    float bonusMaxStamina       = 5.0f;
    float bonusStaminaRegen     = 3.125f;  // 旧 +0.05/帧
    float bonusStaminaDrain     = 1.25f;   // 旧 +0.02/帧
    int   bonusMaxAmmo          = 20;
    float levelUpFlashTime      = 1.92f;   // 旧 levelUpTimer 120 帧

    // ================================ 子弹 ================================
    float bulletSpeed  = 625.0f;  // 旧 bulletspeed 10 px/帧
    float bulletDamage = 10.0f;   // 普通敌人 10 血 = 一枪一个（旧版就是秒杀）
    float bulletRadius = 4.0f;    // 旧 8x8 的子弹
    float bulletLife   = 3.0f;    // 出界本来就会消失，这只是保险

    // ================================ 敌人 ================================
    float enemyHp              = 10.0f;   // 一枪的量
    float enemySpeedMin        = 62.5f;   // 旧 speed = 1+rand()%4，即 1~4 px/帧
    float enemySpeedMax        = 250.0f;
    int   enemyWidth           = 35;
    int   enemyHeight          = 35;
    float enemyContactDamage   = 20.0f;   // 撞到玩家扣 20
    float enemyDashChance      = 0.10f;   // 旧 rand()%100<10

    // 冲刺型：间隔到了就沿玩家方向瞬移一段（旧版不是加速，是闪现）
    float enemyBlinkInterval    = 4.8f;   // 旧 dashTimer 300 帧
    float enemyBlinkDistance    = 350.0f;
    float enemyBlinkMinDistance = 400.0f; // 距离够远才闪

    // 波次。旧版：第 round 回合共 round*150 只，场上不足 15 只就补刷
    int   enemyAtLeast         = 15;
    int   enemyPerRound        = 150;
    // 旧版：3 + 19/(1+exp(round-6)/1.5) 帧，换算成秒要除以 62.5
    float enemySpawnBase       = 0.048f;  // 旧 3 帧
    float enemySpawnAmp        = 0.304f;  // 旧 19 帧
    float enemySpawnCenter     = 6.0f;
    float enemySpawnK          = 1.5f;
    float enemySpawnMin        = 0.048f;  // 旧下限 3 帧
    float roundWaitTime        = 4.8f;    // 旧 300 帧
    float enemyHpGrowth        = 0.0f;    // 旧版不随难度成长，留着给以后调
    float enemySpeedGrowth     = 0.0f;
    int   enemyDeathScore      = 1;
    int   enemyDeathExp        = 10;

    // ================================ Boss ================================
    // 旧版每回合末都出 Boss（不是按分数/时间触发）
    int   bossEveryRounds      = 1;
    float bossHpBase           = 300.0f;  // 旧 bossMaxHp 从 30 起、每回合 +10，1 血 = 1 发
    float bossHpGrowth         = 100.0f;
    float bossSpeed            = 125.0f;  // 旧 2 px/帧
    int   bossWidth            = 100;
    int   bossHeight           = 100;
    float bossContactDamage    = 60.0f;
    float bossWarnTime         = 9.6f;    // 旧 bossWarnTime 600 帧（剩下 50 只时开始闪）
    int   bossSummonCount      = 10;      // 半血召唤
    float bossSummonSpeed      = 312.5f;  // 旧 speed=5 px/帧
    float bossSummonSpread     = 120.0f;
    float bossPhase2Ratio      = 0.5f;
    float bossInvincibleTime   = 4.8f;    // 旧 bossInvincibleTimer 300 帧
    // 白条：旧版是纯视觉的延迟指示（真血立刻掉，白条慢慢追上来）
    float bossWhiteDropRatio       = 0.625f;  // 旧 maxHp*0.01/帧
    float bossWhiteDropMin         = 31.25f;  // 旧 0.5/帧
    float bossWhiteFreezeTime      = 0.48f;   // 旧 bossWhiteDelay 30 帧，挨打时冻结
    bool  bossWhiteShield          = true;    // 白条是否真的记账（false = 旧版那种纯视觉延迟）
    int   bossDeathScore       = 50;
    int   bossDeathExp         = 300;

    // ================================ 补给 ================================
    float pickupAmmoChance   = 0.09f;   // 代码实测值（旧图鉴写 15%，与代码不符）
    int   pickupAmmoMin      = 20;
    int   pickupAmmoMax      = 35;
    float pickupHealthChance = 0.04f;   // 代码实测值（旧图鉴写 10%）
    int   pickupHealthAmount = 10;
    float pickupLifetime     = 19.2f;   // 旧 lifeTimer 1200 帧
    int   pickupSize         = 24;

    // ================================ 控制台 ================================
    int   consoleMaxHistory   = 8;
    float consoleInvincibleTime = 192.0f;  // crazyyy → 旧 invincibleTimer 12000 帧

    // ======================================================================

    int Load(const char* iniFile) {
        CSimpleIniA ini;
        ini.SetUnicode();

        if (ini.LoadFile(iniFile) < 0) {
            std::cerr << "load ini failed: " << (iniFile ? iniFile : "(null)")
                      << " —— 用内置默认值继续" << std::endl;
            return 1;
        }

        // ---- [Game] ----
        tickRate    = getInt(ini, "Game", "TickRate", tickRate);
        frameRate   = getInt(ini, "Game", "FrameRate", frameRate);
        worldWidth  = getFloat(ini, "Game", "WorldWidth", worldWidth);
        worldHeight = getFloat(ini, "Game", "WorldHeight", worldHeight);
        playerHp    = getInt(ini, "Game", "PlayerDefaultHp", playerHp);  // 旧键，兼容

        // ---- [Player] ----
        playerHp             = getInt(ini, "Player", "Hp", playerHp);
        playerSpeed          = getFloat(ini, "Player", "Speed", playerSpeed);
        playerSprintSpeed    = getFloat(ini, "Player", "SprintSpeed", playerSprintSpeed);
        playerWidth          = getInt(ini, "Player", "Width", playerWidth);
        playerHeight         = getInt(ini, "Player", "Height", playerHeight);
        playerAmmo           = getInt(ini, "Player", "Ammo", playerAmmo);
        playerMaxAmmo        = getInt(ini, "Player", "MaxAmmo", playerMaxAmmo);
        playerFireInterval   = getFloat(ini, "Player", "FireInterval", playerFireInterval);
        playerInvincibleTime = getFloat(ini, "Player", "InvincibleTime", playerInvincibleTime);
        playerWhiteShield      = getBool(ini, "Player", "WhiteShield", playerWhiteShield);
        playerWhiteShieldDelay = getFloat(ini, "Player", "WhiteShieldDelay", playerWhiteShieldDelay);
        playerWhiteShieldRate  = getFloat(ini, "Player", "WhiteShieldRate", playerWhiteShieldRate);
        whiteShieldMaxSettle   = getFloat(ini, "Game", "WhiteShieldMaxSettle", whiteShieldMaxSettle);
        staminaMax           = getFloat(ini, "Player", "StaminaMax", staminaMax);
        staminaDrain         = getFloat(ini, "Player", "StaminaDrain", staminaDrain);
        staminaRegen         = getFloat(ini, "Player", "StaminaRegen", staminaRegen);
        staminaRecoverRatio  = getFloat(ini, "Player", "StaminaRecoverRatio", staminaRecoverRatio);
        expBase              = getInt(ini, "Player", "ExpBase", expBase);
        expStep              = getInt(ini, "Player", "ExpStep", expStep);
        ammoPerLevel         = getInt(ini, "Player", "AmmoPerLevel", ammoPerLevel);
        healOnLevelUp        = getFloat(ini, "Player", "HealOnLevelUp", healOnLevelUp);
        maxLevel             = getInt(ini, "Player", "MaxLevel", maxLevel);
        bonusEveryLevels     = getInt(ini, "Player", "BonusEveryLevels", bonusEveryLevels);
        bonusMaxStamina      = getFloat(ini, "Player", "BonusMaxStamina", bonusMaxStamina);
        bonusStaminaRegen    = getFloat(ini, "Player", "BonusStaminaRegen", bonusStaminaRegen);
        bonusStaminaDrain    = getFloat(ini, "Player", "BonusStaminaDrain", bonusStaminaDrain);
        bonusMaxAmmo         = getInt(ini, "Player", "BonusMaxAmmo", bonusMaxAmmo);
        levelUpFlashTime     = getFloat(ini, "Player", "LevelUpFlashTime", levelUpFlashTime);

        // ---- [Bullet] ----
        bulletSpeed  = getFloat(ini, "Bullet", "Speed", bulletSpeed);
        bulletDamage = getFloat(ini, "Bullet", "Damage", bulletDamage);
        bulletRadius = getFloat(ini, "Bullet", "Radius", bulletRadius);
        bulletLife   = getFloat(ini, "Bullet", "Life", bulletLife);

        // ---- [Enemy] ----
        enemyHp              = getFloat(ini, "Enemy", "Hp", enemyHp);
        enemySpeedMin        = getFloat(ini, "Enemy", "SpeedMin", enemySpeedMin);
        enemySpeedMax        = getFloat(ini, "Enemy", "SpeedMax", enemySpeedMax);
        enemyWidth           = getInt(ini, "Enemy", "Width", enemyWidth);
        enemyHeight          = getInt(ini, "Enemy", "Height", enemyHeight);
        enemyContactDamage   = getFloat(ini, "Enemy", "ContactDamage", enemyContactDamage);
        enemyDashChance      = getFloat(ini, "Enemy", "DashChance", enemyDashChance);
        enemyBlinkInterval   = getFloat(ini, "Enemy", "BlinkInterval", enemyBlinkInterval);
        enemyBlinkDistance   = getFloat(ini, "Enemy", "BlinkDistance", enemyBlinkDistance);
        enemyBlinkMinDistance = getFloat(ini, "Enemy", "BlinkMinDistance", enemyBlinkMinDistance);
        enemyAtLeast         = getInt(ini, "Enemy", "AtLeast", enemyAtLeast);
        enemyPerRound        = getInt(ini, "Enemy", "PerRound", enemyPerRound);
        enemySpawnBase       = getFloat(ini, "Enemy", "SpawnBase", enemySpawnBase);
        enemySpawnAmp        = getFloat(ini, "Enemy", "SpawnAmp", enemySpawnAmp);
        enemySpawnCenter     = getFloat(ini, "Enemy", "SpawnCenter", enemySpawnCenter);
        enemySpawnK          = getFloat(ini, "Enemy", "SpawnK", enemySpawnK);
        enemySpawnMin        = getFloat(ini, "Enemy", "SpawnMin", enemySpawnMin);
        roundWaitTime        = getFloat(ini, "Enemy", "RoundWaitTime", roundWaitTime);
        enemyHpGrowth        = getFloat(ini, "Enemy", "HpGrowth", enemyHpGrowth);
        enemySpeedGrowth     = getFloat(ini, "Enemy", "SpeedGrowth", enemySpeedGrowth);
        enemyDeathScore      = getInt(ini, "Enemy", "DeathScore", enemyDeathScore);
        enemyDeathExp        = getInt(ini, "Enemy", "DeathExp", enemyDeathExp);

        // ---- [Boss] ----
        bossEveryRounds    = getInt(ini, "Boss", "EveryRounds", bossEveryRounds);
        bossHpBase         = getFloat(ini, "Boss", "HpBase", bossHpBase);
        bossHpGrowth       = getFloat(ini, "Boss", "HpGrowth", bossHpGrowth);
        bossSpeed          = getFloat(ini, "Boss", "Speed", bossSpeed);
        bossWidth          = getInt(ini, "Boss", "Width", bossWidth);
        bossHeight         = getInt(ini, "Boss", "Height", bossHeight);
        bossContactDamage  = getFloat(ini, "Boss", "ContactDamage", bossContactDamage);
        bossWarnTime       = getFloat(ini, "Boss", "WarnTime", bossWarnTime);
        bossSummonCount    = getInt(ini, "Boss", "SummonCount", bossSummonCount);
        bossSummonSpeed    = getFloat(ini, "Boss", "SummonSpeed", bossSummonSpeed);
        bossSummonSpread   = getFloat(ini, "Boss", "SummonSpread", bossSummonSpread);
        bossPhase2Ratio    = getFloat(ini, "Boss", "Phase2Ratio", bossPhase2Ratio);
        bossInvincibleTime = getFloat(ini, "Boss", "InvincibleTime", bossInvincibleTime);
        bossWhiteDropRatio = getFloat(ini, "Boss", "WhiteDropRatio", bossWhiteDropRatio);
        bossWhiteDropMin   = getFloat(ini, "Boss", "WhiteDropMin", bossWhiteDropMin);
        bossWhiteFreezeTime = getFloat(ini, "Boss", "WhiteFreezeTime", bossWhiteFreezeTime);
        bossWhiteShield    = getBool(ini, "Boss", "WhiteShield", bossWhiteShield);
        bossDeathScore     = getInt(ini, "Boss", "DeathScore", bossDeathScore);
        bossDeathExp       = getInt(ini, "Boss", "DeathExp", bossDeathExp);

        // ---- [Pickup] ----
        pickupAmmoChance   = getFloat(ini, "Pickup", "AmmoChance", pickupAmmoChance);
        pickupAmmoMin      = getInt(ini, "Pickup", "AmmoMin", pickupAmmoMin);
        pickupAmmoMax      = getInt(ini, "Pickup", "AmmoMax", pickupAmmoMax);
        pickupHealthChance = getFloat(ini, "Pickup", "HealthChance", pickupHealthChance);
        pickupHealthAmount = getInt(ini, "Pickup", "HealthAmount", pickupHealthAmount);
        pickupLifetime     = getFloat(ini, "Pickup", "Lifetime", pickupLifetime);
        pickupSize         = getInt(ini, "Pickup", "Size", pickupSize);

        // ---- [Console] ----
        consoleMaxHistory     = getInt(ini, "Console", "MaxHistory", consoleMaxHistory);
        consoleInvincibleTime = getFloat(ini, "Console", "InvincibleTime", consoleInvincibleTime);

        return 0;
    }

private:
    static int getInt(const CSimpleIniA& ini, const char* section, const char* key, int fallback) {
        return static_cast<int>(ini.GetLongValue(section, key, static_cast<long>(fallback)));
    }
    static float getFloat(const CSimpleIniA& ini, const char* section, const char* key, float fallback) {
        return static_cast<float>(ini.GetDoubleValue(section, key, static_cast<double>(fallback)));
    }
    static bool getBool(const CSimpleIniA& ini, const char* section, const char* key, bool fallback) {
        return ini.GetBoolValue(section, key, fallback);
    }
};

// 全局配置。默认值就是能跑的一套，所以不存在"忘记 Load 就读到未初始化成员"的问题。
// 只在启动时写一次，之后所有线程都只读，不需要加锁。
inline Config& config() {
    static Config instance;
    return instance;
}

#endif
