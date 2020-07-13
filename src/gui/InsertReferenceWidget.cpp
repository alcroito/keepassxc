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

#include "core/Database.h"
#include "core/EntrySearcher.h"
#include "InsertReferenceWidget.h"

#include "ui_InsertReferenceWidget.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QLineEdit>
#include <QDebug>

InsertReferenceWidget::InsertReferenceWidget(QSharedPointer<Database> database, QWidget *parent) :
    QWidget(parent),
    m_database(database),
    m_entrySearcher(new EntrySearcher(false)),
    m_ui(new Ui::InsertReferenceWidget)
{
    m_ui->setupUi(this);

//    m_ui->buttonClose->setShortcut(Qt::Key_Escape);

    connect(m_ui->buttonApply, &QPushButton::clicked, this, &InsertReferenceWidget::applyChosenReference);
    connect(m_ui->buttonClose, &QPushButton::clicked, this, &InsertReferenceWidget::closed);

    connect(m_ui->searchWidget, &SearchWidget::search, this, &InsertReferenceWidget::search);
    connect(m_ui->searchWidget, &SearchWidget::escapePressed, this, &InsertReferenceWidget::escapeReceived);
    connect(this, &InsertReferenceWidget::escapePressed, this, &InsertReferenceWidget::escapeReceived);

    //    connect(this, &InsertReferenceWidget::clearSearch,
//            m_ui->searchWidget, &SearchWidget::clear);

    connect(m_ui->entryView, &EntryView::entrySelectionChanged,
        this, &InsertReferenceWidget::entrySelectionChangedSignalReceived);

    connect(m_ui->entryView, &EntryView::entrySelectionChanged,
        this, &InsertReferenceWidget::updateAttributeValue);

    connect(m_ui->attributeComboBox, &QComboBox::currentTextChanged, this, &InsertReferenceWidget::updateAttributeName);

    m_ui->attributeComboBox->addItems(getAvailableAttributes());

    m_ui->entryView->header()->hideSection(EntryModel::Totp);
    m_ui->entryView->header()->hideSection(EntryModel::Paperclip);
    m_ui->entryView->displayGroup(m_database->rootGroup());
}

InsertReferenceWidget::~InsertReferenceWidget()
{
}

InsertReferenceWidget* InsertReferenceWidget::popupInsertReferenceWidget(
        QWidget* parent,
        QSharedPointer<Database> database)
{
    auto insertReferenceWidget = new InsertReferenceWidget(database, parent);
    insertReferenceWidget->setWindowModality(Qt::ApplicationModal);
    insertReferenceWidget->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    connect(insertReferenceWidget, &InsertReferenceWidget::closed, &QWidget::deleteLater);

    insertReferenceWidget->show();
    insertReferenceWidget->raise();
    insertReferenceWidget->activateWindow();
    insertReferenceWidget->adjustSize();

    return insertReferenceWidget;
}

void InsertReferenceWidget::popupReferenceInserterWidget(QLineEdit* target,
                                                         QSharedPointer<Database> database)
{
    auto insertReferenceWidget =
            InsertReferenceWidget::popupInsertReferenceWidget(target, database);
    connect(insertReferenceWidget, &InsertReferenceWidget::appliedReference,
            target, [target](const QString& text) {
        target->clear();
        target->insert(text);
    });
}

void InsertReferenceWidget::injectActionIntoContextMenu(QMenu* menu, QLineEdit* target,
                                                        QSharedPointer<Database> database)
{
    auto insertReferenceAction = menu->addAction(tr("🔗 Insert reference"));
    insertReferenceAction->setToolTip(
                tr("Open a dialog to insert a reference to another entry field"));
    connect(insertReferenceAction, &QAction::triggered, target, [target, database]() {
        popupReferenceInserterWidget(target, database);
    });
}

void InsertReferenceWidget::handleContextMenuEvent(QLineEdit* target, QContextMenuEvent *event,
                                                   QSharedPointer<Database> database)
{
    QMenu *menu = target->createStandardContextMenu();
    InsertReferenceWidget::injectActionIntoContextMenu(menu, target, database);
    menu->exec(event->globalPos());
    delete menu;
}

void InsertReferenceWidget::applyChosenReference()
{
    const QString reference = getReferenceForCurrentEntry();
    emit appliedReference(reference);
    emit closed();
}

void InsertReferenceWidget::search(const QString& searchtext)
{
    qDebug() << "search " << searchtext;
    if (searchtext.isEmpty()) {
        endSearch();
        return;
    }

    Group* searchGroup = m_database->rootGroup();

    QList<Entry*> searchResult = m_entrySearcher->search(searchtext, searchGroup);
    m_ui->entryView->displaySearch(searchResult);
    m_lastSearchText = searchtext;
}

bool InsertReferenceWidget::isSearchActive() const
{
    return m_ui->entryView->inSearchMode();
}

void InsertReferenceWidget::endSearch()
{
    if (isSearchActive()) {
//        // Show the normal entry view of the current group
//        emit listModeAboutToActivate();
        Group* searchGroup = m_database->rootGroup();
        m_ui->entryView->displayGroup(searchGroup);
//        emit listModeActivated();
//        m_entryView->setFirstEntryActive();
    }

//    m_searchingLabel->setVisible(false);
//    m_searchingLabel->setText(tr("Searching..."));

    m_lastSearchText.clear();
    qDebug() << "endSearch";

    // Tell the search widget to clear
//    emit clearSearch();
}

QStringList InsertReferenceWidget::getAvailableAttributes() const
{
    const QList<QString> attributes = EntryAttributes::DefaultAttributes;
    return attributes;
}

void InsertReferenceWidget::entrySelectionChangedSignalReceived(Entry* entry)
{
    if (!entry) {
        return;
    }
    m_selectedEntry = entry;
}

QString InsertReferenceWidget::getReferenceForCurrentEntry() const
{
    Q_ASSERT(m_selectedEntry);
    const QString reference = Entry::buildReference(m_selectedEntry->uuid(),
                                                    m_selectedAttribute);
    return reference;
}

void InsertReferenceWidget::updateAttributeName(const QString &attributeName)
{
    m_selectedAttribute = attributeName;
    updateAttributeValue();
}

void InsertReferenceWidget::updateAttributeValue()
{
    if (m_selectedAttribute.isEmpty()) {
        return;
    }

    if (!m_selectedEntry) {
        m_ui->attributeValue->setText(QString());
        return;
    }
    const auto attributeValue =
            m_selectedEntry->attributes()->hasKey(m_selectedAttribute) ?
                m_selectedEntry->attributes()->value(m_selectedAttribute) : QString();
    m_ui->attributeValue->setText(attributeValue);
}

void InsertReferenceWidget::escapeReceived()
{
    qDebug() << "escapeReceived";
    if (m_lastSearchText.isEmpty()) {
        qDebug() << "escapeReceived closed";
        emit closed();
    }
}

bool InsertReferenceWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            qDebug() << "InsertReferenceWidget::escapePressed eventFilter";
            emit escapeReceived();
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}
