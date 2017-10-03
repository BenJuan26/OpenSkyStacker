#include "mainwindow.h"

using namespace openskystacker;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui_(new Ui::MainWindow)
{
    ui_->setupUi(this);

    positionAndResizeWindow();

    // cv::Mat can't be passed through a signal without this declaration
    qRegisterMetaType<cv::Mat>("cv::Mat");

    stacker_ = new ImageStacker();
    worker_thread_ = new QThread();

    stacker_->moveToThread(worker_thread_);
    worker_thread_->start();

    image_file_filter_
            << tr("All files (*)")
            << tr("Image files (*.jpg *.jpeg *.png *.tif)")
            << tr("Raw image files (*.NEF *.CR2 *.DNG *.RAW)")
            << tr("FITS image files (*.fit *.fits)");

    QTableView *table = ui_->imageListView;
    table->setModel(&table_model_);
    connect(&table_model_, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this,
            SLOT(checkTableData()));
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
    connect(table,SIGNAL(customContextMenuRequested(QPoint)),this,
            SLOT(showTableContextMenu(QPoint)));

    QItemSelectionModel *selection = table->selectionModel();
    connect(selection, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(imageSelectionChanged()));

    // Signals / slots for buttons
    connect(ui_->buttonSelectLightFrames, SIGNAL(released()), this,
            SLOT(handleButtonLightFrames()));
    connect(ui_->buttonSelectDarkFrames, SIGNAL(released()), this,
            SLOT(handleButtonDarkFrames()));
    connect(ui_->buttonSelectDarkFlatFrames, SIGNAL(released()), this,
            SLOT(handleButtonDarkFlatFrames()));
    connect(ui_->buttonSelectFlatFrames, SIGNAL(released()), this,
            SLOT(handleButtonFlatFrames()));
    connect(ui_->buttonSelectBiasFrames, SIGNAL(released()), this,
            SLOT(handleButtonBiasFrames()));
    connect(ui_->buttonStack, SIGNAL(released()), this,
            SLOT(handleButtonStack()));
    connect(ui_->buttonOptions, SIGNAL(released()), this,
            SLOT(handleButtonOptions()));
    connect(ui_->buttonSaveList, SIGNAL(released()), this,
            SLOT(handleButtonSaveList()));
    connect(ui_->buttonLoadList, SIGNAL(released()), this,
            SLOT(handleButtonLoadList()));

    // Signals / slots for stacker
    connect(this, SIGNAL (stackImages()), stacker_,
            SLOT(Process()));
    connect(this, SIGNAL(readQImage(QString)), stacker_,
            SLOT(ReadQImage(QString)));
    connect(this, SIGNAL(detectStars(QString,int)), stacker_,
            SLOT(detectStars(QString,int)));
    connect(stacker_, SIGNAL(doneDetectingStars(int)), this,
            SLOT(stackerDoneDetectingStars(int)));
    connect(stacker_, SIGNAL(Finished(cv::Mat, QString)), this,
            SLOT(finishedStacking(cv::Mat)));
    connect(stacker_, SIGNAL(Finished(cv::Mat, QString)), this,
            SLOT(clearProgress(cv::Mat, QString)));
    connect(stacker_, SIGNAL(ProcessingError(QString)), this,
            SLOT(processingError(QString)));
    connect(stacker_, SIGNAL(UpdateProgress(QString, int)), this,
            SLOT(updateProgress(QString, int)));
    connect(stacker_, SIGNAL(QImageReady(QImage)), this,
            SLOT(setImage(QImage)));
}

void MainWindow::finishedStacking(cv::Mat image) {
    QString path = stacker_->GetSaveFilePath();
    try {
        // OpenCV doesn't support grayscale 32-bit tiff images
        if (image.channels() == 1) {
            cv::Mat converted;
            image.convertTo(converted, CV_16U, 65535);
            imwrite(path.toUtf8().constData(), converted);
        } else {
            imwrite(path.toUtf8().constData(), image);
        }
    }
    catch (std::exception) {
        QMessageBox errorBox;
        errorBox.setText(tr("Couldn't write file to disk. Make sure the "
                "extension is valid."));
        errorBox.exec();
        return;
    }

    setMemImage(Mat2QImage(image));

    qDebug() << "Done stacking";
}

