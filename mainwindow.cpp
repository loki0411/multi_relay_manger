#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QModelIndex>
#include "QCheckBoxDelegate.h"
#include "QOnOffPushButton.h"
#include "CDeviceDelegate.h"
#include "QIpAddrDelegate.h"
#include "QDeviceMainGroupDelegate.h"
#include "QDeviceStatusDelegate.h"
#include "editparamdialog.h"
#include "qeditipconfigdialog.h"
#include "qpasswordmangerdialog.h"
#include "qioexpendsettingdialog.h"
#include "view_items_select.h"
 #include <QCoreApplication>
#include "AboutDialog.h"
#include <QThread>
#include <QFile>
#include "dialogcountdownoutput.h"
#include "tryversiondialog.h"


#include "debug.h"

#define THISINFO            0
#define THISERROR           0
#define THISASSERT          0





QDeviceControlWidget::QDeviceControlWidget(QWidget * parent)
    : QTableWidget(parent)
{
    this->setIoExternSetAction = new QAction(tr("&IoExpendSetting..."), this);
    connect(setIoExternSetAction, SIGNAL(triggered()), this, SLOT(IoSettingDialog()));
    CountdownMangerAct = new QAction(tr("Count Down Manger..."), this);
    connect(CountdownMangerAct, SIGNAL(triggered()), this, SLOT(IoOutCountDownSettingAct()));
}

void QDeviceControlWidget::contextMenuEvent(QContextMenuEvent *event)
{
    const QPoint & pos = event->pos();
    QTableWidgetItem * item = this->itemAt(pos);
    if(item) {
        QVariant var = item->data(0);
       RelayDeviceSharePonterType pdev = qVariantValue<RelayDeviceSharePonterType>(var);
        if(pdev) {
            //debuginfo(("item is exist: item type:(%d,%d)%s",item->row(),item->column(),pdev->GetDeviceName().toAscii().data()));
            QMenu menu(this);
            setIoExternSetAction->setData(var);
            menu.addAction(setIoExternSetAction);
#if 0 //这里是倒计时的快捷面板
            CountdownMangerAct->setData(var);
            menu.addAction(CountdownMangerAct);
#endif //倒计时面板
            menu.exec(event->globalPos());
        } else {
            debuginfo(("pdev is not valid!"));
        }
    } else {
        debuginfo(("item is not exist: item type"));
    }
}

void  QDeviceControlWidget::IoSettingDialog(void)
{
    QAction * act = (QAction *)sender();
    QSharedPointer<QRelayDeviceControl> pdev = qVariantValue<RelayDeviceSharePonterType>(act->data());
    //debuginfo((" this io devcie is = %s",pdev->GetDeviceName().toAscii().data()));
    QIoExpendSettingDialog dlg(pdev);
    dlg.setWindowTitle(pdev->GetDeviceName());
    dlg.exec();
}
void  QDeviceControlWidget::IoOutCountDownSettingAct(void)
{
    QAction * act = (QAction *)sender();
    QSharedPointer<QRelayDeviceControl> pdev = qVariantValue<RelayDeviceSharePonterType>(act->data());
    //debuginfo((" this io devcie is = %s",pdev->GetDeviceName().toAscii().data()));

    //先读定时器寄存器，延时1S
   // pdev->TcpStartReadTimimgs();

    if(pdev->io_timing_initialized) {
        DialogCountdownOutput dlg(pdev);
        dlg.setWindowTitle(pdev->GetDeviceName());
        dlg.exec();
    }
}

void  QDeviceControlWidget::RemoveOneItemActioN(void)
{
}



QDelay::QDelay()
{
}
void QDelay::run()
{
}

void QDelay::delay(unsigned int ms)
{
    this->sleep(ms);
}































MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    CreateDevcieTable();
    CreateAction();
    CreateMenu();
    createContextMenu();

    QGridLayout *mainLayout = new QGridLayout;
        mainLayout->addWidget(deviceGroupBox, 0, 0, 1, 2);
        centralWidget->setLayout(mainLayout);
        this->resize(1000,300);
        setWindowTitle(tr("Remote multiple device manger"));

        statusBar()->showMessage(tr("Ready"));

        //初始化UDP接口
        InitUdpSocket();

        resize(app_info.app_with,app_info.app_height);
#if 0
        try_version_count = 10;
        try_version_count_times = 125;
        try_version_timer.start(1000);
        connect(&try_version_timer,SIGNAL(timeout()),this,SLOT(try_timer_slot()));
