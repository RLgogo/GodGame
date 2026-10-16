# GodGame

Qt6 + C++17 的 2D 打怪小游戏。玩法是照旧版那个 1450 行的单文件 `GameWidget` 复刻的，
但底子重写过：**逻辑和渲染分开、参数全在 `cfg.ini` 里**。

## 构建

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/Users/Admin/Qt/6.11.1/macos
cmake --build build -j8
./build/GodGame          # 会在工作目录里找 cfg.ini，找不到就用代码里的默认值
```

用 CLion 直接打开根目录即可（`cmakelists.txt`）。里面写的 `CMAKE_CXX_STANDARD 17` 是必须的 ——
Qt6 的头文件会直接拒绝 C++11 编译器。

## 目录结构

```
include/core/       逻辑层的契约，一个字节 Qt Gui 都不依赖
  EntityId.h        实体句柄 {下标, 世代号}：槽位被复用就 +1，老句柄自动失效
  Kind.h            实体分类位掩码（Player / Enemy / PlayerBullet / Pickup / Boss / Damageable）
  Damage.h          DamageInfo / DamageResult / DamageType 位掩码
  RenderItem.h      逻辑交给渲染的纯数据（位置、形状、样式、血条比例…）
  Hud.h             HudState：界面要显示的每一个数字
  Input.h           InputState：瞬时输入状态，不是事件流
  Bullet.h          子弹：POD 数组，不走实体系统
  World.h           实体能看到的「世界」接口
  Game.h            游戏本体：一条逻辑线程 + 一份不可变快照

include/entity/     实体层
  Entity.h          只有位置、存活标记、kinds()、renderItem()
  Player.h/.cpp     移动、冲刺、体力、开火、瞄准
  Enemy.h/.cpp      追人、冲刺型瞬移、撞人自爆
  Boss.h/.cpp       半血变身、无敌窗口、召唤帮手、白条
  Pickup.h/.cpp     弹药箱 / 血包
  actor/            Health + Buff + StatusList + Actor（伤害管线）
    Actor.h/.cpp    takeDamage 五段钩子管线
    Health.h        只有 hp / maxHp，不含任何免疫逻辑
    Buff.h          钩子基类 + 优先级约定
    StatusList.h    buff 容器：增删走队列，绝不在遍历中改动
    Buffs.h         免疫 / 无敌 / 护盾 / 白条 / 减速 / 霸体

src/core/Game.cpp   世界推进、碰撞、掉落、回合状态机、HUD 组装
src/render/Menu.cpp 唯一的 QWidget：画菜单 + 画游戏 + 收输入
src/main.cpp        读 cfg.ini、开窗口

