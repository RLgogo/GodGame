#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QFileInfo>
#include <QString>
#include <QStringList>

#include "include/render/Menu.h"
#include "include/utils/simpleIni/config.h"

namespace {

// cfg.ini 可能在三个地方：当前工作目录、可执行文件旁边（打包后）、工程根目录（开发时）
QString findIni() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates{
        QStringLiteral("cfg.ini"),
        appDir + QStringLiteral("/cfg.ini"),
        appDir + QStringLiteral("/../cfg.ini"),
        appDir + QStringLiteral("/../../cfg.ini"),
    };

    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return QStringLiteral("cfg.ini");
}

}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 读不到就用 Config 里那套默认值接着跑，不特殊处理
    const QByteArray iniPath = findIni().toUtf8();
    config().Load(iniPath.constData());

    Menu w;
    w.show();
    return a.exec();
}
