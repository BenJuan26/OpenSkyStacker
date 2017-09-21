#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QDebug>

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

signals:
    void detectStars(int);

private slots:
    void handleButtonDetectStars();
    void valuesChanged(int thresh);
    void setDetectedStars(int stars);

private:
    Ui::OptionsDialog *ui;
    int thresh_;
};

#endif // OPTIONSDIALOG_H
