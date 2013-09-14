#include "Rockete.h"

#include <QTime>
#include <QTemporaryFile>
#include <QFileDialog>
#include <QTextEdit>
#include <QFileInfo>
#include <QInputDialog>
#include <QTextDocument>
#include <QGridLayout>
#include <QStackedLayout>
#include <QColorDialog>
#include <QComboBox>
#include <QApplication>
#include <QMessageBox>
#include <QClipboard>
#include <QDebug>
#include "AttributeTreeModel.h"
#include "RocketSystem.h"
#include "ActionManager.h"
#include "ToolManager.h"
#include "EditionHelper.h"
#include "Settings.h"
#include <QDirIterator>
#include <QShortcut>
#include "ProjectManager.h"
#include <QPluginLoader>
#include "QDRuler.h"
#include "QDLabel.h"
#include "LocalizationManagerInterface.h"
#include "OpenedLuaScript.h"

const int kTexturePreviewTabIndex = 1;
const int kCuttingImagePreviewTabIndex = 1;

void logMessageOutput( QtMsgType type, const char *msg );

struct LocalScreenSizeItem
{
    LocalScreenSizeItem(const int _width, const int _height, const char *text=NULL)
    {
        width = _width;
        height = _height;
        displayedString = QString::number(width) + "x" + QString::number(height);

        if (text)
        {
            displayedString += " (";
            displayedString += text;
            displayedString += ")";
        }
    }

    QString displayedString;
    int width;
    int height;
};

// new global log/message handler:
void logMessageOutput( QtMsgType type, const char *msg )
{
        QString sMsg = msg;
        if(sMsg.isEmpty()) return;
        if(sMsg.length() >= 8195) sMsg.truncate(8195);

        Rockete *mw = Rockete::instance;

#if defined(Q_CC_MSVC) || defined(Q_CC_MSVC_NET)
        OutputDebugString( (sMsg + "\n"). ucs2() );
#endif

        QTextDocument text; text.setPlainText(sMsg); sMsg = text.toHtml(); // make special chars printable in html
        switch ( type )
        {
        case QtDebugMsg:
                fprintf( stderr, "%s Debug: %s \n", QTime::currentTime().toString().toAscii().data(), msg );
                sMsg = "<font color=blue>"+sMsg+"</font>";
                mw->logMessage(""+sMsg);
                break;
        case QtCriticalMsg:
                fprintf( stderr, "%s Critical: %s \n", QTime::currentTime().toString().toAscii().data(), msg );
                sMsg = "<font color=red><b>"+sMsg+"</></font>";
                mw->logMessage("<b>Critical:</b> "+sMsg);
                break;
        case QtWarningMsg:
                fprintf( stderr, "%s Warning: %s \n", QTime::currentTime().toString().toAscii().data(), msg );
                sMsg = "<font color=red>"+sMsg+"</font>";
                mw->logMessage("<b>Warning:</b> "+sMsg);
                break;
        case QtFatalMsg:
                fprintf( stderr, "%s Fatal: %s \n", QTime::currentTime().toString().toAscii().data(), msg );
                mw->logMessage("Fatal: "+sMsg);
                abort();
        }
}


Rockete::Rockete(QWidget *parent, Qt::WFlags flags)
    : QMainWindow(parent, flags), isReloadingFile(false)
{
    qInstallMsgHandler( logMessageOutput );

    loadPlugins();

    instance = this;

    ui.setupUi(this);
    if (!selectedTextureTmp.open())
        qDebug() << "Failed to save image in temporary file.";

#ifdef Q_WS_MACX
    setAttribute(Qt::WA_MacSmallSize);
    ui.labelCuttingDim->setAttribute(Qt::WA_MacSmallSize);
    ui.labelCuttingDimLabel->setAttribute(Qt::WA_MacSmallSize);
    ui.labelCuttingVCapLabel->setAttribute(Qt::WA_MacSmallSize);
    ui.labelCuttingHCapLabel->setAttribute(Qt::WA_MacSmallSize);
    ui.labelCuttingMask->setAttribute(Qt::WA_MacSmallSize);
    ui.checkBoxCuttingMask->setAttribute(Qt::WA_MacSmallSize);
#endif

    ui.currentToolTab->setLayout(new QGridLayout());

    connect(ui.menuRecent, SIGNAL(triggered(QAction*)), this, SLOT(menuRecentFileClicked(QAction*)));
    generateMenuRecent();

    renderingView = ui.renderingView;
    addRulers();
    renderingView->setRulers(horzRuler, vertRuler);

    // Toolbar.
    ui.mainToolBar->addAction(ui.actionNew_project);
    ui.mainToolBar->addAction(ui.actionOpen);
    ui.mainToolBar->addAction(ui.actionSave);
    ui.mainToolBar->addSeparator();
    ui.mainToolBar->addAction(ui.actionUndo);
    ui.mainToolBar->addAction(ui.actionRedo);
    ui.mainToolBar->addSeparator();
    ui.mainToolBar->addAction(ui.actionReload);
    ui.mainToolBar->addSeparator();
    ui.mainToolBar->addAction(ui.actionZoom_in);
    ui.mainToolBar->addAction(ui.actionZoom_out);

    ui.mainToolBar->addSeparator();

    if( QApplication::arguments().count() > 1 )
    {
        openProject(QApplication::arguments()[1].toAscii().data());
    }
    else if(!Settings::getProject().isEmpty() && QFile::exists(Settings::getProject()))
    {
        openProject(Settings::getProject().toUtf8().constData());
    }
    else
    {
        newProject();
    }

    ToolManager::getInstance().initialize();
    EditionHelper::initialize();

    ToolManager::getInstance().setup(ui.mainToolBar, ui.menuTools);

    attributeTreeModel = new AttributeTreeModel();
    propertyTreeModel = new PropertyTreeModel();

    languageBox = new QComboBox(this);
    languageBox->setEditable(false);
    languageBox->setInsertPolicy( QComboBox::InsertAlphabetically );

    if(LocalizationManagerInterface::hasInstance())
    {
        foreach(LocalizationManagerInterface::LocalizationLanguage language, LocalizationManagerInterface::getInstance().getSupportedLanguages())
        {
            languageBox->addItem(LocalizationManagerInterface::getInstance().getLanguageNameForLanguage(language), (int)language);
        }

        ui.statusBar->showMessage( "Localization Activated", 10000 );
    }

    connect(languageBox, SIGNAL(activated(const QString&)), (QObject*)this, SLOT(languageBoxActivated()));
    ui.mainToolBar->addSeparator();
    ui.mainToolBar->addWidget(languageBox);


    QAction *new_action = new QAction(QIcon(":/images/new_button.png"), "create button", this);
    connect(new_action, SIGNAL(triggered()), (QObject*)this, SLOT(newButtonWizardActivated()));
    ui.mainToolBar->addAction(new_action);
    

    labelZoom = new QLabel(parent);
    labelZoom->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    labelZoom->setText("Zoom: 100%");
    labelPos = new QLabel(parent);
    labelPos->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    labelPos->setText("Cursor: -/-");

    ui.statusBar->addPermanentWidget(labelPos);
    ui.statusBar->addPermanentWidget(labelZoom);

    renderingView->setPosLabel(labelPos);

    fileWatcher = new QFileSystemWatcher();

    setFocusPolicy(Qt::StrongFocus);
    wizard = NULL;

    connect(fileWatcher, SIGNAL(fileChanged(const QString &)), (QObject*)this, SLOT(fileHasChanged(const QString &)));

    hierarchyEventFilter = new DocumentHierarchyEventFilter();
    ui.documentHierarchyTreeWidget->viewport()->installEventFilter(hierarchyEventFilter);

    ui.snippetsListWidget->initialize();

    QShortcut *triggerFindNext = new QShortcut(QKeySequence::FindNext, this);
    connect(triggerFindNext, SIGNAL(activated()), (QObject*)this, SLOT(findNextTriggered()));

    QShortcut *triggerFindPrevious = new QShortcut(QKeySequence::FindPrevious, this);
    connect(triggerFindPrevious, SIGNAL(activated()), (QObject*)this, SLOT(findPreviousTriggered()));

    QShortcut *triggerFind = new QShortcut(QKeySequence::Find, this);
    connect(triggerFind, SIGNAL(activated()), (QObject*)this, SLOT(findTriggered()));

    // texture preview:
    connect(ui.texturePreviewLabel, SIGNAL(resizeLabel(QResizeEvent*)), (QObject*)this, SLOT(resizeTexturePreview(QResizeEvent*)));

    // cutting panel:
    labelCuttingMask = new QLabel(ui.tabWidgetCuttingPreview);
    const QPoint origin = ui.tabImage->mapTo(ui.tabWidgetCuttingPreview, ui.labelCuttingPreview->geometry().topLeft());
    labelCuttingMask->setGeometry(origin.x(), origin.y(), ui.labelCuttingPreview->geometry().width(), ui.labelCuttingPreview->geometry().height());
    connect(ui.labelCuttingPreview, SIGNAL(resizeLabel(QResizeEvent*)), (QObject*)this, SLOT(resizeCuttingPreview(QResizeEvent*)));
    connect(ui.spinCuttingHCap, SIGNAL(valueChanged(int)), (QObject*)this, SLOT(spinCuttingChanged(int)));
    connect(ui.spinCuttingVCap, SIGNAL(valueChanged(int)), (QObject*)this, SLOT(spinCuttingChanged(int)));
    connect(ui.tabWidgetCuttingPreview, SIGNAL(currentChanged(int)), (QObject*)this, SLOT(cuttingPreviewTabChange(int)));

    // mask alpha:
    connect(ui.horizontalCuttingPreviewSize, SIGNAL(valueChanged(int)), (QObject*)this, SLOT(cuttingPrevSizeChanged(int)));
    connect(ui.checkBoxCuttingMask, SIGNAL(toggled(bool)), this, SLOT(cuttingMaskToggle(bool)));

    ui.searchReplaceDockWidget->hide();

    // set initial tab:
    ui.bottomTabWidget->setCurrentIndex(0);
    ui.tabWidget->setCurrentIndex(0);
    ui.tabWidgetDoc->setCurrentIndex(0);
    ui.tabWidgetCuttingPreview->setCurrentIndex(0);
}

