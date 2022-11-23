#include "recollrunner.h"

#include <KIO/OpenUrlJob>
#include <klocalizedstring.h>

RecollRunner::RecollRunner(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : AbstractRunner(parent, data, args)
{
    setPriority(LowPriority);
}

void RecollRunner::init()
{
    reloadConfiguration();
    connect(this, &Plasma::AbstractRunner::prepare, this, []() {
        // Initialize data for the match session. This gets called from the main thread
    });
    connect(this, &Plasma::AbstractRunner::teardown, this, []() {
        // Cleanup data from the match session. This gets called from the main thread
    });
}

void RecollRunner::match(Plasma::RunnerContext &context)
{
    QString query = context.query();
    if (query == QLatin1Char('.') || query == QLatin1String("..")) {
        return;
    }
    // This should not get in the way of the Help-Runner which gets triggered by
    // queries starting with '?'
    if (query.startsWith(QLatin1Char('?'))) {
        return;
    }

    if (!m_triggerWord.isEmpty()) {
        if (!query.startsWith(m_triggerWord)) {
            return;
        }
        query.remove(0, m_triggerWord.length());
    }

    if (query.length() > 3) {
        query.append(QLatin1Char('*'));
    }

    QList<Plasma::QueryMatch> matches;
    
    if (!context.isValid()) {
        return;
    }

    for (;;) {
        QString path;
        QString file;
        Plasma::QueryMatch match(this);
        match.setText(i18n("Open %1", path));
        match.setData(path);
        match.setId(path);
//        QIcon icon = QIcon::fromTheme(mimeDb.mimeTypeForFile(path).iconName());
        QIcon icon = QIcon::fromTheme(QString());
        match.setIcon(icon);

        // Adjust relevance (the example uses wildcards)
        if (file.compare(query, Qt::CaseInsensitive)) {
            match.setRelevance(1.0);
            match.setType(Plasma::QueryMatch::ExactMatch);
        } else {
            match.setRelevance(0.8);
        }

        // Could also call context.addMatch() for each match instead of buffering
        matches.append(match);
    }
    context.addMatches(matches);
}

void RecollRunner::run(const Plasma::RunnerContext & /*context*/, const Plasma::QueryMatch &match)
{
    // KIO::OpenUrlJob autodeletes itself, so we can just create it and forget it!
    auto *job = new KIO::OpenUrlJob(QUrl::fromLocalFile(match.data().toString()));
//    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->setRunExecutables(false);
    job->start();
}

void RecollRunner::reloadConfiguration()
{
    KConfigGroup c = config();
    m_triggerWord = c.readEntry("trigger", QString());
    if (!m_triggerWord.isEmpty()) {
        m_triggerWord.append(QLatin1Char(' '));
    }
    QString m_path;
#ifdef theEXAMPLE
    m_path = c.readPathEntry("path", QDir::homePath());
    QFileInfo pathInfo(m_path);
    if (!pathInfo.isDir()) {
        m_path = QDir::homePath();
    }
#endif
    
    QList<Plasma::RunnerSyntax> syntaxes;
    Plasma::RunnerSyntax syntax(QStringLiteral("%1:q:").arg(m_triggerWord),
                                i18n("Finds files matching :q: in the %1 folder", m_path));
    syntaxes.append(syntax);
    setSyntaxes(syntaxes);
}

K_PLUGIN_CLASS_WITH_JSON(RecollRunner, "recollrunner.json")
#include "recollrunner.moc"
