#ifndef GUI_CLEANTEXTURESPATH_H
#define GUI_CLEANTEXTURESPATH_H

#include <QDialog>

namespace Ui {
class GUI_CleanTexturesPath;
}

class GUI_CleanTexturesPath : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_CleanTexturesPath(QString texPath, QWidget *parent = nullptr);
    ~GUI_CleanTexturesPath();

    void clean();

private:
    Ui::GUI_CleanTexturesPath *ui;
    QString _texPath;
};

#endif // GUI_CLEANTEXTURESPATH_H