#endif


}

void MainWindow::try_timer_slot(void)
{
    if(try_version_count) {
        --try_version_count;
        if(try_version_count == 0) {
            --try_version_count_times;
            if(try_version_count_times) {
                TryVersionDialog dlg;
                dlg.setWindowTitle(tr("Warning"));
                dlg.setTryVersionString(tr("This is beta version software, \nand for genuine version please contact:"),tr("ShenZhen Jingruida Network Technology Co., Ltd."));
                dlg.exec();
                try_version_count = 60;
            } else {
                TryVersionDialog dlg;
                dlg.setWindowTitle(tr("Warning"));
                dlg.setTryVersionString(tr("This is beta version software, \nthe system will quit."),tr("ShenZhen Jingruida Network Technology Co., Ltd."));
                dlg.exec();
                qApp->quit();
            }
        }
    }
}

MainWindow::~MainWindow()
{
    app_info.app_with = width();
    app_info.app_height = height();
    app_info.row_height = deviceTable->rowHeight(0);

    for(int i=0;i<deviceTable->columnCount();i++) {
        app_info.colomn_withs[i] = deviceTable->columnWidth(i);
    }

    delete ui;
}

void MainWindow::sleep(unsigned int ms)
{
    QDelay d;
    d.delay(ms);
}

void MainWindow::DevcieAckStstus(QString ackstr)
{
    statusBar()->showMessage(tr("Status:")+ackstr);
}

void MainWindow::CreateAction(void)
{
    this->quitact = new QAction(tr("&Quit"), this);
    connect(quitact, SIGNAL(triggered()), this, SLOT(Quit()));

    this->view_select_item_act = new QAction(tr("ItemViewSetting..."), this);
    connect(view_select_item_act, SIGNAL(triggered()), this, SLOT(ViewSelectItemAct()));

    this->edit_device_param_act = new QAction(tr("&Edit device parameter..."), this);
    connect(edit_device_param_act, SIGNAL(triggered()), this, SLOT(EditDeviceParam()));

    this->cleardevicetable = new QAction(tr("&Clear all device"), this);
    connect(cleardevicetable, SIGNAL(triggered()), this, SLOT(ClearDeviceTable()));

    this->edit_device_ipconfig = new QAction(tr("&Change deivce ip config..."), this);
    connect(edit_device_ipconfig, SIGNAL(triggered()), this, SLOT(EditDeviceIpconfig()));

    password_manger = new QAction(tr("&Communication password..."),this);
    connect(password_manger,SIGNAL(triggered()),this,SLOT(PasswordConfig()));

    secect_all = new QAction(QIcon(":/sys/sys_icon/sources/selall.png"),tr("&Selected all device"),this);
    connect(secect_all,SIGNAL(triggered()),this,SLOT(SelectAll()));
    desecect_all = new QAction(QIcon(":/sys/sys_icon/sources/deselall.png"),tr("&Deselected all device"),this);
    connect(desecect_all,SIGNAL(triggered()),this,SLOT(DesecectAll()));

    open_all_device = new QAction(QIcon(":/sys/sys_icon/sources/sON.png"),tr("&Set all selected device output ON"),this);
    connect(open_all_device,SIGNAL(triggered()),this,SLOT(OpenAllListDeviceIoOutput()));
    close_all_device = new QAction(QIcon(":/sys/sys_icon/sources/sOFF.png"),tr("&Set all selected device output OFF"),this);
    connect(close_all_device,SIGNAL(triggered()),this,SLOT(CloseAllListDeviceIoOutput()));

    selectChineseLanguage = new QAction(tr("Chinese"),this);
    connect(selectChineseLanguage,SIGNAL(triggered()),this,SLOT(SelectChineseLanguageAction()));

    about_act = new QAction(tr("About"),this);
    connect(about_act,SIGNAL(triggered()),this,SLOT(AboutAction()));


    //系统后台图标
    minimizeAction = new QAction(tr("Mi&nimize"), this);
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

    maximizeAction = new QAction(tr("Ma&ximize"), this);
    connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));


    sysicon.addPixmap(QPixmap(":/sys/sys_icon/sources/LOGO.png"));
    trayIconMenu = new QMenu(this);
    //trayIconMenu->addAction(minimizeAction);
    //trayIconMenu->addAction(maximizeAction);

    //trayIconMenu->addSeparator();
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addAction(quitact);

    trayIcon = new QSystemTrayIcon(sysicon,this);
    trayIcon->setContextMenu(trayIconMenu);

    setWindowIcon(sysicon);
    trayIcon->setIcon(sysicon);
    trayIcon->setVisible(true);
    trayIcon->setToolTip(this->windowTitle());
    trayIcon->show();

}


