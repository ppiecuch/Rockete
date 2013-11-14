#include "texturemodel.h"


TextureModel::TextureModel(QObject *parent):QAbstractItemModel(parent)
{
	atlasWidth = 1024.0;
	atlasHeight = 1024.0;

	autoArrangeImages=true;

	resultImage = QImage(QSize(atlasWidth,atlasHeight), QImage::Format_ARGB32_Premultiplied);


	connect(&arrangeThread,SIGNAL(arranged()), this,SLOT(arranged()));
	connect(&arrangeThread,SIGNAL(cantMakeAtlas()), this,SLOT(cantMakeAtlasSlot()));

	progressDialog = new QProgressDialog("Operation in progress.", "Cancel", 0, 100);
	//progressDialog->setAttribute(Qt::WA_DeleteOnClose, true);
	progressDialog->setWindowModality(Qt::WindowModal);
	progressDialog->setMinimumDuration(200);
	connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancel()));
	connect(&arrangeThread, SIGNAL(changeProgress(int)), progressDialog, SLOT(setValue(int)));
	connect(&arrangeThread, SIGNAL(cantMakeAtlas()), progressDialog, SLOT(cancel()));
	connect(&arrangeThread, SIGNAL(arranged()), progressDialog, SLOT(cancel()));
	connect(&arrangeThread, SIGNAL(canceled()), progressDialog, SLOT(cancel()));
}


void TextureModel::arranged()
{
	makeAtlas();
}

void TextureModel::cantMakeAtlasSlot()
{
	makeAtlas();
	emit cantMakeAtlas();
}


TextureModel::~TextureModel()
{
	clear();
	progressDialog->deleteLater();
}


void TextureModel::clear()
{
	textures.clear();
	makeAtlas();
	reset();
}

int TextureModel::addTexture(QString path, bool mustRemakeAtlas)
{
	QFileInfo fi(path);

	QImage img;
	if (!img.load(path))
		return -1;

	QString imageNameToAdd;
	imageNameToAdd = fi.fileName();//baseName();

	int lastPosPoint = imageNameToAdd.lastIndexOf(QChar('.'));
	if (lastPosPoint != -1)
		imageNameToAdd = imageNameToAdd.left(lastPosPoint);
	imageNameToAdd.replace(QChar(' '),QChar('_'));
        imageNameToAdd.replace(QChar('.'),QChar('_'));
        imageNameToAdd.replace(QChar('-'),QChar('_'));

	///check- maybe we added this texture
	for (int i=0; i<textures.size(); i++)
		if (textures.value(i).name == imageNameToAdd)//if (textures.value(i).img == img)//not working correctly
			return i;

	beginInsertRows(QModelIndex(), textures.size(), textures.size());

	textures.push_back(TTexture());
	textures.last().img = img;

	textures.last().name = imageNameToAdd;

	textures.last().size = img.width()*img.height();
	textures.last().texNum = textures.size()-1;
	endInsertRows();

	if ((mustRemakeAtlas) && (autoArrangeImages))
		arrangeImages();
	else
		emit atlasTextureUpdated();

	return (textures.size()-1);
}

int TextureModel::addTextures(QStringList pathList)
{
	if (pathList.size()==0)
		return 0;
	for (int i=0; i<pathList.size(); i++)
		addTexture(pathList.at(i), false);
	if (autoArrangeImages)
		arrangeImages();
	else
		emit atlasTextureUpdated();
	return 0;
}

void TextureModel::addDir(QString _dirPath)
{
	QStringList listPathTextures = findFilesInDir(_dirPath);
	if (listPathTextures.size()>0)
		addTextures(listPathTextures);
}


