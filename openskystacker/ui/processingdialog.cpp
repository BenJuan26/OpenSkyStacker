#include "processingdialog.h"
#include "ui_processingdialog.h"

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
    ui->progressBar->setValue(percentComplete);
}

void ProcessingDialog::complete(cv::Mat image, QString message) {
    Q_UNUSED(image);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    ui->label->setText(message);
    ui->progressBar->setValue(100);
}

ProcessingDialog::~ProcessingDialog()
{
    delete ui;
}