void MainWindow::ViewSelectItemAct(void)
{
    debuginfo(("ViewSelectItemAct"));
    view_items_select dlg;
    dlg.exec();
}


void MainWindow::showMinimized(void)
{
    debuginfo(("shwo minized..."));
}

void MainWindow::changeEvent(QEvent * event )
{
     if(windowState() & Qt::WindowMinimized)
     {
         debuginfo(("change event."));
         //setWindowState(windowState() & ~Qt::WindowMinimized);
         //hide();
      //此处无论event->accept()或者event->ignore()都不好使。
     }
}

void MainWindow::showEvent ( QShowEvent * event )
{
    setVisible(true);
}

void MainWindow::hideEvent ( QHideEvent * event )
{
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible()) {
        QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information;
        trayIcon->showMessage(tr("Note!"),tr("The program will keep running."), icon, 0);
        hide();
        event->ignore();
    }
}

void MainWindow::setVisible(bool visible)
{
    minimizeAction->setEnabled(visible);
    maximizeAction->setEnabled(!isMaximized());
    restoreAction->setEnabled(isMaximized() || !visible);
    QMainWindow::setVisible(visible);
}

void MainWindow::CreateMenu(void)
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(quitact);
    //fileMenu = menuBar()->addMenu(tr("View"));
    //fileMenu->addAction(this->view_select_item_act);
    toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(edit_device_param_act);
    toolsMenu->addAction(edit_device_ipconfig);
    toolsMenu->addAction(password_manger);
    toolsMenu->addSeparator();
    toolsMenu->addAction(cleardevicetable);
#if 0
    toolsMenu->addSeparator();
    toolsMenu->addAction(selectChineseLanguage);
#endif
    toolsMenu = menuBar()->addMenu(tr("&Operation"));

    toolsMenu->addAction(secect_all);

    toolsMenu->addAction(desecect_all);
    toolsMenu->addAction(open_all_device);
    toolsMenu->addAction(close_all_device);
    toolsMenu = menuBar()->addMenu(tr("Help"));
    toolsMenu->addAction(about_act);

#if 0
    //ToolBar
    fileToolBar = addToolBar(tr("Edit"));
    fileToolBar->addAction(secect_all);
    fileToolBar->addAction(desecect_all);
    fileToolBar->addAction(open_all_device);
    fileToolBar->addAction(close_all_device);
#endif

#if     1 //for debug
    password_item itm;
#if 0 //2233
    itm.alias = "2233";
    itm.pwd = "2233";
    password_list.push_back(itm);
#endif //2233
    itm.alias = "admin";
    itm.pwd = "admin";
    password_list.push_back(itm);
#endif
     //QList<password_item>   password_list;
}

void MainWindow::createContextMenu(void)
{
    deviceTable->setContextMenuPolicy(Qt::DefaultContextMenu); //Qt::ActionsContextMenu);
    deviceTable->addAction(password_manger);
    deviceTable->addAction(cleardevicetable);
}

void MainWindow::Quit(void)
{
    trayIcon->setVisible(false);
    close();
}

void MainWindow::EditDeviceParam(void)
{    
    EditParamDialog dialog;

    this->update();

    int row = deviceTable->rowCount();
    for(int i=0;i<row;i++) {
        QTableWidgetItem * item = deviceTable->item(i,0);
        QVariant var = item->data(0);
        QSharedPointer<QRelayDeviceControl> pdev = qVariantValue<RelayDeviceSharePonterType>(var);
        if(pdev->is_checked) {
            dialog.InsertDevice(pdev);
        }
    }
    dialog.exec();
}
void MainWindow::EditDeviceIpconfig(void)
{
    QEditIpConfigDialog dialog;
    this->update();

    int row = deviceTable->rowCount();
    for(int i=0;i<row;i++) {
        QTableWidgetItem * item = deviceTable->item(i,0);
        QVariant var = item->data(0);
        QSharedPointer<QRelayDeviceControl> pdev = qVariantValue<RelayDeviceSharePonterType>(var);
        if(pdev->is_checked) {
            dialog.InsertDevice(pdev);
        }
    }
    dialog.exec();
}

