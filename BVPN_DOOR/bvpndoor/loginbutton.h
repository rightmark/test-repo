#pragma once

#include <QtWidgets/QLabel>

class CLoginButton : public QLabel
{
    Q_OBJECT

public:
    CLoginButton(QWidget* parent);
    ~CLoginButton() Q_DECL_NOEXCEPT {}

public Q_SLOTS:
    void identified(QString);

Q_SIGNALS:
    void click(bool);

protected:
    // overridables
    void enterEvent(QEvent*);
    void leaveEvent(QEvent*);
    void mouseReleaseEvent(QMouseEvent*);

private:
    // helper methods
    void OnEnter(void) Q_DECL_NOEXCEPT;
    void OnLeave(void) Q_DECL_NOEXCEPT;

    void updateLabelImage(const QString&) Q_DECL_NOEXCEPT;
    void updateLoginText(const QString&) Q_DECL_NOEXCEPT;
    void updateLogoutText(const QString&) Q_DECL_NOEXCEPT;
    void updateLogoutText(const QString&, const QString&) Q_DECL_NOEXCEPT;

private:
    QString m_strUserName;  // user credentials to log in
    // buddy label colors
    QColor m_clrNorm;       // normal color
    QColor m_clrHovr;       // hover color
    QColor m_clrUser;       // user name span color
};