QStringList TextureModel::findFilesInDir(QString _dirPath)
{
	QStringList resList;
	QDir dir(_dirPath);
	//dir.setPath();
	dir.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
	QStringList filters;
	filters << "*";
	dir.setNameFilters(filters);

	QStringList listFiles = dir.entryList();

	for (int i = 0; i < listFiles.size(); ++i)
	{
		//qDebug() << listFiles.at(i);
		resList.push_back(_dirPath+dir.separator()+listFiles.at(i));
	}


	QDir dir2(_dirPath);
	//dir.setPath();
	dir2.setFilter(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
	dir2.setNameFilters(filters);
	QStringList listDirs = dir2.entryList();
	for (int i = 0; i < listDirs.size(); ++i)
	{
		resList.append(findFilesInDir(_dirPath+dir.separator()+listDirs.at(i)));
	}

	return resList;
}


QModelIndex TextureModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid())
		return QModelIndex();

	if ((column !=0) || (row >= textures.size()))
		return QModelIndex();
	else
	{
		TTexture *t = const_cast<TTexture *>(&textures[row]);
		return createIndex(row, column, t);
	}
	return QModelIndex();
}

QModelIndex TextureModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}

int TextureModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return  0;
	else
		return textures.size();
}

int TextureModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return  0;
	else
		return textures.size();
}

QVariant TextureModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if ((index.column() != 0) ||(index.row() >= textures.size()))
		return QVariant();

	switch (role)
	{
		case Qt::DisplayRole:
		case Qt::EditRole:
			return textures.value(index.row()).name;
			break;
		case Qt::DecorationRole:
			return textures.value(index.row()).img.scaled(20,20);
			break;
		case Qt::UserRole:
			return textures.value(index.row()).img;
			break;

		case Qt::UserRole+1:
			return textures.value(index.row()).texNum;
			break;
	}
	return QVariant();
}


bool TextureModel::hasChildren(const QModelIndex &parent) const
{
	if (parent.isValid())
		return false;
	else
		return true;
}

Qt::ItemFlags TextureModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;
	return (Qt::ItemIsDragEnabled|Qt::ItemIsSelectable|Qt::ItemIsEnabled);
}


void TextureModel::cancel()
{
	arrangeThread.cancel();
}


void TextureModel::arrangeImages()
{
	//bool cantMake=false;
	progressDialog->setValue(0);
	progressDialog->reset();

	//


	arrangeThread.arrangeImages(&textures, atlasWidth, atlasHeight);


	//if (!abort)
		//progress.setValue(100);
	//if ((textures.size()>0) &&(minTotalHeight > atlasHeight))
		//emit cantMakeAtlas();
}

CPoint TextureModel::pixelSpaceToUVSpace(CPoint xy)
{
	return CPoint(xy.x / ((float)atlasWidth), xy.y / ((float)atlasHeight));
}

void TextureModel::pixelCoordToUVCoord(TTexture *texItem)
{
	//12
	//03
	//CPoint lowerLeftUV = pixelSpaceToUVSpace(CPoint(texItem->x, texItem->y));
	CPoint topLeftUV = pixelSpaceToUVSpace(CPoint(texItem->x, texItem->y));
	topLeftUV.y = 1.0f - topLeftUV.y;

	//CPoint UVDimensions = pixelSpaceToUVSpace(CPoint(texItem->img.width(), texItem->img.height()));
	CPoint UVDimensions = pixelSpaceToUVSpace(CPoint(texItem->img.width(), texItem->img.height()));

	CPoint p[4];

	p[0] = CPoint(topLeftUV.x, topLeftUV.y-UVDimensions.y);// Upper-left
	p[1] = topLeftUV;// Lower-left
	p[2] = CPoint(topLeftUV.x+UVDimensions.x, topLeftUV.y);// Upper-right
	p[3] = CPoint(topLeftUV.x+UVDimensions.x, topLeftUV.y-UVDimensions.y);// Lower-left

	for (int np=0, nv=0; np<4; np++, nv+=2)
	{
		texItem->texVerts[nv] = p[np].x;
		texItem->texVerts[nv+1] = p[np].y;
	}
}

