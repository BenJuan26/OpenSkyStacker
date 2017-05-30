#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "processingdialog.h"
#include "processing/stardetector.h"
#include "model/imagerecord.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QGraphicsPixmapItem>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include <QDesktopWidget>
#include <stdexcept>
#ifdef WIN32
#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>
#endif

using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Centre window on startup
    QRect desktopRect = QApplication::desktop()->availableGeometry(this);
    QPoint center = desktopRect.center();
    this->move(center.x() - this->width()*0.5, center.y() - this->height()*0.5);

    // cv::Mat can't be passed through a signal without this declaration
    qRegisterMetaType<cv::Mat>("cv::Mat");

    stacker = new ImageStacker();
    stacker->setBitsPerChannel(ImageStacker::BITS_32);
    workerThread = new QThread();

    stacker->moveToThread(workerThread);
    workerThread->start();

    imageFileFilter << tr("All files (*)") << tr("Image files (*.jpg *.jpeg *.png *.tif)");
    imageFileFilter << tr("Raw image files (*.NEF *.CR2 *.DNG *.RAW)");

    selectedDir = QDir::home();

    QTableView *table = ui->imageListView;
    table->setModel(&tableModel);
    table->setColumnWidth(0,25);  // checked
    table->setColumnWidth(1,260); // filename
    table->setColumnWidth(2,80);  // type
    table->setColumnWidth(3,80);  // exposure
    table->setColumnWidth(4,60);  // iso
    table->setColumnWidth(5,140); // timestamp
    table->setColumnWidth(6,80);  // width
    table->setColumnWidth(7,80);  // height

    // Right-click support for the image list
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showTableContextMenu(QPoint)));

    QItemSelectionModel *selection = table->selectionModel();
    connect(selection, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(imageSelectionChanged()));

    // Signals / slots for buttons
    connect(ui->buttonSelectLightFrames, SIGNAL (released()), this, SLOT (handleButtonLightFrames()));
    connect(ui->buttonSelectDarkFrames, SIGNAL (released()), this, SLOT (handleButtonDarkFrames()));
    connect(ui->buttonSelectDarkFlatFrames, SIGNAL (released()), this, SLOT (handleButtonDarkFlatFrames()));
    connect(ui->buttonSelectFlatFrames, SIGNAL (released()), this, SLOT (handleButtonFlatFrames()));
    connect(ui->buttonSelectBiasFrames, SIGNAL (released()), this, SLOT (handleButtonBiasFrames()));
    connect(ui->buttonStack, SIGNAL (released()), this, SLOT (handleButtonStack()));

    // Signals / slots for stacker
    connect(this, SIGNAL (stackImages()), stacker, SLOT(process()));
    connect(this, SIGNAL(readQImage(QString)), stacker, SLOT(readQImage(QString)));
    connect(stacker, SIGNAL(finished(cv::Mat)), this, SLOT(finishedStacking(cv::Mat)));
    connect(stacker, SIGNAL(finishedDialog(QString)), this, SLOT(clearProgress(QString)));
    connect(stacker, SIGNAL(processingError(QString)), this, SLOT(processingError(QString)));
    connect(stacker, SIGNAL(updateProgress(QString,int)), this, SLOT(updateProgress(QString,int)));
    connect(stacker, SIGNAL(QImageReady(QImage)), this, SLOT(setImage(QImage)));

}

void MainWindow::finishedStacking(Mat image) {
    QString path = stacker->getSaveFilePath();
    imwrite(path.toUtf8().constData(), image);
    setMemImage(Mat2QImage(image));

    qDebug() << "Done stacking";
}

// For the main window this is only used to update the progress indicator in the taskbar
//  i.e. green bar in Windows
void MainWindow::updateProgress(QString message, int percentComplete)
{
    Q_UNUSED(message);
    Q_UNUSED(percentComplete);
#ifdef WIN32
    QWinTaskbarButton *button = new QWinTaskbarButton(this);
    button->setWindow(this->windowHandle());

    QWinTaskbarProgress *progress = button->progress();
    progress->setVisible(true);
    progress->setValue(percentComplete);
#endif // WIN32
}

