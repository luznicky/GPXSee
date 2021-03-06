#include <QPainter>
#include "config.h"
#include "sliderinfoitem.h"


#define SIZE 5

SliderInfoItem::SliderInfoItem(QGraphicsItem *parent) : QGraphicsItem(parent)
{

}

void SliderInfoItem::updateBoundingRect()
{
	QFont font;
	font.setPixelSize(FONT_SIZE);
	font.setFamily(FONT_FAMILY);
	QFontMetrics fm(font);

	_boundingRect = QRectF(-SIZE/2, 0, fm.width(_text) + SIZE, fm.height());
}

void SliderInfoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem
  *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	QFont font;
	font.setPixelSize(FONT_SIZE);
	font.setFamily(FONT_FAMILY);
	QFontMetrics fm(font);
	painter->setFont(font);

	painter->setPen(Qt::red);
	painter->drawText(SIZE, fm.height() - fm.descent(), _text);
	painter->drawLine(QPointF(-SIZE/2, 0), QPointF(SIZE/2, 0));

	//painter->drawRect(boundingRect());
}

void SliderInfoItem::setText(const QString &text)
{
	_text = text;
	updateBoundingRect();
	prepareGeometryChange();
}
