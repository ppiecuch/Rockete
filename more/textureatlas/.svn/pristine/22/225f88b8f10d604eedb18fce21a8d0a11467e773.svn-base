#ifndef TEXTUREMODEL_H
#define TEXTUREMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QPixmap>
#include <QIcon>
#include <QFileInfo>
#include <QPainter>

#include <QDebug>
#include <QDir>
#include <QProgressDialog>

#include <vector>

#include "common.h"
#include "arrangethread.h"



class TextureModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	TextureModel(QObject *parent=0);
	~TextureModel();


	int addTexture(QString path, bool mustRemakeAtlas=true);
	int addTextures(QStringList pathList);
	void addDir(QString _dirPath);
	void delTexture(int num)
	{
		if ((num>=0) && (num<textures.size()))
		{
			textures.remove(num,1);

			makeAtlas();
			reset();//FXIME:переделать на beginDelete
			emit textureDeleted();
		}
	}


	void delTextures(QModelIndexList &selectedInd)
	{
		if (selectedInd.count()==0)
			return;
		QVector<int> _numRows;
		for (int i=0; i<selectedInd.count(); i++)
			_numRows.push_back(selectedInd.at(i).row());

		qSort(_numRows);
		for (int i=_numRows.count()-1; i>=0; i--)
		{
			int num = _numRows[i];
			if ((num>=0) && (num<textures.size()))
				textures.remove(num,1);
		}

		makeAtlas();
		reset();//FXIME:переделать на beginDelete
		emit textureDeleted();
	}

	void delTexture(TTexture *tex)
	{
		for (int i=0; i<textures.size(); i++)
			if (&textures[i] == tex)
			{
				textures.remove(i,1);

				makeAtlas();
				reset();//FIXME:переделать на beginDelete
				emit textureDeleted();
			}
	}

	void moveTexture(TTexture *tex, const QPoint &dp);

	void selectItems(QModelIndexList &selectedInd);

	QModelIndex index(int row, int column, const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &child) const;

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;

	QVariant data(const QModelIndex &index, int role) const;
	bool hasChildren(const QModelIndex &parent) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	void LoadAtlas(QString path);

	void SaveAtlas(QString path);



	bool isAutoArrangeImages() const { return autoArrangeImages; }

signals:
	void atlasTextureUpdated();
	void textureDeleted();
	void cantMakeAtlas();
	void currentProgress(int percent);
	void selectionChanged();

public slots:
	void saveSelectedImages(QString _dir, QModelIndexList &selectedInd);
	void clear();
	void arrangeImages();//
	void makeAtlas();
	void setAtlasSize(int w, int h, bool _remakeAtlas=true);/// change atlas size
	void setAutoArrangeImages(bool _on=true){ autoArrangeImages = _on; }
	void unsetAutoArrangeImages(){ setAutoArrangeImages(false); }
	void cancel();
private slots:
	void arranged();
	void cantMakeAtlasSlot();
private:
	CPoint pixelSpaceToUVSpace(CPoint xy);
	void pixelCoordToUVCoord(TTexture *texItem);

	QStringList findFilesInDir(QString _dirPath);
public:
	QImage resultImage;
	QVector <TTexture> textures;
	QVector <TTexture *> tempTextures;

	float atlasWidth;
	float atlasHeight;

private:
	bool autoArrangeImages;//!< Авторасстановка текстур(при добавлении).

	ArrangeThread arrangeThread;
	QProgressDialog *progressDialog;
};

#endif // TEXTUREMODEL_H
