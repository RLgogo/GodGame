#include "render/Menu.h"

#include <QCloseEvent>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>

#include <algorithm>
#include <cmath>

#include "utils/simpleIni/config.h"

namespace {

// 旧版到处写死"微软雅黑"。Windows 上有，macOS 上没有（会 fallback 并刷一屏警告），
// 所以这里按优先级挑一个装了的。
QString uiFamily() {
    static const QString family = [] {
        const QStringList wanted{
            QStringLiteral("微软雅黑"), QStringLiteral("Microsoft YaHei"),
            QStringLiteral("PingFang SC"), QStringLiteral("Hiragino Sans GB"),
            QStringLiteral("Heiti SC"), QStringLiteral("Noto Sans CJK SC"),
        };
        const QStringList installed = QFontDatabase::families();
        for (const QString& name : wanted) {
            if (installed.contains(name)) {
                return name;
            }
        }
        return QString();
    }();
    return family;
}

QFont uiFont(int pixelSize, bool bold = false) {
    QFont font(uiFamily());
    font.setPixelSize(pixelSize);
    font.setBold(bold);
    return font;
}

QFont monoFont(int pixelSize) {
    QFont font(QStringLiteral("Consolas"));
    font.setStyleHint(QFont::Monospace);
    font.setPixelSize(pixelSize);
    return font;
}

}

Menu::Menu(QWidget* parent) : QWidget(parent) {
    setWindowTitle(QStringLiteral("GodGame"));
    resize(1400, 1000);
    setMinimumSize(1000, 800);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAutoFillBackground(true);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);

    updateButtonPosition();

    timer_.setInterval(1000 / std::max(1, config().frameRate));
    connect(&timer_, &QTimer::timeout, this, &Menu::onTick);
    timer_.start();
}

Menu::~Menu() {
    game_.stop();
}

// ================================= 每帧回调 =================================

void Menu::onTick() {
    ++frameCounter_;

    syncInput();

    // 逻辑线程宣布的"结束"只有一种：血没了。暂停/控制台是界面自己的事
    if (state_ == GameState::Playing) {
        if (const Game::FramePtr frame = game_.frame()) {
            if (frame->hud.status == GameStatus::GameOver) {
                state_ = GameState::GameOver;
                clearKeyState();
            }
        }
    }

    update();
}

void Menu::syncInput() {
    InputState input;

    // 只有真正在打的时候才把输入送过去；其他状态一律空输入，
    // 免得暂停时按住的键在恢复后突然生效
    if (state_ == GameState::Playing && !game_.paused()) {
        input.up       = keyUp_;
        input.down     = keyDown_;
        input.left     = keyLeft_;
        input.right    = keyRight_;
        input.sprint   = keySprint_;
        input.fire     = mouseHeld_ || keyFire_;
        input.aimX     = static_cast<float>(mousePos_.x());
        input.aimY     = static_cast<float>(mousePos_.y());
        input.aimValid = true;
    }

    game_.setInput(input);
}

void Menu::clearKeyState() {
    keyUp_ = keyDown_ = keyLeft_ = keyRight_ = keySprint_ = keyFire_ = false;
    mouseHeld_ = false;
}

// ================================= 状态切换 =================================

void Menu::startGame() {
    clearKeyState();
    game_.start();
    state_ = GameState::Playing;
    updateButtonPosition();
    setFocus();
    update();
}

void Menu::backToMenu() {
    game_.stop();
    clearKeyState();
    state_ = GameState::Menu;
    updateButtonPosition();
    update();
}

void Menu::pauseGame(bool paused) {
    game_.setPaused(paused);
    state_ = paused ? GameState::Paused : GameState::Playing;
    clearKeyState();
    update();
}

void Menu::openConsole() {
    // 旧版 updategame() 在非 Playing 状态直接 return，所以控制台里世界也是停的
    game_.setPaused(true);
    state_ = GameState::Console;
    consoleInput_.clear();
    clearKeyState();
    update();
}

void Menu::closeConsole() {
    game_.setPaused(false);
    state_ = GameState::Playing;
    update();
}