Rockete::~Rockete()
{
    delete attributeTreeModel;
    delete propertyTreeModel;
    delete labelCuttingMask;
}

void Rockete::repaintRenderingView()
{
    renderingView->repaint();
}

void Rockete::fillAttributeView()
{
    if(getCurrentDocument())
    {
        attributeTreeModel->setupData(getCurrentDocument(), getCurrentDocument()->selectedElement);
        ui.attributeTreeView->reset();
        ui.attributeTreeView->setModel(attributeTreeModel);
        ui.attributeTreeView->header()->setResizeMode(0, QHeaderView::ResizeToContents);
        ui.attributeTreeView->header()->setResizeMode(1, QHeaderView::Stretch);
        ui.attributeTreeView->header()->setResizeMode(2, QHeaderView::ResizeToContents);
        ui.attributeTreeView->header()->setStretchLastSection(false);
    }
}

void Rockete::fillPropertyView()
{
    if(getCurrentDocument())
    {
        propertyTreeModel->setupData(getCurrentDocument(), getCurrentDocument()->selectedElement);
        ui.propertyTreeView->reset();
        ui.propertyTreeView->setModel(propertyTreeModel);
        ui.propertyTreeView->expandAll();
        ui.propertyTreeView->header()->setResizeMode(0, QHeaderView::ResizeToContents);
        ui.propertyTreeView->header()->setResizeMode(1, QHeaderView::Stretch);
        //ui.propertyTreeView->header()->setResizeMode(2, QHeaderView::ResizeToContents);
        //ui.propertyTreeView->header()->setStretchLastSection(false);
    }
}

void Rockete::selectElement(Element *element)
{
    //if(element != getCurrentDocument()->selectedElement)
    {
        getCurrentDocument()->selectedElement = element;
        repaintRenderingView();
        fillAttributeView();
        fillPropertyView();
        
        foreach(QTreeWidgetItem *item, ui.documentHierarchyTreeWidget->findItems(element->GetTagName().CString(), Qt::MatchRecursive))
        {
            if((Element *)item->data(0,Qt::UserRole).value<void*>() == element)
            {
                ui.documentHierarchyTreeWidget->setCurrentItem(item);
                break;
            }
        }
    }
}

void Rockete::reloadCurrentDocument()
{
    if (getCurrentDocument())
    {
        renderingView->reloadDocument();
        selectedTreeViewItem = NULL;
        getCurrentDocument()->populateHierarchyTreeView(ui.documentHierarchyTreeWidget);
    }
}

int Rockete::getTabIndexFromFileName(const char * name)
{
    for (int i = 0; i < ui.codeTabWidget->count(); ++i)
    {
        if (ui.codeTabWidget->tabText(i) == name)
        {
            return i;
        }
    }

    return -1;
}

OpenedDocument *Rockete::getCurrentTabDocument(int index)
{
    if(index < 0)
        index = ui.codeTabWidget->currentIndex();

    if (OpenedDocument *file = qobject_cast<OpenedDocument *>(ui.codeTabWidget->widget(index)))
        return file;

    return NULL;
}

OpenedStyleSheet *Rockete::getCurrentTabStyleSheet(int index)
{
    if(index < 0)
        index = ui.codeTabWidget->currentIndex();

    if (OpenedStyleSheet *file = qobject_cast<OpenedStyleSheet *>(ui.codeTabWidget->widget(index)))
        return file;

    return NULL;
}

OpenedFile *Rockete::getOpenedFile(const char * file_path, const bool try_to_open)
{
    QFileInfo file_info(file_path);

    for (int i = 0; i < ui.codeTabWidget->count(); ++i)
    {
        OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->widget(i));
        if (file && file->fileInfo.fileName() == file_info.fileName())
        {
            return file;
        }
    }

    if (try_to_open)
    {
        openFile(file_info.filePath());
        return getOpenedFile(file_path);
    }

    return NULL;
}

QString Rockete::getPathForFileName(const QString &filename)
{
    QTreeWidgetItem *item;
    QString searched_string;
    QList<QTreeWidgetItem*> items_found;

    searched_string = filename;
    if(!filename.contains("."))
        searched_string += ".";
    
    items_found = ui.projectFilesTreeWidget->findItems(searched_string, Qt::MatchStartsWith | Qt::MatchRecursive, 1);

    if(!items_found.isEmpty())
    {
        item = items_found.first();
        return item->data(0,Qt::UserRole).toString();
    }
    
    return filename;
}

void Rockete::setZoomLevel(float level)
{
    QString string_level;

    string_level.setNum(level * 100.0f);
    string_level += "%";

    labelZoom->setText("Zoom: "+string_level);

    horzRuler->setRulerZoom(level);
    vertRuler->setRulerZoom(level);
}

// Public slots:

void Rockete::menuOpenClicked()
{
    QString file_path = QFileDialog::getOpenFileName(this, tr("Open libRocket file..."), "", tr("libRocket files (*.rml *.rcss);;Rockete project(*.rproj)"));

    if (!file_path.isEmpty())
    {
        openFile(file_path);
    }
}

void Rockete::menuNewProjectClicked()
{
    newProject();
}

void Rockete::menuOpenProjectClicked()
{
    QString file_path = QFileDialog::getOpenFileName(this, tr("Open Rockete project..."), "", tr("libRocket project (*.rproj)"));

    if (!file_path.isEmpty() && QFile::exists(file_path))
    {
        openProject(file_path.toUtf8().constData());
    }
}

void Rockete::menuSaveProjectClicked()
{
    QString file_path = QFileDialog::getSaveFileName(this, tr("Open Rockete project..."), "", tr("libRocket project (*.rproj)"));

    if (!file_path.isEmpty())
    {
        saveProject(file_path.toUtf8().constData());
    }
    // reload project from new path:
    openProject(file_path.toUtf8().constData());
}

void Rockete::menuReloadProjectClicked()
{

}

void Rockete::menuSaveClicked()
{
    if(ui.codeTabWidget->count()==0)
        return;

    OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->currentWidget());

    Q_ASSERT(file);

    file->save();
    ui.codeTabWidget->setTabText(ui.codeTabWidget->currentIndex(), file->fileInfo.fileName());
}

