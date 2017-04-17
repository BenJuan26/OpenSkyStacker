#ifndef IMAGETABLEMODEL_H
#define IMAGETABLEMODEL_H

#include <QAbstractTableModel>
#include "imagerecord.h"


class ImageTableModel : public QAbstractTableModel
{
public:
    ImageTableModel(QObject *parent = {});
    int rowCount(const QModelIndex &parent) const override;
    int rowCount();
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void append(ImageRecord *record);
    ImageRecord* at(int i);
    void removeAt(int i);
private:
    QList<ImageRecord*> list;
};

#endif // IMAGETABLEMODEL_H
