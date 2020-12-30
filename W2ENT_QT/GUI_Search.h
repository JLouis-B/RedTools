#ifndef GUI_SEARCH_H
#define GUI_SEARCH_H

#include <QDialog>
#include <QThread>
#include <QFileInfo>

namespace Ui {
class GUI_Search;
}

class SearchEngine : public QObject
{
    Q_OBJECT

public:
    SearchEngine(QString rootDir, QStringList keywords, QStringList extensions, bool searchFolders, bool useFafSearch, QString &index);


public slots:
    void run();
    void quitThread();

signals:
    void onProgress(int);
    void finished();
    void sendItem(QString);

private:
    QString _rootDir;
    QStringList _keywords;
    QStringList _extensions;
    bool _searchFolders;
    bool _useFafSearch;
    QString _fafSearchFilesIndex;
    bool _stopped;

    void scanFolder(QString repName, int level);
    void isASearchedFile(QFileInfo& fileInfo);
    void fafSearch();
};

class GUI_Search : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_Search(QWidget *parent = nullptr);
    ~GUI_Search();


public slots:
    void search();
    void load();
    void enableButton();
    void translate();
    void destroyWindow();
    void setProgress(int progress);
    void searchEnd();
    void addResult(QString item);

private:
    Ui::GUI_Search *_ui;
    QThread* _thread;
    SearchEngine* _searchEngine;

    QString _rootDir;

    bool _useFafSearch;
    QString _fafSearchFilesIndex;

    void killThread();
    void updateFafSettings();

signals:
    void loadPressed(QString);
};



#endif // GUI_SEARCH_H
