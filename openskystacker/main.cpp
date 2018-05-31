#include "ui/mainwindow.h"
#include <QApplication>
#include <QTranslator>

#include "spdlog/spdlog.h"

// cv::Mat can't be passed through a signal without this declaration
Q_DECLARE_METATYPE(cv::Mat)

std::shared_ptr<spdlog::logger> logger;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &qMsg) {
    if (!logger) {
        return;
    }

    const char *msg = qMsg.toUtf8().constData();

    // Since spdlog has no notion of "fatal", all levels were bumped down by
    //  one and fatal was added at the end. This is defined in spdlog/tweakme.h.
    switch (type) {
    case QtDebugMsg:
        logger->trace(msg);
        break;
    case QtInfoMsg:
        logger->debug(msg);
        break;
    case QtWarningMsg:
        logger->info(msg);
        break;
    case QtCriticalMsg:
        logger->warn(msg);
        break;
    case QtFatalMsg:
        logger->critical(msg);
        QApplication::exit(1);
        break;
    }
}

int main(int argc, char *argv[])
{

#ifdef Q_OS_UNIX
    // On UNIX platforms, put the logs in ~/.openskystacker
    QString qLogDir = QDir::homePath() + "/.openskystacker";
    QDir dir(qLogDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    std::string logDir = qLogDir.toUtf8().constData();
#else
    // on Windows, put the logs beside the application
    std::string logDir = QApplication::applicationDirPath().toUtf8().constData();
#endif

#ifndef QT_NO_DEBUG
    // Only show Debug-level messages when built in debug mode
    spdlog::set_level(spdlog::level::trace);
#else
    spdlog::set_level(spdlog::level::debug);
#endif

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    logger = spdlog::rotating_logger_mt("logger", logDir + "/log.txt", 1048576, 2);
    qInstallMessageHandler(messageHandler);

    QApplication a(argc, argv);

//    QTranslator translator;
//    if (!translator.load("openskystacker_es", QCoreApplication::applicationDirPath())) {
//        qDebug() << "translate load failed";
//    }
//    a.installTranslator(&translator);

    MainWindow w;
    w.show();

    return a.exec();
}
