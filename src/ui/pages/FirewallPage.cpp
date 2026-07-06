#include "FirewallPage.h"
#include "IconHelper.h"

#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QFont>
#include <QFile>
#include <QHostInfo>
#include <QMouseEvent>

#include <functional>

// A bare widget that runs a callback when clicked. Used for the collapsible
// panel headers so the whole header strip toggles the body, mirroring the way
// Windows' firewall network sections expand/collapse. No signals are needed, so
// this stays a plain QWidget (no Q_OBJECT / moc).
class ClickableWidget : public QWidget {
public:
    using QWidget::QWidget;
    std::function<void()> onClick;

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && onClick)
            onClick();
        QWidget::mousePressEvent(event);
    }
};

// Data gathering
// Read a single shell-style `KEY=value` field out of a config file, stripping
// surrounding single or double quotes. ufw's config files (/etc/ufw/ufw.conf,
// /etc/default/ufw) are world-readable sourced shell fragments, so this is the
// same state KDE's firewall module reads, no root required.
static QString confField(const QString &path, const QString &key)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    const QString text = QString::fromUtf8(f.readAll());
    const QList<QStringView> lines = QStringView(text).split(u'\n');
    const QString prefix = key + QLatin1Char('=');
    for (const QStringView &raw : lines) {
        const QStringView line = raw.trimmed();
        if (line.startsWith(QLatin1Char('#')))         // skip comments
            continue;
        if (!line.startsWith(prefix))
            continue;
        QString val = line.mid(prefix.size()).trimmed().toString();
        if (val.size() >= 2
            && ((val.startsWith('"') && val.endsWith('"'))
                || (val.startsWith('\'') && val.endsWith('\''))))
            val = val.mid(1, val.size() - 2);
        return val;
    }
    return QString();
}

