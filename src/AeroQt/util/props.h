#pragma once

#include <QObject>
#include <QMetaObject>
#include <QEvent>
#include <QVariant>

#include "objecteventlistener.h"

/**
 * Create a property in a single line. Inplements m_ member, getter, setter, and nttify signal.
 */

#define MAKE_PROPERTY(type, name, Name)                      \
Q_PROPERTY(type name READ name WRITE set##Name NOTIFY name##Changed)    \
    private:                                          \
    type m_##name;                                \
    public:                                           \
    type &name() { return m_##name; }\
    void set##Name(type &value) {          \
        if (m_##name != value) {                 \
            m_##name = value;                    \
            Q_EMIT name##Changed(m_##name);        \
        }                                           \
    }                                                 \
    Q_SIGNAL void name##Changed(type &);

/**
 * Bind a property on one object to a property on another. Akin to g_object_bind_property()
 */

template <typename Src, typename NotifyFn>
QMetaObject::Connection bind_prop(
    Src* src, const char* SRC_PROP,
    QObject* dst, const char* DST_PROP,
    NotifyFn notify, bool syncOnBind = false, std::function<QVariant(QVariant)> convert = nullptr)
{
    convert = (convert != nullptr) ? convert : [](QVariant v) { return v; };

    auto syncFunc = [src, dst, SRC_PROP, DST_PROP, convert](auto&&...) {
        dst->setProperty(DST_PROP, convert(src->property(SRC_PROP)));
    };

    auto binding = QObject::connect(src, notify, syncFunc);

    if (syncOnBind)
        syncFunc();

    return binding;
}

template <typename Src>
QMetaObject::Connection bind_prop(
    Src* src, const char* SRC_PROP,
    QObject* dst, const char* DST_PROP,
    QEvent::Type evType, bool syncOnBind = false, std::function<QVariant(QVariant)> convert = nullptr)
{
    convert = (convert != nullptr) ? convert : [](QVariant v) { return v; };

    auto syncFunc = [src, dst, SRC_PROP, DST_PROP, convert](auto&&...) {
        dst->setProperty(DST_PROP, convert(src->property(SRC_PROP)));
    };

    auto binding = QObject::connect(ObjectEventListener::instance(), &ObjectEventListener::eventSignal, src, [=](QObject *evDst, QEvent *ev) {
        if (evDst == src && ev->type() == evType)
            syncFunc();
    });

    if (syncOnBind)
        syncFunc();

    return binding;
}