/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2020 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "WidgetResizeHandler_p.h"
#include "FloatingWindow_p.h"
#include "TitleBar_p.h"
#include "DragController_p.h"
#include "Config.h"
#include "Qt5Qt6Compat_p.h"
#include "Utils_p.h"

#include <QEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>

#if defined(Q_OS_WIN)
# include <QtGui/private/qhighdpiscaling_p.h>
# include <Windowsx.h>
# include <Windows.h>
# if defined(Q_CC_MSVC)
#  pragma comment(lib,"User32.lib")
# endif
#endif

namespace  {
int widgetResizeHandlerMargin = 4; //4 pixel
}

using namespace KDDockWidgets;

bool WidgetResizeHandler::s_disableAllHandlers = false;
WidgetResizeHandler::WidgetResizeHandler(bool filterIsGlobal, QWidgetOrQuick *target)
    : QObject(target)
    , mFilterIsGlobal(filterIsGlobal)
{
    setTarget(target);
}

WidgetResizeHandler::~WidgetResizeHandler()
{
}

bool WidgetResizeHandler::eventFilter(QObject *o, QEvent *e)
{
    if (s_disableAllHandlers)
        return false;

    auto widget = qobject_cast<QWidgetOrQuick*>(o);
    if (!widget)
        return false;

    if (!mFilterIsGlobal && (!widget->isTopLevel() || o != mTarget))
        return false;

    switch (e->type()) {
    case QEvent::MouseButtonPress: {
        if (mTarget->isMaximized())
            break;
        auto mouseEvent = static_cast<QMouseEvent *>(e);
        auto cursorPos = cursorPosition(Qt5Qt6Compat::eventGlobalPos(mouseEvent));
        if (cursorPos == CursorPosition_Undefined)
            return false;

        const QRect widgetRect = mTarget->rect().marginsAdded(QMargins(widgetResizeHandlerMargin, widgetResizeHandlerMargin, widgetResizeHandlerMargin, widgetResizeHandlerMargin));
        const QPoint cursorPoint = mTarget->mapFromGlobal(Qt5Qt6Compat::eventGlobalPos(mouseEvent));
        if (!widgetRect.contains(cursorPoint))
            return false;
        if (mouseEvent->button() == Qt::LeftButton) {
            mResizeWidget = true;
        }

        mNewPosition = Qt5Qt6Compat::eventGlobalPos(mouseEvent);
        mCursorPos = cursorPos;
        return true;
    }
    case QEvent::MouseButtonRelease: {
        if (mTarget->isMaximized())
            break;
        auto mouseEvent = static_cast<QMouseEvent *>(e);
        if (mouseEvent->button() == Qt::LeftButton) {
            mResizeWidget = false;
            mTarget->releaseMouse();
            mTarget->releaseKeyboard();
            return true;
        }
        break;
    }
    case QEvent::MouseMove: {
        if (mTarget->isMaximized())
            break;
        auto mouseEvent = static_cast<QMouseEvent *>(e);
        mResizeWidget = mResizeWidget && (mouseEvent->buttons() & Qt::LeftButton);
        const bool state = mResizeWidget;
        mResizeWidget = ((o == mTarget) && mResizeWidget);
        const bool consumed = mouseMoveEvent(mouseEvent);
        mResizeWidget = state;
        return consumed;
    }
    default:
        break;
    }
    return false;
}

