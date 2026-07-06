#ifndef IMAGEBUTTON_H
#define IMAGEBUTTON_H

#include <QWidget>
#include <QPushButton>
#include <QPropertyAnimation>

namespace Aero {
namespace Private {

class ImageButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal fade READ fade WRITE setFade)

public:
    ImageButton(const QString& normal,
                const QString& hover,
                const QString& pressed,
                const QString& disabled,
                int fadeDuration,
                QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void changeEvent(QEvent* e) override;

protected:
    QPixmap pixmapNormal;
    QPixmap pixmapHover;
    QPixmap pixmapPressed;
    QPixmap pixmapDisabled;

    virtual const QPixmap* statePixmap() const;
    void updateState();

private:
    const QPixmap* current = nullptr;
    const QPixmap* previous = nullptr;

    QPropertyAnimation* anim;
    qreal fadeValue = 1.0;

    qreal fade() const
    {
        return fadeValue;
    }

    void setFade(qreal v)
    {
        fadeValue = v;
        update();
    }
};

} // namespace Private
} // namespace Aero

#endif // IMAGEBUTTON_H