void Menu::runConsoleCommand() {
    QString command = consoleInput_.trimmed();
    consoleInput_.clear();

    if (command.startsWith(QLatin1Char('/'))) {
        command.remove(0, 1);          // 旧版会把前导斜杠剥掉
    }
    if (command.isEmpty()) {
        update();
        return;
    }

    consoleHistory_.push_back(QStringLiteral("> ") + command);

    if (command == QStringLiteral("crazyyy")) {
        game_.postCommand(Game::Command::GrantInvincible);
        consoleHistory_.push_back(QStringLiteral("  >> 无敌 ")
                                  + QString::number(static_cast<int>(config().consoleInvincibleTime))
                                  + QStringLiteral(" 秒"));
    } else if (command == QStringLiteral("boom")) {
        game_.postCommand(Game::Command::ClearEnemies);
        consoleHistory_.push_back(QStringLiteral("  >> 清屏"));
    } else if (command == QStringLiteral("help")) {
        consoleHistory_.push_back(QStringLiteral("  >> 可用指令: crazyyy / boom / help"));
        consoleHistory_.push_back(QStringLiteral("     crazyyy : 无敌"));
        consoleHistory_.push_back(QStringLiteral("     boom    : 清屏敌人"));
    } else {
        consoleHistory_.push_back(QStringLiteral("  >> 未知指令: ") + command);
    }

    const std::size_t keep = static_cast<std::size_t>(std::max(1, config().consoleMaxHistory));
    while (consoleHistory_.size() > keep) {
        consoleHistory_.erase(consoleHistory_.begin());
    }

    update();
}

// =================================== 输入 ===================================

void Menu::keyPressEvent(QKeyEvent* event) {
    const int key = event->key();

    // ---- 控制台先吃掉所有按键 ----
    if (state_ == GameState::Console) {
        if (key == Qt::Key_Escape) {
            closeConsole();
        } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            runConsoleCommand();
        } else if (key == Qt::Key_Backspace) {
            if (!consoleInput_.isEmpty()) {
                consoleInput_.chop(1);
                update();
            }
        } else if (!event->text().isEmpty() && event->text().at(0).isPrint()) {
            consoleInput_ += event->text();
            update();
        }
        return;
    }

    // ---- 移动键只置位，自动重复也无所谓 ----
    switch (key) {
        case Qt::Key_W: case Qt::Key_Up:    keyUp_ = true;     return;
        case Qt::Key_S: case Qt::Key_Down:  keyDown_ = true;   return;
        case Qt::Key_A: case Qt::Key_Left:  keyLeft_ = true;   return;
        case Qt::Key_D: case Qt::Key_Right: keyRight_ = true;  return;
        case Qt::Key_Shift:                 keySprint_ = true; return;
        case Qt::Key_Space:
            keyFire_ = true;
            if (!event->isAutoRepeat() && state_ == GameState::Menu) {
                startGame();
            }
            return;
        default:
            break;
    }

    if (event->isAutoRepeat()) {
        return;   // 长按 Esc 不该反复切换暂停
    }

    switch (state_) {
        case GameState::Menu:
            if (key == Qt::Key_Return || key == Qt::Key_Enter) {
                startGame();
            }
            break;

        case GameState::About:
        case GameState::Guide:
            if (key == Qt::Key_Escape) {
                state_ = GameState::Menu;
            }
            break;

        case GameState::Playing:
            if (key == Qt::Key_Escape || key == Qt::Key_P) {
                pauseGame(true);
            } else if (key == Qt::Key_QuoteLeft || key == Qt::Key_AsciiTilde) {
                openConsole();
            }
            break;

        case GameState::Paused:
            if (key == Qt::Key_Escape || key == Qt::Key_P) {
                pauseGame(false);
            }
            break;

        case GameState::GameOver:
            if (key == Qt::Key_R) {
                startGame();
            } else if (key == Qt::Key_Escape) {
                backToMenu();
            }
            break;

        case GameState::Console:
            break;
    }

    update();
}

void Menu::keyReleaseEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_W: case Qt::Key_Up:    keyUp_ = false;     break;
        case Qt::Key_S: case Qt::Key_Down:  keyDown_ = false;   break;
        case Qt::Key_A: case Qt::Key_Left:  keyLeft_ = false;   break;
        case Qt::Key_D: case Qt::Key_Right: keyRight_ = false;  break;
        case Qt::Key_Shift:                 keySprint_ = false; break;
        case Qt::Key_Space:                 keyFire_ = false;   break;
        default: break;
    }
    QWidget::keyReleaseEvent(event);
}

