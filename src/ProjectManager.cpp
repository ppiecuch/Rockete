#include "ProjectManager.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include "LocalizationManagerInterface.h"

#ifdef Q_WS_MAC
# include <CoreFoundation/CoreFoundation.h>
#endif

ProjectManager::ProjectManager()
{
}

ProjectManager::~ProjectManager()
{
}

void ProjectManager::Initialize(const QString &filename)
{
    qDebug() << "Initialize project manager at: " << filename;

    QFileInfo file_info(filename);
    QFile file(filename);
    domDocument.clear();
    fontPaths.clear();
    texturePaths.clear();
    interfacePaths.clear();

    if (QFile::exists(filename)) {
        if (!file.open(QIODevice::ReadOnly))
            return;
        if (!domDocument.setContent(file.readAll())) {
            file.close();
            return;
        }
        file.close();
    }

    projectName = file_info.baseName();

#ifdef Q_WS_MAC
    CFURLRef appUrlRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFStringRef macPath = CFURLCopyFileSystemPath(appUrlRef, kCFURLPOSIXPathStyle);
    const char *bndlPathPtr = CFStringGetCStringPtr(macPath, kCFStringEncodingUTF8)?:".";
    // char path[PATH_MAX]; CFStringGetFileSystemRepresentation(s, path, sizeof(path)); // TODO: check if this is needed.

    qDebug() << "Using mac bundle path: " << bndlPathPtr;
#endif

    QDomNodeList node_list = domDocument.elementsByTagName("Font");

    if (node_list.count()) for(int i = 0; i < node_list.count(); i++)
    {
        QDomNode node = node_list.at(i);
        if(node.firstChild().isText())
        {
            const QString& nodeText = node.firstChild().toText().data();
            if(!(nodeText.contains(":") || nodeText.startsWith("/")))
            {

	            fontPaths << file_info.path() + "/" + nodeText;
            } 
            else
            {
	            fontPaths << nodeText;
            }
            if(!fontPaths.last().endsWith('/'))
            {
                fontPaths.last().append("/");
            }
        }
    } else {
#ifdef Q_WS_MAC
        fontPaths << (QString(bndlPathPtr) + "/font");
#endif
    }

    node_list = domDocument.elementsByTagName("Texture");

    if (node_list.count()) for(int i = 0; i < node_list.count(); i++)
    {
        QDomNode node = node_list.at(i);
        if(node.firstChild().isText())
        {
            const QString& nodeText = node.firstChild().toText().data();
            if(!(nodeText.contains(":") || nodeText.startsWith("/")))
            {

                texturePaths << file_info.path() + "/" + nodeText;
            } 
            else
            {
                texturePaths << nodeText;
            }
            if(!texturePaths.last().endsWith('/'))
            {
                texturePaths.last().append("/");
            }
        }
    } else {
#ifdef Q_WS_MAC
        texturePaths << (QString(bndlPathPtr) + "/textures");
#endif
    }

    node_list = domDocument.elementsByTagName("Interface");

    for(int i = 0; i < node_list.count(); i++)
    {
        QDomNode node = node_list.at(i);
        if(node.firstChild().isText())
        {
            const QString& nodeText = node.firstChild().toText().data();
            if(!(nodeText.contains(":") || nodeText.startsWith("/")))
            {

                interfacePaths << file_info.path() + "/" + nodeText;
            } 
            else
            {
                interfacePaths << nodeText;
            }
            if(!interfacePaths.last().endsWith('/'))
            {
                interfacePaths.last().append("/");
            }
        }
    }

    node_list = domDocument.elementsByTagName("WordList");

    if(!node_list.isEmpty())
    {
        QDomNode node = node_list.at(0);
        if(node.firstChild().isText())
        {
            const QString& nodeText = node.firstChild().toText().data();
            if(!(nodeText.contains(":") || nodeText.startsWith("/")))
            {

                wordListsPath = file_info.path() + "/" + nodeText;
            } 
            else
            {
                wordListsPath = nodeText;
            }
            if(!wordListsPath.endsWith('/'))
            {
                wordListsPath.append("/");
            }
        }
    }
    else
    {
        wordListsPath = "word_lists/";
    }

    node_list = domDocument.elementsByTagName("LocalizationFile");

    if(!node_list.isEmpty())
    {
        QDomNode node = node_list.at(0);
        if(node.firstChild().isText())
        {
            if(LocalizationManagerInterface::hasInstance())
            {
                LocalizationManagerInterface::getInstance().initialize(node.firstChild().toText().data());
            }
        }
    }

    node_list = domDocument.elementsByTagName("LocalizationOpeningTag");

    if(!node_list.isEmpty())
    {
        QDomNode node = node_list.at(0);
        if(node.firstChild().isText())
        {
            localizationOpeningTag = node.firstChild().toText().data();
        }
    }

    node_list = domDocument.elementsByTagName("LocalizationClosingTag");

    if(!node_list.isEmpty())
    {
        QDomNode node = node_list.at(0);
        if(node.firstChild().isText())
        {
            localizationClosingTag = node.firstChild().toText().data();
        }
    }

    node_list = domDocument.elementsByTagName("SnippetsFolder");

    if(!node_list.isEmpty())
    {
        QDomNode node = node_list.at(0);
        if(node.firstChild().isText())
        {
            const QString& nodeText = node.firstChild().toText().data();
            if(!(nodeText.contains(":") || nodeText.startsWith("/")))
            {

                snippetsFolderPath = file_info.path() + "/" + nodeText;
            } 
            else
            {
                snippetsFolderPath = nodeText;
            }
            if(!snippetsFolderPath.endsWith('/'))
            {
                snippetsFolderPath.append("/");
            }
        }
    }
    else
    {
        snippetsFolderPath = "snippets/";
    }

#ifdef Q_WS_MAC
    CFRelease(appUrlRef);
    CFRelease(macPath);
#endif
}
