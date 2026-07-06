#ifndef OBJECTEVENTLISTENER_H
#define OBJECTEVENTLISTENER_H

#include <QObject>
#include <QHash>
#include <QEvent>
#include <functional>

class ObjectEventListener : public QObject
{
    Q_OBJECT

public:
    static ObjectEventListener *instance();

    bool eventFilter(QObject *recipient, QEvent *event) override;

    void onEvent(QObject *obj, QEvent::Type type, std::function<void(QEvent *)> cb);

Q_SIGNALS:
    void eventSignal(QObject *obj, QEvent *ev);   // This is an alternative method of using this object. You can connect to this signal, but it's slower: On(num. connected listeners) vs On(num objects)

private:
    explicit ObjectEventListener(QObject *parent = nullptr);

    QHash<
        QObject *,  // Each watched object
        QHash<      // Has a map
            QEvent::Type,   // Between event types
            QList<std::function<void(QEvent *)>>    // And lists of callbacks to be called upon each one
        >
    > watched;
};

template <typename T = QEvent, typename Fn>
void onEvent(QObject *obj, QEvent::Type type, Fn&& cb)
{
    ObjectEventListener::instance()->onEvent(obj, type,
        [cb = std::forward<Fn>(cb)](QEvent *ev) {
            cb(static_cast<T *>(ev));
        }
    );
}

#endif // OBJECTEVENTLISTENER_H