void Menu::mousePressEvent(QMouseEvent* event) {
    const QPoint pos = event->position().toPoint();
    mousePos_ = pos;
    updateButtonPosition();

    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    switch (state_) {
        case GameState::Menu:
            if (startButton.contains(pos)) {
                startGame();
            } else if (aboutButton.contains(pos)) {
                state_ = GameState::About;
            } else if (guideButton.contains(pos)) {
                state_ = GameState::Guide;
            }
            break;

        case GameState::About:
        case GameState::Guide:
            if (backButton.contains(pos)) {
                state_ = GameState::Menu;
            }
            break;

        case GameState::Playing:
            mouseHeld_ = true;      // 按住左键连射
            break;

        case GameState::Paused:
            if (resumeButton.contains(pos)) {
                pauseGame(false);
            } else if (quitButton.contains(pos)) {
                backToMenu();
            }
            break;

        case GameState::GameOver:
        case GameState::Console:
            break;
    }

    update();
}

void Menu::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        mouseHeld_ = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void Menu::mouseMoveEvent(QMouseEvent* event) {
    mousePos_ = event->position().toPoint();
    QWidget::mouseMoveEvent(event);
}

void Menu::resizeEvent(QResizeEvent* event) {
    updateButtonPosition();
    QWidget::resizeEvent(event);
}

void Menu::closeEvent(QCloseEvent* event) {
    game_.stop();
    QWidget::closeEvent(event);
}

void Menu::updateButtonPosition() {
    startButton  = QRect((width() - 220) / 2, height() / 2 - 30, 220, 80);
    aboutButton  = QRect((width() - 200) / 2, startButton.bottom() + 20, 200, 70);
    guideButton  = QRect((width() - 200) / 2, aboutButton.bottom() + 20, 200, 70);
    resumeButton = QRect((width() - 200) / 2, height() / 2 - 50, 200, 65);
    quitButton   = QRect((width() - 200) / 2, height() / 2 + 20, 200, 65);
    backButton   = QRect((width() - 200) / 2, height() - 120, 200, 60);
}

// =================================== 绘制 ===================================

void Menu::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    switch (state_) {
        case GameState::Menu:
            drawMenu(painter);
            break;
        case GameState::About:
            drawAbout(painter);
            break;
        case GameState::Guide:
            drawGuide(painter);
            break;
        case GameState::Playing:
        case GameState::Paused:
        case GameState::GameOver:
        case GameState::Console:
            drawGame(painter);
            break;
    }

    if (state_ == GameState::Paused) {
        drawPauseOverlay(painter);
    } else if (state_ == GameState::GameOver) {
        drawGameOver(painter);
    } else if (state_ == GameState::Console) {
        drawConsole(painter);
    }
}

void Menu::drawButton(QPainter& painter, const QRect& box, const QString& text, bool primary) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(primary ? QColor(200, 200, 200) : QColor(80, 80, 80));
    painter.drawRoundedRect(box, 15, 15);

    painter.setFont(uiFont(28, true));
    painter.setPen(primary ? QColor(20, 20, 20) : QColor(230, 230, 230));
    painter.drawText(box, Qt::AlignCenter, text);
}

void Menu::drawMenu(QPainter& painter) {
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setFont(uiFont(72, true));
    painter.setPen(Qt::gray);
    painter.drawText(rect().adjusted(0, -150, 0, 0), Qt::AlignCenter, QStringLiteral("GodGame"));

    drawButton(painter, startButton, QStringLiteral("开始游戏"), true);
    drawButton(painter, aboutButton, QStringLiteral("关于我们"), false);
    drawButton(painter, guideButton, QStringLiteral("图鉴"), false);

    painter.setFont(uiFont(20, true));
    painter.setPen(Qt::gray);
    painter.drawText(20, height() - 20, QStringLiteral("Version 0.1"));
}

