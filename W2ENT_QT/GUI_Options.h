#ifndef GUI_OPTIONS_H
#define GUI_OPTIONS_H

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
    QString _originalTheme;

    void updateBackgroundColorButtonColor();
    void translate();

signals:
    void optionsValidation();
};

#endif // GUI_OPTIONS_H