void MainWindow::PasswordConfig(void)
{
    QPasswordMangerDialog dialog;
    this->update();
    dialog.InitPasswordList(password_list);
    dialog.exec();
    if(dialog.is_ok) {
        password_list = dialog.GetPasswordList();
        debuginfo(("is ok,get password count:%d",password_list.count()));
    } else {
        debuginfo(("is cancel"));
    }
}

void MainWindow::ClearDeviceTable(void)
{
    int rowcount = deviceTable->rowCount();
    for(int i=0;i<rowcount;i++) {
        deviceTable->removeRow(0);
    }
    mydevicemap.clear();
    mydevicemap.empty();
}
void MainWindow::SelectAll(void)
{
    int rowcount = deviceTable->rowCount();
    for(int i=0;i<rowcount;i++) {
        QTableWidgetItem * item = deviceTable->item(i,0);
        QVariant var = item->data(0);
        QSharedPointer<QRelayDeviceControl> pdev = qVariantValue<RelayDeviceSharePonterType>(var);
        pdev->is_checked = true;
        pdev->Updata();
    }
}
void MainWindow::DesecectAll(void)
{
    int rowcount = deviceTable->rowCount();
    for(int i=0;i<rowcount;i++) {
        QTableWidgetItem * item = deviceTable->item(i,0);
        QVariant var = item->data(0);
        QSharedPointer<QRelayDeviceControl> pdev = qVariantValue<RelayDeviceSharePonterType>(var);
        pdev->is_checked = false;
        pdev->Updata();
    }
}
void MainWindow::OpenAllListDeviceIoOutput(void)
{
    int rowcount = deviceTable->rowCount();
    for(int i=0;i<rowcount;i++) {
        QTableWidgetItem * item = deviceTable->item(i,0);
        QVariant var = item->data(0);
        QSharedPointer<QRelayDeviceControl> pdev = qVariantValue<RelayDeviceSharePonterType>(var);
        if(pdev->is_checked) {
            QBitArray bitmsk;
            bitmsk.resize(pdev->GetIoOutNum());
            bitmsk.fill(true,pdev->GetIoOutNum());
            pdev->MultiIoOutSet(0,bitmsk);
        }
    }
}

void MainWindow::CloseAllListDeviceIoOutput(void)
{
    int rowcount = deviceTable->rowCount();
    for(int i=0;i<rowcount;i++) {
        QTableWidgetItem * item = deviceTable->item(i,0);
        QVariant var = item->data(0);
        QSharedPointer<QRelayDeviceControl> pdev = qVariantValue<RelayDeviceSharePonterType>(var);
        if(pdev->is_checked) {
            QBitArray bitmsk;
            bitmsk.resize(pdev->GetIoOutNum());
            bitmsk.fill(false,pdev->GetIoOutNum());
            pdev->MultiIoOutSet(0,bitmsk);
        }
    }
}

