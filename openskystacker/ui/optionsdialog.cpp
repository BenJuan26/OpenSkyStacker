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
            SLOT(valuesChanged(int)));
    connect(ui->spinboxThreshold, SIGNAL(valueChanged(int)), this,
            SLOT(valuesChanged(int)));
    connect(ui->buttonDetectStars, SIGNAL(released()), this,
            SLOT(handleButtonDetectStars()));

    QSettings settings("OpenSkyStacker", "OpenSkyStacker");

    int threads = settings.value("ImageStacker/threads", QThread::idealThreadCount()).toInt();
    ui->spinboxThreads->setValue(threads);

    int thresh = settings.value("StarDetector/thresholdCoeff", 20).toInt();
    valuesChanged(thresh);
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::valuesChanged(int thresh)
{
    ui->sliderThreshold->blockSignals(true);
    ui->spinboxThreshold->blockSignals(true);
    ui->sliderThreshold->setValue(thresh);
    ui->spinboxThreshold->setValue(thresh);
    ui->sliderThreshold->blockSignals(false);
    ui->spinboxThreshold->blockSignals(false);

    SetThresh(thresh);
}

void OptionsDialog::setDetectedStars(int stars)
{
    QString label;
    if (stars < 0) {
        label = tr("Couldn't perform star detection. Make sure you have a reference image (displayed in bold).");
    } else {
        label = tr("Detected %n stars", "", stars);
    }
    ui->labelDetectStars->setText(label);
}

int OptionsDialog::GetThresh() const
{
    return thresh;
}

void OptionsDialog::SetThresh(int value)
{
    thresh = value;
}

void OptionsDialog::handleButtonDetectStars()
{
    ui->labelDetectStars->setText(tr("Detecting stars..."));
    emit detectStars(GetThresh());
}

void OptionsDialog::on_buttonBrowse_released()
{
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    QString path = settings.value("files/savePath", settings.value(
            "files/lightFramesDir", QDir::homePath())).toString();

    QString saveFilePath = QFileDialog::getSaveFileName(this,
            tr("Select Output Image"), path,
            tr("TIFF Image (*.tif)"));

    if (saveFilePath.isEmpty()) {
        return;
    }

    // Linux doesn't force the proper extension unlike Windows and Mac
    QRegularExpression regex("\\.tif$");
    if (!regex.match(saveFilePath).hasMatch()) {
        saveFilePath += ".tif";
    }

    QFileInfo info(saveFilePath);
    settings.setValue("files/savePath", info.absoluteFilePath());
    ui->lineEditFileName->setText(saveFilePath);
}

QString OptionsDialog::GetPath() const
{
    return path;
}

void OptionsDialog::SetPath(const QString &path_)
{
    path = path_;
}

void OptionsDialog::on_spinboxThreads_valueChanged(int value)
{
    threads = value;
}

int OptionsDialog::GetThreads() const
{
    return threads;
}

void OptionsDialog::SetThreads(int threads)
{
    threads = threads;
}

void OptionsDialog::on_lineEditFileName_textChanged(const QString &text)
{
    path = text;
}
