#include "Rockete.h"

#include <math.h>
#include <QTime>
#include <QRawFont>
#include <QGlyphRun>
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
#include <Rocket/Debugger.h>
#include "AttributeTreeModel.h"
#include "RocketSystem.h"
#include "ActionManager.h"
#include "ToolManager.h"
#include "EditionHelper.h"
#include "Settings.h"
#include <QDirIterator>
#include <QShortcut>
#include "DocumentPreview.h"
#include "ProjectManager.h"
#include <QPluginLoader>
#include "QDRuler.h"
#include "QDLabel.h"
#include "LocalizationManagerInterface.h"
#include "OpenedLuaScript.h"
#include "qtplist/PListParser.h"

const int kTexturePreviewTabIndex = 1;
const int kCuttingImagePreviewTabIndex = 1;

void logMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &amsg );

TestFrameInfo testFrames[] = {
    { ":/images/frame-android-xsmall.png", true, false, LocalScreenSizeItem(320, 480, "Android") },
    { ":/images/frame-android-small.png", true, false, LocalScreenSizeItem(480, 640, "Android VGA") },
    { ":/images/frame-android-medium.png", true, false, LocalScreenSizeItem(480, 854, "Android FWVGA") },
    { ":/images/frame-iphone.png", true, false, LocalScreenSizeItem(320, 480, "iPhone3") },
    { ":/images/frame-monitor.png", false, true, LocalScreenSizeItem(800, 480, "WVGA ") },
    { ":/images/frame-monitor.png", true, false, LocalScreenSizeItem(800, 600) },
    { ":/images/frame-iphone.png", true, true, LocalScreenSizeItem(640, 960, "iPhone4") },
    { ":/images/frame-iphone5.png", true, true, LocalScreenSizeItem(1136, 640, "iPhone5") },
    { ":/images/frame-ipad.png", true, true, LocalScreenSizeItem(768, 1024, "iPad") },
    { ":/images/frame-monitor.png", false, false, LocalScreenSizeItem(1024, 768) },
    { ":/images/frame-playbook.png", true, true, LocalScreenSizeItem(1024, 600, "Playbook") },
    { ":/images/frame-z10.png", true, true, LocalScreenSizeItem(768, 1280, "BB Z10") },
    { ":/images/frame-monitor.png", false, false, LocalScreenSizeItem(1280, 720) },
    { ":/images/frame-monitor.png", false, false, LocalScreenSizeItem(1600, 1024) },
    { ":/images/frame-monitor.png", true, false, LocalScreenSizeItem(1920, 1080, "HD") },
    { ":/images/frame-ipad.png", true, false, LocalScreenSizeItem(1536, 2048, "iPad3") },
    { NULL }
};

// new global log/message handler:
void logMessageOutput( QtMsgType type, const QMessageLogContext &context, const QString &amsg )
{
        Q_UNUSED(context)

        if (amsg.contains("QNSView mouseDragged", Qt::CaseInsensitive)) {
            // ignore this message
            return;
        }

        QString msg = amsg;
        if(msg.isEmpty()) return;
        if(msg.length() >= 8195) msg.truncate(8195);

        Rockete *mw = Rockete::instance;

#if defined(Q_CC_MSVC) || defined(Q_CC_MSVC_NET)
        OutputDebugString( (msg + "\n").unicode() );
#endif

        msg = msg.toHtmlEscaped(); // make special chars printable in html
        switch ( type )
        {
        case QtDebugMsg:
                fprintf( stderr, "%s Debug: %s \n", QTime::currentTime().toString().toLatin1().constData(), amsg.toUtf8().constData() );
                msg = "<font color=black><small>"+msg+"</small></font>";
                if (mw) mw->logHtmlMessage(msg);
                break;
        case QtCriticalMsg:
                fprintf( stderr, "%s Critical: %s \n", QTime::currentTime().toString().toLatin1().constData(), amsg.toUtf8().constData() );
                msg = "<font color=red><b>Critical: "+msg+"</></font>";
                if (mw) mw->logHtmlMessage(msg);
                break;
        case QtWarningMsg:
                fprintf( stderr, "%s Warning: %s \n", QTime::currentTime().toString().toLatin1().constData(), amsg.toUtf8().constData() );
                msg = "<font color=blue><small><b>Warning:</b> "+msg+"</small></font>";
                if (mw) mw->logHtmlMessage(msg);
                break;
        case QtFatalMsg:
                fprintf( stderr, "%s Fatal: %s \n", QTime::currentTime().toString().toLatin1().constData(), amsg.toUtf8().constData() );
                msg = "<big><b>Fatal: "+msg+"</b></big>";
                if (mw) mw->logHtmlMessage(msg);
                abort();
        }
}


