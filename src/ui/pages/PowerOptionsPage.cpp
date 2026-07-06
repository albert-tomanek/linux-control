#include "PowerOptionsPage.h"

#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QFont>
#include <QPalette>
#include <QRadioButton>
#include <QButtonGroup>
#include <QToolButton>
#include <QSignalBlocker>
#include <QMouseEvent>
#include <QEvent>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDBusArgument>

// power-profiles-daemon access//
// PPD lives on the system bus. All access goes through the standard
// org.freedesktop.DBus.Properties interface on its single object, so we never
// need PPD-specific introspection to be present.

namespace {
constexpr auto kService   = "net.hadess.PowerProfiles";
constexpr auto kPath      = "/net/hadess/PowerProfiles";
constexpr auto kInterface = "net.hadess.PowerProfiles";

QDBusInterface ppdProperties()
{
    return QDBusInterface(QString::fromLatin1(kService),
                          QString::fromLatin1(kPath),
                          QStringLiteral("org.freedesktop.DBus.Properties"),
                          QDBusConnection::systemBus());
}
} // namespace

QStringList PowerOptionsPage::availableProfiles(bool &ok) const
{
    ok = false;
    QDBusInterface props = ppdProperties();
    if (!props.isValid())
        return {};

    QDBusReply<QVariant> reply =
        props.call(QStringLiteral("Get"), QString::fromLatin1(kInterface),
                   QStringLiteral("Profiles"));
    if (!reply.isValid())
        return {};

    ok = true;

    // Profiles is aa{sv}: an array of string-to-variant maps. Each map carries
    // a "Profile" key with the profile's name ("balanced", etc); the rest
    // (drivers, etc.) we don't need.
    QStringList ids;
    const QDBusArgument arg = reply.value().value<QDBusArgument>();
    arg.beginArray();
    while (!arg.atEnd()) {
        QVariantMap entry;
        arg >> entry;
        const QString id = entry.value(QStringLiteral("Profile")).toString();
        if (!id.isEmpty())
            ids << id;
    }
    arg.endArray();
    return ids;
}

QString PowerOptionsPage::activeProfile() const
{
    QDBusInterface props = ppdProperties();
    if (!props.isValid())
        return {};
    QDBusReply<QVariant> reply =
        props.call(QStringLiteral("Get"), QString::fromLatin1(kInterface),
                   QStringLiteral("ActiveProfile"));
    return reply.isValid() ? reply.value().toString() : QString();
}

void PowerOptionsPage::setActiveProfile(const QString &profileId)
{
    if (profileId.isEmpty())
        return;
    QDBusInterface props = ppdProperties();
    if (!props.isValid())
        return;
    // Fire-and-forget so a slow daemon never stalls the UI thread. polkit lets
    // the active local session set this without a password prompt, just like
    // `powerprofilesctl set`.
    props.asyncCall(QStringLiteral("Set"), QString::fromLatin1(kInterface),
                    QStringLiteral("ActiveProfile"),
                    QVariant::fromValue(QDBusVariant(profileId)));
}

// Sidebar
QStringList PowerOptionsPage::sidebarLinks()
{
    // "Control Panel Home" is prepended by the sidebar shell itself, so it must
    // not be repeated here.
    return {
        "Require a password when the computer wakes",
        "Choose what the power buttons do",
        "Create a power plan",
        "Choose when to turn off the display",
    };
}

QStringList PowerOptionsPage::sidebarSeeAlso()
{
    return {
        "Personalization",
        "User Accounts",
    };
}

