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
    explicit GUI_Options(QWidget* parent = nullptr);
    ~GUI_Options();

public slots:
    void ok();
    void cancel();

    void changeTheme(QString newThemeName);

    void resetViewPanel();
    void selectBackgroundColor();
    void changeExportMode();
    void selectExportDir();
    void selectTW3TexDir();

private:
    Ui::GUI_Options* _ui;
    QColor _originalBackgroundColor;
    void translate();
    void updateBackgroundColorButtonColor();

signals:
    void optionsValidation();
};

#endif // OPTIONS_H