Rockete::Rockete(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags), isReloadingFile(false)
{
    instance = this;

    qInstallMessageHandler( logMessageOutput );
    loadPlugins();
    ui.setupUi(this);

    if (!selectedTextureTmp.open())
        qDebug() << "Failed to save image in temporary file.";

#if defined Q_OS_MAC || defined Q_WS_MAC
    setAttribute(Qt::WA_MacSmallSize);
    ui.labelCuttingDim->setAttribute(Qt::WA_MacSmallSize);
    ui.labelCuttingDimLabel->setAttribute(Qt::WA_MacSmallSize);
    ui.labelCuttingTopCapLabel->setAttribute(Qt::WA_MacSmallSize);
    ui.labelCuttingLeftCapLabel->setAttribute(Qt::WA_MacSmallSize);
    ui.labelCuttingBottomCapLabel->setAttribute(Qt::WA_MacSmallSize);
    ui.labelCuttingRightCapLabel->setAttribute(Qt::WA_MacSmallSize);
    ui.labelCuttingMask->setAttribute(Qt::WA_MacSmallSize);
    ui.checkBoxCuttingMask->setAttribute(Qt::WA_MacSmallSize);
    ui.documentHierarchyTreeWidget->setAttribute(Qt::WA_MacSmallSize);
    ui.documentHierarchyTreeWidget->setIndentation(15); // ui 10 is too small for osx
    ui.splitter->setHandleWidth(4); // default 7 is bit too big
    ui.rightPaneSplitter->setHandleWidth(4);
    ui.leftPaneSplitter->setHandleWidth(4);
#endif

    // splitters change (for saving state):

    ui.splitter->restoreState(Settings::getSplitterState(ui.splitter->objectName()));
    ui.rightPaneSplitter->restoreState(Settings::getSplitterState(ui.rightPaneSplitter->objectName()));
    ui.leftPaneSplitter->restoreState(Settings::getSplitterState(ui.leftPaneSplitter->objectName()));

    connect(ui.splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMovedChanges(int, int)));
    connect(ui.rightPaneSplitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMovedChanges(int, int)));
    connect(ui.leftPaneSplitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMovedChanges(int, int)));

    // setup Tool tab:
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
        openProject(QApplication::arguments()[1].toUtf8().data());
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

    if(LocalizationManagerInterface::hasInstance())
    {
        int langs = 0; foreach(LocalizationManagerInterface::LocalizationLanguage language, LocalizationManagerInterface::getInstance().getSupportedLanguages())
        {
            languageBox->addItem(LocalizationManagerInterface::getInstance().getLanguageNameForLanguage(language), (int)language); ++langs;
        }

        if (langs) {
            languageBox = new QComboBox(this);
            languageBox->setEditable(false);
            languageBox->setInsertPolicy( QComboBox::InsertAlphabetically );

            connect(languageBox, SIGNAL(activated(const QString&)), (QObject*)this, SLOT(languageBoxActivated()));
            ui.mainToolBar->addSeparator();
            ui.mainToolBar->addWidget(languageBox);

            ui.statusBar->showMessage( "Localization Activated", 10000 );
        }
    }

    QAction *new_action = new QAction(QIcon(":/images/new_button.png"), "create button", this);
    connect(new_action, SIGNAL(triggered()), (QObject*)this, SLOT(newButtonWizardActivated()));
    ui.mainToolBar->addAction(new_action);
    
    ui.mainToolBar->addSeparator();

    int orient = Settings::getInt("ScreenSizeOrient", 0);

    QAction *portrait_action = new QAction(QIcon(":/images/orientation-portrait.png"), "Portrait", this);
    portrait_action->setProperty("orientation", 0);
    portrait_action->setCheckable(true);
    if (orient == 0) portrait_action->setChecked( true );
    QAction *landscape_action = new QAction(QIcon(":/images/orientation-landscapeleft.png"), "Landscape", this);
    landscape_action->setProperty("orientation", 1);
    landscape_action->setCheckable(true);
    if (orient == 1) landscape_action->setChecked( true );
    QActionGroup * group = new QActionGroup( this );
    group->addAction(portrait_action);
    group->addAction(landscape_action);
    ui.mainToolBar->addActions(group->actions());

    QObject::connect(group, SIGNAL(triggered(QAction*)),
            this, SLOT(orientationChange(QAction*)));

    // quick resolution change:
    QMenu *dim = new QMenu(tr("Dimension"));
    dim->menuAction()->setIcon(QIcon(":/images/dimensions.png"));
    ui.mainToolBar->addAction(dim->menuAction());

    TestFrameInfo *e = &testFrames[0]; int x = 0; while(e->image) {
        if (e->toolbar) {
            QAction *res_action = new QAction(QIcon(e->image), e->size.displayedString, this);
            res_action->setProperty("index", x);
            connect(res_action, SIGNAL(triggered()), (QObject*)this, SLOT(newScreenSizeAction()));
            dim->addAction(res_action);
        }
        ++e; ++x;
    }

    ui.mainToolBar->addSeparator();
    ui.mainToolBar->addAction(ui.actionDbg_outline);
    ui.actionDbg_outline->setChecked( Settings::getInt("display_debugger") );
    ui.mainToolBar->addAction(ui.actionDisplay_grid);
    ui.actionDisplay_grid->setChecked( Settings::getInt("display_grid", true) );

    labelZoom = new QLabel(parent);
    labelZoom->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    labelZoom->setText("Zoom: 100%");
    labelPos = new QLabel(parent);
    labelPos->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    labelPos->setText("Cursor: -/-");
    labelScreenSize = new QLabel(parent);
    labelScreenSize->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    labelScreenSize->setText(QString("Screen: %1x%2").arg(RocketSystem::getInstance().context_width()).arg(RocketSystem::getInstance().context_height()));

    ui.statusBar->addPermanentWidget(labelScreenSize);
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
    connect(ui.spinCuttingTopCap, SIGNAL(valueChanged(int)), (QObject*)this, SLOT(spinCuttingChanged(int)));
    connect(ui.spinCuttingLeftCap, SIGNAL(valueChanged(int)), (QObject*)this, SLOT(spinCuttingChanged(int)));
    connect(ui.spinCuttingBottomCap, SIGNAL(valueChanged(int)), (QObject*)this, SLOT(spinCuttingChanged(int)));
    connect(ui.spinCuttingRightCap, SIGNAL(valueChanged(int)), (QObject*)this, SLOT(spinCuttingChanged(int)));
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
        ui.attributeTreeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        ui.attributeTreeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        ui.attributeTreeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
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
        ui.propertyTreeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        ui.propertyTreeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        //ui.propertyTreeView->header()->setResizeMode(2, QHeaderView::ResizeToContents);
        //ui.propertyTreeView->header()->setStretchLastSection(false);
    }
}

void Rockete::selectElement(Element *element)
{
    //if(element != getCurrentDocument()->selectedElement)

    if (element) {
        // skip debugger elements
        if (Rocket::Debugger::IsVisible() && (
            (element->GetId() == "event-log-button")
            || (element->GetId() == "debug-info-button")
            || (element->GetId() == "outlines-button")
        ))
            return;
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
    if (QFile::exists(projectFile))
        openProject(projectFile.toUtf8().constData());
    else
        newProject();
}

void Rockete::menuSaveClicked()
{
    if(ui.codeTabWidget->count()==0)
        return;

    OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->currentWidget());

    Q_ASSERT(file);

    file->save();
    ui.codeTabWidget->setTabText(ui.codeTabWidget->currentIndex(), file->fileInfo.fileName());
    reloadCurrentDocument();
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

    QString changed_outside = fileChangedOutsideArray[index]; // handle outside changes
    fileChangedOutsideArray.remove(index);

    if(ui.codeTabWidget->tabText(index).startsWith("*") || !changed_outside.isEmpty()) // if changed in the app or outside
    {
        OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->widget(index));
        QString question;
        question = "Save ";
        question += file->fileInfo.fileName();
        question += " before closing?";

        response = QMessageBox::question(this, "file modified/changed", question, QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
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

void Rockete::newScreenSizeAction()
{
    // get preview index and resize view:
    const QObject *source = QObject::sender();
    const int index = source->property("index").toInt();
    setScreenSize(testFrames[index].size.width, testFrames[index].size.height);
}

void Rockete::orientationChange(QAction *action)
{
    const int index = action->property("orientation").toInt();
    const int curr = Settings::getInt("ScreenSizeOrient");
    // reload document if orientation changed
    if (index != curr)
        setScreenSize(RocketSystem::getInstance().context_width(), RocketSystem::getInstance().context_height(), index);
    Settings::setValue("ScreenSizeOrient", index);
}

void Rockete::menuSetScreenSizeClicked()
{
    QList<LocalScreenSizeItem*> item_list;
    QStringList item_string_list;
    int index_to_select = 0;

    TestFrameInfo *e = &testFrames[0]; while(e->image) {
        item_list.push_back(&e->size); ++e;
    }

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
                setScreenSize(item->width, item->height);
                break;
            }
        }
    }
}

