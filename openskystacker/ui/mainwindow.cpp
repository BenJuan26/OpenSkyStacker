#include "mainwindow.h"

using namespace openskystacker;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    positionAndResizeWindow();

    // cv::Mat can't be passed through a signal without this declaration
    qRegisterMetaType<cv::Mat>("cv::Mat");

    stacker = new ImageStacker();
    workerThread = new QThread();

    stacker->moveToThread(workerThread);
    workerThread->start();

    imageFileFilter
            << tr("All files (*)")
            << tr("Image files (*.jpg *.jpeg *.png *.tif)")
            << tr("Raw image files (*.NEF *.CR2 *.DNG *.RAW)")
            << tr("FITS image files (*.fit *.fits)");

    QTableView *table = ui->imageListView;
    table->setModel(&tableModel);
    connect(&tableModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this,
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
    connect(ui->buttonSelectLightFrames, SIGNAL(released()), this,
            SLOT(handleButtonLightFrames()));
    connect(ui->buttonSelectDarkFrames, SIGNAL(released()), this,
            SLOT(handleButtonDarkFrames()));
    connect(ui->buttonSelectDarkFlatFrames, SIGNAL(released()), this,
            SLOT(handleButtonDarkFlatFrames()));
    connect(ui->buttonSelectFlatFrames, SIGNAL(released()), this,
            SLOT(handleButtonFlatFrames()));
    connect(ui->buttonSelectBiasFrames, SIGNAL(released()), this,
            SLOT(handleButtonBiasFrames()));
    connect(ui->buttonStack, SIGNAL(released()), this,
            SLOT(handleButtonStack()));
//    connect(ui_->buttonOptions, SIGNAL(released()), this,
//            SLOT(handleButtonOptions()));
    connect(ui->buttonSaveList, SIGNAL(released()), this,
            SLOT(handleButtonSaveList()));
    connect(ui->buttonLoadList, SIGNAL(released()), this,
            SLOT(handleButtonLoadList()));

    // Signals / slots for stacker
    connect(this, SIGNAL (stackImages(int, int)), stacker,
            SLOT(process(int, int)));
    connect(this, SIGNAL(readQImage(QString)), stacker,
            SLOT(readQImage(QString)));
    connect(this, SIGNAL(detectStars(QString,int)), stacker,
            SLOT(detectStars(QString,int)));
    connect(stacker, SIGNAL(doneDetectingStars(int)), this,
            SLOT(stackerDoneDetectingStars(int)));
    connect(stacker, SIGNAL(finished(cv::Mat, QString)), this,
            SLOT(finishedStacking(cv::Mat)));
    connect(stacker, SIGNAL(finished(cv::Mat, QString)), this,
            SLOT(clearProgress(cv::Mat, QString)));
    connect(stacker, SIGNAL(processingError(QString)), this,
            SLOT(processingError(QString)));
    connect(stacker, SIGNAL(updateProgress(QString, int)), this,
            SLOT(updateProgress(QString, int)));
    connect(stacker, SIGNAL(qImageReady(QImage)), this,
            SLOT(setImage(QImage)));
}

void MainWindow::finishedStacking(cv::Mat image) {
    QString path = stacker->getSaveFilePath();
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

    setMemImage(mat2QImage(image));
}

// For the main window this is only used to update the progress indicator
// in the taskbar, i.e. green bar in Windows
void MainWindow::updateProgress(QString message, int percentComplete)
{
    Q_UNUSED(message);
    Q_UNUSED(percentComplete);
#ifdef WIN32
    QWinTaskbarProgress *progress = taskbarButton->progress();
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
    QWinTaskbarProgress *progress = taskbarButton->progress();
    progress->setVisible(false);
#endif // WIN32
}

void MainWindow::showTableContextMenu(QPoint pos)
{
    QMenu *menu = new QMenu(this);
    QTableView *table = ui->imageListView;

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
    QTableView *table = ui->imageListView;
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
    ImageRecord *record = tableModel.At(i);

    if (record->type != ImageRecord::LIGHT) {
        QMessageBox msg;
        msg.setText(tr("Reference frame must be a light frame."));
        msg.exec();
        return;
    }

    clearReferenceFrame();

    record->reference = true;
}

