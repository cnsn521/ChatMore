#include "tcpserverfile.h"
#include "ui_tcpserverfile.h"

#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>

TcpServerFile::TcpServerFile(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TcpServerFile)
{
    ui->setupUi(this);

    setFixedSize(350,180);

    //����һ��tcp����
    tcpPort = 6666;
    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(sendMessage()));

    //��ʼ��������
    initServer();
}

TcpServerFile::~TcpServerFile()
{
    delete ui;
}

// ��ʼ��
void TcpServerFile::initServer()
{
    //�Ի������ͷ����ֽڽ��г�ʼ��
    payloadSize = 64*1024;
    TotalBytes = 0;
    bytesWritten = 0;
    bytesToWrite = 0;

    ui->serverStatusLabel->setText(tr("Choose a file to start!"));
    ui->progressBar->reset();
    ui->serverOpenBtn->setEnabled(true);
    ui->serverSendBtn->setEnabled(false);

    tcpServer->close();
}

// ��ʼ��������
void TcpServerFile::sendMessage()
{
    ui->serverSendBtn->setEnabled(false);
    clientConnection = tcpServer->nextPendingConnection();
    connect(clientConnection, SIGNAL(bytesWritten(qint64)),
            this, SLOT(updateClientProgress(qint64)));

    ui->serverStatusLabel->setText(tr("Transmission %1!").arg(theFileName));

    localFile = new QFile(fileName);
    if(!localFile->open((QFile::ReadOnly))){
        QMessageBox::warning(this, tr("Application"), tr("Reach File Failed %1:\n%2")
                             .arg(fileName).arg(localFile->errorString()));
        return;
    }
    TotalBytes = localFile->size();
    QDataStream sendOut(&outBlock, QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_4_7);
    time.start();  // ��ʼ��ʱ
    QString currentFile = fileName.right(fileName.size()
                                         - fileName.lastIndexOf('/')-1);
    sendOut << qint64(0) << qint64(0) << currentFile;
    TotalBytes += outBlock.size();
    sendOut.device()->seek(0);
    sendOut << TotalBytes << qint64((outBlock.size() - sizeof(qint64)*2));
    bytesToWrite = TotalBytes - clientConnection->write(outBlock);
    outBlock.resize(0);
}

// ���½�����
void TcpServerFile::updateClientProgress(qint64 numBytes)
{
    //�����ڴ�����ļ�ʱ���ڲ��ᶳ�� timer�����ú�ʱ�ĵȴ����� ����ÿһ�����֮��Խ���������һ�θ���
    qApp->processEvents();
    bytesWritten += (int)numBytes;
    if (bytesToWrite > 0) {
        outBlock = localFile->read(qMin(bytesToWrite, payloadSize));
        bytesToWrite -= (int)clientConnection->write(outBlock);
        outBlock.resize(0);
    } else {
        localFile->close();
    }
    ui->progressBar->setMaximum(TotalBytes);
    ui->progressBar->setValue(bytesWritten);

    float useTime = time.elapsed();
    double speed = bytesWritten / useTime;
    ui->serverStatusLabel->setText(tr("Done %1MB (%2MB/s) "
                                      "\Total%3MB Time used:%4s\nTime left: %5s")
                   .arg(bytesWritten / (1024*1024))
                   .arg(speed*1000 / (1024*1024), 0, 'f', 2)
                   .arg(TotalBytes / (1024 * 1024))
                   .arg(useTime/1000, 0, 'f', 0)
                   .arg(TotalBytes/speed/1000 - useTime/1000, 0, 'f', 0));

    if(bytesWritten == TotalBytes) {
        localFile->close();
        tcpServer->close();
        ui->serverStatusLabel->setText(tr("FileReceived %1").arg(theFileName));
    }
}

// �򿪰�ť ����һ���Ի���ѡ���ļ�
void TcpServerFile::on_serverOpenBtn_clicked()
{
    fileName = QFileDialog::getOpenFileName(this);
    if(!fileName.isEmpty())
    {
        theFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/')-1);
        ui->serverStatusLabel->setText(tr("Slected:%1 ").arg(theFileName));
        ui->serverSendBtn->setEnabled(true);
        ui->serverOpenBtn->setEnabled(false);
    }
}

// ���Ͱ�ť ������Ͱ�ť֮��   ʹ��udp�㲥���ļ���Ϣ���͸��ͻ���
//�ͻ����յ��㲥֮�󵯳�һ���Ի��� ͬ��֮���½�һ���ͻ��� �����ʹ��tcp�����ļ�����
void TcpServerFile::on_serverSendBtn_clicked()
{
    if(!tcpServer->listen(QHostAddress::Any,tcpPort))//��ʼ����
    {
        qDebug() << tcpServer->errorString();
        close();
        return;
    }

    ui->serverStatusLabel->setText(tr("Waiting for the response......"));
    //�����ź�
    emit sendFileName(theFileName);
}

// �رհ�ť
void TcpServerFile::on_serverCloseBtn_clicked()
{
    if(tcpServer->isListening())
    {
        tcpServer->close();
        if (localFile->isOpen())
            localFile->close();
        clientConnection->abort();
    }
    close();
}

// ���Է��ܾ� �ͻ��˵���tcpClient->abort()�� �������˹رյ�ǰ��tcp����
void TcpServerFile::refused()
{
    tcpServer->close();
    ui->serverStatusLabel->setText(tr("Be refused"));
}

// �ر��¼�
void TcpServerFile::closeEvent(QCloseEvent *)
{
    on_serverCloseBtn_clicked();
}
