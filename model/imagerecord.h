#ifndef IMAGERECORD_H
#define IMAGERECORD_H

#include <QString>
#include <ctime>

//! Contains metadata regarding an image.
class ImageRecord
{
public:
    //! Constructor.
    ImageRecord();

    //! Describes the type of frame (e.g. Light, Dark, etc.).
    enum FRAME_TYPE {
        LIGHT,     /*!< Light frame. */
        DARK,      /*!< Dark frame. */
        DARK_FLAT, /*!< Dark flat frame. */
        FLAT,      /*!< Flat frame. */
        BIAS       /*!< Bias/offset frame. */
    };

    QString getFilename() const;
    void setFilename(const QString &value);

    FRAME_TYPE getType() const;
    void setType(const FRAME_TYPE &value);

    float getShutter() const;
    void setShutter(float value);

    float getIso() const;
    void setIso(float value);

    bool isReference() const;
    void setReference(bool value);

    time_t getTimestamp() const;
    void setTimestamp(const time_t &value);

    bool isChecked() const;
    void setChecked(bool value);

    int getWidth() const;
    void setWidth(int value);

    int getHeight() const;
    void setHeight(int value);

private:
    QString filename;
    FRAME_TYPE type;
    float shutter;
    float iso;
    time_t timestamp;
    bool reference;
    bool checked;
    int width;
    int height;
};

#endif // IMAGERECORD_H