void MainWindow::CreateDevcieTable(void)
{
        int index = 0;
        deviceGroupBox = new QGroupBox(tr("DeviceTable"));
        deviceTable = new QDeviceControlWidget;

        deviceTable->verticalHeader()->resizeSections(QHeaderView::Interactive);
        deviceTable->verticalHeader()->setDefaultSectionSize(app_info.row_height);

        deviceTable->setSelectionMode(QAbstractItemView::NoSelection);
        deviceTable->setItemDelegateForColumn(index++,new QCheckBoxDelegate(this));
        deviceTable->setItemDelegateForColumn(index++,new QDeviceNameDelegate(this));
        deviceTable->setItemDelegateForColumn(index++,new QIpAddressDelegate(this));
        deviceTable->setItemDelegateForColumn(index++,new QDeviceMainGroupDelegate(this));
        deviceTable->setItemDelegateForColumn(index++,new QDeviceSlaveGroupDelegate(this));
        deviceTable->setItemDelegateForColumn(index++,new QRelayValueSingalChannalButtonDelegate(this));
        deviceTable->setItemDelegateForColumn(index++,new QSetOnPushDelegate(this));
        deviceTable->setItemDelegateForColumn(index++,new QSetOffPushDelegate(this));
        deviceTable->setItemDelegateForColumn(index++,new QDeviceStatusDelegate(this));
#if APP_DISPLAY_TIME
        deviceTable->setItemDelegateForColumn(index++,new QDeviceTimeDelegate(this));
#endif

        QStringList labels;
    //! [22] //! [23]
        labels << tr("S") << tr("DeviceName") << tr("IpAddr")<<tr("Group1") << tr("Group2")<<tr("Control")<<tr("AllOn")<<tr("AllOff")<<tr("Status");
#if APP_DISPLAY_TIME
        labels << tr("DeviceTime");
#endif

        deviceTable->horizontalHeader()->setDefaultSectionSize(100);
        deviceTable->setColumnCount(labels.size());
        deviceTable->setHorizontalHeaderLabels(labels);
        //deviceTable->horizontalHeader()->setResizeMode(0, QHeaderView::Fixed);
        index = 0;
        deviceTable->horizontalHeader()->resizeSection(index,app_info.colomn_withs[index]);
        index++;
        //
        deviceTable->horizontalHeader()->resizeSection(index,app_info.colomn_withs[index]);
        index++;
        deviceTable->horizontalHeader()->resizeSection(index++,100);
        deviceTable->horizontalHeader()->resizeSection(index++,80);

        deviceTable->horizontalHeader()->resizeSection(index++,80);

        //deviceTable->horizontalHeader()->setResizeMode(index, QHeaderView::Fixed);
        //deviceTable->horizontalHeader()->resizeSection(index++,300);
        deviceTable->horizontalHeader()->resizeSection(index++,app_info.colomn_withs[5]);

        deviceTable->horizontalHeader()->resizeSection(index++,60);
        deviceTable->horizontalHeader()->resizeSection(index++,60);
        deviceTable->horizontalHeader()->resizeSection(index++,50);
#if APP_DISPLAY_TIME
        deviceTable->horizontalHeader()->resizeSection(index++,120);
#endif
        //QHeaderView::InteractivedeviceTable->verticalHeader()->hide();


/* 一下可以隐藏某些功能  */
        //deviceTable->hideColumn(2);
        //deviceTable->hideColumn(3);
        //deviceTable->hideColumn(4);
        //deviceTable->hideColumn(6);
        ///deviceTable->hideColumn(7);
        //deviceTable->hideColumn(index-1);
/*  以上是影藏某些按钮的  */
        //排版
        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(deviceTable);
        deviceGroupBox->setLayout(layout);
}

void MainWindow::manualAddDevice(int index)
{
}
 void MainWindow::InsertDevice(QSharedPointer<QRelayDeviceControl> & pdev)
 {
     int row = deviceTable->rowCount();
     deviceTable->setRowCount(row + 1);
     QTableWidgetItem *item0 = new CDeviceTableWidgetItem(pdev);
     QTableWidgetItem *item1 = new CDeviceTableWidgetItem(pdev);
     QTableWidgetItem *item2 = new CDeviceTableWidgetItem(pdev);
     QTableWidgetItem *item3 = new CDeviceTableWidgetItem(pdev);
     QTableWidgetItem *item4 = new CDeviceTableWidgetItem(pdev);
     QTableWidgetItem *item5 = new CDeviceTableWidgetItem(pdev);
     QTableWidgetItem *item6 = new CDeviceTableWidgetItem(pdev);
     QTableWidgetItem *item7 = new CDeviceTableWidgetItem(pdev);
     QTableWidgetItem *item8 = new CDeviceTableWidgetItem(pdev);
#if APP_DISPLAY_TIME
     QTableWidgetItem *item9 = new CDeviceTableWidgetItem(pdev);
#endif
     //
     int index = 0;
     deviceTable->setItem(row, index++, item0);
     deviceTable->setItem(row, index++, item1);
     deviceTable->setItem(row, index++, item2);
     deviceTable->setItem(row, index++, item3);
     deviceTable->setItem(row, index++, item4);
     deviceTable->setItem(row, index++, item5);
     deviceTable->setItem(row, index++, item6);
     deviceTable->setItem(row, index++, item7);
     deviceTable->setItem(row, index++, item8);
#if APP_DISPLAY_TIME
     deviceTable->setItem(row, index++, item9);
#endif

    // item0->setCheckState(Qt::Checked);
    // item2->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);//表格只读属性

     deviceTable->openPersistentEditor(item0);  //不用双击就可以显示控件的形式
     deviceTable->openPersistentEditor(item2);  //不用双击就可以显示控件的形式
     deviceTable->openPersistentEditor(item5);  //不用双击就可以显示控件的形式
     deviceTable->openPersistentEditor(item6);  //不用双击就可以显示控件的形式
     deviceTable->openPersistentEditor(item7);
     deviceTable->openPersistentEditor(item8);
#if APP_DISPLAY_TIME
     //deviceTable->openPersistentEditor(item9);
#endif
     //
     //添加信号
     //updateGeometries
     //modelReset
     connect(pdev.data(),SIGNAL(DeviceInfoChanged(QString)),this,SLOT(DeviceInfoChanged(QString))); //,Qt::QueuedConnection);
     connect(pdev.data(),SIGNAL(DeviceAckStatus(QString)),this,SLOT(DevcieAckStstus(QString)));

     //deviceTable->setRowHeight(row,80);

 }

