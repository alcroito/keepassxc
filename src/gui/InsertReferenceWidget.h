/*
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSX_INSERTREFERENCEWIDGET_H
#define KEEPASSX_INSERTREFERENCEWIDGET_H

#include <QWidget>

#include "core/Entry.h"

namespace Ui
{
    class InsertReferenceWidget;
}

class QMenu;
class QLineEdit;

class Database;
class EntrySearcher;

class InsertReferenceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InsertReferenceWidget(QSharedPointer<Database> database, QWidget* parent = nullptr);
    ~InsertReferenceWidget();

    static InsertReferenceWidget* popupInsertReferenceWidget(QWidget* target,
                                                             QSharedPointer<Database> database);
    static void injectActionIntoContextMenu(QMenu* menu,
                                            QLineEdit* target,
                                            QSharedPointer<Database> database);
    static void handleContextMenuEvent(QLineEdit* target,
                                       QContextMenuEvent *event,
                                       QSharedPointer<Database> database);
    bool isSearchActive() const;
public slots:
    void applyChosenReference();
    void endSearch();

signals:
    void appliedReference(const QString& password);
    void closed();
    void clearSearch();
    void escapePressed();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void search(const QString& searchtext);
    void entrySelectionChangedSignalReceived(Entry* entry);
    void updateAttributeName(const QString& attributeName);
    void updateAttributeValue();
    void escapeReceived();

private:
    static void popupReferenceInserterWidget(QLineEdit* target, QSharedPointer<Database> database);
    QStringList getAvailableAttributes() const;
    QString getReferenceForCurrentEntry() const;

    QSharedPointer<Database> m_database;
    QScopedPointer<EntrySearcher> m_entrySearcher;
    const QScopedPointer<Ui::InsertReferenceWidget> m_ui;
    QPointer<Entry> m_selectedEntry;
    QString m_selectedAttribute;
    QString m_lastSearchText;
};

#endif // KEEPASSX_INSERTREFERENCEWIDGET_H