void Menu::drawAbout(QPainter& painter) {
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setFont(uiFont(56, true));
    painter.setPen(Qt::white);
    painter.drawText(rect().adjusted(0, -420, 0, 0), Qt::AlignCenter, QStringLiteral("关于我们"));

    painter.setFont(uiFont(26));
    painter.setPen(QColor(200, 200, 200));

    const QStringList lines{
        QStringLiteral("GodGame —— 用 Qt6 手写的一个小打怪游戏"),
        QStringLiteral(""),
        QStringLiteral("WASD 移动，按住鼠标左键开火，Shift 冲刺"),
        QStringLiteral("Esc / P 暂停，` 打开控制台"),
        QStringLiteral(""),
        QStringLiteral("整个游戏只有两个部分："),
        QStringLiteral("一条逻辑线程按固定步长推进世界，"),
        QStringLiteral("主线程只负责把这一帧画出来。"),
        QStringLiteral(""),
        QStringLiteral("参数都在 cfg.ini 里，改完重启即可"),
    };

    int y = height() / 2 - 150;
    for (const QString& line : lines) {
        painter.drawText(QRect(0, y, width(), 40), Qt::AlignCenter, line);
        y += 34;
    }

    drawButton(painter, backButton, QStringLiteral("返回"), false);
}

void Menu::drawGuide(QPainter& painter) {
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setFont(uiFont(56, true));
    painter.setPen(Qt::white);
    painter.drawText(rect().adjusted(0, -380, 0, 0), Qt::AlignCenter, QStringLiteral("图  鉴"));

    struct Column {
        QString              title;
        QStringList          lines;
    };

    const Column columns[] = {
        {QStringLiteral("【 操作 】"),
         {QStringLiteral("W A S D    移动"),
          QStringLiteral("Shift      冲刺（耗体力）"),
          QStringLiteral("鼠标左键   开火（按住连射）"),
          QStringLiteral("Esc / P    暂停"),
          QStringLiteral("`          控制台"),
          QStringLiteral("R          结算后重开")}},
        {QStringLiteral("【 补给品 】"),
         {QStringLiteral("弹药箱   +20~35 发"),
          QStringLiteral("血包     +10 点血"),
          QStringLiteral("19 秒后消失"),
          QStringLiteral("击杀掉落：弹药 9%"),
          QStringLiteral("          血包 4%"),
          QStringLiteral("Boss 必掉两样")}},
        {QStringLiteral("【 敌人 】"),
         {QStringLiteral("普通   深红椭圆，速度随机"),
          QStringLiteral("冲刺型 黑底灰边，会瞬移"),
          QStringLiteral("Boss   紫色巨物，半血无敌"),
          QStringLiteral("        并召唤 10 个帮手"),
          QStringLiteral("撞到你：普通 -20"),
          QStringLiteral("        Boss -60")}},
    };

    const int columnWidth = width() / 3;
    const int top         = height() / 2 - 160;

    for (int i = 0; i < 3; ++i) {
        const QRect area(i * columnWidth, top, columnWidth, 340);

        painter.setFont(uiFont(30, true));
        painter.setPen(QColor(120, 200, 255));
        painter.drawText(QRect(area.left(), area.top(), area.width(), 40), Qt::AlignCenter, columns[i].title);

        painter.setFont(uiFont(22));
        painter.setPen(QColor(215, 215, 215));
        int y = area.top() + 60;
        for (const QString& line : columns[i].lines) {
            painter.drawText(QRect(area.left(), y, area.width(), 32), Qt::AlignCenter, line);
            y += 34;
        }
    }

    drawButton(painter, backButton, QStringLiteral("返回"), false);
}

void Menu::drawGame(QPainter& painter) {
    const Game::FramePtr frame = game_.frame();
    if (!frame) {
        return;
    }

    if (frame->hud.bossWarning > 0) {
        drawBossWarning(painter, frame->hud);
    }

    painter.setRenderHint(QPainter::Antialiasing, true);
    for (const RenderItem& item : frame->items) {
        drawItem(painter, item);
    }
    painter.setRenderHint(QPainter::Antialiasing, false);

    drawHud(painter, frame->hud);
}