旧版行为与数值规格.md   从旧代码里抽出的完整数值表，含旧版 26 个坑
```

## 架构

三层，方向单一：`render →（InputState / 控制台命令）→ core →（RenderItem / HudState）→ render`。

* **逻辑线程**：`Game::run()` 按固定步长（`cfg.ini` 的 `TickRate`）推进世界。累加器写法 +
  帧率节流，卡顿后最多补 5 步，追不上就把欠账丢掉，不会越追越慢。
* **渲染线程（主线程）**：只调 `game.frame()` 拿一份 `shared_ptr<const Frame>`。
  快照一旦发布就不再改动，所以「渲染边画、逻辑边改」这种竞争根本不存在。
* **实体不互相持有指针**：要找别人就 `world.get(id)`，要打别人就 `world.queueDamage(id, …)`。
  句柄带世代号，目标死了再打只会打空，不会打到那个槽位后来的新东西。

几条刻意维持的不变量：

* 伤害只有一个入口 `Actor::takeDamage`，它先把 `DamageInfo` 拷一份再处理 —— 否则一次爆炸
  复用同一份 `DamageInfo` 时，第一个免疫目标会把 `cancelled` 写进公共数据，后面全部跟着免伤。
* buff 不许直接改伤害数值，只能改 `multiplier` / `flatReduce`，于是连乘可交换，
  「先免疫还是先减伤」这种顺序依赖就不存在了。
* `StatusList` 遍历时只读，所有增删进队列，下一轮排空（最多 8 轮，挡连锁）。
* 死亡只 `kill()` 标记，回收统一交给 `Game::sweep()` —— 遍历中删元素是 UB。

## cfg.ini

七个节。缺项、缺节、整个文件读不到都会退回代码里的默认值，不会有未初始化参数。

| 节 | 管什么 |
|---|---|
| `[Game]` | TPS、帧率、世界尺寸 |
| `[Player]` | 血量、走/冲刺速度、弹药、射速、体力、升级曲线 |
| `[Bullet]` | 速度、伤害、半径、存活时间 |
| `[Enemy]` | 血量、速度区间、撞击伤害、瞬移参数、波次数量与出场节奏 |
| `[Boss]` | 血量与成长、半血变身、召唤、白条速度、奖励 |
| `[Pickup]` | 掉落概率与数量、存活时间 |
| `[Console]` | 控制台行为 |

单位统一成**秒**和**像素/秒**。旧代码跑在 `QTimer(16ms)`（约 62.5 tick/秒）上，
换算就是 `tick / 62.5`，`cfg.ini` 里每一条都注了旧值方便对照。

## 自测

三个脚手架在 `/tmp` 下（不进仓库，直接把本工程的源码编进去）：

| 目录 | 干什么 |
|---|---|
| `/tmp/gg-game` | 整局逻辑自测：手工步进一个 `Game`，12 项断言（移动/冲刺/力竭、开火击杀、无敌帧、拾取、升级、控制台、阵亡、快照隔离、回合到 Boss、Boss 半血与击杀） |
| `/tmp/gg-logic` | 单元级：伤害管线、buff 栈策略、护盾/无敌/免疫、状态免疫 |
| `/tmp/gg-check` | 渲染层：离屏跑真实 `QWidget`，用真的 `QMouseEvent` / `QKeyEvent` 走一遍菜单→关于→图鉴→开打→暂停→控制台→阵亡→重开，每步存一张 PNG |

```bash
cd /tmp/gg-game  && cmake -S . -B build && cmake --build build -j8 && ./build/gggame
cd /tmp/gg-check && cmake -S . -B build && cmake --build build -j8 && QT_QPA_PLATFORM=offscreen ./build/ggcheck
```

## 和旧版不一样的地方

玩法数值照搬，但旧版有 26 处 bug / 死代码（完整清单在 `旧版行为与数值规格.md` 第 10 节）。
这里修掉了大部分：

* 重开一局不再沿用上一局的刷怪间隔（旧版 `spawnInterval` 残留）
* 死敌人不再占着列表不放（`sweep()` 统一回收）
* 升级加弹夹会被 clamp 到上限（旧版能超）
* 控制台 `boom` 现在走正常结算，会跑升级判定（旧版只加经验值、不升级）
* 撞死敌人不再「提前 return」跳过本帧剩余逻辑
* 画普通敌人不再继承冲刺敌人那支 3px 灰笔
* 子弹方向用浮点归一化（旧版 `int(dx/dist*10)` 截断，45° 每轴固定 7）
* 刷怪点采样 50 次失败后不再飞到 (0,0)
* 删掉一批定义了但从没读过的死字段（`respawnDelay` / `enemySpeed` / `bossPhase` / `bossCoolDown` / `consoleHistory` …）

特意保留 / 改造的：

* Boss 血量每回合 +10 —— 留着当难度曲线
* 冲刺型敌人是「瞬移一段」而不是加速（旧版就是瞬移，容易看错）
* Boss 白条是纯视觉的延迟指示；真正的「延迟结算白条」做成了 `WhiteShieldBuff` 放进工具箱，暂时没有实体用它
* 图鉴里的掉落概率按**代码实测**写（弹药 9% / 血包 4%），旧版文案写的 15%/10% 和代码不符
* 控制台从屏幕中央的小盒子挪到底部面板，能滚历史、能看回显