void Rockete::setScreenSize(int width, int height)
{
    setScreenSize(width, height, Settings::getInt("ScreenSizeOrient"));
}

void Rockete::setScreenSize(int width, int height, int orientation)
{
    if (orientation == 1) { // swap for landscape landscape
        if (height > width) {
            int t = width; width = height; height = t;
        }
    } else {                // swap for portrait
        if (width > height) {
            int t = width; width = height; height = t;
        }
    };
    RocketSystem::getInstance().resizeContext(width, height);
    if(getCurrentDocument())
    {
        reloadCurrentDocument();
    }
    Settings::setValue("ScreenSizeWidth", width);
    Settings::setValue("ScreenSizeHeight", height);
    labelScreenSize->setText(QString("Screen: %1x%2").arg(width).arg(height));
    ui.renderingView->repaint();
}

// open preview window with size w x h
void Rockete::openPreviewWindow(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);

    DocumentPreview *preview = new DocumentPreview();
    preview->show();
}

void Rockete::menuLoadFonts()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Open font files...",
        NULL,
        "OTF/TTF files (*.otf *.ttf *.fon);Bitmap fonts files (*.bmf *.fnt)"
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
    {   if(item->data(2,  Qt::UserRole).isValid()) {
            QString info = item->data(2,  Qt::UserRole).toString();
            if (info.compare("auto", Qt::CaseInsensitive) == 0) { // autogenerated file
                int tab_index = openFile(item->data(0,  Qt::UserRole).toString());
                if (tab_index >=0) {
                    if (OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->widget(tab_index)))
                        file->setProperty("auto", true);
                }
            }
        } else
            openFile(item->text(1));
    }
}

void Rockete::fileTreeClicked(QTreeWidgetItem *item, int /*column*/)
{
    QClipboard *clipboard = QApplication::clipboard();
    QString data = item->data(1, Qt::UserRole).toString(); // check for texture subimages
    if(item->text(1).endsWith("png")||item->text(1).endsWith("jpg")||data.length()) // no native support for tga
    {
        if (ui.tabWidgetDoc->currentIndex() != kTexturePreviewTabIndex) ui.texturePreviewLabel->updateGeometry(); // update size of preview if it is hidden
        QString key;
        if (data.length()) {
            QString texture = item->data(0, Qt::UserRole).toString();
            QImage image(texture);
            int l=0,b=0,w=0,h=0; int n = sscanf(data.toLatin1().constData(), "%dpx %dpx %dpx %dpx", &l, &b, &w, &h); if (n!=4)
                qWarning() << "Invalid format: " << data;
            selectedTexture = image.copy( l, b, w, h);
            selectedTexture.save(selectedTextureTmp.fileName(), "png");
            ui.texturePreviewLabel->setPixmap(QPixmap::fromImage(selectedTexture.scaled(QSize(ui.texturePreviewLabel->width(),ui.texturePreviewLabel->height()),Qt::KeepAspectRatio,Qt::SmoothTransformation)));
            QString file = QFileInfo(texture).fileName();
            clipboard->setText(file + " " + QString("%1px %2px %3px %4px").arg(l).arg(b).arg(l+w).arg(b+h)); // place texture coordinates in clipboard
            updateCuttingTab(file, item->text(1), l, b, w, h); // flip y for bottom-left origin
            key = file+":"+item->text(1); // key for cutting info
        } else {
            selectedTexture = QImage( getPathForFileName(item->text(1)) );
            selectedTexture.save(selectedTextureTmp.fileName(), "png");
            ui.texturePreviewLabel->setPixmap(QPixmap::fromImage(selectedTexture.scaled(QSize(ui.texturePreviewLabel->width(),ui.texturePreviewLabel->height()),Qt::KeepAspectRatio,Qt::SmoothTransformation)));
            clipboard->setText(item->text(1)); // place filename in clipboard
            updateCuttingTab(item->text(1), "", 0, 0, selectedTexture.width(), selectedTexture.height());
            key = item->text(1);
        }
        const CssCuttingInfo inf = ProjectManager::getInstance().getCuttingInfo(key); if (!inf.isEmpty()) { // restore cutting info
            ui.spinCuttingLeftCap->setValue(inf.left);
            ui.spinCuttingTopCap->setValue(inf.top);
            ui.spinCuttingRightCap->setValue(inf.left);
            ui.spinCuttingBottomCap->setValue(inf.bottom);
        }
    } else if(item->text(1).endsWith("ttf")||item->text(1).endsWith("otf")||item->text(1).endsWith("fon")) {
        const int kCharsSpace = 2;
        const int kPreviewMargin = 3;
        // font preview
        qDebug() << "Preview font " << item->data(0,  Qt::UserRole).toString();
        QPixmap preview(ui.texturePreviewLabel->width(), ui.texturePreviewLabel->height());
        QRawFont fnt = QRawFont(item->data(0,  Qt::UserRole).toString(), 36);
        QGlyphRun gr;
        gr.setRawFont(fnt);
        const QVector<quint32> &gl = fnt.glyphIndexesForString("01234567890!@#$%^&*()_+<>?:\"';|\\}{][`~ABCDEFGHIJKLMNOPQRSTUWXYZabcdefghijklmnopqrstuwxyz");
        const QVector<QPointF> &adv = fnt.advancesForGlyphIndexes(gl);
        QVector<QPointF> pos;
        QPointF cr(0,0); pos << cr;
        foreach(const QPointF &pt, adv) {
            if (cr.x()+2*pt.x()+kCharsSpace*2 >= preview.width()-kPreviewMargin*2) {
                cr.setX(0); cr.setY(cr.y()+fnt.ascent()+fnt.descent());
            } else
                cr.setX(cr.x()+pt.x()+kCharsSpace);
            pos << cr;
        }
        gr.setGlyphIndexes(gl);
        gr.setPositions(pos);
        preview.fill(Qt::transparent);
        QPainter paint;
        paint.begin(&preview);
        paint.setPen(QPen(Qt::black, 0));
        paint.setBrush(Qt::NoBrush);
        paint.drawGlyphRun(QPointF(kPreviewMargin, fnt.ascent()), gr);
        paint.end();
        ui.texturePreviewLabel->setPixmap(preview);
    } else {
        ui.texturePreviewLabel->setText("");
        ui.texturePreviewLabel->clear();
    }
}

void Rockete::resizeTexturePreview(QResizeEvent * /*event*/)
{
    if (!selectedTexture.isNull()) {
        ui.texturePreviewLabel->setPixmap(QPixmap::fromImage(selectedTexture.scaled(QSize(ui.texturePreviewLabel->width(),ui.texturePreviewLabel->height()),Qt::KeepAspectRatio,Qt::SmoothTransformation)));
    }
}

