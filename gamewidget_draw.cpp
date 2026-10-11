#include "gamewidget.h"
#include <QPainter>
#include <cmath>
#include <QDebug>

void GameWidget::drawMenu(QPainter &painter)
{
    QFont titleFont;
    titleFont.setFamily("微软雅黑");
    titleFont.setPixelSize(72);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(Qt::gray);
    painter.drawText(rect().adjusted(0,-150,0,0),Qt::AlignCenter,"Qt Defencer");

    painter.setBrush(Qt::gray);
    painter.setPen(Qt::white);
    painter.drawRoundedRect(startButton,15,15);

    QFont btnFont;
    btnFont.setFamily("微软雅黑");
    btnFont.setPixelSize(32);
    btnFont.setBold(true);
    painter.setFont(btnFont);
    painter.setPen(Qt::white);
    painter.drawText(startButton,Qt::AlignCenter,"开始游戏");       //绘制开始界面

    painter.setBrush(QColor(100,100,100));
    painter.setPen(Qt::white);;
    painter.drawRoundedRect(aboutButton,15,15);

    QFont aboutFont;                //“关于我们”按钮
    aboutFont.setFamily("微软雅黑");
    aboutFont.setPixelSize(28);
    aboutFont.setBold(true);
    painter.setFont(aboutFont);
    painter.setPen(Qt::white);
    painter.drawText(aboutButton,Qt::AlignCenter,"关于我们");

    painter.setBrush(QColor(100,100,100));
    painter.setPen(Qt::white);
    painter.drawRoundedRect(guideButton,15,15);

    QFont guideFont;
    guideFont.setFamily("微软雅黑");
    guideFont.setPixelSize(28);
    guideFont.setBold(true);
    painter.setFont(guideFont);
    painter.setPen(Qt::white);
    painter.drawText(guideButton,Qt::AlignCenter,"图鉴");

    QFont versiontFont;
    versiontFont.setFamily("微软雅黑");
    versiontFont.setPixelSize(24);
    versiontFont.setBold(true);
    painter.setFont(versiontFont);
    painter.setPen(Qt::gray);
    painter.drawText(rect().adjusted(5,0,0,0),Qt::AlignLeft|Qt::AlignBottom,"Version 1.1 test");

    return;
}

void GameWidget::drawAbout(QPainter &painter)
{
    QFont titleFont;
    titleFont.setFamily("微软雅黑");
    titleFont.setPixelSize(56);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(Qt::gray);
    painter.drawText(rect().adjusted(0,-500,0,0),Qt::AlignCenter,"关于我们");

    QFont contentFont;
    contentFont.setFamily("微软雅黑");
    contentFont.setPixelSize(40);
    contentFont.setBold(true);
    painter.setFont(contentFont);
    painter.setPen(Qt::white);
    QString text="Defencer\n"
                 "Created by c++\n"
                 "Environment : Qt 5.9.9 Mingw53_32\n"
                 "Creator : RL\n"
                 "Designer : GG & gg\n"
                 "Version : 1.1(test)\n"
                 "Date : 2026.10.7\n\n\n\n"
                 "Press Esc to Return Meun";
    painter.drawText(rect().adjusted(0,130,0,0),Qt::AlignCenter,text);
    return;
}
                                //绘制控制台
void GameWidget::drawConsole(QPainter &painter)
{
    painter.fillRect(rect(),QColor(0,0,0,180));

    const int boxW=800;
    const int boxH=60;
    const int boxX=(width()-boxW)/2;
    const int boxY=(height()-boxH)/2;

    painter.setBrush(QColor(0,0,0,200));
    painter.setPen(QPen(Qt::white,2));
    painter.drawRect(boxX,boxY,boxW,boxH);

    QFont titleFont;
    titleFont.setFamily("Consolas");
    titleFont.setPixelSize(28);
    titleFont.setBold(true);
    titleFont.setItalic(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(230,230,230,200));
    painter.drawText(QRect(boxX,boxY+7,boxW,40),Qt::AlignCenter|Qt::AlignVCenter,"Console (输入 /help 查看指令)");

    QFont inputfont;
    inputfont.setFamily("Consolas");
    inputfont.setPixelSize(28);
    painter.setFont(inputfont);
    painter.setPen(Qt::white);

    int textX=boxX+15;
    int textY=boxY+boxH-18;

    painter.drawText(textX,textY,">");
    painter.drawText(textX+25,textY,consoleInput);

    bool cursorVisible=(survivalTimer.elapsed()/500)%2==0;
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    int textW=painter.fontMetrics().horizontalAdvance(consoleInput);
#else
    int textW=painter.fontMetrics().width(consoleInput);
#endif
    if(cursorVisible){
        painter.drawLine(textX+25+textW,textY+4,textX+35+textW,textY+4);
    }
}

