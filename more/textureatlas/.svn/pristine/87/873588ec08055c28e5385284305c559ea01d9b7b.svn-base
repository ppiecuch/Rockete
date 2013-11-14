#include "texturelistwidget.h"

TextureListWidget::TextureListWidget(QWidget *parent):QListView(parent)
{
	setSelectionRectVisible(true);
	setSpacing(2);
	setTextElideMode(Qt::ElideRight);
	setViewMode(QListView::ListMode);
	setMovement(QListView::Static);
	setFlow(QListView::TopToBottom);

	setDragEnabled(false);
	setAcceptDrops(true);
	setDropIndicatorShown(true);

	setSelectionBehavior(QAbstractItemView::SelectItems);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	connect(this,SIGNAL(clicked( const QModelIndex &)), this,SLOT(itemClicked()));
}

//////////////////////
void TextureListWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasFormat("image/x-texture"))
		event->accept();
	else
		event->acceptProposedAction();
	//event->ignore();
}

void TextureListWidget::dragMoveEvent(QDragMoveEvent *event)
{
	if (event->mimeData()->hasFormat("image/x-texture"))
	{
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
	else
		event->acceptProposedAction();
	//event->ignore();
}

void TextureListWidget::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasFormat("image/x-texture"))
	{
		QByteArray pieceData = event->mimeData()->data("image/x-texture");
		QDataStream dataStream(&pieceData, QIODevice::ReadOnly);
		QPixmap pixmap;
		//QPoint location;
		//dataStream >> pixmap >> location;
		//dataStream >> pixmap;

		// addPiece(pixmap, location);

		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
	else
	{
		if (event->mimeData()->hasUrls())
		{
			QList<QUrl> urlList = event->mimeData()->urls();
			QStringList _listFiles;
			for (int i = 0; i < urlList.size(); ++i)
			{
				_listFiles.append(urlList.at(i).toLocalFile());
			}

			static_cast<TextureModel *>( this->model())->addTextures(_listFiles);

			event->setDropAction(Qt::MoveAction);
			event->accept();
		}
		else
		event->ignore();
	}
}


void TextureListWidget::startDrag(Qt::DropActions supportedActions)
{
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    //QPixmap pixmap = qVariantValue<QPixmap>(item->data(Qt::UserRole));
    //QPixmap pixmap = qVariantValue<QPixmap>(this->currentIndex().data(Qt::UserRole));
    QPixmap pixmap = qVariantValue<QPixmap>(this->model()->data(this->currentIndex(),Qt::DecorationRole));

    //QPoint location = item->data(Qt::UserRole+1).toPoint();
    //dataStream << pixmap << location;
	int num = qVariantValue<int>(this->model()->data(this->currentIndex(),Qt::UserRole+1));
	dataStream << pixmap << num;

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("image/x-texture", itemData);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
    drag->setPixmap(pixmap);

    drag->exec(Qt::MoveAction);
    //if (drag->exec(Qt::MoveAction) == Qt::MoveAction)
      //  delete takeItem(row(item));
}

void TextureListWidget::keyPressEvent(QKeyEvent *event)
{

	switch (event->key())
	{
		case Qt::Key_Delete:
		{
			QModelIndexList selectedInd = this->selectedIndexes();
			//if (this->currentIndex().isValid())
				//static_cast<TextureModel *>( this->model())->delTexture(this->currentIndex().row());
			TextureModel *_model = qobject_cast<TextureModel *>(this->model());
			if (_model)
			{
				_model->delTextures(selectedInd);
			}
			//static_cast<TextureModel *>( this->model())->delTextures(selectedInd);
		}
			break;
		default: QWidget::keyPressEvent(event);
	}
}

void TextureListWidget::itemClicked()
{
	TextureModel *_model = qobject_cast<TextureModel *>(this->model());
	if (_model)
	{
		QModelIndexList selectedInd = this->selectedIndexes();
		_model->selectItems(selectedInd);
	}
	//static_cast<TextureModel *>( this->model())->delTextures(selectedInd);
}

void TextureListWidget::saveSelectedImages()
{
	QModelIndexList selectedInd = this->selectedIndexes();
	QString dirPath = QFileDialog::getExistingDirectory(this, tr("Export selected images..."),
													 QString(),
													 QFileDialog::ShowDirsOnly
													 | QFileDialog::DontResolveSymlinks);
	if (!dirPath.isEmpty())
	{
		TextureModel *_model = qobject_cast<TextureModel *>(this->model());
		if (_model)
		{
			_model->saveSelectedImages(dirPath,selectedInd);
		}
	}

}
