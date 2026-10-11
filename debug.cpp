#include <gamewidget.h>
#include <QStringList>
#include <QDebug>
#include <QKeyEvent>

void GameWidget::handleConsoleCommand(const QString &cmd)
{
    QString text = cmd.trimmed();
    if(text.isEmpty())return;

    if(text.startsWith("/"))text=text.mid(1);

    QStringList parts=text.split(' ',QString::SkipEmptyParts);
    if(parts.isEmpty())return;

    QString name=parts[0].toLower();

    if(name=="crazyyy"){
        invincibleTimer=12000;
    }
    else if(name=="boom"){
        for(Enemy &e:enemies){
            if(e.alive){
                e.alive=false;
                e.respawnTimer=-1;
                score++;
                expValue+=10;
            }
        }
    }else if(name=="help"){
        qDebug()<<"可用指令";
        qDebug()<<"crazyyy : 无敌时间200s";
        qDebug()<<"boom : 清屏敌人";
        qDebug()<<"可用指令";
        qDebug()<<"可用指令";
        qDebug()<<"可用指令";
    }
}
