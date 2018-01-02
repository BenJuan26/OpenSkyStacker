#ifndef IMAGERECORD_H
#define IMAGERECORD_H

#include "libstacker/libstacker_global.h"

#include <QString>
#include <ctime>

namespace openskystacker {

//! Contains metadata regarding an image.
class LIBSTACKER_EXPORT ImageRecord
{
public:
    //! Constructor.
    ImageRecord();

    //! Describes the type of frame (e.g. Light, Dark, etc.).
    enum FrameType {
        LIGHT,     /*!< Light frame. */
        DARK,      /*!< Dark frame. */
        DARK_FLAT, /*!< Dark flat frame. */
        FLAT,      /*!< Flat frame. */
        BIAS       /*!< Bias/offset frame. */
    };

    QString GetFilename() const;
    void SetFilename(const QString &value);

    FrameType GetType() const;
    void SetType(const FrameType &value);

    float GetShutter() const;
    void SetShutter(float value);

    float GetIso() const;
    void SetIso(float value);

    bool IsReference() const;
    void SetReference(bool value);

    time_t GetTimestamp() const;
    void SetTimestamp(const time_t &value);

    bool IsChecked() const;
    void SetChecked(bool value);

    int GetWidth() const;
    void SetWidth(int value);

    int GetHeight() const;
    void SetHeight(int value);

private:
    QString filename_;
    FrameType type_;
    float shutter_;
    float iso_;
    time_t timestamp_;
    bool reference_;
    bool checked_;
    int width_;
    int height_;
};

}

#endif // IMAGERECORD_H
