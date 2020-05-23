/*
  This file is part of KDDockWidgets.

  Copyright (C) 2018-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "GuestWidget.h"
#include "Item_p.h"

using namespace Layouting;

QSize GuestWidget::minSize() const
{
    return widgetMinSize(asWidget());
}

QSize GuestWidget::maxSize() const
{
    return asWidget()->maximumSize();
}

QSize GuestWidget::size() const
{
    return asWidget()->size();
}

QSize GuestWidget::widgetMinSize(const QWidget *w)
{
    const int minW = w->minimumWidth() > 0 ? w->minimumWidth()
                                           : w->minimumSizeHint().width();

    const int minH = w->minimumHeight() > 0 ? w->minimumHeight()
                                            : w->minimumSizeHint().height();

    return QSize(minW, minH).expandedTo(Item::hardcodedMinimumSize);
}