void Rockete::menuSaveAsClicked()
{
    if(ui.codeTabWidget->count()==0)
        return;

    OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->currentWidget());

    Q_ASSERT(file);
    QString file_path = QFileDialog::getSaveFileName( this, tr( "Save as..." ), "", tr("libRocket Markup Language (*.rml);;libRocket CSS (*.rcss)") );

    file->saveAs(file_path);
    openFile(file_path);
}

void Rockete::menuCloseClicked()
{
    codeTabRequestClose(ui.codeTabWidget->currentIndex());
}

void Rockete::codeTextChanged()
{
    QString tab_text = ui.codeTabWidget->tabText(ui.codeTabWidget->currentIndex());
    if (!tab_text.startsWith("*"))
    {
        ui.codeTabWidget->setTabText(ui.codeTabWidget->currentIndex(), "*" + tab_text);
    }
}

void Rockete::codeTabChanged( int index )
{
    if(index < 0 || isReloadingFile)
    {
        return;
    }

    OpenedDocument *document;
    if ((document = getCurrentTabDocument(index)))
    {
        if(document != getCurrentDocument())
        {
            renderingView->changeCurrentDocument(document);
            selectedTreeViewItem = NULL;
            getCurrentDocument()->populateHierarchyTreeView(ui.documentHierarchyTreeWidget);
        }
    }

    ui.snippetsListWidget->filterSnippetsForLanguage(ui.codeTabWidget->tabText(ui.codeTabWidget->currentIndex()).split(".").at(1));
}

void Rockete::codeTabRequestClose(int index)
{
    int
        response;

    if(ui.codeTabWidget->tabText(index).contains("*"))
    {
        OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->widget(0));
        QString question;
        question = "Save ";
        question += file->fileInfo.absoluteFilePath();
        question += " before closing?";

        response = QMessageBox::question(this, "file modified", question, QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
    }
    else // if we have no changes, save anyway. I don't want changes to be lost if rockete simply didn't detect a change
    {
        response = QMessageBox::Yes;
    }

    if(response==QMessageBox::Yes)
        closeTab(index,true);
    else if(response==QMessageBox::No)
        closeTab(index,false);
}

void Rockete::unselectElement()
{
    if (!getCurrentDocument())
    {
        return;
    }
    getCurrentDocument()->selectedElement = NULL;
    fillAttributeView();
    fillPropertyView();
    ToolManager::getInstance().getCurrentTool()->onUnselect();
    repaintRenderingView();
}

void Rockete::menuReloadClicked()
{
    reloadCurrentDocument();
}

void Rockete::menuSetScreenSizeClicked()
{
    QList<LocalScreenSizeItem*> item_list;
    QStringList item_string_list;
    int index_to_select = 0;

    item_list.push_back(new LocalScreenSizeItem(480, 320, "iPhone3"));
    item_list.push_back(new LocalScreenSizeItem(960, 640, "iPhone4"));
    item_list.push_back(new LocalScreenSizeItem(800, 480, "WVGA "));
    item_list.push_back(new LocalScreenSizeItem(800, 600));
    item_list.push_back(new LocalScreenSizeItem(1024, 768));
    item_list.push_back(new LocalScreenSizeItem(1136, 640, "iPhone5"));
    item_list.push_back(new LocalScreenSizeItem(1280, 720));
    item_list.push_back(new LocalScreenSizeItem(1600, 1024));
    item_list.push_back(new LocalScreenSizeItem(1920, 1080, "HD"));
    item_list.push_back(new LocalScreenSizeItem(2048, 1536, "iPad3"));

    for (int i=0; i<item_list.size(); ++i)
    {
        LocalScreenSizeItem *item = item_list[i];
        item_string_list << item->displayedString;
        if(item->width == RocketSystem::getInstance().getContext()->GetDimensions().x && item->height == RocketSystem::getInstance().getContext()->GetDimensions().y)
        {
            index_to_select = i;
        }
    }

    bool ok;
    QString item_selected = QInputDialog::getItem(this, tr("Set screen size..."), tr("Size: "), item_string_list, index_to_select, false, &ok);
    if (ok && !item_selected.isEmpty())
    {
        foreach (LocalScreenSizeItem * item, item_list)
        {
            if (item_selected == item->displayedString)
            {
                RocketSystem::getInstance().createContext(item->width, item->height);
                if(getCurrentDocument())
                {
                    getCurrentDocument()->rocketDocument = NULL;
                    reloadCurrentDocument();
                }
                break;
            }
        }
    }

    foreach (LocalScreenSizeItem * item, item_list)
    {
       delete item;
    }
}

void Rockete::menuLoadFonts()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Open font files...",
        NULL,
        "OTF/TTF files (*.otf *.ttf)"
        );

    foreach (const QString & file, files)
    {
        RocketSystem::getInstance().loadFont(file);
    }
}

void Rockete::menuUndoClicked()
{
    ActionManager::getInstance().applyPrevious();
    fillAttributeView();
}

void Rockete::menuRedoClicked()
{
    ActionManager::getInstance().applyNext();
    fillAttributeView();
}

void Rockete::menuReloadAssetsClicked()
{
    RocketHelper::unloadAllDocument();
    for(int i = 0; i < ui.codeTabWidget->count(); i++)
    {
        OpenedDocument *document;
        if ((document = getCurrentTabDocument(i)))
        {
            document->selectedElement = NULL;
            document->rocketDocument = RocketHelper::loadDocumentFromMemory(document->toPlainText());
            document->rocketDocument->RemoveReference();
        }
    }
}

void Rockete::menuFormatTextClicked()
{
    if(ui.codeTabWidget->count()==0)
        return;

    QDomDocument dom_document;
    OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->currentWidget());
    Q_ASSERT(file);

    if (!file->fileInfo.fileName().contains("rml"))
    {
        return;
    }

    dom_document.setContent(file->toPlainText());
    QString reformatted_content = dom_document.toString(Settings::getTabSize());
    file->setTextEditContent(reformatted_content, true);
}

void Rockete::propertyViewClicked(const QModelIndex &/*index*/)
{
}

void Rockete::attributeViewClicked(const QModelIndex &index)
{
    if (index.column() == 2 && index.internalPointer())
    {
        QString result;
        if (reinterpret_cast<EditionHelper*>(index.internalPointer())->help(result))
        {
            QModelIndex dataIndex;
            dataIndex = ui.attributeTreeView->model()->index(index.row(), 1, QModelIndex());
            ui.attributeTreeView->model()->setData(dataIndex, QVariant(result));
        }
    }
}

void Rockete::menuRecentFileClicked(QAction *action)
{
    openFile(action->text());
}

void Rockete::menuBackgroundChangeColor()
{
    QColor color = QColorDialog::getColor();

    renderingView->SetClearColor( color.redF(), color.greenF(), color.blueF(), 1.0f );
}

void Rockete::menuBackgroundChangeImage()
{
    QString file_path = QFileDialog::getOpenFileName(this, tr("Open image file..."), "", tr("Image files (*.png *.tga)")); // support jpeg?

    if (!file_path.isEmpty())
    {
        Settings::setBackroundFileName( file_path );
        repaintRenderingView();
    }
}

void Rockete::languageBoxActivated()
{
    if(LocalizationManagerInterface::hasInstance())
        LocalizationManagerInterface::getInstance().setLanguage((LocalizationManagerInterface::LocalizationLanguage)languageBox->itemData(languageBox->currentIndex()).toInt());
}

void Rockete::newButtonWizardActivated()
{
    if(!getCurrentDocument())
        return;
    if(wizard)
        delete(wizard);
    wizard = new WizardButton(this);
    wizard->setModal(true);
    wizard->show();
}

void Rockete::fileTreeDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    if(item->text(1).endsWith("rml") || item->text(1).endsWith("rcss") || item->text(1).endsWith("txt") || item->text(1).endsWith("rproj") || item->text(1).endsWith("lua") || item->text(1).endsWith("snippet"))
    {
        openFile(item->text(1));
    }
}

