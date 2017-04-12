#ifndef PROCESSINGDIALOG_H
#define PROCESSINGDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class ProcessingDialog;
}

class ProcessingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProcessingDialog(QWidget *parent = 0);
    ~ProcessingDialog();

public slots:
    void updateProgress(QString message, int percentComplete);
    void complete(QString message);

signals:
    void cancelProcessing();

private:
    Ui::ProcessingDialog *ui;
};

#endif // PROCESSINGDIALOG_H