void GameWidget::drawGuide(QPainter &painter)
{
    QFont titleFont;
    titleFont.setFamily("微软雅黑");
    titleFont.setPixelSize(48);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(Qt::gray);
    painter.drawText(rect().adjusted(0,-420,0,0),Qt::AlignCenter,"图 鉴");

    int colW=width()/3;
    int topY=120;
    {
        int x=40;
        QFont secFont;
        secFont.setFamily("微软雅黑");
        secFont.setPixelSize(28);
        secFont.setBold(true);
        painter.setFont(secFont);
        painter.setPen(QColor(80,200,255));
        painter.drawText(x,topY,"【界面说明】");

        QFont textFont;
        textFont.setFamily("微软雅黑");
        textFont.setPixelSize(20);
        textFont.setBold(false);
        painter.setFont(textFont);
        painter.setPen(Qt::white);

        QString content =
                  "HP    血条\n"
                  "      归零则游戏结束\n\n"
                  "SP    体力条\n"
                  "      按 Shift 冲刺，\n"
                  "      体力耗尽后需恢复\n"
                  "      到 15% 才能再冲刺\n\n"
                  "EXP   经验条\n"
                  "      击杀敌人 +10 经验\n"
                  "      升级后升级需求 +50\n\n"
                  "AMMO  弹药数\n"
                  "      为 0 时无法射击\n\n"
                  "Score 得分\n"
                  "Time  存活时间";

      painter.drawText(QRect(x, topY + 20, colW - 60, height() - topY - 120),Qt::AlignLeft | Qt::AlignTop,content);
    }
    {
       int x = colW + 20;
       QFont secFont;
       secFont.setFamily("微软雅黑");
       secFont.setPixelSize(28);
       secFont.setBold(true);
       painter.setFont(secFont);
       painter.setPen(QColor(80, 220, 120));
       painter.drawText(x, topY, "【 补给品 】");

       QFont textFont;
       textFont.setFamily("微软雅黑");
       textFont.setPixelSize(20);
       painter.setFont(textFont);
       painter.setPen(Qt::white);

       QString content =
               "弹药箱\n"
               "  土黄色方块，中间写 A\n"
               "  击败敌人 15% 概率掉落\n"
               "  拾取 +20~35 发子弹\n\n"
               "血包\n"
               "  白色红十字方块\n"
               "  击败敌人 10% 概率掉落\n"
               "  拾取 +10 点 HP\n\n"
               "说明\n"
               "  两种补给 20 秒未拾取\n"
               "  会自动消失\n"
               "  最后 5 秒闪烁提示";

       painter.drawText(QRect(x, topY + 20, colW - 40, height() - topY - 120),Qt::AlignLeft | Qt::AlignTop,content);
       }
    {
       int x = colW * 2;
       QFont secFont;
       secFont.setFamily("微软雅黑");
       secFont.setPixelSize(28);
       secFont.setBold(true);
       painter.setFont(secFont);
       painter.setPen(QColor(255, 100, 100));
       painter.drawText(x, topY, "【 敌人种类 】");

       QFont textFont;
       textFont.setFamily("微软雅黑");
       textFont.setPixelSize(20);
       painter.setFont(textFont);
       painter.setPen(Qt::white);

       QString content =
                "普通敌人\n"
                "  红色圆，颜色越亮越快\n"
                "  速度 1~4，持续追踪玩家\n\n"
                "闪现敌人\n"
                "  黑色白框圆\n"
                "  10% 概率刷新\n"
                "  每 5 秒瞬移 350 距离\n"
                "  距玩家 400 内不闪现\n\n"
                "BOSS\n"
                "  暗粉色大圆，20 点血\n"
                "  移动速度 2\n"
                "  半血时闪烁无敌 5 秒\n"
                "  并召唤 10 个高速敌人\n"
                "  出现前 10 秒有 WARNING";

         painter.drawText(QRect(x, topY + 20, colW - 20, height() - topY - 120),Qt::AlignLeft | Qt::AlignTop,content);
        }
                                                      // 返回按钮
        painter.setBrush(Qt::darkGray);
        painter.setPen(Qt::white);
        painter.drawRoundedRect(guideBackButton, 15, 15);

        QFont backFont;
        backFont.setFamily("微软雅黑");
        backFont.setPixelSize(28);
        backFont.setBold(true);
        painter.setFont(backFont);
        painter.setPen(Qt::white);
        painter.drawText(guideBackButton, Qt::AlignCenter, "返回");

        return;
}