void Rockete::fileTreeClicked(QTreeWidgetItem *item, int /*column*/)
{
    QClipboard *clipboard = QApplication::clipboard();
    if(item->text(1).endsWith("png")||item->text(1).endsWith("jpg")) // no native support for tga
    {
        if (ui.tabWidgetDoc->currentIndex() != kTexturePreviewTabIndex) ui.texturePreviewLabel->updateGeometry(); // update size of preview if it is hidden
        QString data = item->data(1, Qt::UserRole).toString();
        if (data.length()) {
            QString texture = item->data(0, Qt::UserRole).toString();
            QImage image(texture);
            int l=0,b=0,w=0,h=0; int n = sscanf(data.toAscii().constData(), "%dpx %dpx %dpx %dpx", &l, &b, &w, &h); if (n!=4)
                qWarning() << "Invalid format: " << data;
            selectedTexture = image.copy( l, b, w, h);
            selectedTexture.save(selectedTextureTmp.fileName(), "png");
            ui.texturePreviewLabel->setPixmap(QPixmap::fromImage(selectedTexture.scaled(QSize(ui.texturePreviewLabel->width(),ui.texturePreviewLabel->height()),Qt::KeepAspectRatio)));
            clipboard->setText(QFileInfo(texture).fileName() + " " + data); // place texture coordinates in clipboard
            updateCuttingTab(QFileInfo(texture).fileName(), l, b, w, h);
        } else {
            selectedTexture = QImage( getPathForFileName(item->text(1)) );
            selectedTexture.save(selectedTextureTmp.fileName(), "png");
            ui.texturePreviewLabel->setPixmap(QPixmap::fromImage(selectedTexture.scaled(QSize(ui.texturePreviewLabel->width(),ui.texturePreviewLabel->height()),Qt::KeepAspectRatio)));
            clipboard->setText(item->text(1)); // place filename
            updateCuttingTab(item->text(1), 0, 0, selectedTexture.width(), selectedTexture.height());
        }
    }
}

void Rockete::resizeTexturePreview(QResizeEvent * event)
{
    if (!selectedTexture.isNull()) {
        ui.texturePreviewLabel->setPixmap(QPixmap::fromImage(selectedTexture.scaled(QSize(ui.texturePreviewLabel->width(),ui.texturePreviewLabel->height()),Qt::KeepAspectRatio)));
    }
}

void Rockete::updateCuttingTab(const QString &file, int l, int b, int w, int h)
{
    ui.labelCuttingDim->setText(QString("x:%1px y:%2px w:%3px h:%4px").arg(l).arg(b).arg(w).arg(h));
    ui.cuttingLog->append(QString("<b>File selected %1</b><br/>").arg(file));
    updateCuttingInfo(ui.spinCuttingHCap->value(), ui.spinCuttingVCap->value());
}

void Rockete::spinCuttingChanged(int value)
{
    updateCuttingInfo(ui.spinCuttingHCap->value(), ui.spinCuttingVCap->value());
}