// For the main window this is only used to update the progress indicator
// in the taskbar, i.e. green bar in Windows
void MainWindow::updateProgress(QString message, int percentComplete)
{
    Q_UNUSED(message);
#ifdef WIN32
    QWinTaskbarProgress *progress = taskbar_button_->progress();
    progress->setVisible(true);
    progress->setValue(percentComplete);
#endif // WIN32
}

// Taskbar progress should clear on completion
void MainWindow::clearProgress(cv::Mat image, QString message)
{
    Q_UNUSED(image);
    Q_UNUSED(message);
#ifdef WIN32
    QWinTaskbarProgress *progress = taskbar_button_->progress();
    progress->setVisible(false);
#endif // WIN32
}

void MainWindow::showTableContextMenu(QPoint pos)
{
    QMenu *menu = new QMenu(this);
    QTableView *table = ui_->imageListView;

    QAction *checkImageAction = new QAction(tr("Check"), this);
    connect(checkImageAction, SIGNAL(triggered(bool)), this,
            SLOT(checkImages()));
    menu->addAction(checkImageAction);

    QAction *uncheckImageAction = new QAction(tr("Uncheck"), this);
    connect(uncheckImageAction, SIGNAL(triggered(bool)), this,
            SLOT(uncheckImages()));
    menu->addAction(uncheckImageAction);

    QAction *setAsReferenceAction = new QAction(tr("Set As Reference"), this);
    connect(setAsReferenceAction, SIGNAL(triggered(bool)), this,
            SLOT(setFrameAsReference()));
    menu->addAction(setAsReferenceAction);

    QAction *removeImageAction = new QAction(tr("Remove"), this);
    connect(removeImageAction, SIGNAL(triggered(bool)), this,
            SLOT(removeSelectedImages()));
    menu->addAction(removeImageAction);

    // Show menu where the user clicked
    menu->popup(table->viewport()->mapToGlobal(pos));
}

void MainWindow::setFrameAsReference()
{
    QTableView *table = ui_->imageListView;
    QItemSelectionModel *select = table->selectionModel();
    QModelIndexList rows = select->selectedRows();

    if (rows.count() > 1) {
        QMessageBox msg;
        msg.setText(tr("Cannot set more than one frame as "
                "the reference frame."));
        msg.exec();
        return;
    }

    int i = rows.at(0).row();
    ImageRecord *record = table_model_.At(i);

    if (record->GetType() != ImageRecord::LIGHT) {
        QMessageBox msg;
        msg.setText(tr("Reference frame must be a light frame."));
        msg.exec();
        return;
    }

    clearReferenceFrame();

    record->SetReference(true);
}

void MainWindow::removeSelectedImages()
{
    QItemSelectionModel *select = ui_->imageListView->selectionModel();
    QModelIndexList rows = select->selectedRows();

    for (int i = 0; i < rows.count(); i++) {
        table_model_.RemoveAt(rows.at(i).row() - i);
    }
}

void MainWindow::imageSelectionChanged()
{
    QItemSelectionModel *selection = ui_->imageListView->selectionModel();
    QModelIndexList rows = selection->selectedRows();

    // We're only going to load a preview image on a new selection
    if (rows.count() != 1)
        return;

    ImageRecord *record = table_model_.At(rows.at(0).row());

    // Asynchronously read the image from disk
    emit readQImage(record->GetFilename());
}

void MainWindow::setImage(QImage image)
{
    QGraphicsScene* scene = new QGraphicsScene(this);
    QGraphicsPixmapItem *p = scene->addPixmap(QPixmap::fromImage(image));
    ui_->imageHolder->setScene(scene);
    ui_->imageHolder->fitInView(p, Qt::KeepAspectRatio);
}

void MainWindow::checkImages()
{
    QItemSelectionModel *selection = ui_->imageListView->selectionModel();
    QModelIndexList rows = selection->selectedRows();

    for (int i = 0; i < rows.count(); i++) {
        ImageRecord *record = table_model_.At(rows.at(i).row());
        record->SetChecked(true);
    }
}

void MainWindow::uncheckImages()
{
    QItemSelectionModel *selection = ui_->imageListView->selectionModel();
    QModelIndexList rows = selection->selectedRows();

    for (int i = 0; i < rows.count(); i++) {
        ImageRecord *record = table_model_.At(rows.at(i).row());
        record->SetChecked(false);
    }

    checkTableData();
}

