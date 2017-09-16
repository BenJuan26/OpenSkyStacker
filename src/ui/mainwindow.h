#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <opencv2/core/core.hpp>
#include <processing/imagestacker.h>
#include <QThread>
#include "model/imagetablemodel.h"
#include "ui/processingdialog.h"
#include "ui/optionsdialog.h"

namespace Ui {
class MainWindow;
}

//! The main window of the application.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //! Constructor.
    explicit MainWindow(QWidget *parent = 0);

    //! Destructor.
    ~MainWindow();

signals:
    //! Asynchronously kicks off the image stacking process.
    void stackImages();

    //! Asynchronously read the image at the given filename.
    /*! @param filename The name of the image file to read. */
    void readQImage(QString filename);

public slots:
    //! Handles the result of the stacking process.
    /*! Saves the processed image at the previously selected path and displays the image in the preview window.
        @param image The final processed image.
    */
    void finishedStacking(cv::Mat image);

    //! Currently only used to update the green progress bar in the windows taskbar.
    /*! @param message A description of the current step in the stacking process (unused).
        @param percentComplete A number out of 100 representing the progress of the stacking process (unused).
    */
    void updateProgress(QString message, int percentComplete);

    //! Clears any progress bars.
    /*! @param message A message describing the update (unused). */
    void clearProgress(cv::Mat image, QString message);

    //! Shows the right-click menu at the given position.
    /*! @param pos The position of the click. */
    void showTableContextMenu(QPoint pos);

    //! Sets the selected frame as the reference frame.
    void setFrameAsReference();

    //! Removes the images that are selected in the table.
    void removeSelectedImages();

    //! Handles the change in image selection.
    /*! Asynchronously reads and displays the selected image if there is only one image selected. */
    void imageSelectionChanged();

    //! Displays the selected image in the preview window.
    /*! @param image The image to display. */
    void setImage(QImage image);

    //! Sets the currently selected images as checked.
    void checkImages();

    //! Sets the currently selected images as unchecked.
    void uncheckImages();

    //! Handles errors in the stacking process.
    /*! @param message The error message describing what went wrong. */
    void processingError(QString message);

    void closeEvent(QCloseEvent *event);

    void checkTableData();

private slots:
    void handleButtonLightFrames();
    void handleButtonStack();
    void handleButtonDarkFrames();
    void handleButtonDarkFlatFrames();
    void handleButtonFlatFrames();
    void handleButtonBiasFrames();
    void handleButtonOptions();
    void handleButtonSaveList();
    void handleButtonLoadList();

private:
    void setFileImage(QString filename);
    void setMemImage(QImage image);
    void clearReferenceFrame();
    void setDefaultReferenceImage();
    void loadImagesIntoStacker();
    QImage Mat2QImage(const cv::Mat &src);

    void positionAndResizeWindow();

    Ui::MainWindow *ui_;
    QThread *worker_thread_;
    ImageStacker *stacker_;
    ProcessingDialog *processing_dialog_;
    QStringList image_file_filter_;
    ImageTableModel table_model_;
    bool has_failed_ = false;
    QString error_message_;
};

#endif // MAINWINDOW_H
