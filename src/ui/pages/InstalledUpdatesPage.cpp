#include "InstalledUpdatesPage.h"
#include "IconHelper.h"

#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QFont>
#include <QFile>
#include <QProcess>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QScrollBar>
#include <QApplication>
#include <QDate>
#include <QColor>
#include <QRegularExpression>
#include <QPushButton>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <algorithm>

// Data gathering
QHash<QString, QString> InstalledUpdatesPage::repositoryMap()
{
    // `pacman -Sl` prints "<repo> <pkg> <version> [installed]" for every package
    // in the configured sync databases. We only need pkg -> repo.
    QHash<QString, QString> map;
    QProcess p;
    p.start("pacman", {"-Sl"});
    if (!p.waitForFinished(8000))
        return map;

    const QString out = QString::fromUtf8(p.readAllStandardOutput());
    const QList<QStringView> lines = QStringView(out).split(u'\n', Qt::SkipEmptyParts);
    for (const QStringView &line : lines) {
        const int firstSp = line.indexOf(u' ');
        if (firstSp < 0) continue;
        const int secondSp = line.indexOf(u' ', firstSp + 1);
        if (secondSp < 0) continue;
        const QString repo = line.left(firstSp).toString();
        const QString pkg  = line.mid(firstSp + 1, secondSp - firstSp - 1).toString();
        map.insert(pkg, repo);
    }
    return map;
}

// Vendor-style label for the Publisher column, derived from the repository.
static QString publisherForRepo(const QString &repo)
{
    if (repo.startsWith("cachyos")) return "CachyOS";
    if (repo == "AUR")             return "Arch User Repository";
    if (repo == "core" || repo == "extra" || repo == "multilib"
        || repo == "testing" || repo == "extra-testing"
        || repo == "multilib-testing" || repo == "core-testing"
        || repo == "community")
        return "Arch Linux";
    return "Linux";
}

