#include "TW2_DZIP_Extractor.h"

TW2_DZIP_Extractor::TW2_DZIP_Extractor(QString file, QString folder) : _file(file), _folder(folder)
{

}

void TW2_DZIP_Extractor::run()
{
    extractDZIP(_folder, _file);
}

void TW2_DZIP_Extractor::extractDZIP(QString exportFolder, QString filename)
{
    emit finished();
}

void TW2_DZIP_Extractor::quitThread()
{
    _stopped = true;
}