// Page
PowerOptionsPage::PowerOptionsPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    bool ok = false;
    QStringList profiles = availableProfiles(ok);
    m_ppdAvailable = ok && !profiles.isEmpty();

    setStyleSheet("background: #FFFFFF;");
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(sidebar);

    auto *contentWrap = new QWidget;
    contentWrap->setStyleSheet("background: #FFFFFF;");
    auto *contentV = new QVBoxLayout(contentWrap);
    contentV->setContentsMargins(28, 18, 28, 20);
    contentV->setSpacing(0);

    // Page title.
    auto *pageTitle = new QLabel("Select a power plan");
    {
        QFont f = pageTitle->font();
        f.setPointSize(12);
        pageTitle->setFont(f);
    }
    pageTitle->setStyleSheet("color: #1A3C7A; background: transparent;");
    contentV->addWidget(pageTitle);
    contentV->addSpacing(10);

    // Intro paragraph with the trailing "Tell me more…" link. A real <a> gives
    // the underlined, palette-coloured link Windows shows; it's informational
    // only, so it points nowhere.
    auto *intro = new QLabel(
        "Power plans can help you maximize your computer's performance or "
        "conserve energy. Make a plan active by selecting it, or choose a plan "
        "and customize it by changing its power settings. "
        "<a href=\"#\">Tell me more about power plans</a>");
    {
        QFont f = intro->font();
        f.setPointSize(9);
        intro->setFont(f);
    }
    intro->setWordWrap(true);
    intro->setOpenExternalLinks(false);
    intro->setTextInteractionFlags(Qt::TextBrowserInteraction);
    {
        QPalette pal = intro->palette();
        pal.setColor(QPalette::Link, QColor("#1F4E99"));
        intro->setPalette(pal);
    }
    intro->setStyleSheet("color: #1A1A1A; background: transparent;");
    contentV->addWidget(intro);
    contentV->addSpacing(16);

    // Section heading: muted label with a hairline rule trailing off to the
    // right, optionally ending in a trailing widget (the expander chevron).
    auto addHeading = [&](const QString &text, QWidget *trailing = nullptr) {
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);

        auto *h = new QLabel(text);
        QFont f = h->font();
        f.setPointSize(9);
        h->setFont(f);
        h->setStyleSheet("color: #555555; background: transparent;");
        row->addWidget(h, 0, Qt::AlignVCenter);

        auto *line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setFixedHeight(1);
        line->setStyleSheet("QFrame { background: #DDDDDD; border: none; }");
        row->addWidget(line, 1, Qt::AlignVCenter);

        if (trailing)
            row->addWidget(trailing, 0, Qt::AlignVCenter);

        contentV->addLayout(row);
        return h;
    };

    m_group = new QButtonGroup(this);
    QObject::connect(m_group, &QButtonGroup::buttonClicked, this,
        [this](QAbstractButton *btn) {
            if (!m_syncing)
                setActiveProfile(btn->property("profileId").toString());
        });

    // Preferred plans
    addHeading("Preferred plans");
    contentV->addSpacing(6);

    m_planList = new QVBoxLayout;
    // Indent the plan rows under the heading, as Windows does.
    m_planList->setContentsMargins(14, 0, 0, 0);
    m_planList->setSpacing(14);
    contentV->addLayout(m_planList);

    // Additional plans (collapsible). Built unconditionally so the layout
    // slots exist; the whole block is hidden below when there are none.
    m_additionalWrap = new QWidget;
    m_additionalWrap->setStyleSheet("background: transparent;");
    m_additionalBox = new QVBoxLayout(m_additionalWrap);
    m_additionalBox->setContentsMargins(14, 0, 0, 0);
    m_additionalBox->setSpacing(14);

    // The round chevron toggles the additional-plans block. ⏶ when expanded
    // (click to hide), ⏷ when collapsed (click to show).
    auto *chevron = new QToolButton;
    chevron->setCheckable(true);
    chevron->setCursor(Qt::PointingHandCursor);
    chevron->setFixedSize(20, 20);
    chevron->setStyleSheet(
        "QToolButton { border: 1px solid #C0CEDA; border-radius: 10px;"
        " background: #F3F7FB; color: #1F4E99; font-size: 9pt; }"
        "QToolButton:hover { background: #E4EEF7; }");

    auto *additionalHeadingLabel = new QLabel;   // text set by applyExpanded

    contentV->addSpacing(16);

    // The heading row lives in its own host widget so it can be hidden wholesale
    // (rule included) when there are no additional plans.
    m_additionalHeader = new QWidget;
    m_additionalHeader->setStyleSheet("background: transparent;");
    {
        auto *row = new QHBoxLayout(m_additionalHeader);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);

        additionalHeadingLabel->setStyleSheet(
            "color: #555555; background: transparent;");
        QFont f = additionalHeadingLabel->font();
        f.setPointSize(9);
        additionalHeadingLabel->setFont(f);
        row->addWidget(additionalHeadingLabel, 0, Qt::AlignVCenter);

        auto *line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setFixedHeight(1);
        line->setStyleSheet("QFrame { background: #DDDDDD; border: none; }");
        row->addWidget(line, 1, Qt::AlignVCenter);

        row->addWidget(chevron, 0, Qt::AlignVCenter);
    }
    contentV->addWidget(m_additionalHeader);
    contentV->addSpacing(6);
    contentV->addWidget(m_additionalWrap);

    auto applyExpanded = [additionalHeadingLabel, chevron, this](bool expanded) {
        m_additionalWrap->setVisible(expanded);
        additionalHeadingLabel->setText(expanded ? "Hide additional plans"
                                                  : "Show additional plans");
        chevron->setText(expanded ? QString::fromUtf8("⏶")   // ⏶
                                   : QString::fromUtf8("⏷")); // ⏷
        QSignalBlocker block(chevron);
        chevron->setChecked(expanded);
    };
    QObject::connect(chevron, &QToolButton::toggled, this, applyExpanded);

    contentV->addStretch(1);
    // Fixed-width content column, left-aligned with the rest filled white,
    // matching the Linux Firewall page so this screen sits at the same width as
    // the app's other detail pages.
    contentWrap->setFixedWidth(700);
    root->addWidget(contentWrap, 0);
    root->addStretch(1);

    // Populate the plan rows, then settle the expander state.
    buildPlans(m_ppdAvailable ? profiles : QStringList());

    if (m_additionalBox->count() == 0) {
        // No additional plans: hide the whole collapsible block and its heading.
        m_additionalHeader->hide();
        m_additionalWrap->hide();
    } else {
        // Expand by default when the active plan is one of the additional ones,
        // so the current selection is never hidden; otherwise start collapsed.
        const QString active = activeProfile();
        bool activeIsAdditional = false;
        for (int i = 0; i < m_additionalBox->count(); ++i) {
            if (auto *w = m_additionalBox->itemAt(i)->widget())
                if (auto *r = w->findChild<QRadioButton *>())
                    if (r->property("profileId").toString() == active)
                        activeIsAdditional = true;
        }
        applyExpanded(activeIsAdditional);
    }

    syncSelection();

    // Stay in sync if the profile is changed elsewhere (KDE tray, power events).
    if (m_ppdAvailable) {
        QDBusConnection::systemBus().connect(
            QString::fromLatin1(kService), QString::fromLatin1(kPath),
            QStringLiteral("org.freedesktop.DBus.Properties"),
            QStringLiteral("PropertiesChanged"), this,
            SLOT(onProfilesPropertiesChanged(QString, QVariantMap, QStringList)));
    }
}