void Menu::drawItem(QPainter& painter, const RenderItem& item) {
    // 快过期的补给按旧版的做法整帧跳过绘制
    const bool isPickup = (item.style == RenderStyle::AmmoBox || item.style == RenderStyle::HealthPack);
    if (isPickup && item.flash) {
        return;
    }

    const QRectF box(item.x, item.y, item.w, item.h);

    switch (item.style) {
        case RenderStyle::Player: {
            painter.setPen(QPen(Qt::white, item.flash ? 3 : 2));
            painter.setBrush(QColor(80, 200, 120));
            painter.drawEllipse(box);

            // 枪管：绕身体中心转到瞄准方向
            painter.save();
            painter.translate(box.center());
            painter.rotate(static_cast<double>(item.angle) * 180.0 / M_PI);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(180, 180, 180));
            painter.drawRect(QRectF(10.0, -4.0, 22.0, 8.0));
            painter.restore();
            break;
        }

        case RenderStyle::Enemy: {
            static const QColor tiers[3] = {
                QColor(120, 0, 0), QColor(200, 0, 0), QColor(255, 50, 50),
            };
            const int tier = std::clamp<int>(item.variant, 0, 2);
            painter.setPen(QPen(QColor(60, 0, 0), 2));
            painter.setBrush(tiers[tier]);
            painter.drawEllipse(box);

            if (item.hpRatio < 0.999f) {
                const QRectF bar(box.left(), box.top() - 7.0, box.width(), 3.0);
                painter.fillRect(bar, QColor(60, 60, 60));
                painter.fillRect(QRectF(bar.left(), bar.top(), bar.width() * item.hpRatio, bar.height()),
                                 QColor(230, 60, 60));
            }
            break;
        }

        case RenderStyle::EnemyDasher: {
            painter.setPen(QPen(QColor(160, 160, 160), 3));
            painter.setBrush(Qt::black);
            painter.drawEllipse(box);

            if (item.hpRatio < 0.999f) {
                const QRectF bar(box.left(), box.top() - 7.0, box.width(), 3.0);
                painter.fillRect(bar, QColor(60, 60, 60));
                painter.fillRect(QRectF(bar.left(), bar.top(), bar.width() * item.hpRatio, bar.height()),
                                 QColor(230, 60, 60));
            }
            break;
        }

        case RenderStyle::Boss: {
            painter.setPen(QPen(Qt::white, 3));
            painter.setBrush(QColor(180, 80, 130));
            painter.drawEllipse(box);
            break;
        }

        case RenderStyle::PlayerBullet:
            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::darkYellow);
            painter.drawEllipse(box);
            break;

        case RenderStyle::EnemyBullet:
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 80, 80));
            painter.drawEllipse(box);
            break;

        case RenderStyle::AmmoBox: {
            painter.setPen(QPen(QColor(120, 90, 40), 2));
            painter.setBrush(QColor(180, 140, 60));
            painter.drawRoundedRect(box, 6, 6);
            painter.setFont(uiFont(14, true));
            painter.setPen(Qt::white);
            painter.drawText(box, Qt::AlignCenter, QStringLiteral("A"));
            break;
        }

        case RenderStyle::HealthPack: {
            painter.setPen(QPen(QColor(200, 40, 40), 2));
            painter.setBrush(Qt::white);
            painter.drawRoundedRect(box, 4, 4);
            painter.setPen(QPen(QColor(230, 60, 60), 3));
            const QPointF c = box.center();
            painter.drawLine(QPointF(c.x() - 6.0, c.y()), QPointF(c.x() + 6.0, c.y()));
            painter.drawLine(QPointF(c.x(), c.y() - 6.0), QPointF(c.x(), c.y() + 6.0));
            break;
        }

        case RenderStyle::Generic:
        default:
            painter.setPen(Qt::white);
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(box);
            break;
    }
}

void Menu::drawBossWarning(QPainter& painter, const HudState&) {
    // 方波闪烁：每 15 帧翻一次
    if ((frameCounter_ / 15) % 2 == 0) {
        return;
    }

    painter.fillRect(rect(), QColor(120, 0, 0, 80));
    painter.setFont(uiFont(80, true));
    painter.setPen(QColor(255, 50, 50));
    painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("/// /// WARNING /// ///"));
}