void Rockete::updateCuttingInfo(int hvalue, int vvalue)
{
    int l, b, w, h;
    int n = sscanf(ui.labelCuttingDim->text().toAscii().constData(), "x:%dpx y:%dpx w:%dpx h:%dpx", &l, &b, &w, &h); if (n!=4) {
        ui.cuttingLog->append(QString("<font color=red>Invalid data format.</font color=red><br/>"));
    } else {
        if (vvalue != 0 || hvalue != 0) {
            if (vvalue == 0) {
                ui.cuttingLog->append(QString("<b><font color=blue>Cutting for h-cap %1px:</font></b><br/>").arg(hvalue));
                ui.cuttingLog->append(QString("&nbsp;<b>left-image:</b> %1px %2 %3px %4px;<br/>").arg(l).arg(b).arg(l+hvalue).arg(b+h));
                ui.cuttingLog->append(QString("&nbsp;<b>center-image:</b> %1px %2px %3 %4px;<br/>").arg(l+hvalue).arg(b).arg(l+w-hvalue).arg(b+h));
                ui.cuttingLog->append(QString("&nbsp;<b>right-image:</b> %1px %2px %3 %4px;<br/>").arg(l+w-hvalue).arg(b).arg(l+w).arg(b+h));
            } else if(hvalue == 0) {
                ui.cuttingLog->append(QString("<b><font color=blue>Cutting for v-cap %1px:</font></b><br/>").arg(vvalue));
                ui.cuttingLog->append(QString("&nbsp;<b>top-image:</b> %1px %2 %3px %4px;<br/>").arg(l).arg(b+h-vvalue).arg(l+w).arg(b+h));
                ui.cuttingLog->append(QString("&nbsp;<b>center-image:</b> %1px %2px %3 %4px;<br/>").arg(l).arg(b+vvalue).arg(l+w).arg(b+h-vvalue));
                ui.cuttingLog->append(QString("&nbsp;<b>bottom-image:</b> %1px %2px %3 %4px;<br/>").arg(l).arg(b).arg(l+w).arg(b+vvalue));
            } else {
                ui.cuttingLog->append(QString("<b><font color=blue>Cutting for cap %1px x %2px:</font></b><br/>").arg(hvalue).arg(vvalue));
                ui.cuttingLog->append(QString("&nbsp;<b>top-left-image:</b> %1px %2 %3px %4px;<br/>").arg(l).arg(b).arg(l+hvalue).arg(b+h));
                ui.cuttingLog->append(QString("&nbsp;<b>top-image:</b> %1px %2 %3px %4px;<br/>").arg(l+hvalue).arg(b).arg(l+w-hvalue).arg(b+h));
                ui.cuttingLog->append(QString("&nbsp;<b>top-right-image:</b> %1px %2 %3px %4px;<br/>").arg(l+w-hvalue).arg(b).arg(l+w).arg(b+h));
                ui.cuttingLog->append(QString("&nbsp;<b>left-image:</b> %1px %2 %3px %4px;<br/>").arg(l).arg(b).arg(l+hvalue).arg(b+h));
                ui.cuttingLog->append(QString("&nbsp;<b>center-image:</b> %1px %2px %3 %4px;<br/>").arg(l+hvalue).arg(b).arg(l+w-hvalue).arg(b+h));
                ui.cuttingLog->append(QString("&nbsp;<b>right-image:</b> %1px %2px %3 %4px;<br/>").arg(l+w-hvalue).arg(b).arg(l+w).arg(b+h));
                ui.cuttingLog->append(QString("&nbsp;<b>bottom-left-image:</b> %1px %2 %3px %4px;<br/>").arg(l).arg(b).arg(l+hvalue).arg(b+h));
                ui.cuttingLog->append(QString("&nbsp;<b>bottom-image:</b> %1px %2 %3px %4px;<br/>").arg(l+hvalue).arg(b).arg(l+w-hvalue).arg(b+h));
                ui.cuttingLog->append(QString("&nbsp;<b>bottom-right-image:</b> %1px %2 %3px %4px;<br/>").arg(l+w-hvalue).arg(b).arg(l+w).arg(b+h));
            }
            if (selectedTextureTmp.isOpen()) {
                labelCuttingMask->setStyleSheet(QString(
                    "border-width: %3px %2px %3px %2px;"
                    "border-image: url(':/images/border-image.png') 10 10 10 10 stretch stretch;").arg(hvalue).arg(vvalue) /* top, right, bottom, left */
                    );
                ui.labelCuttingPreview->setStyleSheet(QString(
                    "border-width: %3px %2px %3px %2px;"
                    "border-image: url('%1') %3 %2 %3 %2 stretch stretch;").arg(selectedTextureTmp.fileName()).arg(hvalue).arg(vvalue) /* top, right, bottom, left */
                    );
                float aspect = (float)w/(float)h;
                if (h>w) {
                    // disable vertical spacers:
                    ui.verticalSpacer1->changeSize(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored);
                    ui.verticalSpacer2->changeSize(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored);
                    // enable horizontal spacers:
                    ui.horizontalSpacer1->changeSize(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
                    ui.horizontalSpacer2->changeSize(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
                    // --
                } else {
                    // disable horizontal spacers:
                    ui.horizontalSpacer1->changeSize(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored);
                    ui.horizontalSpacer2->changeSize(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored);
                    // enable vertical spacers:
                    ui.verticalSpacer1->changeSize(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
                    ui.verticalSpacer2->changeSize(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
                    // --
                }
            }
        } else {
            ui.labelCuttingPreview->setStyleSheet("");
            labelCuttingMask->setStyleSheet("");
        }
    }
}

void Rockete::resizeCuttingPreview(QResizeEvent * event)
{
    // resize mask view:
    //labelCuttingMask->setGeometry(ui.labelCuttingPreview->geometry());
    const QPoint origin = ui.tabImage->mapTo(ui.tabWidgetCuttingPreview, ui.labelCuttingPreview->geometry().topLeft());
    labelCuttingMask->setGeometry(origin.x(), origin.y(), ui.labelCuttingPreview->geometry().width(), ui.labelCuttingPreview->geometry().height());
}

void Rockete::cuttingPrevSizeChanged(int value)
{
}

void Rockete::cuttingMaskToggle(bool value)
{
    if (value)
        labelCuttingMask->show();
    else
        labelCuttingMask->hide();
}

void Rockete::cuttingPreviewTabChange(int tab)
{
    // workaround for problem with mask's transparency
    if (tab == kCuttingImagePreviewTabIndex) {
        if (ui.checkBoxCuttingMask->checkState() == Qt::Unchecked)
            labelCuttingMask->hide();
        else
            labelCuttingMask->show();
    } else
        labelCuttingMask->hide();
}

void Rockete::documentHierarchyDoubleClicked(QTreeWidgetItem *item, int/* column*/)
{
    QString elementId;
    QTreeWidgetItem *nextItem = NULL;
    int parentCount = 0;
    QStringList rcssList;

    selectElement((Element *)item->data(0,Qt::UserRole).value<void*>());
    rcssList = getCurrentDocument()->getRCSSFileList();

    if(hierarchyEventFilter->ShiftPressed)
    {
        //getCurrentDocument()->selectedElement->;
        QStringList strings_to_find;
        QString string_to_find;

        ui.statusBar->clearMessage();

        // "full" definition by id
        if(!item->text(1).isEmpty())
        {
            string_to_find = item->text(0);
            string_to_find += "#";
            string_to_find += item->text(1);

            strings_to_find << string_to_find;
        }

        // specific definition by class
        if(!item->text(2).isEmpty())
        {
            foreach(QString rcss_class, item->text(2).split(' ')){
                string_to_find = item->text(0);
                string_to_find += ".";
                string_to_find += rcss_class;

                strings_to_find << string_to_find;
            }
        }

        // class definition
        if(!item->text(2).isEmpty())
        {
            foreach(QString rcss_class, item->text(2).split(' ')){
                string_to_find = ".";
                string_to_find += rcss_class;

                strings_to_find << string_to_find;
            }
        }

        // tag definition
        strings_to_find << item->text(0);

        foreach(QString search_string, strings_to_find) {
            foreach (QString file, rcssList) {
                OpenedFile *opened_file = getOpenedFile(file.toAscii().data(), true);

                QTextCursor cursor = opened_file->textCursor();
                cursor.clearSelection();
                opened_file->setTextCursor(cursor);

                if(opened_file->find(search_string) || opened_file->find(search_string, QTextDocument::FindBackward))
                {
                    ui.codeTabWidget->setCurrentIndex(getTabIndexFromFileName(opened_file->fileInfo.fileName().toAscii().data()));
                    //opened_file->cursorFind(search_string, true);
                    return;
                }
            }
        }

        ui.statusBar->showMessage( "Definition in RCSS not found, it is either in a template file or not defined", 5000 );

        return;
    }

    ui.statusBar->clearMessage();
    QTextCursor cursor = getCurrentDocument()->textCursor();
    cursor.clearSelection();
    getCurrentDocument()->setTextCursor(cursor);
    ui.codeTabWidget->setCurrentIndex(getTabIndexFromFileName(getCurrentDocument()->fileInfo.fileName().toAscii().data()));

    if(selectedTreeViewItem)
    {
        selectedTreeViewItem->setTextColor(0, QColor::fromRgb(0,0,0));
    }

    nextItem = item;
    do 
    {
        elementId = nextItem->text(1);
        
        if(elementId.isEmpty() && ( !nextItem->parent()))
        {
            ui.statusBar->showMessage( "No ID", 3000 );
            return;
        }
        
        if(elementId.isEmpty())
        {
            nextItem = nextItem->parent();
        }

        parentCount++;
    } while (elementId.isEmpty());

    elementId = "id=\"" + elementId;
    elementId += "\"";

    if(!getCurrentDocument()->find(elementId) && !getCurrentDocument()->find(elementId,QTextDocument::FindBackward))
    {
        QString message = "This (" + QString::number(parentCount - 1);
        message += " higher) ID is in another file";
        ui.statusBar->showMessage( message, 3000 );
        return;
    }

    selectedTreeViewItem = nextItem;
    selectedTreeViewItem->setTextColor(0, QColor::fromRgb(0,255,0));

    if(parentCount>1)
    {
        QString message = "ID not set, went " + QString::number(parentCount - 1);
        message += " parent(s) higher";
        ui.statusBar->showMessage(message, 3000);
    }
}

void Rockete::fileHasChanged(const QString &path)
{
    QFileInfo file_info = path;
    int tab_index = getTabIndexFromFileName(file_info.fileName().toAscii().data());
    fileChangedOutsideArray[tab_index] = path;
}

void Rockete::findTriggered()
{
    ui.searchReplaceDockWidget->show();
    ui.searchReplaceDockWidget->activateWindow();
    ui.searchComboBox->setFocus(Qt::OtherFocusReason);
}

void Rockete::findPreviousTriggered()
{
    if(ui.codeTabWidget->count()==0)
        return;

    OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->currentWidget());
    Q_ASSERT(file);

    bool found_string;

    file->highlightString(ui.searchComboBox->currentText());

    if(!ui.searchComboBox->currentText().isEmpty())
    {
        if(ui.matchCaseCheckBox->isChecked())
        {
            found_string = file->find(ui.searchComboBox->currentText(), QTextDocument::FindBackward | QTextDocument::FindCaseSensitively);
        }
        else
        {
            found_string = file->find(ui.searchComboBox->currentText(), QTextDocument::FindBackward);
        }
    }
    else
    {
        return;
    }

    if(!found_string)
    {
        QTextCursor previousCursor = file->textCursor();
        QTextCursor findCursor = file->textCursor();
        findCursor.movePosition(QTextCursor::End);
        file->setTextCursor(findCursor);

        if(ui.matchCaseCheckBox->isChecked())
        {
            found_string = file->find(ui.searchComboBox->currentText(), QTextDocument::FindBackward | QTextDocument::FindCaseSensitively);
        }
        else
        {
            found_string = file->find(ui.searchComboBox->currentText(), QTextDocument::FindBackward);
        }

        if(!found_string)
            file->setTextCursor(previousCursor);
    }
}

void Rockete::findNextTriggered()
{
    if(ui.codeTabWidget->count()==0)
        return;

    OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->currentWidget());
    Q_ASSERT(file);

    bool found_string;
    file->highlightString(ui.searchComboBox->currentText());

    if(!ui.searchComboBox->currentText().isEmpty())
    {
        if(ui.matchCaseCheckBox->isChecked())
        {
            found_string = file->find(ui.searchComboBox->currentText(), QTextDocument::FindCaseSensitively);
        }
        else
        {
            found_string = file->find(ui.searchComboBox->currentText());
        }
    }
    else
    {
        return;
    }

    if(!found_string)
    {
        QTextCursor previousCursor = file->textCursor();
        QTextCursor findCursor = file->textCursor();
        findCursor.setPosition(0);
        file->setTextCursor(findCursor);

        if(ui.matchCaseCheckBox->isChecked())
        {
            found_string = file->find(ui.searchComboBox->currentText(), QTextDocument::FindCaseSensitively);
        }
        else
        {
            found_string = file->find(ui.searchComboBox->currentText());
        }

        if(!found_string)
            file->setTextCursor(previousCursor);
    }
}

void Rockete::replaceTriggered()
{
    if(ui.codeTabWidget->count()==0)
        return;

    OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->currentWidget());
    Q_ASSERT(file);

    if(file->textCursor().hasSelection())
    {
        file->textCursor().insertText(ui.replaceComboBox->currentText());
    }

    checkTextChanged(-1);

    // return; here to behave like a "real" replace instead of a "replace & find"

    if(!file->find(ui.searchComboBox->currentText(),(ui.matchCaseCheckBox->isChecked() ? QTextDocument::FindCaseSensitively : (QTextDocument::FindFlags)0 )) &&
        file->find(ui.searchComboBox->currentText(), QTextDocument::FindBackward | (ui.matchCaseCheckBox->isChecked() ? QTextDocument::FindCaseSensitively : (QTextDocument::FindFlags)0 )) )
    { // reached end of file but there are more text to find
        QTextCursor findCursor = file->textCursor();
        findCursor.setPosition(0);
        file->setTextCursor(findCursor);
        file->find(ui.searchComboBox->currentText(),(ui.matchCaseCheckBox->isChecked() ? QTextDocument::FindCaseSensitively : (QTextDocument::FindFlags)0 ));
    }
}

void Rockete::replaceAllTriggered()
{
    if(ui.codeTabWidget->count()==0)
        return;

    OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->currentWidget());
    Q_ASSERT(file);

    file->textCursor().beginEditBlock();
    while (file->find(ui.searchComboBox->currentText(),(ui.matchCaseCheckBox->isChecked() ? QTextDocument::FindCaseSensitively : (QTextDocument::FindFlags)0 )) ||
            file->find(ui.searchComboBox->currentText(), QTextDocument::FindBackward | (ui.matchCaseCheckBox->isChecked() ? QTextDocument::FindCaseSensitively : (QTextDocument::FindFlags)0 )))
    {
        if(file->textCursor().hasSelection())
        {
            file->textCursor().insertText(ui.replaceComboBox->currentText());
        }
    }
    file->textCursor().endEditBlock();
    checkTextChanged(-1);
}

