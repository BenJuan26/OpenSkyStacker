#include "imagetablemodel.h"
#include <QFileInfo>
#include <QColor>
#include <QBrush>
#include <QFont>

ImageTableModel::ImageTableModel(QObject *parent) : QAbstractTableModel{parent}
{

}

int ImageTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return list.count();
}

int ImageTableModel::rowCount()
{
    return list.count();
}

int ImageTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 6;
}

QVariant ImageTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::BackgroundRole) {
        return QVariant(QBrush (QColor(Qt::white)));
    }
    if (role == Qt::FontRole && list[index.row()]->isReference()) {
        QFont font;
        font.setBold(true);
        return font;
    }
    if (role == Qt::CheckStateRole) {
        if (index.column() == 5) {
            if (list.at(index.row())->isChecked() == true)
                return Qt::Checked;
            return Qt::Unchecked;
        }
        return {};
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) return {};

    ImageRecord *image = list[index.row()];

    switch (index.column()) {
    case 0: {
        // Strip off the path
//        QFileInfo info(image.getFilename());
//        return info.fileName();
        return image->getFilename();
    }
    case 1:
        switch(image->getType()) {
        case ImageRecord::LIGHT: default: return tr("Light");
        case ImageRecord::DARK: return tr("Dark");
        case ImageRecord::DARK_FLAT: return tr("Dark Flat");
        case ImageRecord::FLAT: return tr("Flat");
        case ImageRecord::BIAS: return tr("Bias");
        }
        break;
    case 2: return QString::number(image->getShutter()) + " s";
    case 3: return image->getIso();
    case 4: {
        std::time_t time = image->getTimestamp();
        std::tm *timeinfo = std::localtime(&time);
        return std::asctime(timeinfo);
    }
    default: return {};
    }
}

QVariant ImageTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};

    switch(section) {
    case 0: return tr("Filename");
    case 1: return tr("Type");
    case 2: return tr("Exposure");
    case 3: return tr("ISO");
    case 4: return tr("Timestamp");
    default: return {};
    }
}

void ImageTableModel::append(ImageRecord *record)
{
    beginInsertRows({}, list.count(), list.count());
    list.append(record);
    endInsertRows();
}

ImageRecord *ImageTableModel::at(int i)
{
    return list.at(i);
}

void ImageTableModel::removeAt(int i)
{
    beginRemoveRows({}, i, i);
    list.removeAt(i);
    endRemoveRows();
}

bool ImageTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.column() == 5) {
        bool checked = false;
        if (value.toInt() == Qt::Checked)
            checked = true;
        ImageRecord *record = list.at(index.row());
        record->setChecked(checked);
    }

    return true;
}

Qt::ItemFlags ImageTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    if (index.column() == 5) {
        flags |= Qt::ItemIsUserCheckable;
        flags |= Qt::ItemIsEnabled;
    }

    return flags;
}