bool WidgetResizeHandler::mouseMoveEvent(QMouseEvent *e)
{
    const QPoint globalPos = Qt5Qt6Compat::eventGlobalPos(e);
    if (!mResizeWidget) {
        const CursorPosition pos = cursorPosition(globalPos);
        updateCursor(pos);
        return pos != CursorPosition_Undefined;
    }

    const QRect oldGeometry = mTarget->geometry();
    QRect newGeometry = oldGeometry;

    {
        int deltaWidth = 0;
        int newWidth = 0;
        const int minWidth = mTarget->minimumWidth();
        const int maxWidth = mTarget->maximumWidth();
        switch (mCursorPos) {
        case CursorPosition_TopLeft:
        case CursorPosition_Left:
        case CursorPosition_BottomLeft: {
            deltaWidth = oldGeometry.left() - globalPos.x();
            newWidth = qBound(minWidth, mTarget->width() + deltaWidth, maxWidth);
            deltaWidth = newWidth - mTarget->width();
            if (deltaWidth != 0) {
                newGeometry.setLeft(newGeometry.left() - deltaWidth);
            }

            break;
        }

        case CursorPosition_TopRight:
        case CursorPosition_Right:
        case CursorPosition_BottomRight: {
            deltaWidth = globalPos.x() - newGeometry.right();
            newWidth = qBound(minWidth, mTarget->width() + deltaWidth, maxWidth);
            deltaWidth = newWidth - mTarget->width();
            if (deltaWidth != 0) {
                newGeometry.setRight(oldGeometry.right() + deltaWidth);
            }
            break;
        }
        default:
            break;
        }
    }

    {
        const int maxHeight = mTarget->maximumHeight();
        const int minHeight = mTarget->minimumHeight();
        int deltaHeight = 0;
        int newHeight = 0;
        switch (mCursorPos) {
        case CursorPosition_TopLeft:
        case CursorPosition_Top:
        case CursorPosition_TopRight: {
            deltaHeight = oldGeometry.top() - globalPos.y();
            newHeight = qBound(minHeight, mTarget->height() + deltaHeight, maxHeight);
            deltaHeight = newHeight - mTarget->height();
            if (deltaHeight != 0) {
                newGeometry.setTop(newGeometry.top() - deltaHeight);
            }

            break;
        }

        case CursorPosition_BottomLeft:
        case CursorPosition_Bottom:
        case CursorPosition_BottomRight: {
            deltaHeight = globalPos.y() - newGeometry.bottom();
            newHeight = qBound(minHeight, mTarget->height() + deltaHeight, maxHeight);
            deltaHeight = newHeight - mTarget->height();
            if (deltaHeight != 0) {
                newGeometry.setBottom(oldGeometry.bottom() + deltaHeight);
            }
            break;
        }
        default:
            break;
        }
    }

    if (newGeometry != mTarget->geometry())
        mTarget->setGeometry(newGeometry);

    return true;
}

#ifdef Q_OS_WIN

/// Handler to enable Aero-snap
bool WidgetResizeHandler::handleWindowsNativeEvent(FloatingWindow *w, const QByteArray &eventType, void *message, Qt5Qt6Compat::qintptr *result)
{
    if (eventType != "windows_generic_MSG")
        return false;

    auto msg = static_cast<MSG *>(message);
    if (msg->message == WM_NCCALCSIZE) {
        *result = 0;
        return true;
    } else if (msg->message == WM_NCHITTEST) {

        if (DragController::instance()->isInClientDrag()) {
            // There's a non-native drag going on.
            *result = 0;
            return false;
        }

        const int borderWidth = 8;
        const bool hasFixedWidth = w->minimumWidth() == w->maximumWidth();
        const bool hasFixedHeight = w->minimumHeight() == w->maximumHeight();

        *result = 0;
        const int xPos = GET_X_LPARAM(msg->lParam);
        const int yPos = GET_Y_LPARAM(msg->lParam);
        RECT rect;
        GetWindowRect(reinterpret_cast<HWND>(w->winId()), &rect);

        if (xPos >= rect.left && xPos <= rect.left + borderWidth &&
                yPos <= rect.bottom && yPos >= rect.bottom - borderWidth) {
            *result = HTBOTTOMLEFT;
        } else if (xPos < rect.right && xPos >= rect.right - borderWidth &&
                   yPos <= rect.bottom && yPos >= rect.bottom - borderWidth) {
            *result = HTBOTTOMRIGHT;
        } else if (xPos >= rect.left && xPos <= rect.left + borderWidth &&
                   yPos >= rect.top && yPos <= rect.top + borderWidth) {
            *result = HTTOPLEFT;
        } else if (xPos <= rect.right && xPos >= rect.right - borderWidth &&
                   yPos >= rect.top && yPos < rect.top + borderWidth) {
            *result = HTTOPRIGHT;
        } else if (!hasFixedWidth && xPos >= rect.left && xPos <= rect.left + borderWidth) {
            *result = HTLEFT;
        } else if (!hasFixedHeight && yPos >= rect.top && yPos <= rect.top + borderWidth) {
            *result = HTTOP;
        } else if (!hasFixedHeight && yPos <= rect.bottom && yPos >= rect.bottom - borderWidth) {
            *result = HTBOTTOM;
        } else if (!hasFixedWidth && xPos <= rect.right && xPos >= rect.right - borderWidth) {
            *result = HTRIGHT;
        } else {
            const QPoint globalPosQt = QHighDpi::fromNativePixels(QPoint(xPos, yPos), w->windowHandle());
            const QRect htCaptionRect = w->dragRect(); // The rect on which we allow for Windows to do a native drag
            if (globalPosQt.y() >= htCaptionRect.top() && globalPosQt.y() <= htCaptionRect.bottom() && globalPosQt.x() >= htCaptionRect.left() && globalPosQt.x() <= htCaptionRect.right()) {
                if (!KDDockWidgets::inDisallowDragWidget(globalPosQt)) { // Just makes sure the mouse isn't over the close button, we don't allow drag in that case.
                   *result = HTCAPTION;
                }
            }
        }

        w->setLastHitTest(*result);
        return *result != 0;
    } else if (msg->message == WM_NCLBUTTONDBLCLK) {
        if ((Config::self().flags() & Config::Flag_DoubleClickMaximizes)) {
            // By returning false we accept Windows native action, a maximize.
            // We could also call titleBar->onDoubleClicked(); here which will maximize if Flag_DoubleClickMaximizes is set,
            // but there's a bug in QWidget::showMaximized() on Windows when we're covering the native title bar, the window is maximized with an offset.
            // So instead, use a native maximize which works well
            return false;
        } else {
            // Let the title bar handle it. It will re-dock the window.

            if (TitleBar *titleBar = w->titleBar()) {
                if (titleBar->isVisible()) { // can't be invisible afaik
                    titleBar->onDoubleClicked();
                }
            }

            return true;
        }
    } else if (msg->message == WM_GETMINMAXINFO) {
        // Qt doesn't work well with windows that don't have title bar but have native frames.
        // When maximized they go out of bounds and the title bar is clipped, so catch WM_GETMINMAXINFO
        // and patch the size

        // According to microsoft docs it only works for the primary screen, but extrapolates for the others
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen || w->windowHandle()->screen() != screen) {
            return false;
        }

        DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);

        const QRect availableGeometry = screen->availableGeometry();

        auto mmi = reinterpret_cast<MINMAXINFO *>(msg->lParam);
        const qreal dpr = screen->devicePixelRatio();

        mmi->ptMaxSize.y = int(availableGeometry.height() * dpr);
        mmi->ptMaxSize.x = int(availableGeometry.width() * dpr) - 1; // -1 otherwise it gets bogus size
        mmi->ptMaxPosition.x = availableGeometry.x();
        mmi->ptMaxPosition.y = availableGeometry.y();

        QWindow *window = w->windowHandle();
        mmi->ptMinTrackSize.x = int(window->minimumWidth() * dpr);
        mmi->ptMinTrackSize.y = int(window->minimumHeight() * dpr);

        *result = 0;
        return true;
    }

    return false;
}