void Menu::drawHud(QPainter& painter, const HudState& hud) {
    const int barX = width() - 320;

    // ---------------- 血条 ----------------
    const QRect hpBar(barX, 20, 300, 24);
    painter.fillRect(hpBar, QColor(60, 60, 60));

    const float hpRatio = hud.maxHp > 0 ? static_cast<float>(hud.hp) / static_cast<float>(hud.maxHp) : 0.0f;

    // 白条那截是"已经记在账上、还没转成真血的伤害"：它紧贴在真血顶端，
    // 等它缩没了，伤害才算真的到账。颜色档位按**安全血量**取，
    // 否则会出现"血条只剩一成却还是绿的"这种自欺欺人的画面
    const float pending   = std::clamp(hud.hpPending, 0.0f, hpRatio);
    const float safeRatio = hpRatio - pending;

    QColor hpColor(230, 60, 60);
    if (safeRatio > 0.6f) {
        hpColor = QColor(80, 220, 80);
    } else if (safeRatio > 0.3f) {
        hpColor = QColor(230, 200, 60);
    }

    if (pending > 0.0f) {
        painter.fillRect(QRect(hpBar.left(), hpBar.top(),
                               static_cast<int>(static_cast<float>(hpBar.width()) * hpRatio),
                               hpBar.height()),
                         QColor(250, 250, 250, 190));
    }
    painter.fillRect(QRect(hpBar.left(), hpBar.top(),
                           static_cast<int>(static_cast<float>(hpBar.width()) * safeRatio), hpBar.height()),
                     hpColor);

    painter.setFont(uiFont(20, true));
    painter.setPen(Qt::white);
    painter.drawText(hpBar, Qt::AlignCenter,
                     QStringLiteral("HP:%1/%2").arg(hud.hp).arg(hud.maxHp));

    // ---------------- 体力条 ----------------
    const QRect spBar(barX - 120, 20, 110, 15);
    painter.fillRect(spBar, QColor(60, 60, 60));
    const float spRatio = hud.maxStamina > 0.0f ? hud.stamina / hud.maxStamina : 0.0f;
    painter.fillRect(QRect(spBar.left(), spBar.top(),
                           static_cast<int>(static_cast<float>(spBar.width()) * spRatio), spBar.height()),
                     hud.exhausted ? QColor(180, 180, 180) : QColor(80, 200, 255));

    painter.setFont(uiFont(14, true));
    painter.setPen(Qt::white);
    painter.drawText(spBar, Qt::AlignCenter, QStringLiteral("SP:%1").arg(static_cast<int>(hud.stamina)));

    // ---------------- 经验 ----------------
    const QRect expBar(barX, 52, 300, 14);
    painter.fillRect(expBar, QColor(60, 60, 60));
    painter.fillRect(QRect(expBar.left(), expBar.top(),
                           static_cast<int>(static_cast<float>(expBar.width()) * hud.expRatio),
                           expBar.height()),
                     QColor(80, 200, 255));

    painter.setFont(uiFont(20, true));
    painter.setPen(Qt::cyan);
    painter.drawText(barX, 90, QStringLiteral("Lv:%1  EXP:%2/%3")
                                   .arg(hud.level).arg(hud.exp).arg(hud.expNext));

    // ---------------- 弹药 ----------------
    painter.setFont(uiFont(36, true));
    painter.setPen(Qt::yellow);
    painter.drawText(width() - 80, 110, QString::number(hud.ammo));

    // ---------------- 左上角信息 ----------------
    painter.setFont(uiFont(24, true));
    painter.setPen(Qt::white);
    painter.drawText(20, 40, QStringLiteral("Score: %1").arg(hud.score));
    painter.drawText(20, 90, QStringLiteral("Time: %1 s").arg(hud.elapsed, 0, 'f', 1));
    painter.drawText(20, 140, QStringLiteral("Round: %1").arg(hud.round));
    painter.drawText(20, 170, QStringLiteral("Enemy remained : %1").arg(hud.enemiesLeft));

    // 回合清空倒计时：旧版是最后 3 秒开始闪
    if (hud.roundWait > 0 && hud.roundWait <= 180 && (frameCounter_ / 15) % 2 == 0) {
        painter.setPen(Qt::yellow);
        painter.drawText(20, 200, QStringLiteral("Next round in %1 s")
                                      .arg(static_cast<double>(hud.roundWait) / config().tickRate, 0, 'f', 1));
    }

    // ---------------- 升级提示 ----------------
    if (hud.levelUpFlash > 0) {
        painter.setFont(uiFont(48, true));
        painter.setPen(QColor(255, 215, 0));
        painter.drawText(rect().adjusted(0, 120, 0, 0), Qt::AlignCenter, QStringLiteral("LEVEL UP!"));
    }

    // ---------------- Boss 血条 ----------------
    if (hud.bossActive && hud.bossMaxHp > 0) {
        const QRect bossBar((width() - 600) / 2, 45, 600, 30);
        painter.fillRect(bossBar, QColor(60, 60, 60));

        // 白条铺在真血顶端：待结算的那部分。和玩家血条是同一种东西、同一种画法
        const float bossPending = std::clamp(hud.bossPending, 0.0f, hud.bossHpRatio);
        if (bossPending > 0.0f) {
            painter.fillRect(QRect(bossBar.left(), bossBar.top(),
                                   static_cast<int>(static_cast<float>(bossBar.width()) * hud.bossHpRatio),
                                   bossBar.height()),
                             QColor(235, 235, 235));
        }
        painter.fillRect(QRect(bossBar.left(), bossBar.top(),
                               static_cast<int>(static_cast<float>(bossBar.width())
                                                * (hud.bossHpRatio - bossPending)),
                               bossBar.height()),
                         QColor(200, 40, 40));

        painter.setFont(uiFont(18, true));
        painter.setPen(Qt::white);
        painter.drawText(bossBar, Qt::AlignCenter,
                         QStringLiteral("BOSS %1/%2").arg(hud.bossHp).arg(hud.bossMaxHp));
    }
}

