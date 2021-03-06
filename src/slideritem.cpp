#include <QPainter>
#include "slideritem.h"


#define SIZE        10

SliderItem::SliderItem(QGraphicsObject *parent) : QGraphicsObject(parent)
{
	setFlag(ItemIsMovable);
	setFlag(ItemSendsGeometryChanges);
}

QRectF SliderItem::boundingRect() const
{
	return QRectF(-SIZE/2, -_area.height(), SIZE, _area.height());
}

void SliderItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
  QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	painter->setPen(Qt::red);
	painter->drawLine(0, 0, 0, -_area.height());

	//painter->drawRect(boundingRect());
}

QVariant SliderItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemPositionChange && scene()) {
		QPointF pos = value.toPointF();

		if (!_area.contains(QRectF(pos, boundingRect().size()))) {
			pos.setX(qMin(_area.right(), qMax(pos.x(), _area.left())));
			pos.setY(qMin(_area.bottom(), qMax(pos.y(), _area.top()
			  + boundingRect().height())));

			return pos;
		}
	}

	if (change == ItemPositionHasChanged)
		emit positionChanged(value.toPointF());

	return QGraphicsItem::itemChange(change, value);
}
