#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QImage>

//12
//03
struct TTexture
{
	QString name;
	QImage img;
	int texNum;
	int size;
	float texVerts[8];

	int x,y;
	bool isPacked;
	bool markSelected;
	//QRectF rect;
	TTexture()
	{
		markSelected=false;
		x=0;
		y=0;
	}
};


struct fsRect{
	float x,y,w,h;
	fsRect(float _x=0,float _y=0,float _w=0,float _h=0):x(_x),y(_y),w(_w),h(_h) {}
};

struct CPoint
{
	float x,y;
	CPoint(float _x=0.0,float _y=0.0):x(_x),y(_y){}
};

#endif // COMMON_H
