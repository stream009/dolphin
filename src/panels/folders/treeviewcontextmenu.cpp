/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz19@gmail.com>        *
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "treeviewcontextmenu.h"

#include <KFileItem>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/JobUiDelegate>
#include <QMenu>
#include <QIcon>
#include <KJobWidgets>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KUrlMimeData>
#include <KFileItemListProperties>
#include <KLocalizedString>
#include <KIO/PasteJob>
#include <KIO/Paste>
#include <KIO/FileUndoManager>
#include <KPropertiesDialog>

#include "folderspanel.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QPointer>

TreeViewContextMenu::TreeViewContextMenu(FoldersPanel* parent,
                                         const KFileItem& fileInfo) :
    QObject(parent),
    m_parent(parent),
    m_fileItem(fileInfo)
{
}

TreeViewContextMenu::~TreeViewContextMenu()
{
}

void TreeViewContextMenu::open()
{
    QMenu* popup = new QMenu(m_parent);

    if (!m_fileItem.isNull()) {
        KFileItemListProperties capabilities(KFileItemList() << m_fileItem);

        // insert 'Cut', 'Copy' and 'Paste'
        QAction* cutAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-cut")), i18nc("@action:inmenu", "Cut"), this);
        cutAction->setEnabled(capabilities.supportsMoving());
        connect(cutAction, &QAction::triggered, this, &TreeViewContextMenu::cut);

        QAction* copyAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18nc("@action:inmenu", "Copy"), this);
        connect(copyAction, &QAction::triggered, this, &TreeViewContextMenu::copy);

        const QMimeData *mimeData = QApplication::clipboard()->mimeData();
        bool canPaste;
        const QString text = KIO::pasteActionText(mimeData, &canPaste, m_fileItem);
        QAction* pasteAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-paste")), text, this);
        connect(pasteAction, &QAction::triggered, this, &TreeViewContextMenu::paste);
        pasteAction->setEnabled(canPaste);

        popup->addAction(cutAction);
        popup->addAction(copyAction);
        popup->addAction(pasteAction);
        popup->addSeparator();

        // insert 'Rename'
        QAction* renameAction = new QAction(i18nc("@action:inmenu", "Rename..."), this);
        renameAction->setEnabled(capabilities.supportsMoving());
        renameAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
        connect(renameAction, &QAction::triggered, this, &TreeViewContextMenu::rename);
        popup->addAction(renameAction);

        // insert 'Move to Trash' and (optionally) 'Delete'
        KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::IncludeGlobals);
        KConfigGroup configGroup(globalConfig, "KDE");
        bool showDeleteCommand = configGroup.readEntry("ShowDeleteCommand", false);

        const QUrl url = m_fileItem.url();
        if (url.isLocalFile()) {
            QAction* moveToTrashAction = new QAction(QIcon::fromTheme(QStringLiteral("user-trash")),
                                                    i18nc("@action:inmenu", "Move to Trash"), this);
            const bool enableMoveToTrash = capabilities.isLocal() && capabilities.supportsMoving();
            moveToTrashAction->setEnabled(enableMoveToTrash);
            connect(moveToTrashAction, &QAction::triggered, this, &TreeViewContextMenu::moveToTrash);
            popup->addAction(moveToTrashAction);
        } else {
            showDeleteCommand = true;
        }

        if (showDeleteCommand) {
            QAction* deleteAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("@action:inmenu", "Delete"), this);
            deleteAction->setEnabled(capabilities.supportsDeleting());
            connect(deleteAction, &QAction::triggered, this, &TreeViewContextMenu::deleteItem);
            popup->addAction(deleteAction);
        }

        popup->addSeparator();
    }

    // insert 'Show Hidden Files'
    QAction* showHiddenFilesAction = new QAction(i18nc("@action:inmenu", "Show Hidden Files"), this);
    showHiddenFilesAction->setCheckable(true);
    showHiddenFilesAction->setChecked(m_parent->showHiddenFiles());
    popup->addAction(showHiddenFilesAction);
    connect(showHiddenFilesAction, &QAction::toggled, this, &TreeViewContextMenu::setShowHiddenFiles);

    // insert 'Automatic Scrolling'
    QAction* autoScrollingAction = new QAction(i18nc("@action:inmenu", "Automatic Scrolling"), this);
    autoScrollingAction->setCheckable(true);
    autoScrollingAction->setChecked(m_parent->autoScrolling());
    // TODO: Temporary disabled. Horizontal autoscrolling will be implemented later either
    // in KItemViews or manually as part of the FoldersPanel
    //popup->addAction(autoScrollingAction);
    connect(autoScrollingAction, &QAction::toggled, this, &TreeViewContextMenu::setAutoScrolling);

    if (!m_fileItem.isNull()) {
        // insert 'Properties' entry
        QAction* propertiesAction = new QAction(i18nc("@action:inmenu", "Properties"), this);
        propertiesAction->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
        connect(propertiesAction, &QAction::triggered, this, &TreeViewContextMenu::showProperties);
        popup->addAction(propertiesAction);
    }

    QList<QAction*> customActions = m_parent->customContextMenuActions();
    if (!customActions.isEmpty()) {
        popup->addSeparator();
        foreach (QAction* action, customActions) {
            popup->addAction(action);
        }
    }

    QPointer<QMenu> popupPtr = popup;
    popup->exec(QCursor::pos());
    if (popupPtr.data()) {
        popupPtr.data()->deleteLater();
    }
}

