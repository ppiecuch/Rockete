#include "arrangethread.h"


void recursivePacking(fsRect *S2, QVector <TTexture *> *_tempTextures)
{
	QVector <TTexture *> &tempTextures = *_tempTextures;
	float dp=1;
	for (int i=0; i<tempTextures.size(); i++)
		if ((!tempTextures[i]->isPacked) &&((tempTextures[i]->img.width()+2*dp) <= S2->w) &&
			(((tempTextures[i]->img.height()+2*dp) <= S2->h)))
		{
			tempTextures[i]->x = S2->x+dp;
			tempTextures[i]->y = S2->y+dp;
			tempTextures[i]->isPacked = true;

			fsRect S3,S4;
			S3 = fsRect(S2->x, S2->y+tempTextures[i]->img.height()+2*dp,
						tempTextures[i]->img.width()+2*dp, S2->h - tempTextures[i]->img.height()-2*dp);
			S4 = fsRect(S2->x+tempTextures[i]->img.width()+2*dp, S2->y,
						S2->w - tempTextures[i]->img.width()-2*dp, S2->h);
			if (S3.w*S3.h > S4.w*S4.h)
			{
				*S2 = S3;
				recursivePacking(S2, &tempTextures);
				*S2 = S4;
				recursivePacking(S2, &tempTextures);
			}
			else
			{
				*S2 = S4;
				recursivePacking(S2, &tempTextures);
				*S2 = S3;
				recursivePacking(S2, &tempTextures);
			}
		}
}


ArrangeThread::ArrangeThread(QObject *parent)
	: QThread(parent)
{
	abort = false;
	restart = false;
	textures0=0;
}

ArrangeThread::~ArrangeThread()
{
	mutex.lock();
	abort = true;
	condition.wakeOne();
	mutex.unlock();
	wait();
}


void ArrangeThread::cancel()
{
	QMutexLocker locker(&mutex);
	if (isRunning())
	{
		abort=true;
		condition.wakeOne();
	}
}


void ArrangeThread::arrangeImages(QVector <TTexture> *_textures, float _atlasWidth, float _atlasHeight)
{
	QMutexLocker locker(&mutex);

	textures0 = _textures;
	atlasWidth0 = _atlasWidth;
	atlasHeight0=_atlasHeight;

	abort=false;
	restart=false;
	if (!isRunning())
	{
		start(LowPriority);
	}
	else
	{
		restart = true;
		condition.wakeOne();
	}
}


void ArrangeThread::run()
{
	forever
	{
		mutex.lock();
		QVector <TTexture> &textures = *textures0;
		float atlasWidth = atlasWidth0;
		float atlasHeight = atlasHeight0;
		mutex.unlock();

		QVector <TTexture *> tempTextures;


		QVector <QPoint> optimTex;
		tempTextures.clear();
		for (int t=0; t<textures.size(); t++)
		{
			textures[t].texNum=t;
			tempTextures.push_back(&textures[t]);
			optimTex.push_back(QPoint(0,0));
		}

		float totalHeight = 0;
		float minTotalHeight = 999999999;
		float dp=1;

		int countSteps = 0;
		//FIXME:bad code
		for (int i=0; i<textures.size(); i++)
			for (int j=i; j<textures.size(); j++)
				countSteps++;
		if (countSteps==0)
			countSteps=1;
		int currentStep=0;
		//int currentPercent=0;

		emit changeProgress(0);




		for (int i=0; i<textures.size(); i++)
		{
			for (int j=i; j<textures.size(); j++)
			{
				if (restart)
					break;
				if (abort)
				{
					emit canceled();
					return;
				}

				emit changeProgress((currentStep*100)/countSteps);

				/*
				if (progress.wasCanceled())
				{
					for (int t=0; t<textures.size(); t++)
					{
						textures[t].x = optimTex[t].x();
						textures[t].y = optimTex[t].y();
					}
					makeAtlas();
					return;
				}
				*/

				qSwap(tempTextures[i], tempTextures[j]);

				fsRect S;
				S = fsRect(0,0, atlasWidth, -1);
				totalHeight = 0;

				for (int t=0; t<tempTextures.size(); t++)
				{
					tempTextures[t]->x = dp;
					tempTextures[t]->y = dp;
					tempTextures[t]->isPacked = false;
				}

				bool canMake = true;
				for (int t=0; t<tempTextures.size(); t++)
				{
					if (restart)
						break;
					if (abort)
					{
						emit canceled();
						return;
					}

					if (tempTextures[t]->isPacked)
						continue;

					totalHeight += tempTextures[t]->img.height()+2*dp;

					if ((tempTextures[t]->img.width()+2*dp) > atlasWidth)
					{
						canMake=false;
						break;
					}

					tempTextures[t]->x = S.x+dp;
					tempTextures[t]->y = S.y+dp;
					tempTextures[t]->isPacked = true;

					fsRect S2,S1;
					S2 = fsRect(S.x+tempTextures[t]->img.width()+2*dp, S.y,
								S.w - tempTextures[t]->img.width()-2*dp, tempTextures[t]->img.height()+2*dp);
					S1 = fsRect(S.x, S.y+tempTextures[t]->img.height()+2*dp,
								S.w, -1);

					S = S1;
					recursivePacking(&S2, &tempTextures);
				}

				if ((canMake) && (totalHeight<minTotalHeight))
				{
					minTotalHeight = totalHeight;
					for (int t=0; t<tempTextures.size(); t++)
					{
						optimTex[tempTextures[t]->texNum].setX(tempTextures[t]->x);
						optimTex[tempTextures[t]->texNum].setY(tempTextures[t]->y);
					}
				}

				currentStep++;
				//emit currentProgress(currentPercent);
			}

			if (restart)
				break;
			if (abort)
			{
				emit canceled();
				return;
			}
		}

		for (int t=0; t<textures.size(); t++)
		{
			textures[t].x = optimTex[t].x();
			textures[t].y = optimTex[t].y();
		}

		if (!restart)
		{
			if ((textures.size()>0) &&(minTotalHeight > atlasHeight))
				emit cantMakeAtlas();
			else
				emit arranged();
		}
		////

		mutex.lock();
		if (!restart)
			condition.wait(&mutex);
		restart = false;
		mutex.unlock();

		if (abort)
		{
			emit canceled();
			return;
		}
	}
}
