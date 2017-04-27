/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz (peter.penz@gmx.at)             *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

#include "dolphin_export.h"

#include <QDialog>
#include <KFileItem>
#include <QString>

class QLineEdit;
class QSpinBox;
class QPushButton;

/**
 * @brief Dialog for renaming a variable number of files.
 */
class DOLPHIN_EXPORT RenameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RenameDialog(QWidget* parent, const KFileItemList& items);
    virtual ~RenameDialog();

private slots:
    void slotAccepted();
    void slotTextChanged(const QString& newName);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void renameItems();
    void renameItem(const KFileItem &item, const QString& newName);

    /**
     * @return Returns the string \p name, where the characters represented by
     *         \p indexPlaceHolder get replaced by the index \p index.
     *         E. g. Calling indexedName("Test #.jpg", 12, '#') returns "Test 12.jpg".
     *         A connected sequence of placeholders results in leading zeros:
     *         indexedName("Test ####.jpg", 12, '#') returns "Test 0012.jpg".
     */
    static QString indexedName(const QString& name, int index, const QChar& indexPlaceHolder);

private:
    bool m_renameOneItem;
    QString m_newName;
    QLineEdit* m_lineEdit;
    KFileItemList m_items;
    bool m_allExtensionsDifferent;
    QSpinBox* m_spinBox;
    QPushButton* m_okButton;
};

#endif
