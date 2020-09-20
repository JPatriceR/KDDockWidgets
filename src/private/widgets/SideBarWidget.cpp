/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2020 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "SideBarWidget_p.h"
#include "DockWidgetBase.h"
#include "MainWindowBase.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QAbstractButton>
#include <QStyle>
#include <QStyleOptionToolButton>

using namespace KDDockWidgets;

SideBarWidget::SideBarWidget(SideBarLocation location, MainWindowBase *parent)
    : SideBar(location, parent)
    , m_layout(isVertical() ? static_cast<QBoxLayout*>(new QVBoxLayout(this))
                            : static_cast<QBoxLayout*>(new QHBoxLayout(this))) // ternary operator requires static_cast
{
    m_layout->setSpacing(1);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addStretch();
}

void SideBarWidget::addDockWidget_Impl(DockWidgetBase *dw)
{
    auto button = createButton(this);
    button->setText(dw->title());
    connect(dw, &DockWidgetBase::titleChanged, button, &SideBarButton::setText);
    connect(dw, &DockWidgetBase::removedFromSideBar, button, &QObject::deleteLater);
    connect(dw, &QObject::destroyed, button, &QObject::deleteLater);
    connect(button, &SideBarButton::clicked, this, [this, dw] {
        onButtonClicked(dw);
    });

    const int count = m_layout->count();
    m_layout->insertWidget(count - 1, button);
}

void SideBarWidget::removeDockWidget_Impl(DockWidgetBase *)
{
    // Nothing is needed. Button is removed automatically.
}

SideBarButton *SideBarWidget::createButton(SideBarWidget *parent) const
{
    return new SideBarButton(parent);
}

SideBarButton::SideBarButton(SideBarWidget *parent)
    : QToolButton(parent)
    , m_sideBar(parent)
{
}

bool SideBarButton::isVertical() const
{
    return m_sideBar->isVertical();
}

void SideBarButton::paintEvent(QPaintEvent *)
{
    QPixmap pixmap(size());

    {
        // Draw to an horizontal button, it's easier.

        pixmap.fill(Qt::transparent);

        QStyleOptionToolButton opt;
        initStyleOption(&opt);
        const bool isHovered = opt.state & QStyle::State_MouseOver;

        QPainter p(&pixmap);

        const QRect r = isVertical() ? rect().transposed() : rect();
        const QRect textRect = r.adjusted(3, 0, 5, 0);
        // p.drawRect(r.adjusted(0, 0, -1, -1));
        p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text());

        QPen pen(isHovered ? 0x2ca7ff : 0xc8c8c8);
        pen.setWidth(isHovered ? 2 : 1);
        p.setPen(pen);
        p.drawLine(3, r.bottom() - 1, r.width() - 3*2, r.bottom() - 1);
     }

    QPainter p(this);
    if (isVertical()) {

    } else {
        p.drawPixmap(rect(), pixmap);
    }

}

QSize SideBarButton::sizeHint() const
{
    const QSize hint = QToolButton::sizeHint();
    return isVertical() ? (hint.transposed() + QSize(2, 0))
                        : (hint + QSize(0, 2));
}
