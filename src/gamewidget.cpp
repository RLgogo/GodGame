#include <QPainter>
#include <QKeyEvent>
#include <QTimer>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <QColor>
#include <QPointF>
#include <QDebug>

#include "include/gamewidget.h"

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(1000, 800);
    resize(1400,1000);
    setWindowTitle("Defencer");
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    updateButtonPosition();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        updategame();
        update();
    });
    std::srand(std::time(nullptr));
    timer->start(16);
}

void GameWidget::paintEvent(QPaintEvent *event)         //绘画事件
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if(state==GameState::Menu){drawMenu(painter);return;;}
    if(state==GameState::Guide){drawGuide(painter);return;}
    if(state==GameState::About){drawAbout(painter);return;}

    drawBossWarning(painter);

    drawPlayer(painter);
    drawBullets(painter);
    drawBoss(painter);
    drawEnemies(painter);
    drawSupplies(painter);
    drawHUD(painter);
    drawLevelUp(painter);

    if(state==GameState::Console){
        drawConsole(painter);
    }

    if(state==GameState::GameOver)drawGameOver(painter);
    if(state==GameState::Paused)drawPauseOverlay(painter);
}

void GameWidget::keyPressEvent(QKeyEvent *event)            //键盘点击
{                                   //控制台按键
    if(state==GameState::Console){
        if(event->key()==Qt::Key_Return||event->key()==Qt::Key_Enter){
            handleConsoleCommand(consoleInput);
            consoleInput.clear();
        }
        else if(event->key()==Qt::Key_Backspace){
            if(!consoleInput.isEmpty())consoleInput.chop(1);
        }
        else if(event->key()==Qt::Key_Escape||event->key()==Qt::Key_QuoteLeft){
            totalPaused+=survivalTimer.elapsed()-consolePausedAt;
            state=GameState::Playing;
            consoleInput.clear();
        }
        else if(!event->text().isEmpty()){
            consoleInput+=event->text();
        }
        return;
    }
    if(state==GameState::Playing && event->key()==Qt::Key_QuoteLeft){
        state=GameState::Console;
        consolePausedAt=survivalTimer.elapsed();
        consoleInput.clear();
        keys.clear();
        mouseHeld=false;
        return;
    }

    if(state == GameState::Playing){
    keys.insert(event->key());
    }
    if(state == GameState::Playing && (event->key() == Qt::Key_Escape || event->key()==Qt::Key_P)){
        state = GameState::Paused;
        pausedAt=survivalTimer.elapsed();
        keys.clear();
        mouseHeld=false;
        return;
    }
    if(state == GameState::Paused &&(event->key()==Qt::Key_Escape || event->key()==Qt::Key_P)){
        totalPaused+=survivalTimer.elapsed()-pausedAt;
        state = GameState::Playing;
        return;
    }
    if(state == GameState::GameOver && event->key()==Qt::Key_R){
        resetGame();
    }
    if(state == GameState::GameOver && event->key()==Qt::Key_Escape){
        state=GameState::Menu;
    }
    if(state == GameState::About && event->key() == Qt::Key_Escape){
        state =GameState::Menu;
        return;
    }
    if(state == GameState::Guide && event->key()==Qt::Key_Escape){
        state=GameState::Menu;
        return;
    }
    return;
}


void GameWidget::keyReleaseEvent(QKeyEvent *event)
{
    keys.remove(event->key());
}

