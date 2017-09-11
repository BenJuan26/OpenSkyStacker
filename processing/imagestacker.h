#ifndef IMAGESTACKER_H
#define IMAGESTACKER_H

#include <QObject>
#include <opencv2/core/core.hpp>
#include <QMutex>
#include "processing/stardetector.h"
#include "processing/focas.h"
#include "model/imagerecord.h"
#include <QImage>

//! The main class for handling the image processing.
class ImageStacker : public QObject
{
    Q_OBJECT

#ifdef TEST_OSS
    friend class TestOSS;
#endif
public:
    //! Constructor for ImageStacker.
    explicit ImageStacker(QObject *parent = 0);

    //! Flag set to asynchronously cancel processing.
    bool cancel_;

    //! The extensions that the app will treat as RAW images.
    static const std::vector<QString> RAW_EXTENSIONS;

    //! Used to define the number of bits of the final image.
    enum BitsPerChannel {
        BITS_16, /*!< 16-bit image. */
        BITS_32  /*!< 32-bit image. */
    };

    //! Constructs an ImageRecord from the given file.
    /*!
        @param filename The image file to get the record from.
    */
    ImageRecord* GetImageRecord(QString filename);

    //! Gets an OpenCV Mat from the image at the specified filename.
    /*!
        @param filename The image file to read.
    */
    cv::Mat ReadImage(QString filename);

    // get/set
    QString GetRefImageFileName() const;
    void SetRefImageFileName(const QString &value);

    QStringList GetTargetImageFileNames() const;
    void SetTargetImageFileNames(const QStringList &value);

    QStringList GetDarkFrameFileNames() const;
    void SetDarkFrameFileNames(const QStringList &value);

    QStringList GetDarkFlatFrameFileNames() const;
    void SetDarkFlatFrameFileNames(const QStringList &value);

    QStringList GetFlatFrameFileNames() const;
    void SetFlatFrameFileNames(const QStringList &value);

    QString GetSaveFilePath() const;
    void SetSaveFilePath(const QString &value);

    cv::Mat GetWorkingImage() const;
    void SetWorkingImage(const cv::Mat &value);

    cv::Mat GetRefImage() const;
    void SetRefImage(const cv::Mat &value);

    cv::Mat GetFinalImage() const;
    void SetFinalImage(const cv::Mat &value);

    bool GetUseDarks() const;
    void SetUseDarks(bool value);

    bool GetUseDarkFlats() const;
    void SetUseDarkFlats(bool value);

    bool GetUseFlats() const;
    void SetUseFlats(bool value);

    BitsPerChannel GetBitsPerChannel() const;
    void SetBitsPerChannel(const BitsPerChannel &value);

    QStringList GetBiasFrameFileNames() const;
    void SetBiasFrameFileNames(const QStringList &value);

    bool GetUseBias() const;
    void SetUseBias(bool value);

signals:
    //! Provides the final image when the processing is finished.
    /*!
        @param image The final processed image.
    */
    void Finished(cv::Mat image);

    //! Provides a message when processing is finished, and marks the processing as complete.
    /*!
        @param message The message to accompany the process completion.
    */
    void FinishedDialog(QString message);

    //! Provides a percentage of completion and a description of what's happening.
    /*!
        @param message A description of the current section of processing.
        @param percentComplete An integer, out of 100, representing the percentage of completion.
    */
    void UpdateProgress(QString message, int percentComplete);

    //! Emits a QImage after asynchronously reading it.
    void QImageReady(QImage image);

    //! Emitted when an error occurs during processing.
    void ProcessingError(QString message);
public slots:
    //! The main method for processing the images.
    void Process();

    //! Gets an image from disk and asynchronously reads it as a QImage.
    /*! Emits QImageReady once the image is ready.
        @param filename The image file to read.
    */
    void ReadQImage(QString filename);

private:
    cv::Mat GenerateAlignedImage(cv::Mat ref, cv::Mat target, int *ok = 0);
    cv::Mat AverageImages(cv::Mat img1, cv::Mat img2);

    int ValidateImageSizes();

    QImage Mat2QImage(const cv::Mat &src);

    cv::Mat ConvertAndScaleImage(cv::Mat image);
    cv::Mat RawToMat(QString filename);
    cv::Mat GenerateAlignedImageOld(cv::Mat ref, cv::Mat target);

    void StackDarks();
    void StackDarkFlats();
    void StackFlats();
    void StackBias();

    mutable QMutex mutex_;

    int current_operation_;
    int total_operations_;
    BitsPerChannel bits_per_channel_;

    bool use_darks_ = false;
    bool use_dark_flats_ = false;
    bool use_flats_ = false;
    bool use_bias_ = false;

    QString ref_image_file_name_;
    QStringList target_image_file_names_;
    QStringList dark_frame_file_names_;
    QStringList dark_flat_frame_file_names_;
    QStringList flat_frame_file_names_;
    QStringList bias_frame_file_names_;

    QString save_file_path_;

    cv::Mat working_image_;
    cv::Mat ref_image_;
    cv::Mat final_image_;

    cv::Mat master_dark_;
    cv::Mat master_dark_flat_;
    cv::Mat master_flat_;
    cv::Mat master_bias_;
};

#endif // IMAGESTACKER_H
