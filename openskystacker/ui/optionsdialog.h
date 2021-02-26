#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QThread>

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent = 0);
    ~OptionsDialog();

    int GetThresh() const;
    void SetThresh(int value);

    QString GetPath() const;
    void SetPath(const QString &path_);

    int GetThreads() const;
    void SetThreads(int threads);

signals:
    void detectStars(int);

private slots:
    void handleButtonDetectStars();
    void valuesChanged(int thresh);
    void setDetectedStars(int stars);

    void on_buttonBrowse_released();

    void on_spinboxThreads_valueChanged(int value);

    void on_lineEditFileName_textChanged(const QString &text);

private:
    Ui::OptionsDialog *ui;
    int thresh;
    int threads;
    QString path;
};

#endif // OPTIONSDIALOG_H