void GameWidget::drawHUD(QPainter &painter)
{
    const int barW=300;
    const int barH=24;
    const int barX=width()-barW-20;  // 绘制血条
    const int barY=20;

    painter.setBrush(QColor(60,60,60));
    painter.setPen(Qt::NoPen);
    painter.drawRect(barX,barY,barW,barH);

    double ratio=static_cast<double>(hp)/maxhp;
    int fillW=static_cast<int>(barW*ratio);

    QColor hpColor;
    if(ratio>0.6) hpColor=QColor(80,220,80);
        else if(ratio>0.3)hpColor=QColor(230,200,60);
            else hpColor=QColor(230,60,60);

    painter.setBrush(hpColor);
    painter.setPen(Qt::NoPen);
    painter.drawRect(barX,barY,fillW,barH);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(Qt::white);
    painter.drawRect(barX,barY,barW,barH);

    QFont hpFont;
    hpFont.setFamily("微软雅黑");
    hpFont.setPixelSize(20);
    hpFont.setBold(true);
    painter.setFont(hpFont);
    painter.setPen(Qt::white);
    painter.drawText(QRect(barX,barY,barW,barH),Qt::AlignCenter,QString("HP:%1/%2").arg(hp).arg(maxhp));

    const int staminaBarW=110;      //绘制体力条
    const int staminaBarH=15;
    const int staminaBarY=20;
    const int staminaBarX=barX-staminaBarW-10;

    painter.setBrush(QColor(60,60,60));
    painter.setPen(Qt::NoPen);
    painter.drawRect(staminaBarX,staminaBarY,staminaBarW,staminaBarH);

    double staminaRatio=stamina/maxStamina;
    int staminaFillW=static_cast<int>(staminaBarW*staminaRatio);
    if(exhausted){
        painter.setBrush(QColor(180,180,180));
    }else{
          painter.setBrush(QColor(80,200,255));
    }

    painter.drawRect(staminaBarX,staminaBarY,staminaFillW,staminaBarH);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(Qt::white);;
    painter.drawRect(staminaBarX,staminaBarY,staminaBarW,staminaBarH);

    QFont staminaFont;
    staminaFont.setFamily("微软雅黑");
    staminaFont.setPixelSize(14);
    staminaFont.setBold(true);
    painter.setFont(staminaFont);
    painter.setPen(Qt::white);
    painter.drawText(QRect(staminaBarX,staminaBarY,staminaBarW,staminaBarH),
                     Qt::AlignCenter,QString("SP:%1").arg(static_cast<int>(stamina)));

    QFont expFond;              //绘制经验条
    expFond.setFamily("微软雅黑");
    expFond.setPixelSize(20);
    expFond.setBold(true);;
    painter.setFont(expFond);
    painter.setPen(Qt::cyan);
    painter.drawText(barX,barY+70,QString("Lv:%1 EXP:%2/%3").arg(level).arg(expValue).arg(expToNext));

    const int expBarW=barW;
    const int expBarH=14;
    const int expBarX=barX;
    const int expBarY=barY+barH+8;

    painter.setBrush(QColor(60,60,60));
    painter.setPen(Qt::NoPen);
    painter.drawRect(expBarX,expBarY,expBarW,expBarH);

    double expRatio=static_cast<double>(expValue)/expToNext;
    int expFillW=static_cast<int>(expBarW*expRatio);
    painter.setBrush(QColor(80,200,255));
    painter.drawRect(expBarX,expBarY,expFillW,expBarH);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(Qt::white);
    painter.drawRect(expBarX,expBarY,expBarW,expBarH);

    QFont ammoFont;         //绘制子弹数量
    ammoFont.setFamily("微软雅黑");
    ammoFont.setPixelSize(36);
    ammoFont.setBold(true);;
    painter.setFont(ammoFont);
    painter.setPen(Qt::yellow);
    painter.drawText(barX+240,barY+90,QString("%1").arg(ammo));

                         // 分数
    QFont scoreFont;
    scoreFont.setFamily("微软雅黑");
    scoreFont.setPixelSize(36);
    scoreFont.setBold(true);
    painter.setFont(scoreFont);
    painter.setPen(Qt::white);
    painter.drawText(20, 40, QString("Score:%1").arg(score));

                             // 时间
    qint64 ms = (state == GameState::GameOver)
                ? frozenElapsed
                : (state == GameState::Paused
                   ? pausedAt - totalPaused
                   : survivalTimer.elapsed() - totalPaused);
    int seconds = static_cast<int>(ms / 1000);
    painter.drawText(20, 90, QString("Time:%1 s").arg(seconds));

    QFont roundFont;                //绘制回合数
    roundFont.setFamily("微软雅黑");
    roundFont.setPixelSize(24);
    roundFont.setBold(true);
    painter.setFont(roundFont);
    painter.setPen(Qt::white);
    painter.drawText(20,170,QString("Enemy remained : %1").arg(enemyStanding));
    bool roundVisible=true;
    if(roundState==0 && roundWaitTime>0 && roundWaitTime<=180){
    }
    if(roundState==4 && roundVisible>0 && roundWaitTime<=180){
        roundVisible=(roundWaitTime/15)%2==0;
    }
    if(roundVisible){
    painter.drawText(20,140,QString("Round : %1").arg(round));
    }
}

