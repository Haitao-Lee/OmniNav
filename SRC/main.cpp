#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2)
VTK_MODULE_INIT(vtkInteractionStyle)
VTK_MODULE_INIT(vtkRenderingFreeType)
#include <qDebug>
#include <QApplication>
#include <QCoreApplication>
#include <QSurfaceFormat>
#include <QVTKOpenGLNativeWidget.h>
#include <QFont>
#include <QMap>
#include <QtGlobal>

#include "OmniNav.h"

// ---------------- message handler ----------------
void myMessageOutput(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    if (msg.contains("Unknown property nborder")) return;
    if (msg.contains("connectSlotsByName")) return;

    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stdout, "[Debug] %s\n", localMsg.constData());
        break;
    case QtInfoMsg:
        fprintf(stdout, "[Info] %s\n", localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "[Warning] %s\n", localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "[Critical] %s\n", localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "[Fatal] %s\n", localMsg.constData());
        abort();
    }
    fflush(stdout);
    fflush(stderr);
}

int main(int argc, char *argv[])
{
    // ================== ① OpenGL backend（必须最先） ==================
    // QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough
    );

    // ================== ② OpenGL SurfaceFormat（关键） ==================
    QSurfaceFormat fmt = QVTKOpenGLNativeWidget::defaultFormat();
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(3, 2);               // VTK 最稳
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    fmt.setSamples(0);                  // 先关 MSAA，排雷
    QSurfaceFormat::setDefaultFormat(fmt);

    // ================== ③ QApplication ==================
    QApplication app(argc, argv);

    // ================== ④ Qt / VTK 杂项 ==================
    qInstallMessageHandler(myMessageOutput);

    vtkObject::GlobalWarningDisplayOff();

    qRegisterMetaType<QMap<QString, QString>>("QMap<QString,QString>");
    qRegisterMetaType<QMap<int, QString>>("QMap<int,QString>");

    QFont font = app.font();
    font.setPointSize(12);
    app.setFont(font);

    // ================== ⑤ 主窗口 ==================
    OmniNav on;
    on.showMaximized();
    on.update();
    on.repaint();

    return app.exec();
}
