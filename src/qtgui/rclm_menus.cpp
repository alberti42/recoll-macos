#include "rclmain_w.h"

#include <QMenu>
#include <QPushButton>

void RclMain::buildMenus()
{
    fileMenu = new QMenu();
    fileMenu->setObjectName(QString::fromUtf8("fileMenu"));
    viewMenu = new QMenu();
    viewMenu->setObjectName(QString::fromUtf8("viewMenu"));
    toolsMenu = new QMenu();
    toolsMenu->setObjectName(QString::fromUtf8("toolsMenu"));
    preferencesMenu = new QMenu();
    preferencesMenu->setObjectName(QString::fromUtf8("preferencesMenu"));
    helpMenu = new QMenu();
    helpMenu->setObjectName(QString::fromUtf8("helpMenu"));
    menuResults = new QMenu();
    menuResults->setObjectName(QString::fromUtf8("menuResults"));

    fileMenu->setTitle(QApplication::translate("RclMainBase", "&File", nullptr));
    viewMenu->setTitle(QApplication::translate("RclMainBase", "&View", nullptr));
    toolsMenu->setTitle(QApplication::translate("RclMainBase", "&Tools", nullptr));
    preferencesMenu->setTitle(QApplication::translate("RclMainBase", "&Preferences", nullptr));
    helpMenu->setTitle(QApplication::translate("RclMainBase", "&Help", nullptr));
    menuResults->setTitle(QApplication::translate("RclMainBase", "&Results", nullptr));


    fileMenu->insertAction(fileRebuildIndexAction, fileBumpIndexingAction);
    fileMenu->addAction(fileToggleIndexingAction);
    fileMenu->addAction(fileRebuildIndexAction);
    fileMenu->addAction(actionSpecial_Indexing);
    fileMenu->addSeparator();
    fileMenu->addAction(actionSave_last_query);
    fileMenu->addAction(actionLoad_saved_query);
    fileMenu->addSeparator();
    fileMenu->addAction(fileExportSSearchHistoryAction);
    fileMenu->addAction(fileEraseSearchHistoryAction);
    fileMenu->addSeparator();
    fileMenu->addAction(fileEraseDocHistoryAction);
    fileMenu->addSeparator();
    fileMenu->addAction(fileExitAction);

    viewMenu->addAction(showMissingHelpers_Action);
    viewMenu->addAction(showActiveTypes_Action);
    viewMenu->addAction(actionShow_index_statistics);
    viewMenu->addSeparator();
    viewMenu->addAction(toggleFullScreenAction);

    toolsMenu->addAction(toolsDoc_HistoryAction);
    toolsMenu->addAction(toolsAdvanced_SearchAction);
    toolsMenu->addAction(toolsSpellAction);
    toolsMenu->addAction(actionShowQueryDetails);
    toolsMenu->addAction(actionQuery_Fragments);
    toolsMenu->addAction(actionWebcache_Editor);

    preferencesMenu->addAction(indexConfigAction);
    preferencesMenu->addAction(indexScheduleAction);
    preferencesMenu->addSeparator();
    preferencesMenu->addAction(queryPrefsAction);
    preferencesMenu->addAction(extIdxAction);
    preferencesMenu->addSeparator();
    preferencesMenu->addAction(enbDarkModeAction);
    preferencesMenu->addSeparator();
    preferencesMenu->addAction(enbSynAction);
    preferencesMenu->addSeparator();

    helpMenu->addAction(userManualAction);
    helpMenu->addAction(showMissingHelpers_Action);
    helpMenu->addAction(showActiveTypes_Action);
    helpMenu->addSeparator();
    helpMenu->addAction(helpAbout_RecollAction);

    menuResults->addAction(nextPageAction);
    menuResults->addAction(prevPageAction);
    menuResults->addAction(firstPageAction);
    menuResults->addSeparator();
    menuResults->addAction(actionSortByDateAsc);
    menuResults->addAction(actionSortByDateDesc);
    menuResults->addSeparator();
    menuResults->addAction(actionShowResultsAsTable);
    menuResults->addSeparator();
    menuResults->addAction(actionSaveResultsAsCSV);

    MenuBar->addAction(fileMenu->menuAction());
    MenuBar->addAction(viewMenu->menuAction());
    MenuBar->addAction(toolsMenu->menuAction());
    MenuBar->addAction(menuResults->menuAction());
    MenuBar->addAction(preferencesMenu->menuAction());
    MenuBar->addSeparator();
    MenuBar->addAction(helpMenu->menuAction());

    QMenu *butmenu = new QMenu();
    butmenu->addAction(fileMenu->menuAction());
    butmenu->addAction(viewMenu->menuAction());
    butmenu->addAction(toolsMenu->menuAction());
    butmenu->addAction(menuResults->menuAction());
    butmenu->addAction(preferencesMenu->menuAction());
    butmenu->addSeparator();
    butmenu->addAction(helpMenu->menuAction());
    sSearch->menuPB->setMenu(butmenu);

    return;
}