// Taskbar progress should clear on completion
void MainWindow::clearProgress(QString message)
{
    Q_UNUSED(message);
#ifdef WIN32
    QWinTaskbarButton *button = new QWinTaskbarButton(this);
    button->setWindow(this->windowHandle());

    QWinTaskbarProgress *progress = button->progress();
    progress->setVisible(false);
#endif // WIN32
}

void MainWindow::showTableContextMenu(QPoint pos)
{
    QMenu *menu = new QMenu(this);
    QTableView *table = ui->imageListView;

    QAction *setAsReferenceAction = new QAction(tr("Set As Reference"), this);
    connect(setAsReferenceAction, SIGNAL(triggered(bool)), this,SLOT(setFrameAsReference()));
    menu->addAction(setAsReferenceAction);

    QAction *removeImageAction = new QAction(tr("Remove"), this);
    connect(removeImageAction, SIGNAL(triggered(bool)), this, SLOT(removeSelectedImages()));
    menu->addAction(removeImageAction);

    QAction *checkImageAction = new QAction(tr("Check"), this);
    connect(checkImageAction, SIGNAL(triggered(bool)), this, SLOT(checkImages()));
    menu->addAction(checkImageAction);

    QAction *uncheckImageAction = new QAction(tr("Uncheck"), this);
    connect(uncheckImageAction, SIGNAL(triggered(bool)), this, SLOT(uncheckImages()));
    menu->addAction(uncheckImageAction);

    // Show menu where the user clicked
    menu->popup(table->viewport()->mapToGlobal(pos));
}

void MainWindow::setFrameAsReference()
{
    QTableView *table = ui->imageListView;
    QItemSelectionModel *select = table->selectionModel();
    QModelIndexList rows = select->selectedRows();

    if (rows.count() > 1) {
        QMessageBox msg;
        msg.setText(tr("Cannot set more than one frame as the reference frame."));
        msg.exec();
        return;
    }

    int i = rows.at(0).row();
    ImageRecord *record = tableModel.at(i);

    if (record->getType() != ImageRecord::LIGHT) {
        QMessageBox msg;
        msg.setText(tr("Reference frame must be a light frame."));
        msg.exec();
        return;
    }

    clearReferenceFrame();

    qDebug() << "Set reference frame, row" << i;
    record->setReference(true);
}

void MainWindow::removeSelectedImages()
{
    QItemSelectionModel *select = ui->imageListView->selectionModel();
    QModelIndexList rows = select->selectedRows();

    for (int i = 0; i < rows.count(); i++) {
        tableModel.removeAt(rows.at(i).row() - i);
    }
}

void MainWindow::imageSelectionChanged()
{
    QItemSelectionModel *selection = ui->imageListView->selectionModel();
    QModelIndexList rows = selection->selectedRows();

    // We're only going to load a preview image on a new selection
    if (rows.count() != 1)
        return;

    ImageRecord *record = tableModel.at(rows.at(0).row());

    // Asynchronously read the image from disk
    emit readQImage(record->getFilename());
}

void MainWindow::setImage(QImage image)
{
    QGraphicsScene* scene = new QGraphicsScene(this);
    QGraphicsPixmapItem *p = scene->addPixmap(QPixmap::fromImage(image));
    ui->imageHolder->setScene(scene);
    ui->imageHolder->fitInView(p, Qt::KeepAspectRatio);
}

void MainWindow::checkImages()
{
    QItemSelectionModel *selection = ui->imageListView->selectionModel();
    QModelIndexList rows = selection->selectedRows();

    for (int i = 0; i < rows.count(); i++) {
        ImageRecord *record = tableModel.at(rows.at(i).row());
        record->setChecked(true);
    }
}

void MainWindow::uncheckImages()
{
    QItemSelectionModel *selection = ui->imageListView->selectionModel();
    QModelIndexList rows = selection->selectedRows();

    for (int i = 0; i < rows.count(); i++) {
        ImageRecord *record = tableModel.at(rows.at(i).row());
        record->setChecked(false);
    }
}