void GameWidget::drawLevelUp(QPainter &painter)
{
    if(levelUpTimer<=0)return;
    int alpha=255;
    if(levelUpTimer<30){
        alpha=levelUpTimer*255/30;
    }
    QFont levelFont;
    levelFont.setBold(true);
    levelFont.setFamily("微软雅黑");
    levelFont.setPixelSize(20);
    painter.setFont(levelFont);
    painter.setPen(QColor(80,200,255,alpha));
    painter.drawText(player.center().x()-42,player.center().y()-28,"Level up!");
    qDebug()<<"leveluptext";
}

void GameWidget::drawPlayer(QPainter &painter)
{
    bool palyerVisible=true;
    if(invincibleTimer>0 && invincibleTimer<120){
        palyerVisible=(invincibleTimer/6)%2==0;
    }
    if(palyerVisible){
    painter.setBrush(Qt::green);           //绘制玩家
    painter.setPen(Qt::white);
    painter.drawEllipse(player);
    painter.setRenderHint(QPainter::Antialiasing);

    double dx=mousePos.x()-player.center().x();
    double dy=mousePos.y()-player.center().y();
    double degrees=std::atan2(dy,dx)*180.0/M_PI;

    painter.save();
    painter.translate(player.center());
    painter.rotate(degrees);

    painter.setBrush(Qt::gray);
    painter.setPen(Qt::NoPen);
    painter.drawRect(QRect(12,-4,20,8));
    painter.restore();
    }
}

void GameWidget::drawBullets(QPainter &painter)
{
    painter.setBrush(Qt::darkYellow);       //绘制子弹
    for (const Bullet &b : bullets){
        painter.drawEllipse(b.rect);
    }
    painter.setPen(Qt::white);
    painter.setBrush(Qt::NoBrush);

}

void GameWidget::drawEnemies(QPainter &painter)
{
    painter.setPen(Qt::white);          //绘制敌人
    for(const Enemy &e :enemies){
        if(!e.alive)continue;
        if(e.isDasher){
            painter.setBrush(Qt::black);
            painter.setPen(QPen(Qt::gray,3));
            painter.drawEllipse(e.rect);
        }else{
        if(e.speed>=4)painter.setBrush(QColor(255,50,50));
        else if(e.speed>=2)painter.setBrush(QColor(200,0,0));
        else painter.setBrush(QColor(120,0,0));
        painter.drawEllipse(e.rect);
        }
    }
}

void GameWidget::drawSupplies(QPainter &painter)
{
    for (const AmmoBox &Box:ammoBoxes) {
        if(Box.lifeTimer<300 && (Box.lifeTimer/6)%2==0){
            continue;
        }
        painter.setBrush(QColor(180,140,60));
        painter.setPen(Qt::white);
        painter.drawRoundedRect(Box.rect,6,6);

        QFont boxFont;
        boxFont.setPixelSize(20);
        boxFont.setBold(true);
        boxFont.setFamily("微软雅黑");
        painter.setFont(boxFont);
        painter.setPen(Qt::white);
        painter.drawText(Box.rect,Qt::AlignCenter,"A");
    }

    for(const HealthPack &pack:healthPacks) {
        if(pack.lifeTimer<300 && (pack.lifeTimer/6)%2==0){
            continue;
        }
        painter.setBrush(Qt::white);
        painter.setPen(QPen(QColor(200,0,0),2));
        painter.drawRoundedRect(pack.rect,6,6);

        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);

        int cx=pack.rect.center().x();
        int cy=pack.rect.center().y();
        int armW=14;;
        int armT=4;

        painter.drawRect(cx-armW/2,cy-armT/2,armW,armT);
        painter.drawRect(cx-armT/2,cy-armW/2,armT,armW);
    }
}