void Rockete::updateCuttingTab(const QString &file, const QString &texture, int l, int b, int w, int h)
{
    ui.labelCuttingDim->setText(QString("x:%1px y:%2px w:%3px h:%4px").arg(l).arg(b).arg(w).arg(h));
    ui.labelCuttingDim->setProperty("texture file", file);
    if (texture.isEmpty())
        ui.labelCuttingDim->setProperty("texture key", file);
    else
        ui.labelCuttingDim->setProperty("texture key", file+":"+texture);
    ui.cuttingLog->append(QString("<b><font color=blue>File selected %1</font></b><br/>").arg(file));
    updateCuttingInfo(ui.spinCuttingLeftCap->value(), ui.spinCuttingTopCap->value(), ui.spinCuttingRightCap->value(), ui.spinCuttingBottomCap->value());
    // setup max values for sliders:
    ui.spinCuttingLeftCap->setMaximum(floor(w/2.));
    ui.spinCuttingRightCap->setMaximum(floor(w/2.));
    ui.spinCuttingBottomCap->setMaximum(floor(h/2.));
    ui.spinCuttingTopCap->setMaximum(floor(h/2.));
}

void Rockete::spinCuttingChanged(int /*value*/)
{
    updateCuttingInfo(ui.spinCuttingLeftCap->value(), ui.spinCuttingTopCap->value(), ui.spinCuttingRightCap->value(), ui.spinCuttingBottomCap->value());
    const QString file =  ui.labelCuttingDim->property("texture file").toString();
    QString inf = QString("%1;%2;%3;%4").arg(ui.spinCuttingLeftCap->value()).arg(ui.spinCuttingTopCap->value()).arg(ui.spinCuttingRightCap->value()).arg(ui.spinCuttingBottomCap->value());
    const QString key =  ui.labelCuttingDim->property("texture key").toString();
    ProjectManager::getInstance().setCuttingInfo(key, inf); // save cutting info for this texture
    updateTextureInfoFiles(file); // regenerate styles file with new info
}

