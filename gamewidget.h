#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QSet>
#include <QRect>
#include <QList>
#include <QPoint>
#include <QElapsedTimer>
#include <QString>
#include <QStringList>

struct Bullet{
    QRect rect;
    int dx;
    int dy;
};

struct Enemy{
    QRect rect;
    QPointF precisePos;
    bool alive =true;
    int respawnTimer=0;
    int speed=2;

    bool isDasher=false;
    int dashTimer=300;
};

struct AmmoBox{
    QRect rect;
    int amount;
    int lifeTimer=1200;
};

struct HealthPack{
    QRect rect;
    int heal=10;
    int lifeTimer=1200;
};

enum class GameState
{
   Menu,
    Playing,
    Paused,
    GameOver,
    About,
    Guide,
    Console
};

class GameWidget : public QWidget
{
    Q_OBJECT

public:
    GameWidget(QWidget *parent = nullptr);
    ~GameWidget();
protected:
    void paintEvent(QPaintEvent *event)override;
    void keyPressEvent(QKeyEvent *event)override;
    void keyReleaseEvent(QKeyEvent *event)override;
    void mousePressEvent(QMouseEvent *event)override;
    void mouseReleaseEvent(QMouseEvent *event)override;
    void mouseMoveEvent(QMouseEvent *event)override;
    void resizeEvent(QResizeEvent *event)override;
private:
    void updategame();
    void fireBullet();
    void spawnEnemy();
    void resetGame();
    void updateButtonPosition();
    void spawnBoss();
    void levelUp();

    void drawMenu(QPainter &painter);           //绘制函数声明
    void drawGuide(QPainter &painter);
    void drawAbout(QPainter &painter);
    void drawHUD(QPainter &painter);
    void drawPlayer(QPainter &painter);
    void drawBullets(QPainter &painter);
    void drawEnemies(QPainter &painter);
    void drawSupplies(QPainter &painter);
    void drawBoss(QPainter &painter);
    void drawBossWarning(QPainter &painter);
    void drawPauseOverlay(QPainter &painter);
    void drawGameOver(QPainter &painter);
    double computeSpawnInterval(int round);
    void handleConsoleCommand(const QString &cmd);
    void drawConsole(QPainter &painter);
    void drawLevelUp(QPainter &painter);

    QRect startButton;
    QRect resumeButton;
    QRect quitButton;
    QRect aboutButton;
    QRect backbutton;
    QRect guideButton;
    QRect guideBackButton;

    QString consoleInput;
    QStringList consoleHistory;
    qint64 consolePausedAt=0;

    int ammo=60;            //子弹
    int maxammo=360;
    QList<AmmoBox> ammoBoxes;

    QList<HealthPack> healthPacks;

    QTimer *timer=nullptr;
    QSet<int>keys;
    QRect player{100,100,35,35};            //玩家
    int baseSpeed=2;
    int sprintSpeed=5;

    double stamina=100.0;          //体力条
    double maxStamina=100.0;
    double staminaDrain=0.8;
    double staminaRegen=0.4;

    int level=1;        //经验值
    int expValue=0;
    int expToNext=100;
    int levelUpTimer=0;
    int levelUpDuration=120;

    bool exhausted=false;
    double exhaustThreshold=15.0;

    QList<Bullet>bullets;               //子弹
    int bulletspeed=10;

    QList<Enemy>enemies;                //敌人
    int enemySpeed=2;
    int respawnDelay=60;
    int enemyAtLeast=15;
    int aliveEnemies=0;
                        //回合
    int round=1;
    int roundState=0;
    int roundWaitTime=0;
    int enemyToSpwan=0;
    double spawnCoolDown=0.0;
    double spawnInterval=22.0;
    bool bossSpawnedThisRound=false;
    int bossWarnTime=0;

    int enemyStanding=0;        //剩余敌人数量

    bool bossActivive=false;            //Boss
    int bossPhase=0;
    int bossCoolDown=1800;
    QRect bossRect;
    QPointF bossPreccisePos;
    int bossHp=20;
    int bossMaxHp=20;
    double bossWhiteHp=0.0;
    int bossWhiteDelay=0;
    bool bossInvincible=false;
    int bossInvincibleTimer=0;
    bool bossPhase2Triggered=false;

    QPoint mousePos{0,0};               //鼠标射击
    bool mouseHeld=false;
    int shootCooldown=0;
    int shootInterval=8;

    int score=0;
    GameState state=GameState::Menu;           //起始菜单

    QElapsedTimer survivalTimer;
    qint64 frozenElapsed=0;

    QRect resumeButtom{400,350,200,60};         //按钮
    QRect quitButtom{400,430,200,60};
    qint64 pausedAt=0;
    qint64 totalPaused=0;

    int hp=100;
    int maxhp=100;
    int invincibleTimer=0;
    int invincibleDuration=120;
};
#endif // GAMEWIDGET_H