void TextureModel::makeAtlas()
{
	QPainter painter(&resultImage);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.fillRect(resultImage.rect(), Qt::transparent);

	for (int i=0; i<textures.size(); i++)
	{
		painter.drawImage(textures[i].x, textures[i].y, textures[i].img);

		pixelCoordToUVCoord(&textures[i]);
		//TL
		//float lx = textures[i].x/atlasWidth;
		/*
		float lx = (textures[i].x)/atlasWidth;
		float ty = 1.0-(textures[i].y)/atlasHeight;

		//float rx =  (textures[i].x+textures[i].img.width()-1)/(atlasWidth);
		float rx =  (textures[i].x+textures[i].img.width())/(atlasWidth);
		float by = 1-(textures[i].y+textures[i].img.height())/atlasHeight;

			textures[i].texVerts[0] = lx;
			textures[i].texVerts[1] = by;

			textures[i].texVerts[2] = lx;
			textures[i].texVerts[3] = ty;

			textures[i].texVerts[4] = rx;
			textures[i].texVerts[5] = ty;

			textures[i].texVerts[6] = rx;
			textures[i].texVerts[7] = by;
			*/




		/*
			textures[i].texVerts[0] = textures[i].x/(atlasWidth-1);
			textures[i].texVerts[1] = (atlasHeight-1-(textures[i].y+textures[i].img.height()-1))/(atlasHeight-1);

			textures[i].texVerts[2] = textures[i].x/(atlasWidth-1);
			textures[i].texVerts[3] = (atlasHeight-1-textures[i].y)/(atlasHeight-1);

			textures[i].texVerts[4] = (textures[i].x+textures[i].img.width()-1)/(atlasWidth-1);
			textures[i].texVerts[5] = (atlasHeight-1-textures[i].y)/(atlasHeight-1);

			textures[i].texVerts[6] = (textures[i].x+textures[i].img.width()-1)/(atlasWidth-1);
			textures[i].texVerts[7] = (atlasHeight-1-(textures[i].y+textures[i].img.height()-1))/(atlasHeight-1);
			*/
	}

	painter.end();

	emit atlasTextureUpdated();
}

