#pragma once

#include <KRunner/AbstractRunner>

class RecollRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    RecollRunner(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match) override;
    void reloadConfiguration() override;

protected:
    void init() override;

private:
    QString m_triggerWord;
};
