#ifndef PROCESSINGDIALOG_H
#define PROCESSINGDIALOG_H

#include <QDialog>
#include <QString>
#include <QPushButton>
#include <QDebug>

#include <opencv2/core/core.hpp>

namespace Ui {
class ProcessingDialog;
}

//! Modal dialog that displays the progress of the stacking process.
class ProcessingDialog : public QDialog
{
    Q_OBJECT

public:
    //! Constructor.
    explicit ProcessingDialog(QWidget *parent = 0);

    //! Destructor.
    ~ProcessingDialog();

public slots:
    //! Display a message and update the progress bar.
    /*! @param message A message describing the progress.
        @param percentComplete An integer out of 100 represention the completion of the stacking process.
    */
    void updateProgress(QString message, int percentComplete);

    //! Marks the process as complete and updates the displayed message.
    /*! @param message Description of the stacking result. */
    void complete(cv::Mat image, QString message);

signals:
    //! Asynchronously cancel the stacking process.
    void cancelProcessing();

private:
    Ui::ProcessingDialog *ui;
};

#endif // PROCESSINGDIALOG_H
