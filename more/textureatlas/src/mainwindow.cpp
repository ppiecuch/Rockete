#include "mainwindow.h"

#include <QApplication>
 #include <QMenuBar>
 #include <QGroupBox>
 #include <QGridLayout>
 #include <QSlider>
 #include <QLabel>
 #include <QTimer>
#include <QSettings>


MainWindow::MainWindow()
{
	ui.setupUi(this);

	this->setWindowTitle(tr("Texture Atlas Maker v0.96  (18-08-2011)"));

	readSettings();

///////////////////////////////////////////////
	QGridLayout *gridLayout_2 = new QGridLayout(ui.page_textures);



	TextureListWidget *listViewTextures = new TextureListWidget(ui.page_textures);
	listViewTextures->setObjectName(QString::fromUtf8("listViewTextures"));
			QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Expanding);
			sizePolicy2.setHorizontalStretch(0);
			sizePolicy2.setVerticalStretch(0);
			sizePolicy2.setHeightForWidth(listViewTextures->sizePolicy().hasHeightForWidth());
			listViewTextures->setSizePolicy(sizePolicy2);
	gridLayout_2->addWidget(listViewTextures, 0, 0, 1, 4);


	QToolButton *toolButtonAddFile = new QToolButton(ui.page_textures);
	toolButtonAddFile->setText(tr("add file"));
	toolButtonAddFile->setObjectName(QString::fromUtf8("toolButtonAddFile"));
	gridLayout_2->addWidget(toolButtonAddFile, 1, 0, 1, 2);

	QToolButton *toolButtonAddFolder = new QToolButton(ui.page_textures);
	toolButtonAddFolder->setText(tr("add folder"));
	toolButtonAddFolder->setObjectName(QString::fromUtf8("toolButtonAddFolder"));
	gridLayout_2->addWidget(toolButtonAddFolder, 2, 0, 1, 2);

	QToolButton *toolExport = new QToolButton(ui.page_textures);
	toolExport->setText(tr("export"));
	toolExport->setToolTip(tr("Export selected images"));
	gridLayout_2->addWidget(toolExport, 1, 2, 1, 2);


	QToolButton *toolButtonClear = new QToolButton(ui.page_textures);
	toolButtonClear->setText(tr("clear"));
	toolButtonClear->setObjectName(QString::fromUtf8("toolButtonClear"));
	gridLayout_2->addWidget(toolButtonClear, 2, 2, 1, 2);




	comboBoxResolution = new QComboBox(ui.page_textures);
	comboBoxResolution->setObjectName(QString::fromUtf8("comboBoxResolution"));
	gridLayout_2->addWidget(comboBoxResolution, 3, 0, 1, 2);




	QToolButton *toolButtonAddResolution = new QToolButton(ui.page_textures);
	toolButtonAddResolution->setText(tr("+"));
	toolButtonAddResolution->setObjectName(QString::fromUtf8("toolButtonAddResolution"));
	toolButtonAddResolution->setArrowType(Qt::NoArrow);

	gridLayout_2->addWidget(toolButtonAddResolution, 3, 2, 1, 1);
////////////////////////////////////////

	/////////////////


	comboBoxResolution->addItem("2048*2048", 2048);
	comboBoxResolution->addItem("1024*1024", 1024);
	comboBoxResolution->addItem("512*512", 512);
	comboBoxResolution->addItem("256*256", 256);
	comboBoxResolution->addItem("128*128", 128);

	textureModel = new TextureModel(this);

	listViewTextures->setModel(textureModel);

	ui.workArea->setAcceptDrops(true);
	ui.workArea->setTextureModel(textureModel);
	ui.workArea->setUpdatesEnabled(true);
	ui.workArea->update();


	connect(toolButtonAddFile,SIGNAL(clicked(bool)), this,SLOT(AddFile()));
	connect(toolButtonAddFolder,SIGNAL(clicked(bool)), this,SLOT(AddFolder()));
	connect(toolButtonClear,SIGNAL(clicked(bool)), textureModel,SLOT(clear()));
	connect(toolExport,SIGNAL(clicked(bool)), listViewTextures,SLOT(saveSelectedImages()));


	connect(comboBoxResolution,SIGNAL(currentIndexChanged(int)), this,SLOT(resolutionAtlasChange()));
	connect(toolButtonAddResolution,SIGNAL(clicked(bool)), this,SLOT(AddNewResolution()));



	QAction *bindingAction = ui.toolBar->addAction(tr("binding"));
	bindingAction->setCheckable(true);
	bindingAction->setChecked(false);
	connect(bindingAction, SIGNAL(triggered(bool)), ui.workArea,SLOT(setBinding(bool)));

	QAction *remakeAction = ui.toolBar->addAction(tr("remake"));
	connect(remakeAction,SIGNAL(triggered(bool)), textureModel,SLOT(arrangeImages()));


	QAction *autoRemakeAction = ui.toolBar->addAction(tr("auto arrange"));
	autoRemakeAction->setCheckable(true);
	autoRemakeAction->setChecked(textureModel->isAutoArrangeImages());
	connect(autoRemakeAction, SIGNAL(triggered(bool)), textureModel,SLOT(setAutoArrangeImages(bool)));


	QAction *loadAction = new QAction(tr("&Open"), this);
	connect(loadAction,SIGNAL(triggered(bool)), this,SLOT(loadFile()));
	loadAction->setShortcut(QKeySequence::Open);

	QAction *saveAction = new QAction(tr("&Save"), this);
	saveAction->setShortcut(QKeySequence::Save);
	connect(saveAction,SIGNAL(triggered(bool)), this,SLOT(save()));

	QAction *saveAsAct = new QAction(tr("Save &As..."), this);
	saveAsAct->setShortcuts(QKeySequence::SaveAs);
	saveAsAct->setStatusTip(tr("Save the atlas under a new name"));
	connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

	QAction *exitAct = new QAction(tr("E&xit"), this);
	exitAct->setShortcuts(QKeySequence::Quit);
	exitAct->setStatusTip(tr("Exit the application"));
	connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

	QMenu *fileMenuAct = ui.menubar->addMenu(tr("File"));
	fileMenuAct->addAction(loadAction);
	fileMenuAct->addSeparator();
	fileMenuAct->addAction(saveAction);
	fileMenuAct->addAction(saveAsAct);
	fileMenuAct->addSeparator();
	fileMenuAct->addAction(exitAct);


	ui.workArea->setBinding(bindingAction->isChecked());


	connect(textureModel,SIGNAL(cantMakeAtlas()), this,SLOT(CantMakeAtlas()));

	comboBoxResolution->setCurrentIndex(1);
	resolutionAtlasChange();
}

