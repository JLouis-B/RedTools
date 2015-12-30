#ifndef TW1BIFEXTRACTORUI_H
#define TW1BIFEXTRACTORUI_H

#include <QDialog>
#include <QFileDialog>
#include <QtConcurrent>

namespace Ui {
class tw1bifextractorUI;
}

class tw1bifextractorUI : public QDialog
{
    Q_OBJECT

public:
    explicit tw1bifextractorUI(QWidget *parent = 0);
    ~tw1bifextractorUI();

private:
    Ui::tw1bifextractorUI *ui;

public slots:
    void destroyWindow();
    void selectFolder();
    void selectFile();
    void extract();

    void extractSetProgress(int value);
    void extractEnd();
};

#endif // TW1BIFEXTRACTORUI_H
