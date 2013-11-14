#ifndef ARRANGETHREAD_H
#define ARRANGETHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "common.h"

void recursivePacking(fsRect *S2, QVector <TTexture *> *_tempTextures);

class ArrangeThread : public QThread
{
	Q_OBJECT

public:
	ArrangeThread(QObject *parent = 0);
	~ArrangeThread();

	void arrangeImages(QVector <TTexture> *_textures, float _atlasWidth, float _atlasHeight);
	void cancel();

signals:
	void changeProgress(int percent);
	void arranged();
	void canceled();
	void cantMakeAtlas();

protected:
	void run();

private:
	bool abort;
	bool restart;
	QMutex mutex;
	QWaitCondition condition;

	QVector <TTexture> *textures0;

	float atlasWidth0;
	float atlasHeight0;
};

#endif // ARRANGETHREAD_H