void GameWidget::mousePressEvent(QMouseEvent *event)            //鼠标点击
{
    if(state == GameState::Paused){
        if(resumeButtom.contains(event->pos())){
            totalPaused+=survivalTimer.elapsed()-pausedAt;
            state = GameState::Playing;
        }else if(quitButtom.contains(event->pos())){
            state=GameState::Menu;
            bullets.clear();
            keys.clear();
            enemies.clear();
            ammoBoxes.clear();
            healthPacks.clear();
            mouseHeld=false;   
        }
        return;
    }
    if(state == GameState::Menu){
        if(startButton.contains(event->pos())){
            resetGame();
        }else if(aboutButton.contains(event->pos())){
                state=GameState::About;
            }else if(guideButton.contains(event->pos())){
            state=GameState::Guide;
        }
       return;
    }
    if(state == GameState::About){
        if(backbutton.contains(event->pos())){
            state = GameState::Menu;
        }
        return;
    }
    if(state == GameState::Playing){
        if(event->button() == Qt::LeftButton)
            mouseHeld=true;
        mousePos=event->pos();
    }
    if(state==GameState::Guide){
        if(guideBackButton.contains(event->pos())){
            state=GameState::Menu;
        }
    }return;
}

void GameWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(state != GameState::Playing)
        return;
    if(event->button()==Qt::LeftButton){
        mouseHeld=false;
    }
}

void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(state == GameState::Playing){
         mousePos=event->pos();
    }
}

void GameWidget::fireBullet()                               //发射子弹
{
    if(ammo<=0)return;
    int dx=mousePos.x()-player.center().x();
    int dy=mousePos.y()-player.center().y();
    double dist = std::sqrt(dx*dx+dy*dy);
    if(dist == 0)return;
    Bullet b;
    b.rect=QRect(player.center().x()-4,player.center().y()-4,8,8);
    b.dx=static_cast<int>(dx/dist*bulletspeed);
    b.dy=static_cast<int>(dy/dist*bulletspeed);
    bullets.append(b);

    ammo--;
}
                            //升级事件
void GameWidget::levelUp()
{
    if(level<=100){
       if(level%5==0){
        maxStamina+=5;
        stamina=maxStamina;
        staminaRegen+=0.05;
        staminaDrain+=0.02;
        maxammo+=20;
        }
    }
    levelUpTimer=levelUpDuration;
}

void GameWidget::spawnEnemy()                               //敌人生成
{
    Enemy e;
    int x,y;
    e.rect=QRect(0,0,35,35);
    e.alive=true;
    e.respawnTimer=0;
    e.speed=1+std::rand()%4;

    e.isDasher=(std::rand()%100<10);
    e.dashTimer=300;

    const int minDist=300;
    bool placed=false;
    for(int tries=0;tries<50;++tries){
        x=std::rand()%(width()-e.rect.width());
        y=std::rand()%(height()-e.rect.height());

        int ex=x+e.rect.width()/2;
        int ey=y+e.rect.height()/2;
        int px=player.center().x();
        int py=player.center().y();

        int dx=ex-px;
        int dy=ey-py;
        double dist=std::sqrt(dx*dx+dy*dy);

        if(dist>=minDist){
            e.rect.moveTo(x,y);
            e.precisePos=QPointF(x,y);
            placed=true;
            break;
        }
    }
    if(!placed){
        e.rect.moveTo(0,0);
        e.precisePos=QPointF(0,0);
    }
    enemies.append(e);
}

void GameWidget::spawnBoss()            //生成Boss
{
    const int bw=100,bh=100;
    QRect bestRect(0,0,bw,bh);
    double bestDist = -1;
    bool placed=false;;

    for(int tries=0;tries<100;++tries){
        int x=std::rand()%(width()-bw);
        int y=std::rand()%(height()-bh);
        QRect candidate(x,y,bw,bh);

        int dx=candidate.center().x()-player.center().x();
        int dy=candidate.center().y()-player.center().y();
        double d=std::sqrt(dx*dx+dy*dy);

        if(d>bestDist){
            bestDist=d;
            bestRect=candidate;
        }
        if(d>=650){
            bossRect=candidate;
            placed=true;
            break;
        }

         bossInvincible=false;
         bossInvincibleTimer=0;
         bossPhase2Triggered=false;
         bossActivive=true;
         bossPhase=2;
    }
    if(!placed)bossRect=bestRect;

    bossPreccisePos=QPointF(bossRect.x(),bossRect.y());
    bossMaxHp+=10;
    bossHp=bossMaxHp;
    bossWhiteHp=bossMaxHp;
    bossWhiteDelay=0;
    bossInvincible=false;
    bossInvincibleTimer=0;
    bossPhase2Triggered=false;
    bossActivive=true;
    bossPhase=2;
}

