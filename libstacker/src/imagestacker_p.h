#ifndef IMAGESTACKER_P_H
#define IMAGESTACKER_P_H

#include "libstacker/imagestacker.h"

namespace openskystacker {

class ImageStackerPrivate
{
    Q_DISABLE_COPY(ImageStackerPrivate)
    Q_DECLARE_PUBLIC(ImageStacker)
    ImageStacker * const q_ptr;

public:
    ImageStackerPrivate(ImageStacker *parent = 0);

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

    QStringList GetBiasFrameFileNames() const;
    void SetBiasFrameFileNames(const QStringList &value);

    bool GetUseBias() const;
    void SetUseBias(bool value);

//public slots:
    //! The main method for processing the images.
    void Process(int tolerance, int threads);

    //! Gets an image from disk and asynchronously reads it as a QImage.
    /*! Emits QImageReady once the image is ready.
        @param filename The image file to read.
    */
    void ReadQImage(QString filename);

    void detectStars(QString filename, int threshold);

private:
    int GetTotalOperations();

    int ValidateImageSizes();

    mutable QMutex mutex_;

    int current_operation_;
    int total_operations_;

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

}; // class ImageStackerPrivate

} // namespace openskystacker

#endif