void Rockete::updateCuttingInfo(int lvalue, int tvalue, int rvalue, int bvalue)
{
    int l, b, w, h;
    int n = sscanf(ui.labelCuttingDim->text().toLatin1().constData(), "x:%dpx y:%dpx w:%dpx h:%dpx", &l, &b, &w, &h); if (n!=4) {
        ui.cuttingLog->append(QString("<font color=red>Invalid data format.</font color=red><br/>"));
    } else {
        if (lvalue != 0 || tvalue != 0 || rvalue != 0 || bvalue != 0) {
            const QString file =  ui.labelCuttingDim->property("texture file").toString();
            if (tvalue == 0 && bvalue == 0) {
                if (lvalue == rvalue)
                    ui.cuttingLog->append(QString("<b><font color=blue>Cutting for h-cap %1px:</font></b><br/>").arg(lvalue));
                else
                    ui.cuttingLog->append(QString("<b><font color=blue>Cutting for h-cap %1px|%2px:</font></b><br/>").arg(lvalue).arg(rvalue));
                ui.cuttingLog->append(QString("/** !background:%1 %2|%3|%4|%5*/<br/>").arg(file).arg(lvalue).arg(rvalue).arg(bvalue).arg(tvalue));
                ui.cuttingLog->append(QString("<b>background-left-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l).arg(b).arg(l+lvalue).arg(b+h));
                ui.cuttingLog->append(QString("<b>background-center-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l+lvalue).arg(b).arg(l+w-rvalue).arg(b+h));
                ui.cuttingLog->append(QString("<b>background-right-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l+w-rvalue).arg(b).arg(l+w).arg(b+h));
                ui.cuttingLog->append("/** */");
            } else if(lvalue == 0 && tvalue == 0) {
                if (tvalue == bvalue)
                    ui.cuttingLog->append(QString("<b><font color=blue>Cutting for v-cap %1px:</font></b><br/>").arg(bvalue));
                else
                    ui.cuttingLog->append(QString("<b><font color=blue>Cutting for v-cap %1px|%2px:</font></b><br/>").arg(tvalue).arg(bvalue));
                ui.cuttingLog->append(QString("/** !background:%1 %2|%3|%4|%5*/<br/>").arg(file).arg(lvalue).arg(rvalue).arg(bvalue).arg(tvalue));
                ui.cuttingLog->append(QString("<b>background-bottom-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l).arg(b+h-tvalue).arg(l+w).arg(b+h));
                ui.cuttingLog->append(QString("<b>background-bottom-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l).arg(b+bvalue).arg(l+w).arg(b+h-tvalue));
                ui.cuttingLog->append(QString("<b>background-bottom-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l).arg(b).arg(l+w).arg(b+bvalue));
                ui.cuttingLog->append("/** */");
            } else {
                ui.cuttingLog->append(QString("<b><font color=blue>Cutting for cap h:%1px|%2px v:%3px|%4px:</font></b><br/>").arg(lvalue).arg(rvalue).arg(bvalue).arg(tvalue));
                ui.cuttingLog->append(QString("/** !background:%1 %2|%3|%4|%5*/<br/>").arg(file).arg(lvalue).arg(rvalue).arg(bvalue).arg(tvalue));
                ui.cuttingLog->append(QString("<b>background-bottom-left-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l).arg(b+h-tvalue).arg(l+lvalue).arg(b+h));
                ui.cuttingLog->append(QString("<b>background-bottom-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l+lvalue).arg(b+h-tvalue).arg(l+w-rvalue).arg(b+h));
                ui.cuttingLog->append(QString("<b>background-bottom-right-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l+w-rvalue).arg(b+h-tvalue).arg(l+w).arg(b+h));
                ui.cuttingLog->append(QString("<b>background-left-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l).arg(b+bvalue).arg(l+lvalue).arg(b+h-tvalue));
                ui.cuttingLog->append(QString("<b>background-center-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l+lvalue).arg(b+bvalue).arg(l+w-rvalue).arg(b+h-tvalue));
                ui.cuttingLog->append(QString("<b>background-right-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l+w-rvalue).arg(b+bvalue).arg(l+w).arg(b+h-tvalue));
                ui.cuttingLog->append(QString("<b>background-top-left-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l).arg(b).arg(l+lvalue).arg(b+bvalue));
                ui.cuttingLog->append(QString("<b>background-top-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l+lvalue).arg(b).arg(l+w-rvalue).arg(b+bvalue));
                ui.cuttingLog->append(QString("<b>background-top-right-image:</b> %1 %2px %3px %4px %5px;<br/>").arg(file).arg(l+w-rvalue).arg(b).arg(l+w).arg(b+bvalue));
                ui.cuttingLog->append("/** */");
            }
            if (selectedTextureTmp.isOpen()) {
                labelCuttingMask->setStyleSheet(QString(
                    "border-width: %1px %2px %3px %4px;"
                    "border-image: url(':/images/border-image.png') 10 10 10 10 stretch stretch;").arg(tvalue).arg(rvalue).arg(bvalue).arg(lvalue) /* top, right, bottom, left */
                    );
                ui.labelCuttingPreview->setStyleSheet(QString(
                    "border-width: %2px %3px %4px %5px;"
                    "border-image: url('%1') %2 %3 %4 %5 stretch stretch;").arg(selectedTextureTmp.fileName()).arg(tvalue).arg(rvalue).arg(bvalue).arg(lvalue) /* top, right, bottom, left */
                    );
                // float aspect = (float)w/(float)h;
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

void Rockete::resizeCuttingPreview(QResizeEvent * /*event*/)
{
    // resize mask view:
    //labelCuttingMask->setGeometry(ui.labelCuttingPreview->geometry());
    const QPoint origin = ui.tabImage->mapTo(ui.tabWidgetCuttingPreview, ui.labelCuttingPreview->geometry().topLeft());
    labelCuttingMask->setGeometry(origin.x(), origin.y(), ui.labelCuttingPreview->geometry().width(), ui.labelCuttingPreview->geometry().height());
}

void Rockete::cuttingPrevSizeChanged(int /*value*/)
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
                OpenedFile *opened_file = getOpenedFile(file.toUtf8().data(), true);

                QTextCursor cursor = opened_file->textCursor();
                cursor.clearSelection();
                opened_file->setTextCursor(cursor);

                if(opened_file->find(search_string) || opened_file->find(search_string, QTextDocument::FindBackward))
                {
                    ui.codeTabWidget->setCurrentIndex(getTabIndexFromFileName(opened_file->fileInfo.fileName().toUtf8().data()));
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
    ui.codeTabWidget->setCurrentIndex(getTabIndexFromFileName(getCurrentDocument()->fileInfo.fileName().toUtf8().data()));

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
    int tab_index = getTabIndexFromFileName(file_info.fileName().toUtf8().data());
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
        if(getTabIndexFromFileName(file_name.toUtf8().data()) > -1)
        {
            closeTab(getTabIndexFromFileName(file_name.toUtf8().data()), false);
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
    if (!fileChangedOutsideArray.isEmpty())
        qDebug() << "changeEvent with files: " << fileChangedOutsideArray;

    if(event->type() == QEvent::ActivationChange && isActiveWindow() && !fileChangedOutsideArray.isEmpty())
    {
        int new_tab_index;
        const int current_index = ui.codeTabWidget->currentIndex();
        QMapIterator<int, QString> i(fileChangedOutsideArray);
        while (i.hasNext())
        {
            i.next();
            fileChangedOutsideArray.remove(i.key());

            QFileInfo file_info = i.value();
            new_tab_index = getTabIndexFromFileName(file_info.fileName().toUtf8().data());

            bool reload = false;

            if(ui.codeTabWidget->tabText(new_tab_index).contains("*"))
                if( QMessageBox::question(this, "Rockete: file change detected", QFileInfo(i.value()).fileName() + " has been modified,\ndo you want to reload it?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
                    reload = true; // reload silently if no change has been made to the file

            if (reload)
            {
                QWidget *widget;
                isReloadingFile = true;

                closeTab(i.key(), false);
                openFile(i.value());

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
    int response;
    
    // close all tabs:
    while(ui.codeTabWidget->count()>0)
    {
        if(ui.codeTabWidget->tabText(0).startsWith("*"))
        {
            OpenedFile *file = qobject_cast<OpenedFile *>(ui.codeTabWidget->widget(0));
            QString question;
            question = "Save ";
            question += file->fileInfo.fileName();
            question += " before closing?";

            response = QMessageBox::question(this, "file modified", question, QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
        }
        else // if we have no changes, save anyway. I don't want changes to be lost if rockete simply didn't detect a change
        {
            response = QMessageBox::Yes;
        }

        if(response==QMessageBox::Yes)
            closeTab(0, true);
        else if(response==QMessageBox::No)
            closeTab(0, false);
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
        openProject(file_info.absolutePath().toUtf8().constData());
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
                    qWarning("file not found: %s\n", filePath.toUtf8().data());
                    return -1;
                }
            }
        }
    }

    // check if file is already open:
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
        new_tab_index = openDocument(file_info.filePath().toUtf8().data());
    }
    else if (file_info.suffix() == "rcss")
    {
        new_tab_index = openStyleSheet(file_info.filePath().toUtf8().data());
    }
    else if (file_info.suffix() == "lua")
    {
        new_tab_index = openLuaScript(file_info.filePath().toUtf8().data());
    }
    else if (file_info.suffix() == "rproj" || file_info.suffix() == "txt" || file_info.suffix() == "snippet")
    {
        new_tab_index = openASCIIFile(file_info.filePath().toUtf8().data());
    }
    else
    {
        success = false;
    }


    if (success)
    {
        ui.codeTabWidget->setCurrentIndex(new_tab_index);
        ui.codeTabWidget->setTabToolTip(new_tab_index, file_info.absoluteFilePath());
        qInfo("adding file: %s\n", file_info.filePath().toUtf8().data());
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
    texturesAtlasInf.clear();
    Settings::setProject(filePath);
    projectFile = "";

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

void Rockete::openProject(const char *file_path, bool restart)
{
    QFileInfo file_info(file_path);

    if (file_info.suffix() == "rproj")
    {

        if (!ProjectManager::getInstance().Initialize(file_path))
            return; // no file or wrong file

        ui.projectFilesTreeWidget->clear();
        ui.projectFilesTreeWidget->clear();
        texturesAtlasInf.clear();
        Settings::setProject(file_path);
        // save path for reloading action:
        projectFile = file_info.absoluteFilePath();

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

        if (!texturesAtlasInf.isEmpty())
            if(updateTextureInfoFiles() && !restart)
                // reload project if files has been created
                openProject(file_path, true);


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

QString Rockete::readSpriteSheetInfo(QTreeWidgetItem *item, const QString &texture)
{
    // 1. texpack catalog info:
    bool valid_found;
    QImage image(texture);
    QFileInfo tinfo(texture);

    texturesAtlasInf[tinfo.absoluteFilePath()][tinfo.fileName()] = QRect(0, 0, image.width(), image.height()); // texture atlas info

    QString cat_filename = tinfo.absolutePath()+QDir::separator()+QString(tinfo.completeBaseName()+".cat");
    FILE *cat_file = fopen( cat_filename.toUtf8().constData(), "r" );
    if (!cat_file)
        goto format2;
    valid_found = false; while (!feof(cat_file)) {
        int index;
        char tname[256];
        unsigned left;
        unsigned bottom;
        unsigned right;
        unsigned top;
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
                 /*  4 */ &bottom,
                 /*  5 */ &right,
                 /*  6 */ &top,
                 /*  7 */ &left_r,
                 /*  8 */ &bottom_r,
                 /*  9 */ &right_r,
                 /* 10 */ &top_r,
                 /* 11 */ &w,
                 /* 12 */ &h,
                 /* 13 */ &width_r,
                 /* 14 */ &height_r,
                 /* 15 */ &format,
                 /* 16 */ &bpp );
        if (pnum > 4) {
            QTreeWidgetItem *new_item = new QTreeWidgetItem(item);
            QImage copy = image.copy( left, bottom, right-left, top-bottom);
            new_item->setIcon(0, QPixmap::fromImage(copy));
            new_item->setText(1, tname);
            new_item->setToolTip(1, QString("%1:%2").arg(QFileInfo(texture).fileName()).arg(tname));

            QString data = QString("%1px %2px %3px %4px").arg(left).arg(bottom).arg(right-left).arg(top-bottom);
            new_item->setData(0, Qt::UserRole, texture);
            new_item->setData(1, Qt::UserRole, data);

            texturesAtlasInf[tinfo.absoluteFilePath()][tname] = QRect(left, bottom, right-left, top-bottom); // update texture atlas info

            valid_found = true;
        }
    };
    if (valid_found) {
        item->setIcon(0, QIcon(":/images/icon_atlas.png"));
        if (cat_file) fclose(cat_file);
        return cat_filename;
    } // if no valid entry found, continue with next format

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
    QString plist_filename = tinfo.absolutePath()+QDir::separator()+QString(tinfo.completeBaseName()+".plist");
    QFile plistf(plist_filename);
    PListParser plist;
    const QVariant &result = plist.parsePList(&plistf);
    if (!result.isValid())
        goto format3;
    else {
        valid_found = false;

        const QVariantMap &map = result.toMap(); if (map.contains("frames")) {
            const QVariantMap &frames = map["frames"].toMap();
            foreach(const QString &tname, frames.keys()) {
                const QVariantMap &frame = frames.value(tname).toMap();
                const QString rect_s = frame["frame"].toString();
                int l, b, w, h, pnum = sscanf( rect_s.toUtf8().constData(), "{{%d,%d},{%d,%d}}", &l, &b, &w, &h); if (pnum != 4)
                    qInfo("Invalid rect data %s", rect_s.toUtf8().constEnd());
                else {
                    QTreeWidgetItem *new_item = new QTreeWidgetItem(item);
                    QImage copy = image.copy( l, b, w, h);
                    new_item->setText(1, tname);
                    new_item->setIcon(0, QPixmap::fromImage(copy));
                    new_item->setToolTip(1, QString("%1:%2").arg(QFileInfo(texture).fileName()).arg(tname));

                    QString data = QString("%1px %2px %3px %4px").arg(l).arg(b).arg(w).arg(h);
                    new_item->setData(0, Qt::UserRole, texture);
                    new_item->setData(1, Qt::UserRole, data);

                    texturesAtlasInf[tinfo.absoluteFilePath()][tname] = QRect(l, b, w, h); // update texture atlas info

                    valid_found = true;
                }
            }
        }
    }
    if (valid_found) {
        item->setIcon(0, QIcon(":/images/icon_atlas.png"));
        return plist_filename;
    } // if no valid entry found, continue with next format

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
format3:
    // 3. smc
    QString smc_filename = tinfo.absolutePath()+QDir::separator()+QString(tinfo.completeBaseName()+".smc");
    FILE *smc_file = fopen( smc_filename.toUtf8().constData(), "r" );
    if (!smc_file)
        return "";
    valid_found = false;

    // no catalog found or no valid entries.
    return "";
}

bool Rockete::updateTextureInfoFiles()
{
    const QString dummy;
    return updateTextureInfoFiles(dummy);
}

bool Rockete::updateTextureInfoFiles(const QString &force)
{
    bool ret_info = false;
    QFileInfo finfo(force);
    foreach(const QString &atlas_filename, texturesAtlasInf.keys()) {

        const QMap<QString, QRect> &atlas = texturesAtlasInf[atlas_filename];
        QFileInfo tinfo(atlas_filename);

        QString css_filename = tinfo.absolutePath()+QDir::separator()+QString(tinfo.completeBaseName()+".rcss"); // css info file
        if (QFile::exists(css_filename)) {
            if (force.isEmpty() || tinfo.baseName() != finfo.baseName()) {
                if (tinfo.lastModified() < QFileInfo(css_filename).lastModified() ) {
                    qDebug() << "Skipping update of " << css_filename;
                    continue;
                }
            }
        } else
            ret_info = true; // file will be created - should be added to the project
        qDebug() << "Updating " << css_filename;

        FILE *css_file = fopen( css_filename.toUtf8().constData(), "w" );
        if (!css_file)
            qInfo("Failed to open %s file.", css_filename.toUtf8().constData());
        else {
            fprintf(css_file, "/* Autogenerated file. DO NOT MODIFY - WILL BE OVERWRITTEN! */\n");
            fprintf(css_file, "/* Created at: %s */\n\n", QTime::currentTime().toString().toLatin1().constData());
            foreach(const QString &texture, atlas.keys()) {
                const QRect &rc = atlas[texture];
                // check border cutting information
                const CssCuttingInfo inf = ProjectManager::getInstance().getCuttingInfo(tinfo.fileName()+":"+texture); if (!inf.isEmpty()) {
                    const int &l=rc.left(), &b=rc.top(), &w=rc.width(), &h=rc.height();
                    const int &lvalue=inf.left, &rvalue=inf.right, &tvalue=inf.top, &bvalue=inf.bottom;
                    if (tvalue == 0 && bvalue == 0)
                        fprintf(css_file,
                            "/* cutting: l%d|r%d|b%d|t%d */\n"
                            ".%s {\n"
                            "  background1-decorator: tiled-horizontal;\n"
                            "  background1-decorator-id: %s;\n"
                            "  background1-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background1-center-image: %s stretch %dpx %dpx %dpx %dpx;\n"
                            "  background1-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background2-decorator: tiled-horizontal;\n"
                            "  background2-decorator-id: %s-repeat-truncate;\n"
                            "  background2-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background2-center-image: %s repeat-truncate %dpx %dpx %dpx %dpx;\n"
                            "  background2-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background3-decorator: tiled-horizontal;\n"
                            "  background3-decorator-id: %s-repeat-stretch;\n"
                            "  background3-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background3-center-image: %s repeat-truncate %dpx %dpx %dpx %dpx;\n"
                            "  background3-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background4-decorator: tiled-horizontal;\n"
                            "  background4-decorator-id: %s-clamp-truncate;\n"
                            "  background4-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background4-center-image: %s clamp-truncate %dpx %dpx %dpx %dpx;\n"
                            "  background4-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background5-decorator: tiled-horizontal;\n"
                            "  background5-decorator-id: %s-clamp-stretch;\n"
                            "  background5-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background5-center-image: %s clamp-stretch %dpx %dpx %dpx %dpx;\n"
                            "  background5-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "}\n\n"
                            , lvalue, rvalue, bvalue, tvalue
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , tinfo.fileName().toUtf8().constData(), l, b, l+lvalue, b+h
                            , tinfo.fileName().toUtf8().constData(), l+lvalue, b, l+w-rvalue, b+h
                            , tinfo.fileName().toUtf8().constData(), l+w-rvalue, b, l+w, b+h
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , tinfo.fileName().toUtf8().constData(), l, b, l+lvalue, b+h
                            , tinfo.fileName().toUtf8().constData(), l+lvalue, b, l+w-rvalue, b+h
                            , tinfo.fileName().toUtf8().constData(), l+w-rvalue, b, l+w, b+h
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , tinfo.fileName().toUtf8().constData(), l, b, l+lvalue, b+h
                            , tinfo.fileName().toUtf8().constData(), l+lvalue, b, l+w-rvalue, b+h
                            , tinfo.fileName().toUtf8().constData(), l+w-rvalue, b, l+w, b+h
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , tinfo.fileName().toUtf8().constData(), l, b, l+lvalue, b+h
                            , tinfo.fileName().toUtf8().constData(), l+lvalue, b, l+w-rvalue, b+h
                            , tinfo.fileName().toUtf8().constData(), l+w-rvalue, b, l+w, b+h
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , tinfo.fileName().toUtf8().constData(), l, b, l+lvalue, b+h
                            , tinfo.fileName().toUtf8().constData(), l+lvalue, b, l+w-rvalue, b+h
                            , tinfo.fileName().toUtf8().constData(), l+w-rvalue, b, l+w, b+h
                        );
                    else if (lvalue == 0 && rvalue == 0)
                        fprintf(css_file,
                            "/* cutting: l%d|r%d|b%d|t%d */\n"
                            ".%s {\n"
                            "  background1-decorator: tiled-vertical;\n"
                            "  background1-decorator-id: %s;\n"
                            "  background1-bottom-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background1-center-image: %s stretch %dpx %dpx %dpx %dpx;\n"
                            "  background1-top-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background2-decorator: tiled-vertical;\n"
                            "  background2-decorator-id: %s-repeat-truncate;\n"
                            "  background2-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background2-center-image: %s repeat-truncate %dpx %dpx %dpx %dpx;\n"
                            "  background2-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background3-decorator: tiled-vertical;\n"
                            "  background3-decorator-id: %s-repeat-stretch;\n"
                            "  background3-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background3-center-image: %s repeat-truncate %dpx %dpx %dpx %dpx;\n"
                            "  background3-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background4-decorator: tiled-vertical;\n"
                            "  background4-decorator-id: %s-clamp-truncate;\n"
                            "  background4-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background4-center-image: %s clamp-truncate %dpx %dpx %dpx %dpx;\n"
                            "  background4-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background5-decorator: tiled-vertical;\n"
                            "  background5-decorator-id: %s-clamp-stretch;\n"
                            "  background5-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background5-center-image: %s clamp-stretch %dpx %dpx %dpx %dpx;\n"
                            "  background5-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "}\n\n"
                            , lvalue, rvalue, bvalue, tvalue
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , tinfo.fileName().toUtf8().constData(), l, b+h-tvalue, l+w, b+h
                            , tinfo.fileName().toUtf8().constData(), l, b+bvalue, l+w, b+h-tvalue
                            , tinfo.fileName().toUtf8().constData(), l, b, l+w, b+bvalue
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , tinfo.fileName().toUtf8().constData(), l, b+h-tvalue, l+w, b+h
                            , tinfo.fileName().toUtf8().constData(), l, b+bvalue, l+w, b+h-tvalue
                            , tinfo.fileName().toUtf8().constData(), l, b, l+w, b+bvalue
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , tinfo.fileName().toUtf8().constData(), l, b+h-tvalue, l+w, b+h
                            , tinfo.fileName().toUtf8().constData(), l, b+bvalue, l+w, b+h-tvalue
                            , tinfo.fileName().toUtf8().constData(), l, b, l+w, b+bvalue
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , tinfo.fileName().toUtf8().constData(), l, b+h-tvalue, l+w, b+h
                            , tinfo.fileName().toUtf8().constData(), l, b+bvalue, l+w, b+h-tvalue
                            , tinfo.fileName().toUtf8().constData(), l, b, l+w, b+bvalue
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , tinfo.fileName().toUtf8().constData(), l, b+h-tvalue, l+w, b+h
                            , tinfo.fileName().toUtf8().constData(), l, b+bvalue, l+w, b+h-tvalue
                            , tinfo.fileName().toUtf8().constData(), l, b, l+w, b+bvalue
                        );
                    else /* tile-box decorator */
                        fprintf(css_file,
                            "/* cutting: l%d|r%d|b%d|t%d */\n"
                            ".%s {\n"
                            "  background1-decorator: tiled-box;\n"
                            "  background1-decorator-id: %s;\n"
                            "  background1-bottom-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background1-bottom-image: %s stretch %dpx %dpx %dpx %dpx;\n"
                            "  background1-bottom-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background1-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background1-center-image: %s stretch %dpx %dpx %dpx %dpx;\n"
                            "  background1-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background1-top-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background1-top-image: %s stretch %dpx %dpx %dpx %dpx;\n"
                            "  background1-top-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background2-decorator: tiled-box;\n"
                            "  background2-decorator-id: %s-repeat-truncate;\n"
                            "  background2-bottom-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background2-bottom-image: %s repeat-truncate %dpx %dpx %dpx %dpx;\n"
                            "  background2-bottom-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background2-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background2-center-image: %s repeat-truncate %dpx %dpx %dpx %dpx;\n"
                            "  background2-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background2-top-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background2-top-image: %s repeat-truncate %dpx %dpx %dpx %dpx;\n"
                            "  background2-top-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background3-decorator: tiled-box;\n"
                            "  background3-decorator-id: %s-repeat-stretch;\n"
                            "  background3-bottom-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background3-bottom-image: %s repeat-stretch %dpx %dpx %dpx %dpx;\n"
                            "  background3-bottom-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background3-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background3-center-image: %s repeat-stretch %dpx %dpx %dpx %dpx;\n"
                            "  background3-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background3-top-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background3-top-image: %s repeat-stretch %dpx %dpx %dpx %dpx;\n"
                            "  background3-top-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background4-decorator: tiled-box;\n"
                            "  background4-decorator-id: %s-clamp-truncate;\n"
                            "  background4-bottom-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background4-bottom-image: %s clamp-truncate %dpx %dpx %dpx %dpx;\n"
                            "  background4-bottom-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background4-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background4-center-image: %s clamp-truncate %dpx %dpx %dpx %dpx;\n"
                            "  background4-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background4-top-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background4-top-image: %s clamp-truncate %dpx %dpx %dpx %dpx;\n"
                            "  background4-top-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background5-decorator: tiled-box;\n"
                            "  background5-decorator-id: %s-clamp-stretch;\n"
                            "  background5-bottom-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background5-bottom-image: %s clamp-stretch %dpx %dpx %dpx %dpx;\n"
                            "  background5-bottom-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background5-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background5-center-image: %s clamp-stretch %dpx %dpx %dpx %dpx;\n"
                            "  background5-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background5-top-left-image: %s %dpx %dpx %dpx %dpx;\n"
                            "  background5-top-image: %s clamp-stretch %dpx %dpx %dpx %dpx;\n"
                            "  background5-top-right-image: %s %dpx %dpx %dpx %dpx;\n"
                            "}\n\n"
                            , lvalue, rvalue, bvalue, tvalue
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
        #define _decorator_tiled_box() \
                            , tinfo.fileName().toUtf8().constData(), l, b+h-bvalue, l+lvalue, b+h \
                            , tinfo.fileName().toUtf8().constData(), l+lvalue, b+h-bvalue, l+w-rvalue, b+h \
                            , tinfo.fileName().toUtf8().constData(), l+w-rvalue, b+h-bvalue, l+w, b+h \
                            , tinfo.fileName().toUtf8().constData(), l, b+tvalue, l+lvalue, b+h-bvalue \
                            , tinfo.fileName().toUtf8().constData(), l+lvalue, b+tvalue, l+w-rvalue, b+h-bvalue \
                            , tinfo.fileName().toUtf8().constData(), l+w-rvalue, b+tvalue, l+w, b+h-bvalue \
                            , tinfo.fileName().toUtf8().constData(), l, b, l+lvalue, b+tvalue \
                            , tinfo.fileName().toUtf8().constData(), l+lvalue, b, l+w-rvalue, b+tvalue \
                            , tinfo.fileName().toUtf8().constData(), l+w-rvalue, b, l+w, b+tvalue
                                _decorator_tiled_box()
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                                _decorator_tiled_box()
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                                _decorator_tiled_box()
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                                _decorator_tiled_box()
                            , QFileInfo(texture).completeBaseName().toUtf8().constData()
                                _decorator_tiled_box()
                        );
                } else fprintf(css_file,
                    ".%s {\n"
                    "  width: %dpx;\n"
                    "  height: %dpx;\n"
                    "  icon-decorator: image;\n"
                    "  icon-decorator-id: %s;\n"
                    "  icon-image: %s %dpx %dpx %dpx %dpx;\n"
                    "}\n\n"
                    ".%s-scaled {\n"
                    "  icon1-decorator: image;\n"
                    "  icon1-decorator-id: %s-fit;\n"
                    "  icon1-image: %s %dpx %dpx %dpx %dpx;\n"
                    "  icon1-image-scaling: fit;\n"
                    "  icon2-decorator: image;\n"
                    "  icon2-decorator-id: %s-fill;\n"
                    "  icon2-image: %s %dpx %dpx %dpx %dpx;\n"
                    "  icon2-image-scaling: fill;\n"
                    "  icon3-decorator: image;\n"
                    "  icon3-decorator-id: %s-center;\n"
                    "  icon3-image: %s %dpx %dpx %dpx %dpx;\n"
                    "  icon3-image-scaling: center;\n"
                    "}\n\n"
                    , QFileInfo(texture).completeBaseName().toUtf8().constData()
                    , rc.width(), rc.height()
                    , QFileInfo(texture).completeBaseName().toUtf8().constData()
                    , tinfo.fileName().toUtf8().constData()
                    , rc.left(), rc.top(), rc.right(), rc.bottom()
                    , QFileInfo(texture).completeBaseName().toUtf8().constData()
                    , QFileInfo(texture).completeBaseName().toUtf8().constData()
                    , tinfo.fileName().toUtf8().constData()
                    , rc.left(), rc.top(), rc.right(), rc.bottom()
                    , QFileInfo(texture).completeBaseName().toUtf8().constData()
                    , tinfo.fileName().toUtf8().constData()
                    , rc.left(), rc.top(), rc.right(), rc.bottom()
                    , QFileInfo(texture).completeBaseName().toUtf8().constData()
                    , tinfo.fileName().toUtf8().constData()
                    , rc.left(), rc.top(), rc.right(), rc.bottom()
                );
            }
            fclose(css_file);
        }
    }
    return ret_info;
}

void Rockete::populateTreeView(const QString &top_item_name, const QString &directory_path)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(ui.projectFilesTreeWidget, QStringList(top_item_name));
    QList<QTreeWidgetItem *> items;

    QDirIterator directory_walker(directory_path, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);

    QList<QString> ainfo_found;
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
                QString ainfo = readSpriteSheetInfo(new_item, directory_walker.fileInfo().absoluteFilePath());
                if (!ainfo.isEmpty())
                    ainfo_found.append(QFileInfo(ainfo).baseName());
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
                if (ainfo_found.contains(directory_walker.fileInfo().baseName())) {
                    new_item->setIcon(0, QIcon(":/images/icon_rcss_auto.png"));
                    new_item->setData(2, Qt::UserRole, "auto"); // autogenerated file
                }
            }
            else if(directory_walker.fileInfo().suffix() == "lua")
            {
                new_item->setIcon(0, QIcon(":/images/icon_lua.png"));
            }
            else if(directory_walker.fileInfo().suffix() == "ttf" || directory_walker.fileInfo().suffix() == "otf" || directory_walker.fileInfo().suffix() == "fon") {
                // accept freetype fonts
                new_item->setIcon(0, QIcon(":/images/icon_fnt.png"));
            }
            else if(directory_walker.fileInfo().suffix() == "bmf" || directory_walker.fileInfo().suffix() == "fnt") {
                // accept bitmap fonts
                new_item->setIcon(0, QIcon(":/images/icon_bmp_fnt.png"));
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
    #elif defined(Q_OS_MAC) || defined(Q_WS_MAC)
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
    qInfo("removing path: %s\n", file->fileInfo.filePath().toUtf8().data());
    fileChangedOutsideArray.remove(index);
    fileWatcher->removePath(file->fileInfo.filePath());

    if(doc)
    {
        if(getCurrentDocument() && getCurrentDocument()->rocketDocument == doc->rocketDocument)
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

void Rockete::splitterMovedChanges(int /* pos */, int /* index */)
{
    QSplitter *splitter = (QSplitter*)sender();
    Settings::setSplitterState(splitter->objectName(), splitter->saveState());
}

void Rockete::logMessage(QString aMsg)
{
    // use small tag for all log messages - default text is too big.
    logHtmlMessage("<small>"+aMsg+"</small>");
}

void Rockete::logHtmlMessage(QString aMsg)
{
        if(ui.logWindow!=NULL && !aMsg.isEmpty())
                ui.logWindow->append(aMsg);
}

Rockete *Rockete::instance = NULL;
