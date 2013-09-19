#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QDomDocument>
#include "Settings.h"

class ProjectManager
{
public:
    ProjectManager();
    ~ProjectManager();

    static ProjectManager & getInstance() {
        static ProjectManager instance;
        return instance;
    }

    bool Initialize(const QString &filename);
    void Serialize(const QString &filename);

    const QStringList &getFontPaths(){return fontPaths;}
    const QStringList &getTexturePaths(){return texturePaths;}
    const QStringList &getInterfacePaths(){return interfacePaths;}
    const QString &getWordListPath(){return wordListsPath;}
    const QString &getSnippetsFolderPath(){return snippetsFolderPath;}
    const QString &getLocalizationOpeningTag(){return localizationOpeningTag;}
    const QString &getLocalizationClosingTag(){return localizationClosingTag;}
    void setCuttingInfo(const QString &key, const QString &info);
    const QString getCuttingInfo(const QString &key);
private:
    QString projectFile, projectName;
    QStringList fontPaths;
    QStringList texturePaths;
    QStringList interfacePaths;
    QString wordListsPath;
    QString snippetsFolderPath;
    QString localizationOpeningTag;
    QString localizationClosingTag;
    QMap<QString, QString> cutting;
};
#endif