void GameWidget::resetGame()                            //重置游戏
{
    score=0;
    hp=100;
    invincibleTimer=0;
    state=GameState::Playing;

    round=1;
    roundState=1;
    roundWaitTime=300;
    enemyToSpwan=150;
    spawnCoolDown=0.0;
    bossSpawnedThisRound=false;
    bossWarnTime=0;

    level=1;
    expValue=0;
    expToNext=100;

    stamina=100.0;
    maxStamina=100.0;
    staminaDrain=0.8;
    staminaRegen=0.4;

    survivalTimer.start();
    frozenElapsed=0;
    pausedAt=0;
    totalPaused=0;

    player.moveTo(450,350);

    enemies.clear();

    bullets.clear();
    keys.clear();
    mouseHeld=false;
    shootCooldown=0;

    stamina=maxStamina;
    exhausted=false;

    ammo=60;
    ammoBoxes.clear();
    healthPacks.clear();

    bossMaxHp=20;
    bossWhiteHp=bossMaxHp;
    bossWhiteDelay=0;
    bossActivive=false;
    bossPhase=0;
    bossCoolDown=1800;
    bossInvincible=false;
    bossInvincibleTimer=0;
    bossPhase2Triggered=false;
    bossHp=bossMaxHp;

    aliveEnemies=0;
    enemyAtLeast=15;
}

void GameWidget::resizeEvent(QResizeEvent *event){
    QWidget::resizeEvent(event);
    updateButtonPosition();
}

void GameWidget::updateButtonPosition(){
    const int btnW=220;
    const int btnH=80;

    startButton=QRect((width()-btnW)/2,height()/2-30,btnW,btnH);

    const int aboutW=200;
    const int aboutH=70;
    aboutButton=QRect((width()-aboutW)/2,startButton.bottom()+20,aboutW,aboutH);

    guideButton=QRect((width()-aboutW)/2,aboutButton.bottom()+20,aboutW,aboutH);

    const int pauseW=200;
    const int pauseH=65;
    resumeButtom=QRect((width()-pauseW)/2,height()/2-50,pauseW,pauseH);

    quitButtom=QRect((width()-pauseW)/2,height()/2+20,pauseW,pauseH);

    const int backW=200;
    const int backH=60;
    backbutton=QRect((width()-backW)/2,height()-120,backW,backH);

    guideBackButton=backbutton;
}

