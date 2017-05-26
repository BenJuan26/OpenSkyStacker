#ifndef IMAGERECORD_H
#define IMAGERECORD_H

#include <QString>
#include <ctime>

class ImageRecord
{
public:

    ImageRecord();

    enum FRAME_TYPE{LIGHT,DARK,DARK_FLAT,FLAT,BIAS};

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