void TextureModel::LoadAtlas(QString path)
{
	textures.clear();
	reset();

	QString headerName;
	headerName = path;
	if (headerName.endsWith(".png",Qt::CaseInsensitive))
				headerName = headerName.left(headerName.size()-4);
	headerName = headerName +".h";

	QFile file(headerName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QImage loadedImage;
	if (!loadedImage.load(path))
		return;

	/// search line that start with "////loadformat", when found-begin load textures data
	bool startProcessLine = false;
	while (!file.atEnd())
	{
		QByteArray line = file.readLine();
		if (startProcessLine)
		{
			QList<QByteArray> l = line.split('=');
			if (l.size()==2)
			{
				QList<QByteArray> rectS = l.at(1).split(',');
				if (rectS.size()==4)
				{
					int x,y,w,h;
					x = rectS[0].toInt();
					y = rectS[1].toInt();
					w = rectS[2].simplified().toInt();
					h = rectS[3].simplified().toInt();
					TTexture newTex;

					newTex.x = x;
					newTex.y = y;
					newTex.img = loadedImage.copy(x,y,w,h);
					newTex.name = QString(l[0].right(l[0].size()-2));
					newTex.size = newTex.img.width() * newTex.img.height();
					newTex.texNum = textures.size()-1;
					textures.push_back(newTex);
				}
			}

		}
		else
			if (line.startsWith("////loadformat"))
				startProcessLine =true;
	}

	setAtlasSize(loadedImage.width(), loadedImage.height(),false);
	makeAtlas();
	reset();
}

struct elemTex
{
	int num;
	TTexture *tex;
	elemTex(int _num, TTexture *_tex){ num=_num; tex=_tex; }
};

bool texLessThan(const TTexture &t1, const TTexture &t2)
{
	QRegExp regExp("\\d+$");
	int pos;
	int num1,num2;
	QString texName1, texName2;

	num1=num2=0;
	pos = t1.name.indexOf(regExp, 0);
	if (pos!=-1)
	{
		texName1 = t1.name.left(pos);
		QString textNum = t1.name.right(t1.name.length()-pos);
		num1 = textNum.toInt();
	}
	else
		texName1= t1.name;

	pos = t2.name.indexOf(regExp, 0);
	if (pos!=-1)
	{
		texName2 = t2.name.left(pos);
		QString textNum = t2.name.right(t2.name.length()-pos);
		num2 = textNum.toInt();
	}
	else
		texName2= t2.name;

	if (texName1==texName2)
	{
		return (num1 < num2);
	}
	else
		return (texName1 < texName2);
}

void TextureModel::SaveAtlas(QString path)
{
	makeAtlas();
	QFileInfo fi(path);

	QString dir = fi.path();
	if (dir.at(dir.length()-1) !=  QDir::separator())
		dir.append(QDir::separator());

	QString fileName = fi.fileName();

	QString imageFullPath;
	QString headerFName = path;

	if (fileName.endsWith(".png",Qt::CaseInsensitive))
	{
		headerFName = fileName.left(fileName.size()-4);
		imageFullPath = path;
	}
	else if (fileName.endsWith(".h",Qt::CaseInsensitive))
	{
		headerFName = fileName.left(fileName.size()-2);
		imageFullPath = path.left(path.size()-2)+".png";
	}
	else
	{
		imageFullPath = path+".png";
		headerFName = fileName;
	}

	resultImage.save(imageFullPath);

	///
	QFile fileH(dir+headerFName+".h");
	if (fileH.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream outH(&fileH);

		outH << "#ifndef " << headerFName.toUpper() << "_H\n";
		outH << "#define " << headerFName.toUpper() << "_H\n";
		outH << "\n//Created by Texture Atlas Creator @milytia\n";

		outH << "\n//Texture coordinates format( every point in (x,y) ): {left_bottom, left_top, right_top, right_bottom}\n";

		qSort(textures.begin(), textures.end(), texLessThan);

		for (int i=0; i<textures.size(); i++)
			outH << "#define _" << headerFName << "_" << textures[i].name << "_ " << i <<"\n";

		outH << "\n#define " << headerFName << "_textures_count " << textures.size() <<"\n\n\n";


		/*
		for (int i=0; i<textures.size(); i++)
		{
			outH << "extern float " << headerFName << "_" << textures[i].name << "[8];\n";
			outCPP << "float " << headerFName << "_" << textures[i].name << "[8] = {";
			for (int p=0; p<7; p++)
				outCPP << textures[i].texVerts[p] << ", ";
			outCPP << textures[i].texVerts[7] << "};\n";
		}
		*/

		/*
		outH << "\nstatic const float " << headerFName << "[" << textures.size() << "][8] = { ";
		for (int i=0; i<textures.size(); i++)
		{
			outH << "{";
			for (int p=0; p<7; p++)
				outH << textures[i].texVerts[p] << ", ";
			outH << textures[i].texVerts[7] << "}";
			if (i < (textures.size()-1))
				outH << ",";
			outH << "//\t" << textures[i].name << " - " << i << "\n";

		}
		outH << "};\n\n";
		*/

		QString texVertsElemsName = QString("%1_%2").arg(headerFName).arg("verts");
		QString sizesElemsName = QString("%1_%2").arg(headerFName).arg("sizes");

		QStringList texVertsElemsNames;
		for (int i=0; i<textures.size(); i++)
		{
			QString curElemName = QString("%1_%2_verts").arg(headerFName).arg(textures[i].name);
			texVertsElemsNames.push_back(curElemName);
			outH << "static const float " << curElemName << "[] = { ";
			for (int p=0; p<7; p++)
				outH << textures[i].texVerts[p] << ", ";
			outH << textures[i].texVerts[7] << "};";
			outH << "//\t" << textures[i].name << " - " << i << "\n";
		}

		outH << "\nstatic const float *" << texVertsElemsName << "[] = { " << texVertsElemsNames.join(", ") << "};\n";

		outH << "\n\n";


		//FIXME:
		outH << "//{width,height}\n";
		/*
		outH << "static const float " << "size_" << headerFName << "[" << textures.size() << "][2] = { ";
		for (int i=0; i<textures.size(); i++)
		{
			outH << "{" << textures[i].img.width() << ", "<< textures[i].img.height() << "}";
			if (i < (textures.size()-1))
				outH << ",";
			outH << "//" << textures[i].name << " - " << i << "\n";
		}
		outH << "};\n\n";
		*/
		QStringList sizesElemsNames;
		for (int i=0; i<textures.size(); i++)
		{
			QString curElemName = QString("%1_%2_size").arg(headerFName).arg(textures[i].name);
			sizesElemsNames.push_back(curElemName);
			outH << "static const float " << curElemName << "[] = { " << textures[i].img.width() << ", "<< textures[i].img.height() << "};";
			outH << "//" << textures[i].name << " - " << i << "\n";
		}
		outH << "\nstatic const float *" << sizesElemsName << "[] = { " << sizesElemsNames.join(", ") << "};\n";
		outH << "\n\n";






		outH << "///{width/height}\n";
		/*
		outH << "\nstatic const float " << "wh_" << headerFName << "[" << textures.size() << "] = { ";
		for (int i=0; i<textures.size(); i++)
		{
			outH << (float)textures[i].img.width()/(float)textures[i].img.height();
			if (i < (textures.size()-1))
				outH << ",";
			outH << "//" << textures[i].name << " - " << i << "\n";
		}
		outH << "};\n";
		*/
		outH << "\nstatic const float " << headerFName << "_wh[] = { ";
		for (int i=0; i<textures.size(); i++)
		{
			outH << (float)textures[i].img.width()/(float)textures[i].img.height();
			if (i < (textures.size()-1))
				outH << ",";
			outH << "//" << textures[i].name << " - " << i << "\n";
		}
		outH << "};\n";
		outH << "\n\n";


		///////////
		outH << "static void "<< headerFName << "_drawTextureAtPoint(int tex, float x, float y, float z) {" << "\n";
		outH << "   glPushMatrix();" << "\n";
		outH << "   glTexCoordPointer(2, GL_FLOAT, 0, " << texVertsElemsName << "[tex]);" << "\n";
		outH << "   glTranslatef(x, y, z);" << "\n";
		outH << "   glScalef(" << sizesElemsName << "[tex][0], " << sizesElemsName << "[tex][1], 1);" << "\n";
		outH << "   glDrawArrays(GL_TRIANGLE_FAN, 0, 4);" << "\n";
		outH << "   glPopMatrix();" << "\n";
		outH << "}" << "\n";
		///////////

		outH << "#endif\n";

		outH << "////loadformat  (x,y,width,height)\n";
		for (int i=0; i<textures.size(); i++)
		{
			outH << "//" << textures[i].name << "=" << textures[i].x << "," << textures[i].y << ","
												<< textures[i].img.width() << "," << textures[i].img.height() << "\n";
		}
		outH << "\0";

		reset();
	}
}

void TextureModel::setAtlasSize(int w, int h, bool _remakeAtlas)
{
	atlasWidth=w;
	atlasHeight=h;
	resultImage = QImage(QSize(atlasWidth,atlasHeight), QImage::Format_ARGB32_Premultiplied);
	if ((_remakeAtlas) && (autoArrangeImages))
		arrangeImages();
	else
		emit atlasTextureUpdated();
}

void TextureModel::selectItems(QModelIndexList &selectedInd)
{

	for (int i=0; i<textures.size(); i++)
		textures[i].markSelected=false;

	if (selectedInd.count()==0)
		return;
	for (int i=0; i<selectedInd.count(); i++)
	{
		int num = selectedInd.at(i).row();
		if ((num>=0) && (num<textures.size()))
		{
			textures[num].markSelected=true;
		}
	}
	emit selectionChanged();
}

void TextureModel::saveSelectedImages(QString _dir, QModelIndexList &selectedInd)
{
	qDebug() << "============== " << selectedInd.count();
	for (int i=0; i<selectedInd.count(); i++)
	{
		int num = selectedInd.at(i).row();
		qDebug() << "    num=" << num;
		if ((num>=0) && (num<textures.size()))
		{
			qDebug() << (_dir+QDir::separator()+textures[num].name);
			textures[num].img.save(_dir+QDir::separator()+textures[num].name+".png");
		}
	}

}


void TextureModel::moveTexture(TTexture *tex, const QPoint &dp)
{
	if (!tex)
		return;
	tex->x += dp.x();
	tex->y += dp.y();
	makeAtlas();
}
