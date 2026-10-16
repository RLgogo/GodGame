#ifndef GODGAME_RENDER_MENU_H
#define GODGAME_RENDER_MENU_H

#include <QPoint>
#include <QRect>
#include <QString>
#include <QTimer>
#include <QWidget>

#include <vector>

#include "core/Game.h"
#include "core/Hud.h"
#include "core/Input.h"
#include "core/RenderItem.h"

class QPainter;
class QKeyEvent;
class QMouseEvent;
class QResizeEvent;
class QCloseEvent;
class QPaintEvent;

// 界面状态。逻辑侧的 GameStatus 只管 Playing/Paused/GameOver，Menu/About/Guide/
// Console 是纯界面的事，所以两边分开：state_ 归 UI，GameOver 由逻辑宣布。
enum class GameState {
    Menu = 0,
    Playing,
    Paused,
    GameOver,
    About,
    Guide,
    Console,
};

// 渲染层 = 一个 QWidget + 一个 QTimer。
// 它不认识 Entity，也不认识 HP、敌人、回合 —— 它只读 Game 每帧发布出来的那份 Frame，
// 照着 RenderItem 画。所以这一层可以随便重画、随便换配色，碰不到任何玩法逻辑。
class Menu : public QWidget {
    Q_OBJECT

public:
    explicit Menu(QWidget* parent = nullptr);
    ~Menu() override;

    Game& game() { return game_; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onTick();

private:
    void startGame();
    void backToMenu();
    void pauseGame(bool paused);
    void openConsole();
    void closeConsole();
    void runConsoleCommand();
    void clearKeyState();
    void syncInput();
    void updateButtonPosition();

    void drawMenu(QPainter& painter);
    void drawAbout(QPainter& painter);
    void drawGuide(QPainter& painter);
    void drawGame(QPainter& painter);
    void drawItem(QPainter& painter, const RenderItem& item);
    void drawHud(QPainter& painter, const HudState& hud);
    void drawBossWarning(QPainter& painter, const HudState& hud);
    void drawPauseOverlay(QPainter& painter);
    void drawGameOver(QPainter& painter);
    void drawConsole(QPainter& painter);
    void drawButton(QPainter& painter, const QRect& rect, const QString& text, bool primary);

    Game      game_;
    GameState state_ = GameState::Menu;
    QTimer    timer_;

    QRect startButton;
    QRect aboutButton;
    QRect guideButton;
    QRect resumeButton;
    QRect quitButton;
    QRect backButton;

    QPoint mousePos_;
    bool   mouseHeld_ = false;

    bool keyUp_     = false;
    bool keyDown_   = false;
    bool keyLeft_   = false;
    bool keyRight_  = false;
    bool keySprint_ = false;
    bool keyFire_   = false;

    QString              consoleInput_;
    std::vector<QString> consoleHistory_;

    int frameCounter_ = 0;   // 给闪烁用（警告条、回合提示）
};

#endif //GODGAME_RENDER_MENU_H
