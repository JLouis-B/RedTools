#ifndef CLEANTEXTURESPATH_H
#define CLEANTEXTURESPATH_H

#include <QDialog>
#include <QDir>
#include <QFile>

namespace Ui {
class CleanTexturesPath;
}

class CleanTexturesPath : public QDialog
{
    Q_OBJECT

public:
    explicit CleanTexturesPath(QString texPath, QWidget *parent = 0);
    ~CleanTexturesPath();

    void clean();

private:
    Ui::CleanTexturesPath *ui;
    QString _texPath;
};

#endif // CLEANTEXTURESPATH_H