#endif

void WidgetResizeHandler::setTarget(QWidgetOrQuick *w)
{
    if (w) {
        mTarget = w;
        mTarget->setMouseTracking(true);
        if (mFilterIsGlobal) {
            qApp->installEventFilter(this);
        } else {
            mTarget->installEventFilter(this);
        }
    } else {
        qWarning() << "Target widget is null!";
    }
}

void WidgetResizeHandler::updateCursor(CursorPosition m)
{
#ifdef KDDOCKWIDGETS_QTWIDGETS
    //Need for updating cursor when we change child widget
    const QObjectList children = mTarget->children();
    for (int i = 0, total = children.size(); i < total; ++i) {
        if (auto child = qobject_cast<WidgetType*>(children.at(i))) {

            if (!child->testAttribute(Qt::WA_SetCursor)) {
                child->setCursor(Qt::ArrowCursor);
            }
        }
    }
#endif

    switch (m) {
    case CursorPosition_TopLeft:
    case CursorPosition_BottomRight:
        setMouseCursor(Qt::SizeFDiagCursor);
        break;
    case CursorPosition_BottomLeft:
    case CursorPosition_TopRight:
        setMouseCursor(Qt::SizeBDiagCursor);
        break;
    case CursorPosition_Top:
    case CursorPosition_Bottom:
        setMouseCursor(Qt::SizeVerCursor);
        break;
    case CursorPosition_Left:
    case CursorPosition_Right:
        setMouseCursor(Qt::SizeHorCursor);
        break;
    case CursorPosition_Undefined:
        restoreMouseCursor();
        break;
    }
}

void WidgetResizeHandler::setMouseCursor(Qt::CursorShape cursor)
{
    if (mFilterIsGlobal)
        qApp->setOverrideCursor(cursor);
    else
        mTarget->setCursor(cursor);
}

void WidgetResizeHandler::restoreMouseCursor()
{
    if (mFilterIsGlobal)
        qApp->restoreOverrideCursor();
    else
        mTarget->setCursor(Qt::ArrowCursor);
}

WidgetResizeHandler::CursorPosition WidgetResizeHandler::cursorPosition(QPoint globalPos) const
{
    if (!mTarget)
        return CursorPosition_Undefined;

    QPoint pos = mTarget->mapFromGlobal(globalPos);

    const int x = pos.x();
    const int y = pos.y();
    const int margin = widgetResizeHandlerMargin;

    int result = CursorPosition_Undefined;
    if (qAbs(x) <= margin)
        result |= CursorPosition_Left;
    else if (qAbs(x - (mTarget->width() - margin)) <= margin)
        result |= CursorPosition_Right;

    if (qAbs(y) <= margin)
        result |= CursorPosition_Top;
    else if (qAbs(y - (mTarget->height() - margin)) <= margin)
        result |= CursorPosition_Bottom;

    return static_cast<CursorPosition>(result);
}