void PowerOptionsPage::buildPlans(const QStringList &profiles)
{
    // Map a raw PPD profile id onto its Windows-flavoured presentation.
    auto meta = [](const QString &id) -> Plan {
        if (id == QLatin1String("balanced"))
            return { id, QStringLiteral("Balanced (recommended)"),
                     QStringLiteral("Automatically balances performance with "
                                    "energy consumption on capable hardware."),
                     false };
        if (id == QLatin1String("performance"))
            return { id, QStringLiteral("High performance"),
                     QStringLiteral("Favors performance, but may use more "
                                    "energy."),
                     false };
        if (id == QLatin1String("power-saver"))
            return { id, QStringLiteral("Power saver"),
                     QStringLiteral("Saves energy by reducing your computer's "
                                    "performance where possible."),
                     true };
        // Any other profile a custom PPD setup might expose.
        QString pretty = id;
        pretty.replace(QLatin1Char('-'), QLatin1Char(' '));
        if (!pretty.isEmpty())
            pretty[0] = pretty[0].toUpper();
        return { id, pretty,
                 QStringLiteral("A custom power plan provided by your system."),
                 true };
    };

    QList<Plan> plans;

    if (profiles.isEmpty()) {
        // Fall-back: no PPD. Show a single, selected Balanced plan with no
        // profile id (selecting it is a no-op), matching what Windows shows on
        // hardware that exposes no alternative plans.
        plans << Plan{ QString(), QStringLiteral("Balanced (recommended)"),
                       QStringLiteral("Automatically balances performance with "
                                      "energy consumption on capable hardware."),
                       false };
    } else {
        // Preferred plans first, in Windows' order, then the additional ones.
        for (const QString &id : { QStringLiteral("balanced"),
                                   QStringLiteral("performance") })
            if (profiles.contains(id))
                plans << meta(id);
        if (profiles.contains(QLatin1String("power-saver")))
            plans << meta(QStringLiteral("power-saver"));
        for (const QString &id : profiles)
            if (id != QLatin1String("balanced")
                && id != QLatin1String("performance")
                && id != QLatin1String("power-saver"))
                plans << meta(id);
    }

    for (const Plan &plan : plans) {
        QWidget *row = buildPlanRow(plan);
        (plan.additional ? m_additionalBox : m_planList)->addWidget(row);
    }

    // With nothing wired up, pre-select the lone fall-back plan.
    if (!m_ppdAvailable && !m_group->buttons().isEmpty())
        m_group->buttons().first()->setChecked(true);
}