void MainWindow::processingError(QString message)
{
    if (processingDialog)
        processingDialog->reject();

    hasFailed = true;
    errorMessage = message;
}

void MainWindow::handleButtonStack() {
    hasFailed = false;

    QString saveFilePath = QFileDialog::getSaveFileName(
                this, tr("Select Output Image"), selectedDir.absolutePath(), tr("TIFF Image (*.tif)"));

    if (saveFilePath.isEmpty()) {
        qDebug() << "No output file selected. Cancelling.";
        return;
    }

    stacker->setSaveFilePath(saveFilePath);

    setDefaultReferenceImage();
    loadImagesIntoStacker();

    processingDialog = new ProcessingDialog(this);
    connect(stacker, SIGNAL(updateProgress(QString,int)), processingDialog, SLOT(updateProgress(QString,int)));
    connect(stacker, SIGNAL(finishedDialog(QString)), processingDialog, SLOT(complete(QString)));

    // Asynchronously trigger the processing
    emit stackImages();

    if (!processingDialog->exec()) {
        qDebug() << "Cancelling...";
        stacker->cancel = true;
    }

    delete processingDialog;

    if (hasFailed) {
        QMessageBox box;
        box.setText(errorMessage);
        box.exec();
    }
}

void MainWindow::handleButtonLightFrames() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(imageFileFilter);

    if (!dialog.exec()) return;

    QStringList targetImageFileNames = dialog.selectedFiles();

    for (int i = 0; i < targetImageFileNames.length(); i++) {
        ImageRecord *record = stacker->getImageRecord(targetImageFileNames.at(i));
        record->setType(ImageRecord::LIGHT);
        tableModel.append(record);
    }

    QFileInfo info(targetImageFileNames.at(0));
    selectedDir = QDir(info.absoluteFilePath());

    emit readQImage(targetImageFileNames.at(0));

    ui->buttonStack->setEnabled(true);
}

void MainWindow::handleButtonDarkFrames() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(imageFileFilter);

    if (!dialog.exec()) return;

    QStringList darkFrameFileNames = dialog.selectedFiles();

    for (int i = 0; i < darkFrameFileNames.length(); i++) {
        ImageRecord *record = stacker->getImageRecord(darkFrameFileNames.at(i));
        record->setType(ImageRecord::DARK);
        tableModel.append(record);
    }
}

void MainWindow::handleButtonDarkFlatFrames() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(imageFileFilter);

    if (!dialog.exec()) return;

    QStringList darkFlatFrameFileNames = dialog.selectedFiles();

    for (int i = 0; i < darkFlatFrameFileNames.length(); i++) {
        ImageRecord *record = stacker->getImageRecord(darkFlatFrameFileNames.at(i));
        record->setType(ImageRecord::DARK_FLAT);
        tableModel.append(record);
    }
}

void MainWindow::handleButtonFlatFrames() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(imageFileFilter);

    if (!dialog.exec()) return;

    QStringList flatFrameFileNames = dialog.selectedFiles();

    for (int i = 0; i < flatFrameFileNames.length(); i++) {
        ImageRecord *record = stacker->getImageRecord(flatFrameFileNames.at(i));
        record->setType(ImageRecord::FLAT);
        tableModel.append(record);
    }
}

void MainWindow::handleButtonBiasFrames()
{
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(imageFileFilter);

    if (!dialog.exec()) return;

    QStringList biasFrameFileNames = dialog.selectedFiles();

    for (int i = 0; i < biasFrameFileNames.length(); i++) {
        ImageRecord *record = stacker->getImageRecord(biasFrameFileNames.at(i));
        record->setType(ImageRecord::BIAS);
        tableModel.append(record);
    }
}

