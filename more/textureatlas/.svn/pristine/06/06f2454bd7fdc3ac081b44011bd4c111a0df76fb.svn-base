#ifndef TEXTURELISTWIDGET_H
#define TEXTURELISTWIDGET_H

#include <QListView>
#include <QListWidgetItem>

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QUrl>
#include <QFileDialog>

#include "texturemodel.h"


class TextureListWidget : public QListView
{
	Q_OBJECT

public:
	TextureListWidget(QWidget *parent = 0);

protected slots:
	void itemClicked();
	void saveSelectedImages();
protected:
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);
	void startDrag(Qt::DropActions supportedActions);
	void keyPressEvent(QKeyEvent * event);
};

#endif // TEXTURELISTWIDGET_H
