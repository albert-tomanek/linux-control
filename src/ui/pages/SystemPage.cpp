#include "SystemPage.h"
#include "IconHelper.h"
#include "perf/WeiBenchmark.h"

#include <QEvent>
#include <QMouseEvent>

#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QFont>
#include <QFile>
#include <QTextStream>
#include <QSysInfo>
#include <QHostInfo>
#include <QRegularExpression>
#include <cmath>

// Data gathering
// Read a single `key=value` field out of /etc/os-release, stripping any quotes.
static QString osReleaseField(const QString &key)
{
    QFile f(QStringLiteral("/etc/os-release"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    QTextStream in(&f);
    const QString prefix = key + QLatin1Char('=');
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.startsWith(prefix)) {
            QString val = line.mid(prefix.size()).trimmed();
            if (val.size() >= 2 && val.startsWith('"') && val.endsWith('"'))
                val = val.mid(1, val.size() - 2);
            return val;
        }
    }
    return QString();
}

// First matching field from /proc/cpuinfo (e.g. "model name").
//
// These /proc files report a size of 0, and QTextStream::readLine() treats a
// zero-length file as immediately at-EOF, so we must slurp the whole thing
// with readAll() (which reads until the device returns no more bytes) and split
// it ourselves.
static QString cpuInfoField(const QString &key)
{
    QFile f(QStringLiteral("/proc/cpuinfo"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    const QString text = QString::fromUtf8(f.readAll());
    const QList<QStringView> lines = QStringView(text).split(u'\n');
    for (const QStringView &line : lines) {
        const int colon = line.indexOf(u':');
        if (colon < 0)
            continue;
        if (line.left(colon).trimmed() == key)
            return line.mid(colon + 1).trimmed().toString();
    }
    return QString();
}

SystemPage::SysInfo SystemPage::gatherInfo()
{
    SysInfo s;

    // Edition
    s.edition = osReleaseField(QStringLiteral("PRETTY_NAME"));
    if (s.edition.isEmpty())
        s.edition = osReleaseField(QStringLiteral("NAME"));
    if (s.edition.isEmpty())
        s.edition = QSysInfo::prettyProductName();

    s.kernel = QStringLiteral("%1 %2")
                   .arg(QSysInfo::kernelType().replace(0, 1,
                        QSysInfo::kernelType().left(1).toUpper()))
                   .arg(QSysInfo::kernelVersion());

    s.logoIcon = osReleaseField(QStringLiteral("LOGO"));

    // Processor
    s.processor = cpuInfoField(QStringLiteral("model name"));
    if (s.processor.isEmpty())
        s.processor = QStringLiteral("Unknown processor");

    // Prefer the advertised max frequency; fall back to the current clock.
    double mhz = 0.0;
    QFile maxFreq(QStringLiteral(
        "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"));
    if (maxFreq.open(QIODevice::ReadOnly | QIODevice::Text)) {
        mhz = maxFreq.readAll().trimmed().toDouble() / 1000.0;  // kHz to MHz
    }
    if (mhz <= 0.0)
        mhz = cpuInfoField(QStringLiteral("cpu MHz")).toDouble();
    if (mhz > 0.0)
        s.clock = QStringLiteral("%1 GHz").arg(mhz / 1000.0, 0, 'f', 2);

    // Memory
    QFile mem(QStringLiteral("/proc/meminfo"));
    if (mem.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Slurp with readAll() for the same zero-size /proc reason as above.
        const QString text = QString::fromUtf8(mem.readAll());
        const QList<QStringView> lines = QStringView(text).split(u'\n');
        for (const QStringView &line : lines) {
            if (line.startsWith(QLatin1String("MemTotal:"))) {
                const double kb = line.toString()
                                      .split(QRegularExpression("\\s+"))
                                      .value(1).toDouble();
                // MemTotal is what the kernel can use; the physically installed
                // amount is a bit higher (firmware/kernel reserve some). Round
                // the usable figure up to the next even GiB to recover the
                // nominal installed capacity (e.g. 15.5 -> 16).
                const double usableGiB = kb / 1024.0 / 1024.0;
                int installedGiB = static_cast<int>(std::ceil(usableGiB));
                if (installedGiB % 2)
                    ++installedGiB;
                s.memory = QStringLiteral("%1 GB (%2 GB usable)")
                               .arg(installedGiB)
                               .arg(usableGiB, 0, 'f', 1);
                break;
            }
        }
    }

    // System type
    const QString arch = QSysInfo::currentCpuArchitecture();
    const bool is64 = (QSysInfo::WordSize == 64);
    s.systemType = QStringLiteral("%1-bit Operating System (%2)")
                       .arg(is64 ? 64 : 32)
                       .arg(arch);

    // Identity
    s.hostName = QHostInfo::localHostName();
    if (s.hostName.isEmpty())
        s.hostName = QSysInfo::machineHostName();

    // The "Product ID" slot shows the real systemd machine-id, the stable,
    // unique identifier for this OS installation (/etc/machine-id, with the
    // legacy D-Bus path as a fallback).
    for (const char *path : { "/etc/machine-id", "/var/lib/dbus/machine-id" }) {
        QFile f(QString::fromLatin1(path));
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            s.productId = QString::fromLatin1(f.readAll()).trimmed();
            if (!s.productId.isEmpty())
                break;
        }
    }
    if (s.productId.isEmpty())
        s.productId = QString::fromLatin1(QSysInfo::machineUniqueId());

    return s;
}

// Clicking the "Rating" value navigates to the Linux Experience Index page.
bool SystemPage::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_ratingLabel
        && event->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            emit performanceRequested();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

// Sidebar
QStringList SystemPage::sidebarLinks()
{
    return {
        "Device Manager",
        "Remote settings",
        "System protection",
        "Advanced system settings",
    };
}

QStringList SystemPage::sidebarSeeAlso()
{
    return {
        "Action Center",
        "Linux Update",
        "Performance Information and Tools",
    };
}

// Layout helpers
QLabel *SystemPage::addInfoRow(QGridLayout *grid, int row,
                               const QString &label, const QString &value,
                               const QString &trailing, bool valueIsLink)
{
    auto *lbl = new QLabel(label);
    {
        QFont f = lbl->font();
        f.setPointSize(9);
        lbl->setFont(f);
    }
    lbl->setStyleSheet("color: #333333; background: transparent;");
    lbl->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    grid->addWidget(lbl, row, 0, Qt::AlignLeft | Qt::AlignTop);

    auto *val = new QLabel(value);
    {
        QFont f = val->font();
        f.setPointSize(9);
        val->setFont(f);
    }
    val->setWordWrap(true);
    val->setTextInteractionFlags(Qt::TextSelectableByMouse);
    if (valueIsLink) {
        val->setCursor(Qt::PointingHandCursor);
        val->setStyleSheet(
            "QLabel { color: #1F4E99; background: transparent; }"
            "QLabel:hover { color: #0033AA; }");
    } else {
        val->setStyleSheet("color: #000000; background: transparent;");
    }
    if (trailing.isEmpty()) {
        grid->addWidget(val, row, 1, Qt::AlignTop);
    } else {
        // Keep the trailing text (e.g. the clock speed) right beside the value
        // rather than pushed to the far-right column.
        auto *tr = new QLabel(trailing);
        QFont f = tr->font();
        f.setPointSize(9);
        tr->setFont(f);
        tr->setStyleSheet("color: #000000; background: transparent;");

        // A word-wrap label would be squeezed to its widest word inside the
        // HBox; keep the value on one line so the trailing text stays beside it.
        val->setWordWrap(false);

        auto *cell = new QWidget;
        cell->setStyleSheet("background: transparent;");
        auto *h = new QHBoxLayout(cell);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(18);
        h->addWidget(val, 0, Qt::AlignTop);
        h->addWidget(tr, 0, Qt::AlignTop);
        h->addStretch(1);
        grid->addWidget(cell, row, 1, Qt::AlignTop);
    }
    return val;
}

// Page
SystemPage::SystemPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    const SysInfo info = gatherInfo();

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
    auto *pageTitle = new QLabel("View basic information about your computer");
    {
        QFont f = pageTitle->font();
        f.setPointSize(12);
        pageTitle->setFont(f);
    }
    pageTitle->setStyleSheet("color: #1A3C7A; background: transparent;");
    contentV->addWidget(pageTitle);
    contentV->addSpacing(18);

    // Section heading: the muted-blue label with a faint rule trailing off to
    // the right on the same row. An optional `trailing` widget (e.g. a "Change
    // settings" link) is pinned to the far right after the rule.
    auto addHeading = [&](const QString &text, QWidget *trailing = nullptr) {
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);

        auto *h = new QLabel(text);
        QFont f = h->font();
        f.setPointSize(9);
        h->setFont(f);
        h->setStyleSheet("color: #000000; background: transparent;");
        row->addWidget(h, 0, Qt::AlignVCenter);

        auto *line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setFixedHeight(1);
        line->setStyleSheet("QFrame { background: #DDDDDD; border: none; }");
        row->addWidget(line, 1, Qt::AlignVCenter);

        if (trailing)
            row->addWidget(trailing, 0, Qt::AlignVCenter);

        contentV->addLayout(row);
        contentV->addSpacing(6);
    };

    // Linux edition (with distro logo on the right)
    addHeading("Linux edition");

    auto *editionRow = new QHBoxLayout;
    editionRow->setContentsMargins(14, 0, 0, 0);
    editionRow->setSpacing(0);

    // Text wrapper kept separate so it can be top-anchored in the row. Adding
    // the column layout directly would let the QHBoxLayout stretch it to the
    // logo's 96px height, spreading the rows out as if vertically centred.
    auto *editionTextWrap = new QWidget;
    editionTextWrap->setStyleSheet("background: transparent;");
    auto *editionCol = new QVBoxLayout(editionTextWrap);
    editionCol->setContentsMargins(0, 0, 0, 0);
    editionCol->setSpacing(7);   // match the row spacing used by the info grids

    auto *editionName = new QLabel(info.edition);
    {
        QFont f = editionName->font();
        f.setPointSize(9);
        editionName->setFont(f);
    }
    editionName->setStyleSheet("color: #000000; background: transparent;");
    editionCol->addWidget(editionName);

    auto *kernelLine = new QLabel(info.kernel);
    {
        QFont f = kernelLine->font();
        f.setPointSize(9);
        kernelLine->setFont(f);
    }
    kernelLine->setStyleSheet("color: #000000; background: transparent;");

    auto *copyright = new QLabel(
        QString::fromUtf8("Copyright © %1  The Linux community.  "
                          "All rights reserved.").arg(2026));
    {
        QFont f = copyright->font();
        f.setPointSize(9);
        copyright->setFont(f);
    }
    copyright->setStyleSheet("color: #000000; background: transparent;");

    // Order: edition name, copyright, then kernel version at the bottom.
    editionCol->addWidget(copyright);
    editionCol->addWidget(kernelLine);

    editionRow->addWidget(editionTextWrap, 1, Qt::AlignTop);

    // Distro logo, mirroring the big edition badge in the reference.
    auto *logo = new QLabel;
    logo->setFixedSize(96, 96);
    logo->setStyleSheet("background: transparent;");
    logo->setPixmap(themeIcon({ info.logoIcon.toUtf8().constData(),
                                "distributor-logo", "start-here",
                                "computer", "preferences-system" })
                        .pixmap(96, 96));
    editionRow->addWidget(logo, 0, Qt::AlignTop);

    contentV->addLayout(editionRow);
    contentV->addSpacing(16);

    // System
    addHeading("System");

    auto *sysGrid = new QGridLayout;
    sysGrid->setContentsMargins(14, 0, 0, 0);
    sysGrid->setHorizontalSpacing(16);
    sysGrid->setVerticalSpacing(7);
    sysGrid->setColumnStretch(1, 1);

    int r = 0;
    // The rating reflects the cached Linux Experience Index, if one exists, and
    // always links to the Performance Information and Tools page.
    const WeiResult wei = WeiResult::load();
    const QString ratingText = wei.valid && wei.baseScore > 0.0
        ? QStringLiteral("%1   Linux Experience Index")
              .arg(wei.baseScore, 0, 'f', 1)
        : QStringLiteral("Linux Experience Index is not available");
    m_ratingLabel = addInfoRow(sysGrid, r++, "Rating:", ratingText,
                               QString(), /*valueIsLink=*/true);
    m_ratingLabel->installEventFilter(this);
    addInfoRow(sysGrid, r++, "Processor:", info.processor, info.clock);
    addInfoRow(sysGrid, r++, "Installed memory (RAM):", info.memory);
    addInfoRow(sysGrid, r++, "System type:", info.systemType);
    addInfoRow(sysGrid, r++, "Pen and Touch:",
               "No Pen or Touch Input is available for this Display");

    contentV->addLayout(sysGrid);
    contentV->addSpacing(16);

    // Computer name, domain, and workgroup settings
    addHeading("Computer name, domain, and workgroup settings");

    auto *nameGrid = new QGridLayout;
    nameGrid->setContentsMargins(14, 0, 0, 0);
    nameGrid->setHorizontalSpacing(16);
    nameGrid->setVerticalSpacing(7);
    nameGrid->setColumnStretch(1, 1);

    r = 0;
    addInfoRow(nameGrid, r++, "Computer name:", info.hostName);
    addInfoRow(nameGrid, r++, "Full computer name:", info.hostName);
    addInfoRow(nameGrid, r++, "Computer description:", QString());
    addInfoRow(nameGrid, r++, "Workgroup:", "WORKGROUP");

    // "Change settings" sits at the right edge, on the "Computer name:" row.
    auto *change = new QLabel("Change settings");
    {
        QFont cf = change->font();
        cf.setPointSize(9);
        change->setFont(cf);
    }
    change->setCursor(Qt::PointingHandCursor);
    change->setStyleSheet(
        "QLabel { color: #1F4E99; background: transparent; }"
        "QLabel:hover { color: #0033AA; }");
    nameGrid->addWidget(change, 0, 2, Qt::AlignRight | Qt::AlignVCenter);

    contentV->addLayout(nameGrid);
    contentV->addSpacing(16);

    // Linux activation
    addHeading("Linux activation");

    auto *actGrid = new QGridLayout;
    actGrid->setContentsMargins(14, 0, 0, 0);
    actGrid->setHorizontalSpacing(16);
    actGrid->setVerticalSpacing(7);
    actGrid->setColumnStretch(1, 1);

    {
        auto *act = new QLabel("Linux is activated");
        QFont f = act->font();
        f.setPointSize(9);
        act->setFont(f);
        act->setStyleSheet("color: #000000; background: transparent;");
        actGrid->addWidget(act, 0, 0, 1, 2);
    }
    addInfoRow(actGrid, 1, "Product ID:", info.productId);

    contentV->addLayout(actGrid);

    contentV->addStretch(1);
    root->addWidget(contentWrap, 1);
}