QWidget *PowerOptionsPage::buildPlanRow(const Plan &plan)
{
    // Layout: [radio] [ name-line over description ]. Built from nested box
    // layouts rather than a QGridLayout: a word-wrapped label in a grid cell
    // (especially a column-spanning one) is mis-sized by Qt's height-for-width
    // handling and gets clipped to a single line, which is what truncated the
    // descriptions to "Favors performance, but".
    auto *row = new QWidget;
    row->setStyleSheet("background: transparent;");
    auto *rowH = new QHBoxLayout(row);
    rowH->setContentsMargins(0, 0, 0, 0);
    rowH->setSpacing(6);

    auto *radio = new QRadioButton;
    radio->setProperty("profileId", plan.profileId);
    radio->setCursor(Qt::PointingHandCursor);
    radio->setStyleSheet("QRadioButton { background: transparent; }");
    m_group->addButton(radio);
    rowH->addWidget(radio, 0, Qt::AlignTop);

    // The text block fills the rest of the row width.
    auto *textV = new QVBoxLayout;
    textV->setContentsMargins(0, 0, 0, 0);
    textV->setSpacing(2);

    // Name line: the plan name on the left, "Change plan settings" pinned right.
    auto *nameLine = new QHBoxLayout;
    nameLine->setContentsMargins(0, 0, 0, 0);
    nameLine->setSpacing(6);

    auto *name = new QLabel(plan.name);
    {
        QFont f = name->font();
        f.setPointSize(9);
        // The recommended plan's name is shown bold (a fixed emphasis in
        // Windows, independent of which plan is currently selected).
        f.setBold(plan.name.contains(QLatin1String("(recommended)")));
        name->setFont(f);
    }
    name->setStyleSheet("color: #1A1A1A; background: transparent;");
    name->setCursor(Qt::PointingHandCursor);
    // Clicking the name selects the plan, like the label in Windows.
    m_nameToRadio.insert(name, radio);
    name->installEventFilter(this);
    nameLine->addWidget(name, 0, Qt::AlignVCenter);
    nameLine->addStretch(1);

    auto *change = new QLabel("Change plan settings");
    {
        QFont f = change->font();
        f.setPointSize(9);
        change->setFont(f);
    }
    change->setCursor(Qt::PointingHandCursor);
    change->setStyleSheet(
        "QLabel { color: #1F4E99; background: transparent; }"
        "QLabel:hover { color: #0033AA; }");
    nameLine->addWidget(change, 0, Qt::AlignVCenter);

    textV->addLayout(nameLine);

    auto *desc = new QLabel(plan.description);
    {
        QFont f = desc->font();
        f.setPointSize(9);
        desc->setFont(f);
    }
    desc->setWordWrap(true);
    desc->setStyleSheet("color: #333333; background: transparent;");
    textV->addWidget(desc);

    rowH->addLayout(textV, 1);

    return row;
}

void PowerOptionsPage::syncSelection()
{
    if (!m_ppdAvailable)
        return;
    const QString active = activeProfile();
    if (active.isEmpty())
        return;

    m_syncing = true;
    for (QAbstractButton *btn : m_group->buttons()) {
        if (btn->property("profileId").toString() == active) {
            btn->setChecked(true);
            break;
        }
    }
    m_syncing = false;
}

bool PowerOptionsPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto it = m_nameToRadio.constFind(watched);
        if (it != m_nameToRadio.constEnd()) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                QRadioButton *radio = it.value();
                if (!radio->isChecked()) {
                    radio->setChecked(true);
                    if (!m_syncing)
                        setActiveProfile(radio->property("profileId").toString());
                }
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void PowerOptionsPage::onProfilesPropertiesChanged(const QString &interface,
                                                   const QVariantMap &changed,
                                                   const QStringList &invalidated)
{
    if (interface != QLatin1String(kInterface))
        return;
    if (changed.contains(QStringLiteral("ActiveProfile"))
        || invalidated.contains(QStringLiteral("ActiveProfile")))
        syncSelection();
}
