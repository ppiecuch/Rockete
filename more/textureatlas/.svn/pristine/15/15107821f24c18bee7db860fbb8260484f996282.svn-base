#include <QApplication>
#include <QTextCodec>

#include "mainwindow.h"

/// Add multi selection in tree.

int main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(texture);
	QApplication a(argc, argv);

	QTextCodec *codec = QTextCodec::codecForName("UTF-8");
	QTextCodec::setCodecForCStrings(codec);
	QTextCodec::setCodecForLocale(codec);
	QTextCodec::setCodecForTr(codec);

	QCoreApplication::setOrganizationName("milytia");
	//QCoreApplication::setOrganizationDomain("");
	QCoreApplication::setApplicationName("texture_atlas_maker");

	MainWindow w;
	w.show();
	//w.move(800,200);
	//w.showMaximized();
	return a.exec();
}
