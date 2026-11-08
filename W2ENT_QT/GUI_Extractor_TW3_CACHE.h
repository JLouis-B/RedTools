#ifndef GUI_EXTRACTOR_TW3_CACHE_H
#define GUI_EXTRACTOR_TW3_CACHE_H

#include <QDialog>
#include <QQueue>

class Extractor_TW3_CACHE;

namespace Ui {
class GUI_Extractor_TW3_CACHE;
}

class GUI_Extractor_TW3_CACHE : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_Extractor_TW3_CACHE(QWidget *parent = nullptr);
    ~GUI_Extractor_TW3_CACHE();


public slots:
    void destroyWindow();
    void selectFolder();
    void selectFile();
    void extract(QString file);

    void extractSetProgress(int value);
    void extractEnd();
    void extractFail();

    void killExtractThread();

private:
    Ui::GUI_Extractor_TW3_CACHE *_ui;
    QThread* _thread;
    Extractor_TW3_CACHE* _extractor;
    QQueue<QString> _filesQueue;

    int _allFilesToProcessCount;
    int _processedFilesCount;

    void queueCacheFilesFromDir(QString file);
    bool processNextCacheFile();
};

#endif // GUI_EXTRACTOR_TW3_CACHE_H
