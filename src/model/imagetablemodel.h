#ifndef IMAGETABLEMODEL_H
#define IMAGETABLEMODEL_H

#include "imagerecord.h"

#include <QAbstractTableModel>
#include <QDebug>
#include <QCoreApplication>
#include <QFileInfo>
#include <QColor>
#include <QBrush>
#include <QFont>

#include <cmath>

namespace openskystacker {

//! Defines the behaviour of the table of images.
class ImageTableModel : public QAbstractTableModel
{
    Q_DECLARE_TR_FUNCTIONS(ImageTableModel)

public:
    //! Constructor.
    ImageTableModel(QObject *parent = {});

    //! Returns the number of rows in the table.
    /*! Identical to rowCount().
        @return The number of rows.
    */
    int rowCount(const QModelIndex &parent) const override;

    //! Returns the number of rows in the table.
    /*! @return The number of rows. */
    int rowCount();

    //! Returns the number of columns in the table.
    /*! @return The number of columns. */
    int columnCount(const QModelIndex &parent) const override;

    //! A generic function that returns the data for any cell in the table.
    /*! @param index The position in the table.
        @param role The role being queried.
    */
    QVariant data(const QModelIndex &index, int role) const override;

    //! Gets the titles of the columns.
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    //! Sets the value of the cell at the given index.
    /*! @param index The index of the cell to be modified.
        @param value The value to be assigned to the cell.
        @param role The role of the assignment (unused).
    */
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    //! Gets the flags associated with the cell at the given index.
    /*! In this case it's used to set the `Checked` column as checkable.
        @param index The index of the cell.
        @return The flags associated with the cell.
    */
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    //! Adds an ImageRecord to the table.
    /*! @param record The ImageRecord to add to the table. */
    void Append(ImageRecord *record);

    //! Fetches the ImageRecord at the index `i`.
    /*! @param i The index of the desired ImageRecord. */
    ImageRecord* At(int i);

    //! Removes the ImageRecord at the index `i`.
    /*! @param i The index of the ImageRecord to be removed. */
    void RemoveAt(int i);

private:
    QList<ImageRecord*> list;
};

}

#endif // IMAGETABLEMODEL_H
