#ifndef WORKAREA_H
#define WORKAREA_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

#include "texturemodel.h"

class WorkArea : public QWidget
{
	Q_OBJECT
public:
	WorkArea(QWidget *parent = 0);
	~WorkArea();

	void setTextureModel(TextureModel *_textureModel);

protected:
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);

private:
	void drawChessBoard(QPainter *painter);

public slots:
	void setBinding(bool isBinding){ binding=isBinding; }
	void textureDeleted(){ selectedTexture=0; }

public:
	TextureModel *textureModel;
	TTexture *selectedTexture;
	QPointF localPos;
	bool binding;
};

#endif // WORKAREA_H