void Rockete::replaceAllInAllTriggered()
{
    if(ui.codeTabWidget->count()==0)
        return;

    if(QMessageBox::question(this,"Please confirm", "Are you sure?", QMessageBox::Yes, QMessageBox::Cancel) != QMessageBox::Yes)
        return;

    for(int i = 0; i < ui.codeTabWidget->count(); i++)
    {
        OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->widget(i));
        Q_ASSERT(file);

        file->textCursor().beginEditBlock();
        while (file->find(ui.searchComboBox->currentText(),(ui.matchCaseCheckBox->isChecked() ? QTextDocument::FindCaseSensitively : (QTextDocument::FindFlags)0 )) ||
            file->find(ui.searchComboBox->currentText(), QTextDocument::FindBackward | (ui.matchCaseCheckBox->isChecked() ? QTextDocument::FindCaseSensitively : (QTextDocument::FindFlags)0 )))
        {
            if(file->textCursor().hasSelection())
            {
                file->textCursor().insertText(ui.replaceComboBox->currentText());
            }
        }
        file->textCursor().endEditBlock();
        checkTextChanged(i);
    }
}

void Rockete::addSnippetClicked()
{
    QString new_snippet = ui.snippetsListWidget->addSnippet();
    
    if(new_snippet.isEmpty())
        return;
    
    QFileInfo file_info = ProjectManager::getInstance().getSnippetsFolderPath() + new_snippet;
    QTreeWidgetItem *item = ui.projectFilesTreeWidget->findItems("Snippets", Qt::MatchRecursive).at(0);

    QTreeWidgetItem *new_item = new QTreeWidgetItem(item);
    new_item->setText(1,file_info.fileName());
    new_item->setToolTip(1, file_info.absoluteFilePath());
    new_item->setData(0, Qt::UserRole, file_info.filePath());

    new_item->setIcon(0, QIcon(":/images/icon_snippets.png"));


    item->addChild(new_item);
    ui.projectFilesTreeWidget->addTopLevelItem(item);
    item->sortChildren(1,Qt::AscendingOrder);
}

void Rockete::removeSnippetClicked()
{
    QString file_path = ui.snippetsListWidget->removeSnippet();
    QString file_name = QFileInfo(file_path).fileName();
    if(!file_path.isEmpty())
    {
        if(ui.projectFilesTreeWidget->findItems(file_name, Qt::MatchRecursive, 1).count() > 0)
        {
            Q_ASSERT(ui.projectFilesTreeWidget->findItems(file_name, Qt::MatchRecursive, 1).count() == 1);
            QTreeWidgetItem *item = ui.projectFilesTreeWidget->findItems(file_name, Qt::MatchRecursive, 1).at(0);
            ui.projectFilesTreeWidget->findItems("Snippets", Qt::MatchRecursive).at(0)->removeChild(item);
        }
        if(getTabIndexFromFileName(file_name.toAscii().data()) > -1)
        {
            closeTab(getTabIndexFromFileName(file_name.toAscii().data()), false);
        }
        QFile::remove(file_path);
    }
}

void Rockete::snippetsListDoubleClicked(QListWidgetItem *item)
{
    openFile(((CodeSnippet *)item->data(Qt::UserRole).value<void*>())->FilePath);
}

// Protected:

void Rockete::keyPressEvent(QKeyEvent *event)
{
    renderingView->keyPressEvent(event);
}

void Rockete::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::ActivationChange && isActiveWindow() && !fileChangedOutsideArray.isEmpty())
    {
        int new_tab_index;
        int current_index = ui.codeTabWidget->currentIndex();
        QMapIterator<int, QString> i(fileChangedOutsideArray);
        while (i.hasNext())
        {
            i.next();
            fileChangedOutsideArray.remove(i.key());
            if( QMessageBox::question(this, "Rockete: file change detected", i.value() + " has been modified,\ndo you want to reload it?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
            {
                QFileInfo file_info = i.value();
                QWidget *widget;
                isReloadingFile = true;
                closeTab(i.key(), false);
                openFile(i.value());
                new_tab_index = getTabIndexFromFileName(file_info.fileName().toAscii().data());

                widget = ui.codeTabWidget->widget(new_tab_index);
                ui.codeTabWidget->removeTab(new_tab_index);
                ui.codeTabWidget->insertTab(i.key(),widget,file_info.fileName());
                ui.codeTabWidget->setCurrentIndex(current_index);
                isReloadingFile = false;
            }
        }
    }
}

void Rockete::closeEvent(QCloseEvent *event)
{
    int
        response;
    
    while(ui.codeTabWidget->count()>0)
    {
        if(ui.codeTabWidget->tabText(0).contains("*"))
        {
            OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->widget(0));
            QString question;
            question = "Save ";
            question += file->fileInfo.absoluteFilePath();
            question += " before closing?";

            response = QMessageBox::question(this, "file modified", question, QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
        }
        else // if we have no changes, save anyway. I don't want changes to be lost if rockete simply didn't detect a change
        {
            response = QMessageBox::Yes;
        }

        if(response==QMessageBox::Yes)
            closeTab(0,true);
        else if(response==QMessageBox::No)
            closeTab(0,false);
        else
        {
            event->ignore();
            return;
        }
    }
    
    event->accept();

    QMainWindow::closeEvent(event);
}

// Private:

int Rockete::openFile(const QString &filePath)
{
    QFileInfo file_info(filePath);
    bool success = true;
    int new_tab_index;

    if(filePath.contains("memory]")) //librocket name for files in memory is [document in memory] since its not opened in the tabs it tries to open it...
        return -1;
    if(file_info.suffix() == "rproj") {
        openProject(file_info.absolutePath().toAscii().constData());
        return -1;
    }
    if(!file_info.exists())
    {
        foreach(QString path, ProjectManager::getInstance().getInterfacePaths())
        {
            file_info = path + filePath;

            if(file_info.exists())
            {
                break;
            }
        }

        if(!file_info.exists())
        {
            file_info = ProjectManager::getInstance().getWordListPath() + filePath;
            
            if(!file_info.exists())
            {
                file_info = ProjectManager::getInstance().getSnippetsFolderPath() + filePath;

                if(!file_info.exists())
                {
                    qWarning("file not found: %s\n", file_info.absoluteFilePath().toAscii().data());
                    return -1;
                }
            }
        }
    }

    for (int i = 0; i < ui.codeTabWidget->count(); ++i)
    {
        OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->widget(i));
        if (file && file->fileInfo.filePath() == file_info.filePath())
        {
            ui.codeTabWidget->setCurrentIndex(i);
            return i;
        }
    }

    if (fileWatcher->files().isEmpty())
        ui.codeTabWidget->clear();

    if (file_info.suffix() == "rml")
    {
        new_tab_index = openDocument(file_info.filePath().toAscii().data());
    }
    else if (file_info.suffix() == "rcss")
    {
        new_tab_index = openStyleSheet(file_info.filePath().toAscii().data());
    }
    else if (file_info.suffix() == "lua")
    {
        new_tab_index = openLuaScript(file_info.filePath().toAscii().data());
    }
    else if (file_info.suffix() == "rproj" || file_info.suffix() == "txt" || file_info.suffix() == "snippet")
    {
        new_tab_index = openASCIIFile(file_info.filePath().toAscii().data());
    }
    else
    {
        success = false;
    }


    if (success)
    {
        ui.codeTabWidget->setCurrentIndex(new_tab_index);
        ui.codeTabWidget->setTabToolTip(new_tab_index, file_info.absoluteFilePath());
        qInfo("adding file: %s\n", file_info.filePath().toAscii().data());
        fileWatcher->addPath(file_info.filePath());
        Settings::setMostRecentFile(file_info.filePath());
        generateMenuRecent();
        return new_tab_index;
    }

    return -1;
}