void MainWindow::processingError(QString message)
{
    if (processing_dialog_)
        processing_dialog_->reject();

    has_failed_ = true;
    error_message_ = message;
}

void MainWindow::handleButtonStack() {
    has_failed_ = false;
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
        qDebug() << "Filename was missing extension, adding it";
        saveFilePath += ".tif";
    }

    QFileInfo info(saveFilePath);
    settings.setValue("files/savePath", info.absoluteFilePath());
    stacker_->SetSaveFilePath(saveFilePath);

    setDefaultReferenceImage();
    loadImagesIntoStacker();

    processing_dialog_ = new ProcessingDialog(this);
    connect(stacker_, SIGNAL(UpdateProgress(QString, int)), processing_dialog_,
            SLOT(updateProgress(QString, int)));
    connect(stacker_, SIGNAL(Finished(cv::Mat, QString)), processing_dialog_,
            SLOT(complete(cv::Mat, QString)));

    // Asynchronously trigger the processing
    emit stackImages();

    if (!processing_dialog_->exec()) {
        qDebug() << "Cancelling...";
        stacker_->cancel_ = true;
    }

    delete processing_dialog_;

    if (has_failed_) {
        QMessageBox box;
        box.setText(error_message_);
        box.exec();
    }
}

void MainWindow::handleButtonLightFrames() {
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    QDir dir = QDir(settings.value("files/lights/dir",
            QDir::homePath()).toString());

    QFileDialog dialog(this);
    dialog.setDirectory(dir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(image_file_filter_);
    dialog.setWindowTitle(tr("Select Light Frames"));

    QString filter = settings.value("files/lights/filter", QString()).toString();
    if (!filter.isNull()) {
        dialog.selectNameFilter(filter);
    }

    if (!dialog.exec())
        return;

    QStringList targetImageFileNames = dialog.selectedFiles();
    QFileInfo info(targetImageFileNames.at(0));
    settings.setValue("files/lights/dir", info.absolutePath());
    QString newFilter = dialog.selectedNameFilter();
    settings.setValue("files/lights/filter", newFilter);

    for (int i = 0; i < targetImageFileNames.length(); i++) {
        ImageRecord *record = stacker_->GetImageRecord(
                targetImageFileNames.at(i));
        record->SetType(ImageRecord::LIGHT);
        table_model_.Append(record);
    }

    setDefaultReferenceImage();
}

void MainWindow::handleButtonDarkFrames() {
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    QDir dir = QDir(settings.value("files/darks/dir",
            settings.value("files/lights/dir", QDir::homePath())).toString());

    QFileDialog dialog(this);
    dialog.setDirectory(dir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(image_file_filter_);
    dialog.setWindowTitle(tr("Select Dark Frames"));

    QString filter = settings.value("files/darks/filter",
            settings.value("files/lights/filter",QString())).toString();
    if (!filter.isNull()) {
        dialog.selectNameFilter(filter);
    }

    if (!dialog.exec()) return;

    QStringList darkFrameFileNames = dialog.selectedFiles();
    QFileInfo info(darkFrameFileNames.at(0));
    settings.setValue("files/darks/dir", info.absolutePath());
    QString newFilter = dialog.selectedNameFilter();
    settings.setValue("files/darks/filter", newFilter);

    for (int i = 0; i < darkFrameFileNames.length(); i++) {
        ImageRecord *record = stacker_->GetImageRecord(
                darkFrameFileNames.at(i));
        record->SetType(ImageRecord::DARK);
        table_model_.Append(record);
    }
}

void MainWindow::handleButtonDarkFlatFrames() {
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    QDir dir = QDir(settings.value("files/darkflats/dir",
            settings.value("files/lights/dir", QDir::homePath())).toString());

    QFileDialog dialog(this);
    dialog.setDirectory(dir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(image_file_filter_);
    dialog.setWindowTitle(tr("Select Dark Flat Frames"));

    QString filter = settings.value("files/darkflats/filter",
            settings.value("files/lights/filter",QString())).toString();
    if (!filter.isNull()) {
        dialog.selectNameFilter(filter);
    }

    if (!dialog.exec()) return;

    QStringList darkFlatFrameFileNames = dialog.selectedFiles();
    QFileInfo info(darkFlatFrameFileNames.at(0));
    settings.setValue("files/darkflats/dir", info.absolutePath());
    QString newFilter = dialog.selectedNameFilter();
    settings.setValue("files/darkflats/filter", newFilter);

    for (int i = 0; i < darkFlatFrameFileNames.length(); i++) {
        ImageRecord *record = stacker_->GetImageRecord(
                darkFlatFrameFileNames.at(i));
        record->SetType(ImageRecord::DARK_FLAT);
        table_model_.Append(record);
    }
}

void MainWindow::handleButtonFlatFrames() {
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    QDir dir = QDir(settings.value("files/flats/dir",
            settings.value("files/lights/dir", QDir::homePath())).toString());

    QFileDialog dialog(this);
    dialog.setDirectory(dir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(image_file_filter_);
    dialog.setWindowTitle(tr("Select Flat Frames"));

    QString filter = settings.value("files/flats/filter",
            settings.value("files/lights/filter",QString())).toString();
    if (!filter.isNull()) {
        dialog.selectNameFilter(filter);
    }

    if (!dialog.exec()) return;

    QStringList flatFrameFileNames = dialog.selectedFiles();
    QFileInfo info(flatFrameFileNames.at(0));
    settings.setValue("files/flats/dir", info.absolutePath());
    QString newFilter = dialog.selectedNameFilter();
    settings.setValue("files/flats/filter", newFilter);

    for (int i = 0; i < flatFrameFileNames.length(); i++) {
        ImageRecord *record = stacker_->GetImageRecord(
                flatFrameFileNames.at(i));
        record->SetType(ImageRecord::FLAT);
        table_model_.Append(record);
    }
}

void MainWindow::handleButtonBiasFrames()
{
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    QDir dir = QDir(settings.value("files/bias/dir",
            settings.value("files/lights/dir", QDir::homePath())).toString());

    QFileDialog dialog(this);
    dialog.setDirectory(dir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(image_file_filter_);
    dialog.setWindowTitle(tr("Select Bias Frames"));

    QString filter = settings.value("files/bias/filter",
            settings.value("files/lights/filter",QString())).toString();
    if (!filter.isNull()) {
        dialog.selectNameFilter(filter);
    }

    if (!dialog.exec()) return;

    QStringList biasFrameFileNames = dialog.selectedFiles();
    QFileInfo info(biasFrameFileNames.at(0));
    settings.setValue("files/bias/dir", info.absolutePath());
    QString newFilter = dialog.selectedNameFilter();
    settings.setValue("files/bias/filter", newFilter);

    for (int i = 0; i < biasFrameFileNames.length(); i++) {
        ImageRecord *record = stacker_->GetImageRecord(
                biasFrameFileNames.at(i));
        record->SetType(ImageRecord::BIAS);
        table_model_.Append(record);
    }
}

void MainWindow::handleButtonOptions()
{
    OptionsDialog *dialog = new OptionsDialog(this);

    connect(dialog, SIGNAL(detectStars(int)), this,
            SLOT(detectStars(int)));
    connect(this, SIGNAL(doneDetectingStars(int)), dialog,
            SLOT(setDetectedStars(int)));

    if (dialog->exec()) {
        int thresh = dialog->GetThresh();
        QSettings settings("OpenSkyStacker", "OpenSkyStacker");
        settings.setValue("StarDetector/thresholdCoeff", thresh);
    }

    delete dialog;
}

void MainWindow::handleButtonSaveList()
{
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    QString filename = QFileDialog::getSaveFileName(this, tr("Save List"),
            settings.value("files/listDir", settings.value(
            "files/LightFramesDir", QDir::homePath())).toString(),
            "JSON document (*.json)");
    if (filename.isEmpty())
        return;

    // Linux doesn't force the proper extension unlike Windows and Mac
    QRegularExpression regex("\\.json$");
    if (!regex.match(filename).hasMatch()) {
        qDebug() << "Filename was missing extension, adding it";
        filename += ".json";
    }

    QJsonArray images;
    for (int i = 0; i < table_model_.rowCount(); i++) {
        ImageRecord *record = table_model_.At(i);
        QJsonObject image;
        image.insert("filename", record->GetFilename());
        image.insert("type", record->GetType());
        image.insert("checked", record->IsChecked());

        images.insert(images.size(), image);
    }

    QJsonDocument doc(images);
    QByteArray fileContents = doc.toJson();
    QFile file(filename);
    if (file.open(QIODevice::ReadWrite)) {
        QTextStream stream(&file);
        stream << fileContents;
    }

    QFileInfo info(file);
    QString dir = info.absolutePath();
    settings.setValue("files/listDir", dir);
}

void MainWindow::handleButtonLoadList()
{
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    QString dir = settings.value("files/listDir", settings.value(
            "files/LightFramesDir", QDir::homePath())).toString();
    QString filename = QFileDialog::getOpenFileName(this, tr("Load List"), dir, "JSON file (*.json)");
    if (filename.isEmpty()) {
        return;
    }

    int err = 0;
    std::vector<ImageRecord *> records = ImageStacker::LoadImageList(filename, &err);

    switch (err) {
    case -1:
        QMessageBox::information(this, tr("Error loading list"),
                tr("Couldn't read the list file. Top level object is not an array."));
        break;
    case -2:
        QMessageBox::information(this, tr("Error loading list"),
                tr("Couldn't read the list file. One of the objects is not a JSON object."));
        break;
    case -3:
        QMessageBox::information(this, tr("Error loading list"),
                tr("Couldn't read the list file. One of the objects has no valid filename."));
        break;
    case -4:
        QMessageBox::information(this, tr("Error loading list"),
                tr("Couldn't read the list file. One of the objects has no valid type."));
        break;
    default:
        break;
    }

    if (!err) {
        for (int i = 0; i < table_model_.rowCount(); i++) {
            table_model_.RemoveAt(0);
        }

        for (ImageRecord *record : records) {
            table_model_.Append(record);
        }
    } else {
        return;
    }

    setDefaultReferenceImage();
}

QImage MainWindow::Mat2QImage(const cv::Mat &src) {
    QImage dest(src.cols, src.rows, QImage::Format_RGB32);
    int r, g, b;

    for(int x = 0; x < src.cols; x++) {
        for(int y = 0; y < src.rows; y++) {
            if (src.channels() == 1) {
                int pixel = src.at<float>(y,x) * 255;
                dest.setPixel(x, y, qRgb(pixel, pixel, pixel));
            } else {
                cv::Vec3f pixel = src.at<cv::Vec3f>(y,x);
                b = pixel.val[0]*255;
                g = pixel.val[1]*255;
                r = pixel.val[2]*255;
                dest.setPixel(x, y, qRgb(r,g,b));
            }
        }
    }

    return dest;
}

void MainWindow::positionAndResizeWindow()
{
    QSize defaultSize = this->size();

    QRect desktopRect = QApplication::desktop()->availableGeometry(this);
    QPoint center = desktopRect.center();
    this->move(center.x() - this->width()*0.5, center.y() - this->height()*0.5);

    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    this->resize(settings.value("MainWindow/size", defaultSize).toSize());
    this->move(settings.value("MainWindow/pos", QPoint(center.x()
            - this->width()*0.5, center.y() - this->height()*0.5)).toPoint());
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    settings.setValue("MainWindow/size", this->size());
    settings.setValue("MainWindow/pos", this->pos());

    Q_UNUSED(event);
}

void MainWindow::checkTableData()
{
    int lightsChecked = 0;
    for (int i = 0; i < table_model_.rowCount(); i++) {
        if (table_model_.At(i)->IsChecked() && table_model_.At(i)->GetType() == ImageRecord::LIGHT) {
            lightsChecked++;
        }
    }

    if (lightsChecked < 2) {
        ui_->buttonStack->setEnabled(false);
    } else {
        ui_->buttonStack->setEnabled(true);
    }
}

void MainWindow::stackerDoneDetectingStars(int stars)
{
    emit doneDetectingStars(stars);
}

void MainWindow::detectStars(int threshold)
{
    QString refFileName;
    for (int i = 0; i < table_model_.rowCount(); i++) {
        ImageRecord *record = table_model_.At(i);
        if (record->IsReference()) {
            refFileName = record->GetFilename();
            break;
        }
    }

    if (refFileName.isNull()) {
        emit doneDetectingStars(-1);
    } else {
        emit detectStars(refFileName, threshold);
    }
}

void MainWindow::setFileImage(QString filename) {

    QGraphicsScene* scene = new QGraphicsScene(this);
    cv::Mat image = stacker_->ReadImage(filename);
    QGraphicsPixmapItem *p = scene->addPixmap(
            QPixmap::fromImage(Mat2QImage(image)));
    ui_->imageHolder->setScene(scene);
    ui_->imageHolder->fitInView(p, Qt::KeepAspectRatio);
}

void MainWindow::setMemImage(QImage image) {
    QGraphicsScene* scene = new QGraphicsScene(this);
    QGraphicsPixmapItem *p = scene->addPixmap(QPixmap::fromImage(image));
    ui_->imageHolder->setScene(scene);
    ui_->imageHolder->fitInView(p, Qt::KeepAspectRatio);
}

void MainWindow::clearReferenceFrame()
{
    QTableView *table = ui_->imageListView;
    ImageTableModel *model = (ImageTableModel*)table->model();

    for (int i = 0; i < model->rowCount(); i++) {
        ImageRecord *record = model->At(i);
        record->SetReference(false);
    }
}

void MainWindow::setDefaultReferenceImage()
{
    // Is there already a reference image?
    bool referenceSet = false;
    for (int i = 0; i < table_model_.rowCount(); i++) {
        ImageRecord *record = table_model_.At(i);
        if (record->IsReference()) referenceSet = true;
    }

    // Set first light frame as reference
    if (!referenceSet) {
        for (int i = 0; i < table_model_.rowCount(); i++) {
            ImageRecord *record = table_model_.At(i);

            if (record->GetType() == ImageRecord::LIGHT && record->IsChecked()) {
                record->SetReference(true);
                QModelIndex index = ui_->imageListView->model()->index(i, 0);
                ui_->imageListView->selectionModel()->select(index,
                        QItemSelectionModel::Select | QItemSelectionModel::Rows |
                        QItemSelectionModel::Clear);
                break;
            }
        }
    }
}

void MainWindow::selectReferenceImage()
{
    for (int i = 0; i < table_model_.rowCount(); i++) {
        ImageRecord *record = table_model_.At(i);

        if (record->GetType() == ImageRecord::LIGHT && record->IsReference()) {
            QModelIndex index = ui_->imageListView->model()->index(i, 0);
            ui_->imageListView->selectionModel()->select(index,
                    QItemSelectionModel::Select | QItemSelectionModel::Rows |
                    QItemSelectionModel::Clear);
            break;
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

    for (int i = 0; i < table_model_.rowCount(); i++) {
        ImageRecord *record = table_model_.At(i);

        if (!record->IsChecked())
            continue;

        QString filename = record->GetFilename();

        switch (record->GetType()) {
        case ImageRecord::LIGHT:
            if (record->IsReference()) {
                stacker_->SetRefImageFileName(filename);
                break;
            }

            lights.append(filename);
            break;
        case ImageRecord::DARK:
            darks.append(filename);
            stacker_->SetUseDarks(true);
            break;
        case ImageRecord::DARK_FLAT:
            darkFlats.append(filename);
            stacker_->SetUseDarkFlats(true);
            break;
        case ImageRecord::FLAT:
            flats.append(filename);
            stacker_->SetUseFlats(true);
            break;
        case ImageRecord::BIAS:
            bias.append(filename);
            stacker_->SetUseBias(true);
            break;
        default:
            break;
        }
    }

    stacker_->SetTargetImageFileNames(lights);
    stacker_->SetDarkFrameFileNames(darks);
    stacker_->SetDarkFlatFrameFileNames(darkFlats);
    stacker_->SetFlatFrameFileNames(flats);
    stacker_->SetBiasFrameFileNames(bias);
}

MainWindow::~MainWindow()
{
    delete ui_;
    worker_thread_->quit();
    worker_thread_->wait();
    delete worker_thread_;
    delete stacker_;
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

#ifdef WIN32
    taskbar_button_ = new QWinTaskbarButton(this);
    taskbar_button_->setWindow(this->windowHandle());
#endif //WIN32
}
