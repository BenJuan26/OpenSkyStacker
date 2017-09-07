#include "optionsdialog.h"
#include "ui_optionsdialog.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);

    QDialogButtonBox *buttonBox = ui->buttonBox;
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(ui->sliderThreshold, SIGNAL(valueChanged(int)), this,
            SLOT(valuesChanged()));
    connect(ui->sliderThreshold, SIGNAL(valueChanged(int)), ui->labelThreshold,
            SLOT(setNum(int)));
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::valuesChanged()
{
    int thresh = ui->sliderThreshold->value();
    SetThresh(thresh);
}

int OptionsDialog::GetThresh() const
{
    return thresh_;
}

void OptionsDialog::SetThresh(int value)
{
    thresh_ = value;
}
