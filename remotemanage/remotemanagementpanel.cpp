#include "remotemanagementpanel.h"
#include "serverconfigitem.h"
#include "shortcutmanager.h"

#include <DLog>

RemoteManagementPanel::RemoteManagementPanel(QWidget *parent) : CommonPanel(parent)
{
    initUI();
}

void RemoteManagementPanel::refreshPanel()
{
    clearSearchInfo();
    m_listWidget->refreshAllDatas();
    refreshSearchState();
}

void RemoteManagementPanel::refreshSearchState()
{
    if (m_listWidget->count() >= 2) {
        m_searchEdit->show();
    } else {
        m_searchEdit->hide();
    }
}

void RemoteManagementPanel::showCurSearchResult()
{
    QString strTxt = m_searchEdit->text();
    if (strTxt.isEmpty())
        return;
    emit showSearchPanel(strTxt);
}

void RemoteManagementPanel::showAddServerConfigDlg()
{
    ServerConfigOptDlg dlg(ServerConfigOptDlg::SCT_ADD, nullptr, this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshPanel();
    }
}

void RemoteManagementPanel::initUI()
{
    this->setBackgroundRole(QPalette::Base);
    this->setAutoFillBackground(true);

    m_searchEdit = new DSearchEdit(this);
    m_listWidget = new ServerConfigList(this);
    m_pushButton = new DPushButton(this);

    m_searchEdit->setClearButtonEnabled(true);

    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerItem);
    m_listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);

    m_pushButton->setFixedHeight(36);
    m_pushButton->setText(tr("Add Server"));

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->addSpacing(10);
    hlayout->addWidget(m_searchEdit);
    hlayout->addSpacing(10);
    hlayout->setSpacing(0);
    hlayout->setMargin(0);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addSpacing(10);
    btnLayout->addWidget(m_pushButton);
    btnLayout->addSpacing(10);
    btnLayout->setSpacing(0);
    btnLayout->setMargin(0);

    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->addSpacing(10);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(m_listWidget);
    vlayout->addLayout(btnLayout);
    vlayout->addSpacing(12);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);
    setLayout(vlayout);

    connect(m_searchEdit, &DSearchEdit::returnPressed, this, &RemoteManagementPanel::showCurSearchResult);
    connect(m_pushButton, &DPushButton::clicked, this, &RemoteManagementPanel::showAddServerConfigDlg);
    connect(m_listWidget, &ServerConfigList::itemClicked, this, &RemoteManagementPanel::listItemClicked);
    connect(m_listWidget, &ServerConfigList::listItemCountChange, this, &RemoteManagementPanel::refreshSearchState);
}

void RemoteManagementPanel::listItemClicked(ServerConfig *curItemServer)
{
    if (nullptr != curItemServer) {
        QString group = curItemServer->m_group;
        if (!group.isNull() && !group.isEmpty()) {
            emit showServerConfigGroupPanel(group);
        } else {
            emit doConnectServer(curItemServer);
        }
    }
}