void GameWidget::drawBoss(QPainter &painter)
{
    if(bossActivive){
        bool visible=true;
        if(bossInvincible){
            visible=(bossInvincibleTimer/6)%2==0
;        }
        if(visible){
            painter.setBrush(QColor(180,80,130));
            painter.setPen(QPen(Qt::white,3));
            painter.drawEllipse(bossRect);
        }
    }
    if(bossActivive){
        const int bw=600,bh=30;
        const int bx=(width()-bw)/2;
        const int by=20;

        painter.setBrush(QColor(40,40,40));
        painter.setPen(Qt::NoPen);
        painter.drawRect(bx,by+25,bw,bh);

        double whiteRatio=bossWhiteHp/bossMaxHp;
        if(whiteRatio>1.0)whiteRatio=1.0;
        int whiteW=static_cast<int>(bw*whiteRatio);
        painter.setBrush(QColor(250,250,250,190));              //boss血条
        painter.drawRect(bx,by+25,whiteW,bh);

        double ratio=static_cast<double>(bossHp)/bossMaxHp;
        int fillW=static_cast<int>(bw*ratio);
        painter.setBrush(QColor(180,80,130));
        painter.drawRect(bx,by+25,fillW,bh);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(Qt::white);
        painter.drawRect(bx,by+25,bw,bh);

        QFont bossFont;
        bossFont.setFamily("微软雅黑");
        bossFont.setPixelSize(18);
        bossFont.setBold(true);
        painter.setFont(bossFont);
        painter.setPen(Qt::white);
        painter.drawText(QRect(bx,by+25,bw,bh),Qt::AlignCenter,QString("BOSS %1/%2").arg(bossHp).arg(bossMaxHp));
    }
}

void GameWidget::drawBossWarning(QPainter &painter)
{
    if(roundState !=2 )return;
        bool flash=(bossWarnTime/15)%2==0;
        if(flash){
            painter.fillRect(rect(),QColor(120,0,0,80));
        }
        QFont warnFont;
        warnFont.setFamily("微软雅黑");
        warnFont.setPixelSize(80);
        warnFont.setBold(true);
        painter.setFont(warnFont);
        painter.setPen(Qt::red);
        painter.drawText(rect(),Qt::AlignCenter,"/// /// WARNING /// ///");

}

void GameWidget::drawGameOver(QPainter &painter)
{
    qint64 ms=0;
    if(state == GameState::GameOver){
        ms = frozenElapsed;
    }else{
        ms = survivalTimer.elapsed()-totalPaused;
        if(state == GameState::Paused){
            ms = pausedAt-totalPaused;
        }
    }

    QFont infofont;
    infofont.setPixelSize(36);
    infofont.setBold(true);
    infofont.setFamily("微软雅黑");
    painter.setFont(infofont);
    painter.setPen(Qt::white);

    if(state==GameState::GameOver){
        QFont font;
        font.setPointSize(28);
        font.setFamily("微软雅黑");
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(Qt::gray);                                           //结束页面
        painter.drawText(rect(),Qt::AlignCenter,QString("Game  Over \n "
                                                        "Score:%1 \n "
                                                        "Time:%2 \n"
                                                        "Press R to Restart \n "
                                                        "or Press Esc to Return Menu").arg(score).arg(frozenElapsed/1000));
}
}

void GameWidget::drawPauseOverlay(QPainter &painter)
{
    if(state == GameState::Paused){
        painter.fillRect(rect(),QColor(0,0,0,180));
        QFont titleFont;
        titleFont.setPixelSize(64);
        titleFont.setBold(true);
        titleFont.setFamily("微软雅黑");
        painter.setFont(titleFont);
        painter.setPen(Qt::gray);
        painter.drawText(rect().adjusted(0,-200,0,0),Qt::AlignCenter,"已暂停");

        painter.setBrush(Qt::white);
        painter.setPen(Qt::gray);
        painter.drawRoundedRect(resumeButtom,15,15);

        QFont btnfont;
        btnfont.setPixelSize(28);
        btnfont.setBold(true);
        btnfont.setFamily("微软雅黑");
        painter.setFont(btnfont);
        painter.setPen(Qt::black);
        painter.drawText(resumeButtom,Qt::AlignCenter,"继续游戏");

        painter.setBrush(Qt::darkRed);
        painter.drawRoundedRect(quitButtom,15,15);
        painter.setPen(Qt::white);
        painter.drawText(quitButtom,Qt::AlignCenter,"退出到主菜单");
        return;
    }
}
