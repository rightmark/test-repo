#pragma once

#include <QtWidgets/QLabel>

class CLoginButton : public QLabel
{
    Q_OBJECT

public:
    CLoginButton(QWidget* parent);
    ~CLoginButton() Q_DECL_NOEXCEPT {}

public Q_SLOTS:
    void identified(const QString&);

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
    void OnEnter(void) Q_DECL_NOEXCEPT;
    void OnLeave(void) Q_DECL_NOEXCEPT;
    void OnLogin(void) Q_DECL_NOEXCEPT;

private:
    bool m_bPressed;    // mouse button pressed

    QString m_UserName; // user credentials to log in
    // buddy label colors
    QColor m_clrNorm;   // normal color
    QColor m_clrHovr;   // hover color
    QColor m_clrUser;   // user name span color
};