void TreeViewContextMenu::populateMimeData(QMimeData* mimeData, bool cut)
{
    QList<QUrl> kdeUrls;
    kdeUrls.append(m_fileItem.url());
    QList<QUrl> mostLocalUrls;
    bool dummy;
    mostLocalUrls.append(m_fileItem.mostLocalUrl(dummy));
    KIO::setClipboardDataCut(mimeData, cut);
    KUrlMimeData::setUrls(kdeUrls, mostLocalUrls, mimeData);
}

void TreeViewContextMenu::cut()
{
    QMimeData* mimeData = new QMimeData();
    populateMimeData(mimeData, true);
    QApplication::clipboard()->setMimeData(mimeData);
}

void TreeViewContextMenu::copy()
{
    QMimeData* mimeData = new QMimeData();
    populateMimeData(mimeData, false);
    QApplication::clipboard()->setMimeData(mimeData);
}

void TreeViewContextMenu::paste()
{
    KIO::PasteJob *job = KIO::paste(QApplication::clipboard()->mimeData(), m_fileItem.url());
    KJobWidgets::setWindow(job, m_parent);
}

void TreeViewContextMenu::rename()
{
    m_parent->rename(m_fileItem);
}

void TreeViewContextMenu::moveToTrash()
{
    const QList<QUrl> list{m_fileItem.url()};
    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(m_parent);
    if (uiDelegate.askDeleteConfirmation(list, KIO::JobUiDelegate::Trash, KIO::JobUiDelegate::DefaultConfirmation)) {
        KIO::Job* job = KIO::trash(list);
        KIO::FileUndoManager::self()->recordJob(KIO::FileUndoManager::Trash, list, QUrl(QStringLiteral("trash:/")), job);
        KJobWidgets::setWindow(job, m_parent);
        job->ui()->setAutoErrorHandlingEnabled(true);
    }
}

void TreeViewContextMenu::deleteItem()
{
    const QList<QUrl> list{m_fileItem.url()};
    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(m_parent);
    if (uiDelegate.askDeleteConfirmation(list, KIO::JobUiDelegate::Delete, KIO::JobUiDelegate::DefaultConfirmation)) {
        KIO::Job* job = KIO::del(list);
        KJobWidgets::setWindow(job, m_parent);
        job->ui()->setAutoErrorHandlingEnabled(true);
    }
}

void TreeViewContextMenu::showProperties()
{
    KPropertiesDialog* dialog = new KPropertiesDialog(m_fileItem.url(), m_parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void TreeViewContextMenu::setShowHiddenFiles(bool show)
{
    m_parent->setShowHiddenFiles(show);
}

void TreeViewContextMenu::setAutoScrolling(bool enable)
{
    m_parent->setAutoScrolling(enable);
}