void MainWindow::InitUdpSocket(void)
{
    QSharedPointer<QUdpSocket>  pudp(new QUdpSocket);
    pUdpSocket = pudp;
    pUdpSocket->bind(505);
    connect(pUdpSocket.data(), SIGNAL(readyRead()),
                 this, SLOT(UdpreadPendingDatagrams()));
}

void MainWindow::UdpreadPendingDatagrams()
{
    while (pUdpSocket->hasPendingDatagrams()) {
             QByteArray datagram;
             datagram.resize(pUdpSocket->pendingDatagramSize());
             QHostAddress sender;
             quint16 senderPort;

             pUdpSocket->readDatagram(datagram.data(), 1024,&sender, &senderPort);

             processTheDeviceData(datagram,sender,senderPort);
         }
}

void MainWindow::processTheDeviceData(QByteArray & data,
                                      QHostAddress & sender,quint16 senderport)
{
    //解析报文，分发命令
    QString portstr;
    portstr.sprintf("%d",senderport);
    QString str = sender.toString();
    str += ":";
    str += portstr;
    //debuginfo(("precess udp rx data len(%d)",data.size()));

    //根据IP地址和端口号，查找已经有的数据，如果有了，给它发消息，让他自己处理自己的事情
    QMap<QString,QSharedPointer<QRelayDeviceControl> >::const_iterator i = mydevicemap.find(str);
    if(i != mydevicemap.end()) {
        //debuginfo(("found device,send rx data"));
        (*i)->SendRxData(data,this->password_list);
        //DeviceInfoChanged(str);
    } else {
        //debuginfo(("Not found device, try to insert one device"));
        QSharedPointer<QRelayDeviceControl> pdev(new QRelayDeviceControl(this));
        pdev->InitDeviceAddress(sender,senderport,pUdpSocket);
        pdev->SendRxData(data,this->password_list);
        if(pdev->devcie_info_is_useful()) {
            mydevicemap.insert(str,pdev);
            InsertDevice(pdev);
            pdev->index = mydevicemap.size();
        }
    }
    //如果没有找到，则创建一个，然后再转发消息给他处理
    //设备对象是一个完整的对象，可以处理和拥有自己数据
}

void MainWindow::DeviceInfoChanged(QString  hostaddrID)
{
   // debuginfo(("host:%s",hostaddrID.toAscii().data()));
    QMap<QString,QSharedPointer<QRelayDeviceControl> >::const_iterator it = mydevicemap.find(hostaddrID);
    if(it != mydevicemap.end()) {
        int row = deviceTable->rowCount();
        for(int i=0;i<row;i++) {
            QTableWidgetItem * item = deviceTable->item(i,0);
            QVariant var = item->data(0);
            QSharedPointer<QRelayDeviceControl> pdev = qVariantValue<RelayDeviceSharePonterType>(var);
            if((*it) == pdev) {
                QAbstractItemModel * model = deviceTable->model ();
                int index_max = 9;
#if APP_DISPLAY_TIME
                index_max++;
#endif
                for(int j=0;j<index_max;j++) {
                    QModelIndex index = model->index(i,j);
                    deviceTable->update(index);
                }
                emit DeviceUpdata(pdev);
                break;
            }
        }
    }
}


void MainWindow::retranslateUI(void)
{
    deviceGroupBox->setTitle(tr("DeviceTable"));
    quitact->setText(tr("Exit"));
    edit_device_param_act->setText(tr("Edit Device Parameter..."));
    edit_device_ipconfig->setText(tr("Edit Ip Config..."));
}

void MainWindow::SelectChineseLanguage(void)
{
    retranslateUI();
}



void MainWindow::AboutAction(void)
{
    AboutDialog dlg;
    dlg.exec();
}
