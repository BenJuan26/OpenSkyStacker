#include "imagetablemodel.h"
#include <QFileInfo>
#include <QColor>
#include <QBrush>

ImageTableModel::ImageTableModel(QObject *parent) : QAbstractTableModel{parent}
{

}

int ImageTableModel::rowCount(const QModelIndex &parent) const
{
    return list.count();
}

int ImageTableModel::columnCount(const QModelIndex &parent) const
{
    return 4;
}

QVariant ImageTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::BackgroundRole) {
        return QVariant(QBrush (QColor(Qt::white)));
    }
    if (role != Qt::DisplayRole && role != Qt::EditRole) return {};

    ImageRecord image = list[index.row()];

    switch (index.column()) {
    case 0: {
        // Strip off the path
        QFileInfo info(image.getFilename());
        return info.fileName();
    }
    case 1:
        switch(image.getType()) {
        case ImageRecord::LIGHT: default: return "Light";
        case ImageRecord::DARK: return "Dark";
        case ImageRecord::DARK_FLAT: return "Dark Flat";
        case ImageRecord::FLAT: return "Flat";
        case ImageRecord::BIAS: return "Bias";
        }
        break;
    case 2: return image.getShutter();
    case 3: return image.getIso();
    default: return {};
    }
}

QVariant ImageTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};

    switch(section) {
    case 0: return "Filename";
    case 1: return "Type";
    case 2: return "Shutter Speed";
    case 3: return "ISO";
    default: return {};
    }
}

void ImageTableModel::append(const ImageRecord &record)
{
    beginInsertRows({}, list.count(), list.count());
    list.append(record);
    endInsertRows();
}
