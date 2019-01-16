#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>

namespace Ui {
class GUI_Options;
}


class GUI_Options : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_Options(QWidget* parent = nullptr, QString loadedFile = QString());
    ~GUI_Options();

public slots:
    void reset();
    void ok();
    void selectColor();
    void changeExport();
    void selectDir();
    void selectTW3TexDir();

private:
    QColor _col;
    QString _filename;
    Ui::GUI_Options* _ui;

signals:
    void optionsValidation();
};

#endif // OPTIONS_H