void GameWidget::updategame()                   //更新游戏
{
    if(state != GameState::Playing){
        return;
    }

    if(levelUpTimer>0){
        levelUpTimer--;
    }

    bool wantSprint=keys.contains(Qt::Key_Shift);   // 疲劳锁
    if(stamina<=0){
        exhausted=true;
    }
    if(exhausted && stamina>=maxStamina*0.15){
        exhausted=false;
    }

    bool sprinting = wantSprint && !exhausted && stamina > 0;       //体力条刷新
    int currentSpeed=baseSpeed;
    if(sprinting){
        currentSpeed = sprintSpeed;
        stamina -= staminaDrain;
        if(stamina<0){
            stamina=0;
          }
    }else{
        stamina += staminaRegen;
        if(stamina>maxStamina)stamina=maxStamina;
    }

    if (keys.contains(Qt::Key_A))player.moveLeft(player.left() - currentSpeed);
    if (keys.contains(Qt::Key_D))player.moveLeft(player.left() + currentSpeed);
    if (keys.contains(Qt::Key_W))player.moveTop(player.top() - currentSpeed);
    if (keys.contains(Qt::Key_S))player.moveTop(player.top() + currentSpeed);

    if (player.left() < 0)          player.moveLeft(0);
    if (player.right() >= width())  player.moveRight(width() - 1);
    if (player.top() < 0)           player.moveTop(0);
    if (player.bottom() >= height())player.moveBottom(height() - 1);
                                            //Boss生成位置
    if(bossActivive){
          if(bossInvincible){
              bossInvincibleTimer--;
              if(bossInvincibleTimer <= 0){
                  bossInvincible=false;
              }
          }else{
              int dx=player.center().x()-bossRect.center().x();
              int dy=player.center().y()-bossRect.center().y();
              double dist =std::sqrt(dx*dx+dy*dy);
              if(dist>0){
                  double mvX=dx/dist*2;
                  double mvY=dy/dist*2;
                  bossPreccisePos.setX(bossPreccisePos.x()+mvX);
                  bossPreccisePos.setY(bossPreccisePos.y()+mvY);
                  bossRect.moveTo(qRound(bossPreccisePos.x()),qRound(bossPreccisePos.y()));
              }
          }
          if(invincibleTimer==0 && player.intersects(bossRect)){
              hp-=60;
              invincibleTimer=invincibleDuration;
              if(hp<=0){
                  hp=0;
                  state=GameState::GameOver;
                  frozenElapsed=survivalTimer.elapsed()-totalPaused;
                  return;
              }
          }
    }   
                                                             //回合状态
    if(roundState==0){
        roundWaitTime--;
        if(roundWaitTime<=0){
            roundState=1;
            enemyToSpwan=round*150;
            spawnCoolDown=0.0;
            spawnInterval=computeSpawnInterval(round);
            bossSpawnedThisRound=false; 
        }
    }
    else if(roundState==1){
        int aliveEnemies=0;
        for (const Enemy &e :enemies) {
            if(e.alive)aliveEnemies++;
        }
        if(enemyToSpwan>0){
            spawnCoolDown+=1.0;
            if(spawnCoolDown>=spawnInterval){
                spawnEnemy();
                enemyToSpwan--;
                spawnCoolDown=0.0;
            }
            if(aliveEnemies < enemyAtLeast && enemyToSpwan>0){
                spawnEnemy();
                enemyToSpwan--;
            }
        }
            if(enemyToSpwan<=50 && bossWarnTime == 0 && !bossSpawnedThisRound){
                bossWarnTime=600;
            }
            if(enemyToSpwan<=0){
                roundState=2;
                spawnCoolDown=0.0;
            }
        }
     else if(roundState==2){
        bossWarnTime--;
        if(bossWarnTime<=0){
            spawnBoss();
            bossSpawnedThisRound=true;
            roundState=3;
        }
    }
    else if(roundState==3){
        if(bossActivive){                                            //boss阶段生成敌人
          spawnCoolDown+=1.0;
          if(spawnCoolDown>=spawnInterval){
               spawnEnemy();
               spawnCoolDown=0.0;
        }
      }

     bool anyAlive=false;
        for(const Enemy &e : enemies){
            if(e.alive){
                anyAlive=true;
                break;
            }
        }
            if(!anyAlive && !bossActivive){
                roundState=4;
                roundWaitTime=300;
            }
        }else if(roundState==4){
        roundWaitTime--;
        if(roundWaitTime<=0){
            round++;
            roundState=0;
            roundWaitTime=300;
            enemies.clear();
            enemyToSpwan=0;
            bossActivive=false;
        }
    }                                
                                                                        //敌人生成位置
    for (Enemy &e:enemies) {
        if(!e.alive){
            if(e.respawnTimer==-1){
                continue;
            }
            if(e.respawnTimer>0){
                e.respawnTimer--;
            }else{
                const int minDist=300;
                bool placed=false;
                for (int tries=0;tries<50;++tries) {
                    int x=std::rand()%(width()-e.rect.width());
                    int y=std::rand()%(height()-e.rect.height());

                    int ex=x+e.rect.width()/2;
                    int ey=y+e.rect.height()/2;
                    int dx2=ex-player.center().x();
                    int dy2=ey-player.center().y();
                    double d=std::sqrt(dx2*dx2+dy2*dy2);

                    if(d>=minDist){
                        e.rect.moveTo(x,y);
                        e.precisePos=QPointF(x,y);
                        placed=true;
                        break;
                    }
                }
                if(!placed){
                    e.rect.moveTo(0,0);
                    e.precisePos=QPointF(0,0);
                }
                e.alive=true;
                e.isDasher=(std::rand()%100<10);
                e.dashTimer=300;

                int r=std::rand()%10;
                if(r<6)e.speed=1;
                else if(r<9)e.speed=2;
                else e.speed=4;
            }
            continue;
        }

    int dx = player.center().x()-e.rect.center().x();
    int dy = player.center().y()-e.rect.center().y();
    double dist = std::sqrt(dx*dx+dy*dy);

    if(e.isDasher){
        e.dashTimer--;
        if(e.dashTimer<=0){
            if(dist>400){
                if(dist>0){
                    double stepX = (dx/dist*350);
                    double stepY = (dy/dist*350);
                    e.precisePos.setX(e.precisePos.x()+stepX);
                    e.precisePos.setY(e.precisePos.y()+stepY);
                    e.rect.moveTo(qRound(e.precisePos.x()),qRound(e.precisePos.y()));
                }
            }
            e.dashTimer=300;
        }
    }

    if(dist>0){
        double moveX = (dx/dist*e.speed);
        double moveY = (dy/dist*e.speed);
        e.precisePos.setX(e.precisePos.x()+moveX);
        e.precisePos.setY(e.precisePos.y()+moveY);
        e.rect.moveTo(qRound(e.precisePos.x()),qRound(e.precisePos.y()));
    }                                                                              //敌人追踪
}
    enemyStanding=enemyToSpwan;
    for(const Enemy &e:enemies){
        if(e.alive)enemyStanding++;
    }
    if(bossActivive)enemyStanding++;

    if(shootCooldown>0){
        shootCooldown--;
    }
    if(mouseHeld && shootCooldown <=0){
        fireBullet();
        shootCooldown=shootInterval;
    }
    if(bossActivive){
        if(bossWhiteDelay>0){
            bossWhiteDelay--;
        }else{
            if(bossWhiteHp>bossHp){
                double speed=bossMaxHp*0.01;
                if(speed<0.5)speed=0.5;
                bossWhiteHp-=speed;
                if(bossWhiteHp<bossHp){
                    bossWhiteHp=bossHp;
                }
            }
        }
    }
    for(int i=bullets.size()-1;i>=0;i--){
        bullets[i].rect.moveLeft(bullets[i].rect.left()+bullets[i].dx);
        bullets[i].rect.moveTop(bullets[i].rect.top()+bullets[i].dy);
        bool bulletHit=false;
                                        //boss状态
        if(bossActivive && !bossInvincible && bullets[i].rect.intersects(bossRect)){
            bullets.removeAt(i);
            bossHp--;
            bossWhiteDelay=30;

            if(bossHp<=bossMaxHp/2 && !bossPhase2Triggered){
                bossPhase2Triggered=true;
                bossInvincible=true;
                bossInvincibleTimer=300;
                                                //boss半血时生成10个高速敌人
                for (int k=0;k<10;++k) {
                    Enemy e;
                    int offsetX=(std::rand()%240)-120;
                    int offsetY=(std::rand()%240)-120;
                    int nx=qBound(0,bossRect.center().x()+offsetX,width()-35);
                    int ny=qBound(0,bossRect.center().y()+offsetY,height()-35);
                    e.rect=QRect(nx,ny,35,35);
                    e.precisePos=QPointF(nx,ny);
                    e.alive=true;
                    e.respawnTimer=0;
                    e.speed=5;
                    e.isDasher=false;
                    e.dashTimer=300;
                    enemies.append(e);
                }
            }

            if(bossHp<=0){
                bossActivive=false;
                bossWhiteHp=0.0;
                bossWhiteDelay=0;
                bossPhase=0;
                bossCoolDown=3600;
                score += 50;
                expValue+=300;

                AmmoBox box;
                box.rect=QRect(bossRect.center().x()-12,bossRect.center().y()-12,24,24);
                box.amount=20+std::rand()%16;
                ammoBoxes.append(box);

                HealthPack hpBox;
                hpBox.rect=QRect(bossRect.center().x()+6,bossRect.center().y()+6,24,24);
                healthPacks.append(hpBox);

                while (expValue>=expToNext) {
                    expValue-=expToNext;
                    level++;
                    expToNext+=50;
                    levelUp();
                }

                bossInvincible=false;
                bossInvincibleTimer=0;
                bossPhase2Triggered=false;
            }
            continue;
        }
                                                //子弹击中敌人
        for(Enemy &e : enemies){
            if(e.alive && bullets[i].rect.intersects(e.rect)){
                bullets.removeAt(i);
                e.alive=false;
                e.respawnTimer=-1;
                score++;
                expValue+=10;

                while (expValue>=expToNext) {
                    expValue-=expToNext;
                    level++;
                    expToNext+=50;
                    hp=maxhp;
                    ammo+=20;
                    levelUp();
                }
                                                        //掉落物概率
                if(std::rand()%100<9){
                    AmmoBox box;
                    box.rect=QRect(e.rect.center().x()-12,e.rect.center().y()-12,24,24);
                    box.amount=20+std::rand()%16;
                    ammoBoxes.append(box);
                }
                if(std::rand()%100<4){
                    HealthPack hpBox;
                    hpBox.rect=QRect(e.rect.center().x()-12,e.rect.center().y()-12,24,24);
                    healthPacks.append(hpBox);
                }
                bulletHit=true;
                break;
            }
        }
        if(bulletHit)continue;

        if(bullets[i].rect.right()<0
                ||bullets[i].rect.left()>width()
                ||bullets[i].rect.bottom()<0
                ||bullets[i].rect.top()>height()){
            bullets.removeAt(i);
        }       
    }
                    //弹药包
    for (int i =ammoBoxes.size()-1;i>=0;--i) {
        if(player.intersects(ammoBoxes[i].rect)){ammo+=ammoBoxes[i].amount;
        if(ammo>maxammo)ammo=maxammo;
        ammoBoxes.removeAt(i);
        }
    }
    for (int i=ammoBoxes.size()-1;i>=0;i--) {
        ammoBoxes[i].lifeTimer--;
        if(ammoBoxes[i].lifeTimer<=0){
            ammoBoxes.removeAt(i);
        }
    }
                    //血包
    for(int i=healthPacks.size()-1;i>=0;--i){
        if(player.intersects(healthPacks[i].rect)){
           hp+=healthPacks[i].heal;
           if(hp>maxhp)hp=maxhp;
           healthPacks.removeAt(i);
    }
  }
    for (int i=healthPacks.size()-1;i>=0;i--) {
        healthPacks[i].lifeTimer--;
        if(healthPacks[i].lifeTimer<=0){
            healthPacks.removeAt(i);
        }
    }

                                            //无敌时间
    if(invincibleTimer>0){
        invincibleTimer--;
    }
    if(invincibleTimer==0){
        for(Enemy &e : enemies){
        if(e.alive && player.intersects(e.rect)){
            hp-=20;
            e.alive=false;
            e.respawnTimer=-1;
            invincibleTimer=invincibleDuration;

            if(hp<=0){

                hp=0;
                state=GameState::GameOver;
                frozenElapsed = survivalTimer.elapsed()-totalPaused;
            }
            return;
       }
     }
   }
}
// A* 玩家找怪物
double GameWidget::computeSpawnInterval(int round)
{
    const double minInterval=3.0;
    const double maxInterval=22.0;
    const double mid=6.0;
    const double k=1.5;

    return minInterval+(maxInterval-minInterval)/(1.0+std::exp(round-mid)/k);
}

GameWidget::~GameWidget(){

}