MainWindow::~MainWindow()
{
	writeSettings();
}


void MainWindow::readSettings()
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope,
					   QCoreApplication::organizationName(), QCoreApplication::applicationName());

	settings.beginGroup("general");
	lastDir = settings.value("last_path", QString("")).toString();
	settings.endGroup();
}

void MainWindow::writeSettings()
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope,
					   QCoreApplication::organizationName(), QCoreApplication::applicationName());

	settings.beginGroup("general");
	settings.setValue("last_path", lastDir);
	settings.endGroup();
}


void MainWindow::resolutionAtlasChange()
{
	int w= comboBoxResolution->itemData(comboBoxResolution->currentIndex()).toInt();
	ui.workArea->setUpdatesEnabled(false);
	ui.workArea->textureDeleted();
	textureModel->setAtlasSize(w,w);
	ui.workArea->setUpdatesEnabled(true);
}

void MainWindow::setCurrentFileName(const QString &fileName)
{
	this->curFileName = fileName;

	QString shownName;
	if (curFileName.isEmpty())
		shownName = "untitled";
	else
		shownName = QFileInfo(curFileName).fileName();

	setWindowTitle(shownName);
	setWindowModified(false);
}

void MainWindow::loadFile()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Load Atlas..."),
					lastDir, tr("Atlas files (*.png);;All Files (*)"));

	if ((!path.isEmpty()) && (QFile::exists(path)))
	{
		QFileInfo fi(path);
		QString dir = fi.path();
		if (dir.at(dir.length()-1) !=  QDir::separator())
			dir.append(QDir::separator());
		lastDir = dir;

		ui.workArea->setUpdatesEnabled(false);
		ui.workArea->textureDeleted();
		textureModel->LoadAtlas(path);
		ui.workArea->setUpdatesEnabled(true);
		ui.workArea->update();
		setCurrentFileName(path);
	}
}


bool MainWindow::saveAs()
{
	QString fn = QFileDialog::getSaveFileName(this, tr("Save Atlas"),
					lastDir,	QString());
	if (fn.isEmpty())
		return false;
	return saveFile(fn);
}


bool MainWindow::save()
{
	if (curFileName.isEmpty())
		return saveAs();
	else
		return saveFile(curFileName);
}


bool MainWindow::saveFile(QString _fullPath)
{
	QFileInfo fi(_fullPath);
	QString dir = fi.path();
	if (dir.at(dir.length()-1) !=  QDir::separator())
		dir.append(QDir::separator());
	lastDir = dir;

	ui.workArea->setUpdatesEnabled(false);
	textureModel->SaveAtlas(_fullPath);
	ui.workArea->setUpdatesEnabled(true);
	ui.workArea->update();

	setCurrentFileName(_fullPath);
	statusBar()->showMessage(tr("File saved"), 2000);
	return true;
}


void MainWindow::AddFile()
{
	QStringList files = QFileDialog::getOpenFileNames(this, tr("Add files..."),
					lastDir,
					tr("Image Files (*.bmp *.jpg *jpeg *png *tiff);; PNG file (*.png);; JPG file (*.jpg *jpeg);; BMP file (*.bmp);; All Files (*)"));

	QStringList list = files;
	if (list.size()>0)
	{
		QFileInfo fi(list.first());
		QString dir = fi.path();
		if (dir.at(dir.length()-1) !=  QDir::separator())
			dir.append(QDir::separator());
		lastDir = dir;

		ui.workArea->setUpdatesEnabled(false);
		textureModel->addTextures(list);
		ui.workArea->setUpdatesEnabled(true);
		ui.workArea->update();
	}
}

void MainWindow::AddFolder()
{
	QString dirPath = QFileDialog::getExistingDirectory(this, tr("Open image folder"),
													 lastDir,
													 QFileDialog::ShowDirsOnly
													 | QFileDialog::DontResolveSymlinks);
	if (!dirPath.isEmpty())
	{
		lastDir=dirPath;
		ui.workArea->setUpdatesEnabled(false);
		textureModel->addDir(dirPath);
		ui.workArea->setUpdatesEnabled(true);
		ui.workArea->update();
	}
}

void MainWindow::AddNewResolution()
{
	bool ok;
	int i = QInputDialog::getInteger(this, tr("Add new atlas resolution"),
									 tr("Resolution:"), 1024, 1, 100000, 1, &ok);
	if (ok)
	{
		QString s = QString::number(i);
		s = s+"*"+s;
		comboBoxResolution->addItem(s, i);
	}
}


void MainWindow::CantMakeAtlas()
{
	QMessageBox::warning(0, tr("Texture Atlas Maker"),
					tr("Can't make texture atlas\n"), QMessageBox::Ok);
}