void Menu::drawPauseOverlay(QPainter& painter) {
    painter.fillRect(rect(), QColor(0, 0, 0, 160));

    painter.setFont(uiFont(64, true));
    painter.setPen(Qt::white);
    painter.drawText(rect().adjusted(0, -160, 0, 0), Qt::AlignCenter, QStringLiteral("已暂停"));

    drawButton(painter, resumeButton, QStringLiteral("继续游戏"), true);
    drawButton(painter, quitButton, QStringLiteral("退出到主菜单"), false);
}

void Menu::drawGameOver(QPainter& painter) {
    painter.fillRect(rect(), QColor(0, 0, 0, 190));

    const Game::FramePtr frame = game_.frame();
    const HudState hud = frame ? frame->hud : HudState{};

    QFont font = uiFont(40, true);
    painter.setFont(font);
    painter.setPen(Qt::white);

    const QStringList lines{
        QStringLiteral("Game Over"),
        QStringLiteral(""),
        QStringLiteral("Score: %1").arg(hud.score),
        QStringLiteral("Time: %1 s").arg(hud.elapsed, 0, 'f', 1),
        QStringLiteral("Round: %1").arg(hud.round),
        QStringLiteral(""),
        QStringLiteral("按 R 重新开始"),
        QStringLiteral("按 Esc 返回主菜单"),
    };

    int y = height() / 2 - 200;
    for (const QString& line : lines) {
        painter.drawText(QRect(0, y, width(), 56), Qt::AlignCenter, line);
        y += 52;
    }
}

void Menu::drawConsole(QPainter& painter) {
    const int panelHeight = 260;
    const QRect panel(0, height() - panelHeight, width(), panelHeight);
    painter.fillRect(panel, QColor(0, 0, 0, 220));
    painter.setPen(QColor(90, 90, 90));
    painter.drawLine(panel.topLeft(), panel.topRight());

    painter.setFont(monoFont(18));
    painter.setPen(QColor(120, 255, 120));

    int y = panel.top() + 30;
    for (const QString& line : consoleHistory_) {
        painter.drawText(20, y, line);
        y += 24;
    }

    painter.setPen(QColor(120, 255, 120));
    const QString prompt = QStringLiteral("> ") + consoleInput_;
    painter.drawText(20, panel.bottom() - 24, prompt);

    // 光标闪烁
    if ((frameCounter_ / 20) % 2 == 0) {
        const int cursorX = 20 + QFontMetrics(painter.font()).horizontalAdvance(prompt) + 2;
        painter.fillRect(QRect(cursorX, panel.bottom() - 42, 10, 22), QColor(120, 255, 120));
    }

    painter.setFont(uiFont(16));
    painter.setPen(QColor(160, 160, 160));
    painter.drawText(QRect(panel.left(), panel.top() + 4, panel.width() - 20, 24),
                     Qt::AlignRight, QStringLiteral("Esc 关闭控制台  ·  Enter 执行"));
}
