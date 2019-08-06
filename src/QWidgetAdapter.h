/*
  This file is part of KDDockWidgets.

  Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

/**
 * @file
 * @brief A class that is QWidget when building for QtWidgets, and QObject when building for QtQuick.
 *
 * Allows to have the same code base supporting both stacks.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#ifndef KDDOCKWIDGETS_QWIDGETADAPTER_H
#define KDDOCKWIDGETS_QWIDGETADAPTER_H

#include "docks_export.h"

#include <QWidget>

namespace KDDockWidgets {

class DOCKS_EXPORT QWidgetAdapter : public QWidget // TODO ifdef the base class once we support QtQuick
{
    Q_OBJECT
public:
    explicit QWidgetAdapter(QWidget *parent = nullptr);
    ~QWidgetAdapter() override;

protected:
    bool event(QEvent *e) override;
    void resizeEvent(QResizeEvent *) override;

    virtual bool onResize(QSize oldSize, QSize newSize);
    virtual void onLayoutRequest();
};

}

#endif
