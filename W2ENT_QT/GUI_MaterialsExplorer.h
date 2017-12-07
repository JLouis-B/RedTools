#ifndef MATERIALSEXPLORER_H
#define MATERIALSEXPLORER_H

#include <QDialog>
#include <QItemDelegate>
#include <QTableWidgetItem>
#include "GUI_ExtFilesExplorer.h"
#include "QIrrlichtWidget.h"

struct Property
{
    QString name;
    QString type;
    QString data;
};

namespace Ui {
class GUI_MaterialsExplorer;
}

using namespace irr;

class RichTextDelegate: public QItemDelegate
{
public:
    RichTextDelegate(QObject *parent = 0);

    void paint( QPainter *painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index ) const;
};

class QTableWidgetItemWithData: public QTableWidgetItem
{
public:
    QTableWidgetItemWithData(QString richData, QString rawData);

QString _rawData;
};


class GUI_MaterialsExplorer : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_MaterialsExplorer(QWidget *parent = 0, QIrrlichtWidget* irrlicht = 0, QString filename = "");
    ~GUI_MaterialsExplorer();
    void read(QString filename);

public slots :
    void selectFile();
    void selectMaterial(int row);
    void openData(QTableWidgetItem *data);

private:
    QIrrlichtWidget* _irrlicht;
    Ui::GUI_MaterialsExplorer *_ui;

    void loadTW3Materials(io::IReadFile* file);
    void ReadIMaterialProperty(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files);
    void W3_CMaterialInstance(io::IReadFile* file, W3_DataInfos infos, core::array<core::stringc>& strings, core::array<core::stringc>& files);

    std::vector<std::vector<Property>> _materials;
};

#endif // MATERIALSEXPLORER_H