QList<InstalledUpdatesPage::UpdateInfo> InstalledUpdatesPage::gatherUpdates()
{
    QList<UpdateInfo> result;

    QFile log("/var/log/pacman.log");
    if (!log.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    const QHash<QString, QString> repos = repositoryMap();

    // [2026-06-11T19:30:23+0200] [ALPM] upgraded pcsclite (2.5.0-1.1 -> 2.5.1-1.1)
    static const QRegularExpression re(
        R"(^\[([0-9-]{10})T[0-9:+\-]+\] \[ALPM\] upgraded (\S+) \(\S+ -> (\S+)\))");

    // Latest upgrade per package. The log is chronological, so a later line for
    // the same package overwrites the earlier one; we track insertion-time data.
    QHash<QString, UpdateInfo> latest;
    const QString text = QString::fromUtf8(log.readAll());
    const QList<QStringView> lines = QStringView(text).split(u'\n', Qt::SkipEmptyParts);
    for (const QStringView &line : lines) {
        const auto m = re.match(line.toString());
        if (!m.hasMatch()) continue;

        UpdateInfo u;
        u.name    = m.captured(2);
        u.version = m.captured(3);
        u.repo    = repos.value(u.name, QStringLiteral("AUR"));
        u.publisher = publisherForRepo(u.repo);

        const QDate d = QDate::fromString(m.captured(1), "yyyy-MM-dd");
        u.date = d.isValid() ? d.toString("M/d/yyyy") : m.captured(1);

        latest.insert(u.name, u);
    }

    result = latest.values();
    return result;
}

// Sidebar entries
QStringList InstalledUpdatesPage::sidebarLinks()
{
    return { "Control Panel Home", "Uninstall a program",
             "Turn Linux features on or off" };
}

QStringList InstalledUpdatesPage::sidebarSeeAlso()
{
    return {};
}

// Page
InstalledUpdatesPage::InstalledUpdatesPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background: #FFFFFF;");
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(sidebar);

    auto *contentWrap = new QWidget;
    contentWrap->setStyleSheet("background: #FFFFFF;");
    auto *contentV = new QVBoxLayout(contentWrap);
    contentV->setContentsMargins(28, 18, 28, 0);
    contentV->setSpacing(0);

    // Title + instruction.
    auto *title = new QLabel("Uninstall an update");
    {
        QFont f = title->font();
        f.setPointSize(12);
        title->setFont(f);
    }
    title->setStyleSheet("color: #1A3C7A; background: transparent;");
    contentV->addWidget(title);
    contentV->addSpacing(8);

    auto *subtitle = new QLabel(
        "To uninstall an update, select it from the list and then click Uninstall or Change.");
    {
        QFont f = subtitle->font();
        f.setPointSize(9);
        subtitle->setFont(f);
    }
    subtitle->setStyleSheet("color: #000000; background: transparent;");
    contentV->addWidget(subtitle);
    contentV->addSpacing(12);

    auto *topSep = new QFrame;
    topSep->setFrameShape(QFrame::HLine);
    topSep->setStyleSheet("color: #D9D9D9;");
    contentV->addWidget(topSep);

    // "Organize" toolbar row
    auto *toolBar = new QFrame;
    toolBar->setObjectName("iuToolBar");
    toolBar->setFixedHeight(28);
    toolBar->setStyleSheet(
        "#iuToolBar { background: #F4F7FB; border-bottom: 1px solid #D9D9D9; }");
    auto *toolH = new QHBoxLayout(toolBar);
    toolH->setContentsMargins(8, 0, 8, 0);
    toolH->setSpacing(6);

    auto *organize = new QLabel("Organize ▾");
    {
        QFont f = organize->font();
        f.setPointSize(9);
        organize->setFont(f);
    }
    organize->setStyleSheet("color: #1F1F1F; background: transparent;");
    toolH->addWidget(organize);
    toolH->addStretch(1);

    auto *viewIcon = new QLabel;
    viewIcon->setPixmap(themeIcon({"view-list-details", "view-list-text",
                                   "view-choose"}).pixmap(16, 16));
    toolH->addWidget(viewIcon);
    auto *helpIcon = new QLabel;
    helpIcon->setPixmap(themeIcon({"help-contents", "help-browser",
                                   "system-help"}).pixmap(16, 16));
    toolH->addWidget(helpIcon);
    contentV->addWidget(toolBar);

    // Updates tree
    m_tree = new QTreeWidget;
    m_tree->setColumnCount(4);
    m_tree->setHeaderLabels({ "Name", "Program", "Version", "Publisher" });
    m_tree->setRootIsDecorated(false);
    m_tree->setItemsExpandable(false);
    m_tree->setIndentation(0);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setAlternatingRowColors(false);
    m_tree->setFrameShape(QFrame::NoFrame);
    m_tree->header()->setDefaultAlignment(Qt::AlignLeft);
    m_tree->header()->setStretchLastSection(true);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_tree->setColumnWidth(0, 320);
    m_tree->setColumnWidth(1, 150);
    m_tree->setColumnWidth(2, 90);
    {
        // Lay the header out with the same 9pt the section stylesheet draws with,
        // so it doesn't cache an over-tall size hint before the stylesheet polish.
        QFont hf = m_tree->header()->font();
        hf.setPointSize(9);
        m_tree->header()->setFont(hf);
    }
    // Scope styling to the header only and force the native style back onto the
    // scroll bars, matching the approach used on the Linux Update select tree.
    m_tree->header()->setStyleSheet(
        "QHeaderView::section {"
        "  background: #F0F0F0;"
        "  border: none;"
        "  border-bottom: 1px solid #CCCCCC;"
        "  border-right: 1px solid #CCCCCC;"
        "  padding: 4px;"
        "  font-size: 9pt;"
        "}");
    m_tree->verticalScrollBar()->setStyle(QApplication::style());
    m_tree->horizontalScrollBar()->setStyle(QApplication::style());

    // Placeholder shown until the worker thread finishes gathering (see below).
    auto *searching = new QTreeWidgetItem(m_tree);
    searching->setFirstColumnSpanned(true);
    searching->setTextAlignment(0, Qt::AlignHCenter);
    searching->setText(0, "Searching for installed updates...");
    searching->setFlags(Qt::ItemIsEnabled);
    contentV->addWidget(m_tree, 1);

    // Status bar: item count
    auto *statusBar = new QFrame;
    statusBar->setObjectName("iuStatusBar");
    statusBar->setFixedHeight(36);
    statusBar->setStyleSheet(
        "#iuStatusBar { background: #F4F7FB; border-top: 1px solid #D9D9D9; }");
    auto *statusH = new QHBoxLayout(statusBar);
    statusH->setContentsMargins(10, 0, 10, 0);
    statusH->setSpacing(8);

    auto *statusIcon = new QLabel;
    statusIcon->setPixmap(themeIcon({"system-software-update",
                                     "preferences-system"}).pixmap(24, 24));
    statusH->addWidget(statusIcon);

    m_countLbl = new QLabel("0 items");
    {
        QFont f = m_countLbl->font();
        f.setPointSize(9);
        m_countLbl->setFont(f);
    }
    m_countLbl->setStyleSheet("color: #1F1F1F; background: transparent;");
    statusH->addWidget(m_countLbl);
    statusH->addStretch(1);
    contentV->addWidget(statusBar);

    root->addWidget(contentWrap, 1);

    // Navigate first, gather second: the package query shells out to pacman and
    // can take a while, so run it on a worker thread and populate when it lands.
    // The page is already on screen with the "Searching..." placeholder by then.
    m_tree->setCursor(Qt::BusyCursor);
    auto *watcher = new QFutureWatcher<QList<UpdateInfo>>(this);
    connect(watcher, &QFutureWatcher<QList<UpdateInfo>>::finished, this,
            [this, watcher]() {
        populate(watcher->result());
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run(&InstalledUpdatesPage::gatherUpdates));
}

void InstalledUpdatesPage::populate(const QList<UpdateInfo> &updatesIn)
{
    m_tree->setCursor(Qt::ArrowCursor);
    m_tree->clear();   // drop the "Searching..." placeholder

    // Order repositories deterministically: official repos first, then CachyOS,
    // then AUR, then anything else alphabetically.
    auto repoRank = [](const QString &r) -> int {
        if (r == "core")     return 0;
        if (r == "extra")    return 1;
        if (r == "multilib") return 2;
        if (r.startsWith("cachyos")) return 3;
        if (r == "AUR")      return 5;
        return 4;
    };

    QStringList repoOrder;
    QHash<QString, QList<UpdateInfo>> byRepo;
    for (const UpdateInfo &u : updatesIn) {
        if (!byRepo.contains(u.repo)) repoOrder << u.repo;
        byRepo[u.repo].append(u);
    }
    std::sort(repoOrder.begin(), repoOrder.end(),
        [&](const QString &a, const QString &b) {
            const int ra = repoRank(a), rb = repoRank(b);
            return (ra != rb) ? ra < rb : a < b;
        });

    const QIcon updateIcon = themeIcon({"system-software-update",
                                        "package-x-generic", "applications-other"});

    int totalItems = 0;
    for (const QString &repo : repoOrder) {
        QList<UpdateInfo> &items = byRepo[repo];
        std::sort(items.begin(), items.end(),
            [](const UpdateInfo &a, const UpdateInfo &b) {
                return a.name.localeAwareCompare(b.name) < 0;
            });

        auto *group = new QTreeWidgetItem(m_tree);
        group->setFirstColumnSpanned(true);
        group->setText(0, QString("%1 (%2)").arg(repo).arg(items.size()));
        group->setForeground(0, QColor("#1F4E99"));
        group->setFlags(Qt::ItemIsEnabled);   // header: not selectable
        QFont gf = group->font(0);
        gf.setPointSize(9);
        group->setFont(0, gf);

        for (const UpdateInfo &u : items) {
            auto *child = new QTreeWidgetItem(group);
            child->setIcon(0, updateIcon);
            child->setText(0, QString("Update for %1 (%2)").arg(u.name, u.version));
            child->setText(1, u.name);
            child->setText(2, u.version);
            child->setText(3, u.publisher);
            for (int c = 0; c < 4; ++c) {
                QFont cf = child->font(c);
                cf.setPointSize(9);
                child->setFont(c, cf);
            }
            ++totalItems;
        }
    }
    m_tree->expandAll();

    m_countLbl->setText(QString("%1 item%2")
                            .arg(totalItems).arg(totalItems == 1 ? "" : "s"));
}
