#ifndef GUI_MATERIALSEXPLORER_H
#define GUI_MATERIALSEXPLORER_H

#include <QDialog>
#include <QItemDelegate>
#include <QTableWidgetItem>

#include "IO_MeshLoader_W2ENT.h"
#include "IO_MeshLoader_W3ENT.h"

#include <IFileSystem.h>

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
    RichTextDelegate(QObject *parent = nullptr);

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
    explicit GUI_MaterialsExplorer(QWidget *parent = nullptr, irr::io::IFileSystem* fs = nullptr, QString filename = "");
    ~GUI_MaterialsExplorer();
    void read(QString filename);

public slots :
    void selectFile();
    void selectMaterial(int row);
    void openData(QTableWidgetItem *data);

private:
    irr::io::IFileSystem* _Fs;
    Ui::GUI_MaterialsExplorer *_ui;

    void loadTW3Materials(io::IReadFile* file);
    void ReadIMaterialProperty(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files);
    void W3_CMaterialInstance(io::IReadFile* file, W3_DataInfos infos, core::array<core::stringc>& strings, core::array<core::stringc>& files);

    void loadTW2Materials(io::IReadFile* file);
    void W2_CMaterialInstance(io::IReadFile* file, ChunkDescriptor infos, core::array<core::stringc>& strings, core::array<core::stringc>& files);

    std::vector<std::vector<Property>> _materials;
};

#endif // GUI_MATERIALSEXPLORER_H
