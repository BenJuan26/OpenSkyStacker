#include "processingdialog.h"
#include "ui_processingdialog.h"
#include <QPushButton>
#include <QDebug>

ProcessingDialog::ProcessingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProcessingDialog)
{
    ui->setupUi(this);

    QDialogButtonBox *buttonBox = ui->buttonBox;
    QPushButton *buttonOk = buttonBox->button(QDialogButtonBox::Ok);
    buttonOk->setEnabled(false);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void ProcessingDialog::updateProgress(QString message, int percentComplete) {
    ui->label->setText(message);
    qDebug() << "Setting to " << percentComplete << " percent";
    ui->progressBar->setValue(percentComplete);
}

void ProcessingDialog::complete(QString message) {
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    ui->label->setText(message);
}

ProcessingDialog::~ProcessingDialog()
{
    delete ui;
}
