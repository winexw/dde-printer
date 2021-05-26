/*
 * Copyright (C) 2019 ~ 2020 Uniontech Software Co., Ltd.
 *
 * Author:     liurui <liurui_cm@deepin.com>
 *
 * Maintainer: liurui <liurui_cm@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "installprinterwindow.h"
#include "zjobmanager.h"
#include "addprinter.h"
#include "printersearchwindow.h"
#include "installdriverwindow.h"
#include "troubleshootdialog.h"
#include "ztroubleshoot.h"
#include "printerservice.h"
#include "zdrivermanager.h"

#include <DTitlebar>
#include <DSpinner>
#include <DIconButton>
#include <DWidgetUtil>
#include <DFontSizeManager>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMapIterator>

InstallPrinterWindow::InstallPrinterWindow(QWidget *parent)
    : DMainWindow(parent)
    , m_pParentWidget(parent)
    , m_status(Unknown)
    , m_bInstallFail(false)
    , m_testJob(nullptr)

{
    initUI();
    initConnections();
    setStatus(Installing);
    m_pAddPrinterTask = nullptr;
}

InstallPrinterWindow::~InstallPrinterWindow()
{
    if (m_pAddPrinterTask) {
        delete m_pAddPrinterTask;
        m_pAddPrinterTask = nullptr;
    }
}

void InstallPrinterWindow::setTask(AddPrinterTask *task)
{
    m_pAddPrinterTask = task;
}

void InstallPrinterWindow::initUI()
{
    titlebar()->setMenuVisible(false);
    titlebar()->setTitle("");
    titlebar()->setIcon(QIcon(":/images/dde-printer.svg"));
    // 去掉最大最小按钮
    setWindowFlags(Qt::Dialog);
    setWindowModality(Qt::ApplicationModal);
    setAttribute(Qt::WA_DeleteOnClose);
    setFixedSize(682, 532);

    m_pSpinner = new DSpinner();
    m_pSpinner->setFixedSize(32, 32);
    m_pStatusImageLabel = new QLabel();
    m_pStatusImageLabel->setFixedSize(128, 128);
    m_pStatusImageLabel->setAlignment(Qt::AlignCenter);
    m_pStatusLabel = new QLabel(tr("Installing driver..."));
    DFontSizeManager::instance()->bind(m_pStatusLabel, DFontSizeManager::T5, QFont::DemiBold);
    m_pStatusLabel->setAlignment(Qt::AlignCenter);
    m_pTipLabel = new QLabel("");
    m_pTipLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_pTipLabel->setWordWrap(true);
    m_pTipLabel->setMinimumHeight(120);

    m_pDriverCombo = new QComboBox();
    m_pDriverCombo->setMinimumSize(300, 36);
    m_pCancelInstallBtn = new QPushButton(tr("Cancel", "button"));
    m_pCancelInstallBtn->setFixedSize(200, 36);

    m_pCheckPrinterListBtn = new QPushButton(tr("View Printer", "button"));
    m_pCheckPrinterListBtn->setFixedSize(170, 36);
    m_pPrinterTestPageBtn = new QPushButton(tr("Print Test Page", "button"));
    m_pPrinterTestPageBtn->setFixedSize(170, 36);
    QHBoxLayout *pHLayout = new QHBoxLayout();
    pHLayout->setSpacing(20);
    pHLayout->addStretch();
    pHLayout->addWidget(m_pCheckPrinterListBtn);
    pHLayout->addWidget(m_pPrinterTestPageBtn);
    pHLayout->addStretch();

    QVBoxLayout *pMainLayout = new QVBoxLayout();
    pMainLayout->setSpacing(0);
    QSpacerItem *pSpaceItem = new QSpacerItem(300, 52, QSizePolicy::Preferred, QSizePolicy::Preferred);
    pMainLayout->addItem(pSpaceItem);
    pMainLayout->addWidget(m_pSpinner, 0, Qt::AlignCenter);
    pMainLayout->addWidget(m_pStatusImageLabel, 0, Qt::AlignCenter);
    pMainLayout->addSpacing(30);
    pMainLayout->addWidget(m_pStatusLabel, 0, Qt::AlignCenter);
    pMainLayout->addSpacing(13);
    pMainLayout->addWidget(m_pTipLabel, 0, Qt::AlignCenter);
    pMainLayout->addWidget(m_pDriverCombo, 0, Qt::AlignCenter);
    QSpacerItem *pSpaceItem1 = new QSpacerItem(300, 146, QSizePolicy::Minimum, QSizePolicy::Preferred);
    pMainLayout->addItem(pSpaceItem1);
    pMainLayout->addWidget(m_pCancelInstallBtn, 0, Qt::AlignCenter);
    pMainLayout->addLayout(pHLayout);
    pMainLayout->setContentsMargins(0, 66, 0, 20);
    QWidget *widget = new QWidget;
    widget->setLayout(pMainLayout);
    takeCentralWidget();
    setCentralWidget(widget);

    moveToCenter(this);
}

void InstallPrinterWindow::initConnections()
{
    connect(m_pCancelInstallBtn, &QPushButton::clicked, this, &InstallPrinterWindow::cancelBtnClickedSlot);
    connect(m_pCheckPrinterListBtn, &QPushButton::clicked, this, &InstallPrinterWindow::leftBtnClickedSlot);
    connect(m_pPrinterTestPageBtn, &QPushButton::clicked, this, &InstallPrinterWindow::rightBtnClickedSlot);
}

void InstallPrinterWindow::setStatus(InstallationStatus status)
{
    if (m_status != status) {
        m_status = status;
        if (m_status == Installing) {
            m_pSpinner->setVisible(true);
            m_pSpinner->start();
            m_pStatusLabel->setVisible(true);
            m_pStatusLabel->setText(tr("Installing driver..."));
            m_pCancelInstallBtn->setVisible(true);

            m_pStatusImageLabel->setVisible(false);
            m_pTipLabel->setVisible(false);
            m_pDriverCombo->setVisible(false);
            m_pCheckPrinterListBtn->setVisible(false);
            m_pPrinterTestPageBtn->setVisible(false);
        } else if (m_status == Installed) {
            m_pSpinner->setVisible(false);
            m_pSpinner->stop();
            m_pStatusLabel->setVisible(true);
            m_pStatusLabel->setText(tr("Successfully installed ") + m_printerName);
            m_pCancelInstallBtn->setVisible(false);

            m_pStatusImageLabel->setVisible(true);
            m_pStatusImageLabel->setPixmap(QPixmap(":/images/success.svg"));
            m_pTipLabel->setVisible(true);
            m_pTipLabel->setText(tr("You have successfully added the printer."
                                    " Print a test page to check if it works properly."));

            m_pDriverCombo->setVisible(false);
            m_pCheckPrinterListBtn->setVisible(true);
            m_pCheckPrinterListBtn->setText(tr("View Printer", "button"));
            m_pPrinterTestPageBtn->setVisible(true);
            m_pPrinterTestPageBtn->setText(tr("Print Test Page", "button"));
        } else if (m_status == Printing) {
            m_pSpinner->setVisible(false);
            m_pSpinner->stop();
            m_pStatusLabel->setVisible(true);
            m_pStatusLabel->setText(tr("Printing test page..."));
            m_pCancelInstallBtn->setVisible(false);

            m_pStatusImageLabel->setVisible(true);
            m_pStatusImageLabel->setPixmap(QPixmap(":/images/success.svg"));
            m_pTipLabel->setVisible(true);
            m_pTipLabel->setText(tr("You have successfully added the printer."
                                    " Print a test page to check if it works properly."));
            m_pDriverCombo->setVisible(false);
            m_pCheckPrinterListBtn->setVisible(true);
            m_pCheckPrinterListBtn->setText(tr("View Printer", "button"));
            m_pPrinterTestPageBtn->setVisible(true);
            m_pPrinterTestPageBtn->setText(tr("Print Test Page", "button"));
        } else if (m_status == Printed) {
            m_pSpinner->setVisible(false);
            m_pSpinner->stop();
            m_pStatusLabel->setVisible(true);
            m_pStatusLabel->setText(tr("Did you print the test page successfully?"));
            m_pCancelInstallBtn->setVisible(false);

            m_pStatusImageLabel->setVisible(true);
            m_pStatusImageLabel->setPixmap(QPixmap(":/images/success.svg"));
            m_pTipLabel->setVisible(true);
            m_pTipLabel->setText(tr("If succeeded, click Yes; if failed, click No"));
            m_pDriverCombo->setVisible(false);
            m_pCheckPrinterListBtn->setVisible(true);
            m_pCheckPrinterListBtn->setText(tr("No"));
            m_pPrinterTestPageBtn->setVisible(true);
            m_pPrinterTestPageBtn->setText(tr("Yes"));
        } else if (m_status == PrintFailed) {
            m_pSpinner->setVisible(false);
            m_pSpinner->stop();
            m_pStatusLabel->setVisible(true);
            m_pStatusLabel->setText(tr("Print failed"));
            m_pCancelInstallBtn->setVisible(false);

            m_pStatusImageLabel->setVisible(true);
            m_pStatusImageLabel->setPixmap(QPixmap(":/images/fail.svg"));
            m_pTipLabel->setVisible(true);
            m_pTipLabel->setText(tr("Click Reinstall to install the printer driver again, or click Troubleshoot to start troubleshooting."));
            m_pDriverCombo->setVisible(false);
            m_pCheckPrinterListBtn->setVisible(true);
            m_pCheckPrinterListBtn->setText(tr("Reinstall", "button"));
            m_pPrinterTestPageBtn->setVisible(true);
            m_pPrinterTestPageBtn->setText(tr("Troubleshoot", "button"));
        } else if (m_status == Reinstall) {
            m_pSpinner->setVisible(false);
            m_pSpinner->stop();
            m_pStatusLabel->setVisible(true);

            if (m_bInstallFail) {
                m_pStatusLabel->setText(tr("Installation failed"));
            } else {
                m_pStatusLabel->setText(tr("Print failed"));
            }

            m_pCancelInstallBtn->setVisible(false);
            m_pStatusImageLabel->setVisible(true);
            m_pStatusImageLabel->setPixmap(QPixmap(":/images/fail.svg"));
            m_pTipLabel->setVisible(true);
            if (m_pAddPrinterTask)
                m_pTipLabel->setText(m_pAddPrinterTask->getErrorMassge());
            m_pCheckPrinterListBtn->setVisible(true);
            m_pCheckPrinterListBtn->setText(tr("Reinstall", "button"));
            m_pPrinterTestPageBtn->setVisible(true);
            m_pPrinterTestPageBtn->setText(tr("Troubleshoot", "button"));
        }
    }
}

void InstallPrinterWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    if (!m_printerName.isEmpty())
        emit updatePrinterList(m_printerName);
}

void InstallPrinterWindow::setDefaultPrinterName(const QString &name)
{
    m_printerName = name;
}

void InstallPrinterWindow::setDevice(const TDeviceInfo &device)
{
    m_device = device;
}

void InstallPrinterWindow::copyDriverData(QComboBox *source)
{
    if (source) {
        int count = source->count();
        if (count <= 1)
            return;
        for (int i = 0; i < count - 1; i++) {
            m_pDriverCombo->addItem(source->itemText(i), source->itemData(i));
        }
    }
}

void InstallPrinterWindow::copyDriverData(const QMap<QString, QVariant> &itemDataMap)
{
    QMapIterator<QString, QVariant> it(itemDataMap);
    while (it.hasNext()) {
        it.next();
        m_pDriverCombo->addItem(it.key(), it.value());
    }
}

void InstallPrinterWindow::cancelBtnClickedSlot()
{
    if (m_pAddPrinterTask) {
        m_pAddPrinterTask->stop();
        close();
    }
}

void InstallPrinterWindow::feedbackPrintTestPage(bool success)
{
    if (m_pAddPrinterTask) {
        QMap<QString, QVariant> driver = m_pAddPrinterTask->getDriverInfo();

        if (driver[SD_KEY_from].toInt() == PPDFrom_Server) {
            int sid = driver[SD_KEY_sid].toInt();
            QString strReason = m_testJob ? m_testJob->getMessage() : "User feedback";
            QString strFeedback = QString("Uri: %1, device: %2").arg(m_device.uriList.join(" ")).arg(m_device.strDeviceId.isEmpty() ? m_device.strMakeAndModel : m_device.strDeviceId);
            PrinterServerInterface *server = g_printerServer->feedbackResult(sid, success, strReason, strFeedback);
            if (server) {
                server->postToServer();
                server->deleteLater();
            }
        }
    }
}

void InstallPrinterWindow::leftBtnClickedSlot()
{
    if (m_status == Installed) {
        close();
    } else if (m_status == Printed) {
        feedbackPrintTestPage(false);
        setStatus(PrintFailed);
    } else if (m_status == PrintFailed) {
        //由于以前的重新安装驱动界面没有完成，导致在打印测试页失败后点击重新安装会多出一个无用的setStatus(Reinstall)生成的界面
        //现在将这里修改成跳过setStatus(Reinstall)的界面，直接回到上一级界面
        if (m_pParentWidget) {
            emit showParentWindows();
        }
        close();
    } else if (m_status == Reinstall) {
        if (m_pParentWidget) {
            emit showParentWindows();
        }
        close();
    }
}

void InstallPrinterWindow::rightBtnClickedSlot()
{
    if (m_status == Installed) {
        // 打印测试页
        if (m_testJob) {
            m_testJob->stop();
            m_testJob->deleteLater();
        }
        m_testJob = new PrinterTestJob(m_printerName, this, false);
        m_testJob->isPass();
        setStatus(Printed);
    } else if (m_status == Printed) {
        feedbackPrintTestPage(true);
        close();
    } else if (m_status == PrintFailed) {
        TroubleShootDialog dlg(m_printerName, this);
        dlg.setModal(true);
        dlg.exec();
    } else if (m_status == Reinstall) {

        TroubleShootDialog dlg(m_printerName, this);
        dlg.setModal(true);
        dlg.exec();
    }
}

void InstallPrinterWindow::receiveInstallationStatusSlot(int status)
{
    AddPrinterTask *task = static_cast<AddPrinterTask *>(sender());

    setDefaultPrinterName(task->getPrinterInfo().strName);
    if (status == TStat_Suc) {
        m_bInstallFail = false;
        setStatus(Installed);
    } else {
        m_bInstallFail = true;
        setStatus(Reinstall);
        m_printerName.clear();
    }
}
