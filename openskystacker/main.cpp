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

    switch (type) {
    case QtDebugMsg:
        logger->debug(msg);
        break;
    case QtInfoMsg:
        logger->info(msg);
        break;
    case QtWarningMsg:
        logger->warn(msg);
        break;
    case QtCriticalMsg:
        logger->error(msg);
        break;
    case QtFatalMsg:
        logger->critical(msg);
        QApplication::exit(1);
        break;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

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
    printf("logDir: %s\n", logDir.c_str());
#endif

#ifndef QT_NO_DEBUG
    // Only show Debug-level messages when built in debug mode
    spdlog::set_level(spdlog::level::trace);
#else
    spdlog::set_level(spdlog::level::debug);
#endif

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    try {
        logger = spdlog::rotating_logger_mt("logger", logDir + "/log.txt", 1048576, 2);
    } catch (std::exception) {
        printf("Couldn't create logger\n");
    }

    if (!logger) {
        printf("Couldn't create logger\n");
    }
    qInstallMessageHandler(messageHandler);

//    QTranslator translator;
//    if (!translator.load("openskystacker_es", QCoreApplication::applicationDirPath())) {
//        qDebug() << "translate load failed";
//    }
//    a.installTranslator(&translator);

    MainWindow w;
    w.show();

    return a.exec();
}
