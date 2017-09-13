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

private slots:
    void valuesChanged(int thresh);

private:
    Ui::OptionsDialog *ui;
    int thresh_;
};

#endif // OPTIONSDIALOG_H