QImage MainWindow::Mat2QImage(const cv::Mat &src) {
        QImage dest(src.cols, src.rows, QImage::Format_RGB32);
        int r, g, b;

        if (stacker->getBitsPerChannel() == ImageStacker::BITS_16) {
            for(int x = 0; x < src.cols; x++) {
                for(int y = 0; y < src.rows; y++) {

                    Vec<unsigned short,3> pixel = src.at< Vec<unsigned short,3> >(y,x);
                    b = pixel.val[0]/256;
                    g = pixel.val[1]/256;
                    r = pixel.val[2]/256;
                    dest.setPixel(x, y, qRgb(r,g,b));
                }
            }
        }
        else if (stacker->getBitsPerChannel() == ImageStacker::BITS_32) {
            for(int x = 0; x < src.cols; x++) {
                for(int y = 0; y < src.rows; y++) {

                    Vec3f pixel = src.at<Vec3f>(y,x);
                    b = pixel.val[0]*255;
                    g = pixel.val[1]*255;
                    r = pixel.val[2]*255;
                    dest.setPixel(x, y, qRgb(r,g,b));
                }
            }
        }
        return dest;
}

void MainWindow::setFileImage(QString filename) {

    QGraphicsScene* scene = new QGraphicsScene(this);
    cv::Mat image = stacker->readImage(filename);
    QGraphicsPixmapItem *p = scene->addPixmap(QPixmap::fromImage(Mat2QImage(image)));
    ui->imageHolder->setScene(scene);
    ui->imageHolder->fitInView(p, Qt::KeepAspectRatio);
}

void MainWindow::setMemImage(QImage image) {
    QGraphicsScene* scene = new QGraphicsScene(this);
    QGraphicsPixmapItem *p = scene->addPixmap(QPixmap::fromImage(image));
    ui->imageHolder->setScene(scene);
    ui->imageHolder->fitInView(p, Qt::KeepAspectRatio);
}

void MainWindow::clearReferenceFrame()
{
    QTableView *table = ui->imageListView;
    ImageTableModel *model = (ImageTableModel*)table->model();

    for (int i = 0; i < model->rowCount(); i++) {
        ImageRecord *record = model->at(i);
        record->setReference(false);
    }
}

void MainWindow::setDefaultReferenceImage()
{
    // Is there already a reference image?
    bool referenceSet = false;
    for (int i = 0; i < tableModel.rowCount(); i++) {
        ImageRecord *record = tableModel.at(i);
        if (record->isReference()) referenceSet = true;
    }

    // Set first light frame as reference
    if (!referenceSet) {
        for (int i = 0; i < tableModel.rowCount(); i++) {
            ImageRecord *record = tableModel.at(i);

            if (record->getType() == ImageRecord::LIGHT) {
                record->setReference(true);
                break;
            }
        }
    }
}

void MainWindow::loadImagesIntoStacker()
{
    QStringList lights;
    QStringList darks;
    QStringList darkFlats;
    QStringList flats;
    QStringList bias;

    for (int i = 0; i < tableModel.rowCount(); i++) {
        ImageRecord *record = tableModel.at(i);

        if (!record->isChecked())
            continue;

        QString filename = record->getFilename();

        switch (record->getType()) {
        case ImageRecord::LIGHT:
            if (record->isReference()) {
                stacker->setRefImageFileName(filename);
                break;
            }

            lights.append(filename);
            break;
        case ImageRecord::DARK:
            darks.append(filename);
            stacker->setUseDarks(true);
            break;
        case ImageRecord::DARK_FLAT:
            darkFlats.append(filename);
            stacker->setUseDarkFlats(true);
            break;
        case ImageRecord::FLAT:
            flats.append(filename);
            stacker->setUseFlats(true);
            break;
        case ImageRecord::BIAS:
            bias.append(filename);
            stacker->setUseBias(true);
            break;
        default:
            break;
        }
    }

    stacker->setTargetImageFileNames(lights);
    stacker->setDarkFrameFileNames(darks);
    stacker->setDarkFlatFrameFileNames(darkFlats);
    stacker->setFlatFrameFileNames(flats);
    stacker->setBiasFrameFileNames(bias);
}

MainWindow::~MainWindow()
{
    delete ui;
    workerThread->quit();
    workerThread->wait();
    delete workerThread;
    delete stacker;
}