void Rockete::checkTextChanged(int index)
{
    if(ui.codeTabWidget->count()==0)
        return;

    if(index<0)
        index = ui.codeTabWidget->currentIndex();

    OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->widget(index));

    Q_ASSERT(file);

    if(file->document()->isModified())
    {
        QString tab_text = ui.codeTabWidget->tabText(index);
        if (!tab_text.startsWith("*"))
        {
            ui.codeTabWidget->setTabText(ui.codeTabWidget->currentIndex(), "*" + tab_text);
        }
    }
}

void Rockete::newProject()
{
    static int untitled_counted = 1;

    QString filePath = f_ssprintf("<untitled %d>.rproj", untitled_counted);
    ProjectManager::getInstance().Initialize(filePath);

    ui.projectFilesTreeWidget->clear();
    ui.projectFilesTreeWidget->clear();
    Settings::setProject(filePath);

    foreach( QString path, ProjectManager::getInstance().getFontPaths())
    {
        populateTreeView("Fonts", path);
        RocketSystem::getInstance().loadFonts(path);
    }

    foreach( QString path, ProjectManager::getInstance().getTexturePaths())
    {
        populateTreeView("Textures", path);
    }

    foreach( QString path, ProjectManager::getInstance().getInterfacePaths())
    {
        populateTreeView("Interfaces", path);
    }

    populateTreeView("Word Lists", ProjectManager::getInstance().getWordListPath());
    populateTreeView("Snippets", ProjectManager::getInstance().getSnippetsFolderPath());

    ++untitled_counted;
}

void Rockete::openProject(const char *file_path)
{
    QFileInfo file_info(file_path);

    if (file_info.suffix() == "rproj")
    {

        ProjectManager::getInstance().Initialize(file_path);

        ui.projectFilesTreeWidget->clear();
        ui.projectFilesTreeWidget->clear();
        Settings::setProject(file_path);

        foreach( QString path, ProjectManager::getInstance().getFontPaths())
        {
            populateTreeView("Fonts", path);
            RocketSystem::getInstance().loadFonts(path);
        }

        foreach( QString path, ProjectManager::getInstance().getTexturePaths())
        {
            populateTreeView("Textures", path);
        }

        foreach( QString path, ProjectManager::getInstance().getInterfacePaths())
        {
            populateTreeView("Interfaces", path);
        }

        populateTreeView("Word Lists", ProjectManager::getInstance().getWordListPath());
        populateTreeView("Snippets", ProjectManager::getInstance().getSnippetsFolderPath());

        if (file_info.exists()) {
            Settings::setMostRecentFile(file_info.filePath());
            generateMenuRecent();
        }
    } else
        qWarning() << "Not a project file.";
}

void Rockete::saveProject(const char *file_path)
{
    ProjectManager::getInstance().Serialize(file_path);
}

int Rockete::openDocument(const char *file_path)
{
    OpenedDocument *new_document;
    QFileInfo file_info(file_path);

    if (!file_info.exists())
    {
        QMessageBox::information(NULL, "File not found", "File "+QString(file_path)+" not found.", QMessageBox::Ok);
        return -1;
    }
    new_document = new OpenedDocument(file_info);
    new_document->initialize();
    new_document->rocketDocument = RocketHelper::loadDocumentFromMemory(new_document->toPlainText());
    new_document->rocketDocument->RemoveReference();

    return ui.codeTabWidget->addTab(new_document, file_info.fileName());
}

int Rockete::openStyleSheet(const char *file_path)
{
    OpenedStyleSheet *new_style_sheet;
    QFileInfo file_info(file_path);

    new_style_sheet = new OpenedStyleSheet;
    new_style_sheet->fileInfo = file_info;
    new_style_sheet->initialize();
    return ui.codeTabWidget->addTab(new_style_sheet, file_info.fileName());
}

int Rockete::openLuaScript(const char *file_path)
{
    OpenedLuaScript *new_lua_script;
    QFileInfo file_info(file_path);

    new_lua_script = new OpenedLuaScript;
    new_lua_script->fileInfo = file_info;
    new_lua_script->initialize();
    return ui.codeTabWidget->addTab(new_lua_script, file_info.fileName());
}

int Rockete::openASCIIFile(const char *file_path)
{
    OpenedFile *new_ascii_file;
    QFileInfo file_info(file_path);

    new_ascii_file = new OpenedFile;
      
    new_ascii_file->fileInfo = file_info;
    new_ascii_file->initialize();
    new_ascii_file->fillTextEdit();
    return ui.codeTabWidget->addTab(new_ascii_file, file_info.fileName());
}

void Rockete::generateMenuRecent()
{
    ui.menuRecent->clear();
    QStringList recentFileList = Settings::getRecentFileList();

    foreach(const QString &item, recentFileList)
    {
        recentFileActionList.append(ui.menuRecent->addAction(item));
    }
}

