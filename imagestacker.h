#ifndef IMAGESTACKER_H
#define IMAGESTACKER_H

#include <QObject>
#include <opencv2/core.hpp>
#include <QMutex>

class ImageStacker : public QObject
{
    Q_OBJECT
public:
    explicit ImageStacker(QObject *parent = 0);
    bool cancel;

    std::vector<QString> RAW_EXTENSIONS = {"3fr", "ari", "arw", "bay", "crw", "cr2",
            "cap", "data", "dcs", "dcr", "dng", "drf", "eip", "erf", "fff", "gpr",
            "iiq", "k25", "kdc", "mdc", "mef", "mos", "mrw", "nef", "nrw", "obm",
            "orf", "pef", "ptx", "pxn", "r3d", "raf", "raw", "rwl", "rw2", "rwz",
            "sr2", "srf", "srw", "x3f"};

    // get/set
    QString getRefImageFileName() const;
    void setRefImageFileName(const QString &value);

    QStringList getTargetImageFileNames() const;
    void setTargetImageFileNames(const QStringList &value);

    QStringList getDarkFrameFileNames() const;
    void setDarkFrameFileNames(const QStringList &value);

    QStringList getDarkFlatFrameFileNames() const;
    void setDarkFlatFrameFileNames(const QStringList &value);

    QStringList getFlatFrameFileNames() const;
    void setFlatFrameFileNames(const QStringList &value);

    QString getSaveFilePath() const;
    void setSaveFilePath(const QString &value);

    cv::Mat getWorkingImage() const;
    void setWorkingImage(const cv::Mat &value);

    cv::Mat getRefImage() const;
    void setRefImage(const cv::Mat &value);

    cv::Mat getFinalImage() const;
    void setFinalImage(const cv::Mat &value);

    bool getUseDarks() const;
    void setUseDarks(bool value);

    bool getUseDarkFlats() const;
    void setUseDarkFlats(bool value);

    bool getUseFlats() const;
    void setUseFlats(bool value);

signals:
    void finished(cv::Mat image);
    void finishedDialog(QString message);
    void updateProgress(QString message, int percentComplete);
public slots:
    void process();

private:
    cv::Mat generateAlignedImage(cv::Mat ref, cv::Mat target);
    cv::Mat averageImages16UC3(cv::Mat img1, cv::Mat img2);

    void stackDarks();
    void stackDarkFlats();
    void stackFlats();

    mutable QMutex mutex;

    int currentOperation;
    int totalOperations;

    bool useDarks = false;
    bool useDarkFlats = false;
    bool useFlats = false;

    QString refImageFileName;
    QStringList targetImageFileNames;
    QStringList darkFrameFileNames;
    QStringList darkFlatFrameFileNames;
    QStringList flatFrameFileNames;

    QString saveFilePath;

    cv::Mat workingImage;
    cv::Mat refImage;
    cv::Mat finalImage;

    cv::Mat masterDark;
    cv::Mat masterDarkFlat;
    cv::Mat masterFlat;

    cv::Mat convertAndScaleTo16UC3(cv::Mat image);
    cv::Mat rawTo16UC3(QString filename);

    cv::Mat readImage16UC3(QString filename);

};

#endif // IMAGESTACKER_H
