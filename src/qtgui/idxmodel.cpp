#include <string>
#include <iostream>
#include <stack>

#include <QStandardItemModel>

#include "recoll.h"
#include "rclconfig.h"
#include "fstreewalk.h"
#include "idxmodel.h"


class WalkerCB : public FsTreeWalkerCB {
public:
    WalkerCB(RclConfig *config, const std::string& topstring, FsTreeWalker& walker,
             IdxTreeModel *model, const QModelIndex& index)
        : m_config(config), m_topstring(topstring), m_walker(walker), m_model(model)
    {
        m_indexes.push(index);
        m_rows.push(0);
    }
    virtual FsTreeWalker::Status processone(
        const std::string& path, const struct PathStat *, FsTreeWalker::CbFlag flg) override;

    RclConfig *m_config;
    std::string m_topstring;
    FsTreeWalker& m_walker;
    IdxTreeModel *m_model;
    std::stack<QModelIndex> m_indexes;
    std::stack<int> m_rows;
};

FsTreeWalker::Status WalkerCB::processone(
    const std::string& path, const struct PathStat *, FsTreeWalker::CbFlag flg)
{
    if (flg == FsTreeWalker::FtwDirEnter || flg == FsTreeWalker::FtwDirReturn) {
        m_config->setKeyDir(path);
        // Set up filter/skipped patterns for this subtree. 
        m_walker.setOnlyNames(m_config->getOnlyNames());
        m_walker.setSkippedNames(m_config->getSkippedNames());
    }
    if (flg == FsTreeWalker::FtwDirReturn) {
        m_indexes.pop();
        m_rows.pop();
        return FsTreeWalker::FtwOk;
    }
    if (flg == FsTreeWalker::FtwDirEnter) {
        if (m_model->columnCount(m_indexes.top()) == 0) {
            if (!m_model->insertColumn(0, m_indexes.top()))
                return FsTreeWalker::FtwError;
        }
        if (!m_model->insertRow(m_rows.top(), m_indexes.top()))
            return FsTreeWalker::FtwError;
        const QModelIndex child = m_model->index(m_rows.top(), 0, m_indexes.top());
        // Setting the short path in DisplayRole and the real one in EditRole does not seem to work,
        // the treeview shows the EditRole?? So use the ToolTip to store the full value
        std::string disp;
        if (m_topstring.empty()) {
            disp = path_getsimple(path);
        } else {
            disp = m_topstring;
            m_topstring.clear();
        }
        m_model->setData(child, QVariant(path2qs(disp)), Qt::DisplayRole);
        m_model->setData(child, QVariant(path2qs(path)), Qt::ToolTipRole);
        ++m_rows.top();
        m_indexes.push(child);
        m_rows.push(0);
    }
    return FsTreeWalker::FtwOk;
}

static void populateDir(RclConfig *config, const std::string& topstr, IdxTreeModel *model,
                        const QModelIndex& index, const std::string& path, int depth)
{
    FsTreeWalker walker;
    walker.setSkippedPaths(config->getSkippedPaths());
//  walker.setOpts(walker.getOpts() | FsTreeWalker::FtwSkipDotFiles);
    walker.setMaxDepth(depth);
    WalkerCB cb(config, topstr, walker, model, index);
    walker.walk(path, cb);
}

void IdxTreeModel::populate()
{
    QModelIndex index = this->index(0,0);
    auto topdirs = m_config->getTopdirs();

    auto prefix = commonprefix(topdirs);

    if (this->columnCount(index) == 0) {
        if (!this->insertColumn(0, index))
            return;
    }

    int row = 0;
    for (const auto& topdir : topdirs) {
        const QModelIndex child = this->index(row, 0, index);
        std::string topdisp;
        if (prefix.size() > 1) {
            topdisp = topdir.substr(prefix.size());
        } else {
            topdisp = topdir;
        }
        populateDir(m_config, topdisp, this, child, topdir, m_depth);
        ++row;
    }
    sort(0, Qt::AscendingOrder);
}