void MainWindow::removeSelectedImages()
{
    QItemSelectionModel *select = ui->imageListView->selectionModel();
    QModelIndexList rows = select->selectedRows();

    for (int i = 0; i < rows.count(); i++) {
        tableModel.RemoveAt(rows.at(i).row() - i);
    }
}

void MainWindow::imageSelectionChanged()
{
    QItemSelectionModel *selection = ui->imageListView->selectionModel();
    QModelIndexList rows = selection->selectedRows();

    // We're only going to load a preview image on a new selection
    if (rows.count() != 1)
        return;

    ImageRecord *record = tableModel.At(rows.at(0).row());

    // Asynchronously read the image from disk
    emit readQImage(record->filename);
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
        ImageRecord *record = tableModel.At(rows.at(i).row());
        record->checked = true;
    }
}

void MainWindow::uncheckImages()
{
    QItemSelectionModel *selection = ui->imageListView->selectionModel();
    QModelIndexList rows = selection->selectedRows();

    for (int i = 0; i < rows.count(); i++) {
        ImageRecord *record = tableModel.At(rows.at(i).row());
        record->checked = false;
    }

    checkTableData();
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

    OptionsDialog *dialog = new OptionsDialog(this);

    connect(dialog, SIGNAL(detectStars(int)), this,
            SLOT(detectStars(int)));
    connect(this, SIGNAL(doneDetectingStars(int)), dialog,
            SLOT(setDetectedStars(int)));

    if (!dialog->exec()) {
        delete dialog;
        return;
    }

    int thresh = dialog->GetThresh();
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    settings.setValue("StarDetector/thresholdCoeff", thresh);

    int threads = dialog->GetThreads();
    settings.setValue("ImageStacker/threads", threads);
    QString saveFilePath = dialog->GetPath();

    delete dialog;

    stacker->setSaveFilePath(saveFilePath);

    setDefaultReferenceImage();
    loadImagesIntoStacker();

    processingDialog = new ProcessingDialog(this);
    connect(stacker, SIGNAL(updateProgress(QString, int)), processingDialog,
            SLOT(updateProgress(QString, int)));
    connect(stacker, SIGNAL(finished(cv::Mat, QString)), processingDialog,
            SLOT(complete(cv::Mat, QString)));

    float threshold = settings.value("StarDetector/thresholdCoeff", 20).toFloat();

    // Asynchronously trigger the processing
    emit stackImages(threshold, threads);

    if (!processingDialog->exec()) {
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
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    QDir dir = QDir(settings.value("files/lights/dir",
            QDir::homePath()).toString());

    QFileDialog dialog(this);
    dialog.setDirectory(dir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(imageFileFilter);
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
        ImageRecord *record = getImageRecord(
                targetImageFileNames.at(i));
        record->type = ImageRecord::LIGHT;
        tableModel.Append(record);
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
    dialog.setNameFilters(imageFileFilter);
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
        ImageRecord *record = getImageRecord(
                darkFrameFileNames.at(i));
        record->type = ImageRecord::DARK;
        tableModel.Append(record);
    }
}

void MainWindow::handleButtonDarkFlatFrames() {
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    QDir dir = QDir(settings.value("files/darkflats/dir",
            settings.value("files/lights/dir", QDir::homePath())).toString());

    QFileDialog dialog(this);
    dialog.setDirectory(dir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(imageFileFilter);
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
        ImageRecord *record = getImageRecord(
                darkFlatFrameFileNames.at(i));
        record->type = ImageRecord::DARK_FLAT;
        tableModel.Append(record);
    }
}

void MainWindow::handleButtonFlatFrames() {
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    QDir dir = QDir(settings.value("files/flats/dir",
            settings.value("files/lights/dir", QDir::homePath())).toString());

    QFileDialog dialog(this);
    dialog.setDirectory(dir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(imageFileFilter);
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
        ImageRecord *record = getImageRecord(
                flatFrameFileNames.at(i));
        record->type = ImageRecord::FLAT;
        tableModel.Append(record);
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
    dialog.setNameFilters(imageFileFilter);
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
        ImageRecord *record = getImageRecord(
                biasFrameFileNames.at(i));
        record->type = ImageRecord::BIAS;
        tableModel.Append(record);
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
        filename += ".json";
    }

    QStringList types = QStringList() << "light" << "dark" << "darkflat" << "flat" << "bias";
    QJsonArray images;
    for (int i = 0; i < tableModel.rowCount(); i++) {
        ImageRecord *record = tableModel.At(i);
        QJsonObject image;
        image.insert("filename", record->filename);
        if (record->reference) {
            image.insert("type", "ref");
        } else {
            image.insert("type", types.at(record->type));
        }
        image.insert("checked", record->checked);

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
    std::vector<ImageRecord *> records = loadImageList(filename, &err);

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

    if (err)
        return;

    for (int i = 0; i < tableModel.rowCount(); i++) {
        tableModel.RemoveAt(0);
    }

    for (ImageRecord *record : records) {
        tableModel.Append(record);
    }

    QFile file(filename);
    QFileInfo info(file);
    dir = info.absolutePath();
    settings.setValue("files/listDir", dir);

    setDefaultReferenceImage();
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
    for (int i = 0; i < tableModel.rowCount(); i++) {
        if (tableModel.At(i)->checked && tableModel.At(i)->type == ImageRecord::LIGHT) {
            lightsChecked++;
        }
    }

    if (lightsChecked < 2) {
        ui->buttonStack->setEnabled(false);
    } else {
        ui->buttonStack->setEnabled(true);
    }
}

void MainWindow::stackerDoneDetectingStars(int stars)
{
    emit doneDetectingStars(stars);
}

void MainWindow::detectStars(int threshold)
{
    QString refFileName;
    for (int i = 0; i < tableModel.rowCount(); i++) {
        ImageRecord *record = tableModel.At(i);
        if (record->reference) {
            refFileName = record->filename;
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
    QGraphicsPixmapItem *p = scene->addPixmap(
            QPixmap::fromImage(rawToQImage(filename)));
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
        ImageRecord *record = model->At(i);
        record->reference = false;
    }
}

void MainWindow::setDefaultReferenceImage()
{
    // Is there already a reference image?
    bool referenceSet = false;
    for (int i = 0; i < tableModel.rowCount(); i++) {
        ImageRecord *record = tableModel.At(i);
        if (record->reference) referenceSet = true;
    }

    // Set first light frame as reference
    if (!referenceSet) {
        for (int i = 0; i < tableModel.rowCount(); i++) {
            ImageRecord *record = tableModel.At(i);

            if (record->type == ImageRecord::LIGHT && record->checked) {
                record->reference = true;
                QModelIndex index = ui->imageListView->model()->index(i, 0);
                ui->imageListView->selectionModel()->select(index,
                        QItemSelectionModel::Select | QItemSelectionModel::Rows |
                        QItemSelectionModel::Clear);
                break;
            }
        }
    }
}

void MainWindow::selectReferenceImage()
{
    for (int i = 0; i < tableModel.rowCount(); i++) {
        ImageRecord *record = tableModel.At(i);

        if (record->type == ImageRecord::LIGHT && record->reference) {
            QModelIndex index = ui->imageListView->model()->index(i, 0);
            ui->imageListView->selectionModel()->select(index,
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

    for (int i = 0; i < tableModel.rowCount(); i++) {
        ImageRecord *record = tableModel.At(i);

        if (!record->checked)
            continue;

        QString filename = record->filename;

        switch (record->type) {
        case ImageRecord::LIGHT:
            if (record->reference) {
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

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

#ifdef WIN32
    taskbarButton = new QWinTaskbarButton(this);
    taskbarButton->setWindow(this->windowHandle());
#endif //WIN32
}