bool Rockete::readSpriteSheetInfo(QTreeWidgetItem *item, const QString &texture)
{
    // 1. texpack catalog info:
    bool valid_found;
    QImage image(texture);
    QFileInfo tinfo(texture);
    QString cat_filename = tinfo.absolutePath()+QDir::separator()+QString(tinfo.completeBaseName()+".cat");
    FILE *cat_file = fopen( cat_filename.toUtf8().data(), "r" );
    if (!cat_file)
        goto format2;
    valid_found = false; while (!feof(cat_file)) {
        int index;
        char tname[256];
        unsigned left;
        unsigned top;
        unsigned right;
        unsigned bottom;
        unsigned w, h;
        float left_r;
        float right_r;
        float top_r;
        float bottom_r;
        float width_r;
        float height_r;
        unsigned format;
        unsigned bpp;
        int pnum = fscanf( cat_file, "%d;%255[^;];%d;%d;%d;%d;%f;%f;%f;%f;%d;%d;%f;%f;%d;%d;",
                 /*  1 */ &index,
                 /*  2 */ tname,
                 /*  3 */ &left,
                 /*  4 */ &top,
                 /*  5 */ &right,
                 /*  6 */ &bottom,
                 /*  7 */ &left_r,
                 /*  8 */ &top_r,
                 /*  9 */ &right_r,
                 /* 10 */ &bottom_r,
                 /* 11 */ &w,
                 /* 12 */ &h,
                 /* 13 */ &width_r,
                 /* 14 */ &height_r,
                 /* 15 */ &format,
                 /* 16 */ &bpp );
        if (pnum > 4) {
            QTreeWidgetItem *new_item = new QTreeWidgetItem(item);
            QImage copy = image.copy( left, top, right-left, bottom-top);
            new_item->setIcon(0, QPixmap::fromImage(copy));
            new_item->setText(1, tname);
            new_item->setToolTip(1, QString("%1:%2").arg(QFileInfo(texture).fileName()).arg(tname));
            QString data = QString("%1px %2px %3px %4px").arg(left).arg(top).arg(right-left).arg(bottom-top);
            new_item->setData(0, Qt::UserRole, texture);
            new_item->setData(1, Qt::UserRole, data);
            valid_found = true;
        }
    };
    if (valid_found) {
        item->setIcon(0, QIcon(":/images/icon_atlas.png"));
        return true;
    }

    // <?xml version="1.0" encoding="UTF-8"?>
    // <!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
    // <plist version="1.0">
    //     <dict>
    //         <key>frames</key>
    //         <dict>
    //             <key>Icon.png</key>
    //             <dict>
    //                 <key>frame</key>
    //                 <string>{{2,2},{57,57}}</string>
    //                 <key>offset</key>
    //                 <string>{0,0}</string>
    //                 <key>rotated</key>
    //                 <false/>
    //                 <key>sourceColorRect</key>
    //                 <string>{{0,0},{57,57}}</string>
    //                 <key>sourceSize</key>
    //                 <string>{57,57}</string>
    //             </dict>
    //         </dict>
    //         <key>metadata</key>
    //         <dict>
    //             <key>format</key>
    //             <integer>2</integer>
    //             <key>realTextureFileName</key>
    //             <string>nonencryptedAtlas.pvr.ccz</string>
    //             <key>size</key>
    //             <string>{64,64}</string>
    //             <key>smartupdate</key>
    //             <string>$TexturePacker:SmartUpdate:5b30a75137a4f533396670236d41f11c$</string>
    //             <key>textureFileName</key>
    //             <string>nonencryptedAtlas.pvr.ccz</string>
    //         </dict>
    //     </dict>
    // </plist>
format2:
    // 2. cocos2d plist info:
    valid_found = false;

    // ;Sprite Monkey Coordinates, UTF-8
    // ;Transparency, Sprite Sheet Name, Image Width, Image Height
    // Alpha,/Users/jbaker/Desktop/elf_run.png,1024,1024
    // ;Image/Frame Name, Clip X, Clip Y, Clip Width, Clip Height
    // elf run 00001,0,0,256,256
    // elf run 00002,256,0,256,256
    // elf run 00003,512,0,256,256
    // elf run 00004,768,0,256,256
    // elf run 00005,0,256,256,256
    // elf run 00006,256,256,256,256
    // elf run 00007,512,256,256,256
    // elf run 00008,768,256,256,256
    // elf run 00009,0,512,256,256
    // elf run 00010,256,512,256,256
    // elf run 00011,512,512,256,256
    // elf run 00012,768,512,256,256
    // elf run 00013,0,768,256,256
    // elf run 00014,256,768,256,256
    // elf run 00015,512,768,256,256
    // 3. smc
    return false;
}

void Rockete::populateTreeView(const QString &top_item_name, const QString &directory_path)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(ui.projectFilesTreeWidget, QStringList(top_item_name));
    QList<QTreeWidgetItem *> items;

    QDirIterator directory_walker(directory_path, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);

    while(directory_walker.hasNext())
    {
        directory_walker.next();
        if(!directory_walker.fileInfo().isDir() && !directory_walker.fileInfo().isHidden())
        {
            QStringList list;
            list << directory_walker.fileInfo().fileName();

            QTreeWidgetItem *new_item = new QTreeWidgetItem();
            new_item->setText(1,directory_walker.fileInfo().fileName());
            new_item->setToolTip(1, directory_walker.fileInfo().absoluteFilePath());
            new_item->setData(0, Qt::UserRole, directory_walker.fileInfo().filePath());

            bool known = true; if(directory_walker.fileInfo().suffix() == "png" || directory_walker.fileInfo().suffix() == "tga" || directory_walker.fileInfo().suffix() == "jpg")
            {
                new_item->setIcon(0, QIcon(directory_walker.fileInfo().absoluteFilePath()));
                readSpriteSheetInfo(new_item, directory_walker.fileInfo().absoluteFilePath());
            }
            else if(directory_walker.fileInfo().suffix() == "snippet")
            {
                new_item->setIcon(0, QIcon(":/images/icon_snippets.png"));
            }
            else if(directory_walker.fileInfo().suffix() == "rml")
            {
                new_item->setIcon(0, QIcon(":/images/icon_rml.png"));
            }
            else if(directory_walker.fileInfo().suffix() == "rcss")
            {
                new_item->setIcon(0, QIcon(":/images/icon_rcss.png"));
            }
            else if(directory_walker.fileInfo().suffix() == "lua")
            {
                new_item->setIcon(0, QIcon(":/images/icon_lua.png"));
            } else if(directory_walker.fileInfo().suffix() == "ttf" || directory_walker.fileInfo().suffix() == "otf" || directory_walker.fileInfo().suffix() == "fnt") {
                // accept freetype fonts fonts
                new_item->setIcon(0, QIcon(":/images/icon_fnt.png"));
            } else
                known = false;

            if (known) {
                item->addChild(new_item);
                items.append(new_item);
            }
        }
    }

    item->addChildren(items);
    ui.projectFilesTreeWidget->addTopLevelItem(item);
    item->sortChildren(1,Qt::AscendingOrder);
}

void Rockete::loadPlugins()
{
    QDir pluginsDir = QDir(qApp->applicationDirPath());
    #if defined(Q_OS_WIN)
        if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
            pluginsDir.cdUp();
    #elif defined(Q_OS_MAC)
        if (pluginsDir.dirName() == "MacOS") {
            pluginsDir.cdUp();
            pluginsDir.cdUp();
            pluginsDir.cdUp();
        }
    #endif
    pluginsDir.cd("plugins");

    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
        #if defined(Q_OS_WIN)
            if(fileName.endsWith(".dll"))
        #else
            if(fileName.endsWith(".a"))
        #endif
        {
            QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
            QObject *plugin = loader.instance();
            LocalizationManagerInterface *iLocate = qobject_cast<LocalizationManagerInterface *>(plugin);
            if (iLocate) {
                LocalizationManagerInterface::setInstance(iLocate);
            }
        }
    }
}

void Rockete::closeTab(int index, bool must_save)
{
    if(ui.codeTabWidget->count()==0)
        return;

    QWidget *removed_widget = ui.codeTabWidget->widget(index);
    OpenedFile *file = qobject_cast<OpenedFile *>(removed_widget);
    OpenedDocument *doc = qobject_cast<OpenedDocument *>(removed_widget);

    Q_ASSERT(file);

    if(must_save)
        file->save();
    qInfo("removing path: %s\n", file->fileInfo.filePath().toAscii().data());
    fileWatcher->removePath(file->fileInfo.filePath());

    if(doc)
    {
        if(getCurrentDocument()->rocketDocument == doc->rocketDocument)
        {
            ui.documentHierarchyTreeWidget->clear();
            renderingView->changeCurrentDocument(NULL);
        }

        RocketHelper::unloadDocument(doc->rocketDocument);
    }
    delete(removed_widget);
}

void Rockete::addRulers()
{
    //setViewportMargins(RULER_BREADTH,RULER_BREADTH,0,0);
    QGridLayout *layout = (QGridLayout*)ui.renderingViewBack->layout();

    layout->removeWidget(ui.renderingView);

    layout->setSpacing(0);
    layout->setMargin(0);

    horzRuler = new QDRuler(QDRuler::Horizontal, ui.renderingViewBack);
    vertRuler = new QDRuler(QDRuler::Vertical, ui.renderingViewBack);

    QWidget* fake = new QWidget();
    fake->setBackgroundRole(QPalette::Window);
    fake->setFixedSize(RULER_BREADTH,RULER_BREADTH);
    layout->addWidget(fake,0,0);
    layout->addWidget(horzRuler,0,1);
    layout->addWidget(vertRuler,1,0);
    layout->addWidget(ui.renderingView,1,1);

    layout->update();
}

void Rockete::logMessage(QString aMsg)
{
        if(ui.logWindow!=NULL && !aMsg.isEmpty())
                ui.logWindow->append(aMsg);
}

Rockete *Rockete::instance = NULL;
