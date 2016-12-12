#pragma once

#include <QtWidgets/QLabel>

class CConnectButton : public QLabel
{
    Q_OBJECT

public:
    CConnectButton(QWidget* parent);
    ~CConnectButton() Q_DECL_NOEXCEPT {}

public Q_SLOTS:
    void connected(bool);

Q_SIGNALS:
    void click(bool);

protected:
    // overridables
    void enterEvent(QEvent* e);
    void leaveEvent(QEvent* e);
    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);

private:
    // helper methods
    void OnConnect(void) Q_DECL_NOEXCEPT;
    void OnEnter(void) Q_DECL_NOEXCEPT;
    void OnLeave(void) Q_DECL_NOEXCEPT;

private:
    bool m_bConnect;    // connection state
    bool m_bHovered;
    bool m_bPressed;    // mouse button pressed
    // text colors
    QColor m_clrConnect;
    QColor m_clrDisconnect;
};