// True when the machine has a default route (i.e. is on a network). Mirrors the
// Network and Sharing Center's reading of /proc/net/route: the default route is
// the row whose Destination column ("00000000") is all-zero.
static bool hasDefaultRoute()
{
    QFile f(QStringLiteral("/proc/net/route"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const QString text = QString::fromUtf8(f.readAll());
    const QList<QStringView> lines = QStringView(text).split(u'\n');
    for (int i = 1; i < lines.size(); ++i) {           // skip the header row
        const QList<QStringView> cols = lines[i].split(u'\t', Qt::SkipEmptyParts);
        if (cols.size() >= 2 && cols[1].trimmed() == u"00000000")
            return true;
    }
    return false;
}

// Count the `### tuple` rule entries in a ufw user.rules file, one per rule
// the user has added, matching the rule count KDE's firewall module lists.
static int countRules(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0;
    const QString text = QString::fromUtf8(f.readAll());
    int n = 0;
    for (const QStringView &line : QStringView(text).split(u'\n')) {
        if (line.trimmed().startsWith(QLatin1String("### tuple ###")))
            ++n;
    }
    return n;
}

FirewallPage::FwInfo FirewallPage::gatherInfo()
{
    FwInfo fw;

    fw.enabled = confField(QStringLiteral("/etc/ufw/ufw.conf"),
                           QStringLiteral("ENABLED")).compare(
                               QStringLiteral("yes"), Qt::CaseInsensitive) == 0;
    fw.logLevel = confField(QStringLiteral("/etc/ufw/ufw.conf"),
                            QStringLiteral("LOGLEVEL"));

    fw.inputPolicy  = confField(QStringLiteral("/etc/default/ufw"),
                                QStringLiteral("DEFAULT_INPUT_POLICY")).toUpper();
    fw.outputPolicy = confField(QStringLiteral("/etc/default/ufw"),
                                QStringLiteral("DEFAULT_OUTPUT_POLICY")).toUpper();
    if (fw.inputPolicy.isEmpty())
        fw.inputPolicy = QStringLiteral("DROP");

    fw.ruleCount = countRules(QStringLiteral("/etc/ufw/user.rules"))
                 + countRules(QStringLiteral("/etc/ufw/user6.rules"));

    fw.netConnected = hasDefaultRoute();
    fw.networkName  = QStringLiteral("Network");

    return fw;
}

// Sidebar
QStringList FirewallPage::sidebarLinks()
{
    return {
        "Allow a program or feature through Linux Firewall",
        "Change notification settings",
        "Turn Linux Firewall on or off",
        "Restore defaults",
        "Advanced settings",
        "Troubleshoot my network",
    };
}

QStringList FirewallPage::sidebarSeeAlso()
{
    return {
        "Action Center",
        "Network and Sharing Center",
    };
}

// Network-location panel
// Human-readable phrasing for the default-incoming-policy line, mirroring the
// way KDE's firewall module summarises DEFAULT_INPUT_POLICY.
static QString incomingText(const QString &policy)
{
    if (policy == QLatin1String("ACCEPT"))
        return QStringLiteral("Allow all incoming connections");
    if (policy == QLatin1String("REJECT"))
        return QStringLiteral("Reject all connections to programs that are not "
                              "on the list of allowed programs");
    // DROP (the ufw default): silently dropped, matching Windows' "Block all".
    return QStringLiteral("Block all connections to programs that are not on "
                          "the list of allowed programs");
}

// Notification phrasing derived from ufw's LOGLEVEL ("off" disables logging).
static QString notifyText(const QString &logLevel)
{
    if (logLevel.compare(QLatin1String("off"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Do not notify me when Linux Firewall blocks a "
                              "new program");
    return QStringLiteral("Notify me when Linux Firewall blocks a new program");
}

QWidget *FirewallPage::buildLocationPanel(const QString &title,
                                          const QString &connState,
                                          bool expanded,
                                          const FwInfo &info)
{
    auto *panel = new QFrame;
    panel->setObjectName("fwPanel");
    panel->setStyleSheet(
        "#fwPanel { background: #FFFFFF; border: 1px solid #DCDCDC; }");
    auto *panelV = new QVBoxLayout(panel);
    panelV->setContentsMargins(0, 0, 0, 0);
    panelV->setSpacing(0);

    // Header strip: green accent bar, shield icon, title, conn-state, chevron.
    // The header is clickable and toggles the body below, like the Windows
    // firewall network sections.
    auto *header = new ClickableWidget;
    header->setStyleSheet("background: transparent;");
    header->setCursor(Qt::PointingHandCursor);
    auto *headerH = new QHBoxLayout(header);
    headerH->setContentsMargins(0, 0, 8, 0);
    headerH->setSpacing(0);

    auto *accent = new QFrame;
    accent->setFixedWidth(14);
    accent->setMinimumHeight(34);
    accent->setStyleSheet("background: #4E9A2A; border: none;");
    headerH->addWidget(accent);

    auto *shield = new QLabel;
    shield->setFixedSize(20, 20);
    shield->setPixmap(themeIcon({"security-high", "preferences-security-firewall",
                                 "security-medium", "firewall-config"})
                          .pixmap(16, 16));
    shield->setStyleSheet("background: transparent;");
    headerH->addSpacing(8);
    headerH->addWidget(shield, 0, Qt::AlignVCenter);

    auto *titleLabel = new QLabel(title);
    {
        QFont f = titleLabel->font();
        f.setPointSize(11);
        titleLabel->setFont(f);
    }
    titleLabel->setStyleSheet("color: #1A5FB4; background: transparent;");
    headerH->addSpacing(6);
    headerH->addWidget(titleLabel, 0, Qt::AlignVCenter);
    headerH->addStretch(1);

    auto *state = new QLabel(connState);
    {
        QFont f = state->font();
        f.setPointSize(11);
        state->setFont(f);
    }
    state->setStyleSheet("color: #2D2D2D; background: transparent;");
    headerH->addWidget(state, 0, Qt::AlignVCenter);

    auto *chevron = new QLabel(expanded ? QString::fromUtf8("▲")
                                        : QString::fromUtf8("▼"));
    chevron->setStyleSheet("color: #555555; background: transparent; font-size: 7pt;");
    headerH->addSpacing(8);
    headerH->addWidget(chevron, 0, Qt::AlignVCenter);

    header->setMinimumHeight(34);
    panelV->addWidget(header);

    // Body: caption line + the status grid. Built regardless of the initial
    // state so the header can toggle it on click; visibility is set at the end.
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setFixedHeight(1);
    sep->setStyleSheet("QFrame { background: #E2E2E2; border: none; }");
    panelV->addWidget(sep);

    // Body has no horizontal margin so the dividers below span the full panel
    // width; the caption and grid carry their own left inset (kLeftInset) so
    // their text lines up under the green banner rather than under the icon.
    const int kLeftInset = 8;
    const int kRightInset = 16;

    auto *body = new QWidget;
    body->setStyleSheet("background: transparent;");
    auto *bodyV = new QVBoxLayout(body);
    bodyV->setContentsMargins(0, 4, 0, 14);
    bodyV->setSpacing(0);

    auto *caption = new QLabel(
        "Linux Firewall (ufw) applies one set of rules to every network. Unlike "
        "Windows, Linux has no separate Home, Work, or Public network profiles.");
    {
        QFont f = caption->font();
        f.setPointSize(9);
        caption->setFont(f);
    }
    caption->setWordWrap(true);
    caption->setContentsMargins(kLeftInset, 0, kRightInset, 0);
    caption->setStyleSheet("color: #000000; background: transparent;");
    bodyV->addWidget(caption);
    bodyV->addSpacing(12);

    // Divider between the caption and the status grid, as in the Windows panel.
    auto *sep2 = new QFrame;
    sep2->setFrameShape(QFrame::HLine);
    sep2->setFixedHeight(1);
    sep2->setStyleSheet("QFrame { background: #E2E2E2; border: none; }");
    bodyV->addWidget(sep2);
    bodyV->addSpacing(14);

    auto *grid = new QGridLayout;
    grid->setContentsMargins(kLeftInset, 0, kRightInset, 0);
    grid->setHorizontalSpacing(55);
    grid->setVerticalSpacing(12);
    grid->setColumnMinimumWidth(0, 200);
    grid->setColumnStretch(1, 1);

    auto makeLabel = [](const QString &text, bool link = false) {
        auto *l = new QLabel(text);
        QFont f = l->font();
        f.setPointSize(9);
        l->setFont(f);
        l->setWordWrap(true);
        // Align via the label itself, not the grid's addWidget alignment flag:
        // passing an alignment flag to QGridLayout::addWidget suppresses
        // height-for-width, so a word-wrapped label only gets one line of height
        // and any wrapped lines are clipped ("Notify me when Linux" → nothing).
        l->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        if (link) {
            l->setCursor(Qt::PointingHandCursor);
            l->setStyleSheet(
                "QLabel { color: #1F4E99; background: transparent; }"
                "QLabel:hover { color: #0033AA; }");
        } else {
            l->setStyleSheet("color: #000000; background: transparent;");
        }
        return l;
    };

    int r = 0;
    grid->addWidget(makeLabel("Linux Firewall state:"), r, 0);
    grid->addWidget(makeLabel(info.enabled ? "On" : "Off"), r, 1);
    ++r;

    grid->addWidget(makeLabel("Incoming connections:"), r, 0);
    grid->addWidget(makeLabel(incomingText(info.inputPolicy)), r, 1);
    ++r;

    grid->addWidget(makeLabel("Active networks:"), r, 0);
    // Icon + network-name cell.
    {
        auto *cell = new QWidget;
        cell->setStyleSheet("background: transparent;");
        auto *h = new QHBoxLayout(cell);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(6);
        auto *icon = new QLabel;
        icon->setFixedSize(16, 16);
        icon->setPixmap(themeIcon({"network-workgroup", "network-type-public",
                                   "preferences-system-network", "network-wired"})
                            .pixmap(16, 16));
        icon->setStyleSheet("background: transparent;");
        h->addWidget(icon, 0, Qt::AlignVCenter);
        h->addWidget(makeLabel(info.netConnected ? info.networkName
                                                 : QStringLiteral("None")),
                     0, Qt::AlignVCenter);
        h->addStretch(1);
        grid->addWidget(cell, r, 1, Qt::AlignLeft | Qt::AlignTop);
    }
    ++r;

    grid->addWidget(makeLabel("Notification state:"), r, 0);
    grid->addWidget(makeLabel(notifyText(info.logLevel)), r, 1);
    ++r;

    bodyV->addLayout(grid);
    panelV->addWidget(body);

    // Dropdown behaviour: clicking the header expands/collapses the body and
    // flips the chevron, matching the Windows firewall network sections.
    auto applyState = [body, sep, chevron](bool open) {
        sep->setVisible(open);
        body->setVisible(open);
        chevron->setText(open ? QString::fromUtf8("▲") : QString::fromUtf8("▼"));
    };
    applyState(expanded);

    // The mutable lambda is stored in header->onClick, so its captured `open`
    // copy persists between clicks and tracks the open/closed state.
    header->onClick = [applyState, open = expanded]() mutable {
        open = !open;
        applyState(open);
    };

    return panel;
}

// Page
FirewallPage::FirewallPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    const FwInfo info = gatherInfo();

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

    // Page title
    auto *pageTitle =
        new QLabel("Help protect your computer with Linux Firewall");
    {
        QFont f = pageTitle->font();
        f.setPointSize(12);
        pageTitle->setFont(f);
    }
    pageTitle->setStyleSheet("color: #1A3C7A; background: transparent;");
    contentV->addWidget(pageTitle);
    contentV->addSpacing(10);

    auto *blurb = new QLabel(
        "Linux Firewall can help prevent hackers or malicious software from "
        "gaining access to your computer through the Internet or a network.");
    {
        QFont f = blurb->font();
        f.setPointSize(9);
        blurb->setFont(f);
    }
    blurb->setWordWrap(true);
    blurb->setStyleSheet("color: #000000; background: transparent;");
    contentV->addWidget(blurb);
    contentV->addSpacing(10);

    auto addHelpLink = [&](const QString &text) {
        auto *l = new QLabel(text);
        QFont f = l->font();
        f.setPointSize(9);
        l->setFont(f);
        l->setCursor(Qt::PointingHandCursor);
        l->setStyleSheet(
            "QLabel { color: #1F4E99; background: transparent; }"
            "QLabel:hover { color: #0033AA; }");
        contentV->addWidget(l, 0, Qt::AlignLeft);
        contentV->addSpacing(4);
    };
    addHelpLink("How does a firewall help protect my computer?");
    addHelpLink("What are network locations?");

    contentV->addSpacing(12);

    // The network panel. ufw has no per-network profiles (no Home/Work/Public
    // split), so rather than mimic Windows' two location panels we show a
    // single "All networks" section expanded to the live ufw state. It is
    // "Connected" when a default route exists.
    contentV->addWidget(buildLocationPanel(
        "All networks",
        info.netConnected ? "Connected" : "Not Connected",
        /*expanded=*/true, info));

    contentV->addStretch(1);

    // Windows 7 lays the content out at a fixed width and leaves the rest of the
    // window blank on the right rather than stretching to fill a wide window.
    contentWrap->setFixedWidth(700);
    root->addWidget(contentWrap, 0);
    root->addStretch(1);
}
