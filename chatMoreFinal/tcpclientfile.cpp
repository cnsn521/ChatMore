#include "tcpclientfile.h"
#include "ui_tcpclientfile.h"

#include <QTcpSocket>
#include <QDebug>
#include <QMessageBox>

TcpClientFile::TcpClientFile(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TcpClientFile)
{
    ui->setupUi(this);

    setFixedSize(600,180);

    TotalBytes = 0;
    bytesReceived = 0;
    fileNameSize = 0;

    tcpClient = new QTcpSocket(this);
    tcpPort = 6666;
    connect(tcpClient, SIGNAL(readyRead()), this, SLOT(readMessage()));
    connect(tcpClient, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(displayError(QAbstractSocket::SocketError)));
}

TcpClientFile::~TcpClientFile()
{
    delete ui;
}

// �����ļ���
void TcpClientFile::setFileName(QString fileName)
{
    localFile = new QFile(fileName);
}

// ���õ�ַ
void TcpClientFile::setHostAddress(QHostAddress address)
{
    hostAddress = address;
    newConnect();
}

// ����������
void TcpClientFile::newConnect()
{
    blockSize = 0;
    tcpClient->abort();
    tcpClient->connectToHost(hostAddress, tcpPort);
    time.start();
}

// ��ȡ����
void TcpClientFile::readMessage()
{
    QDataStream in(tcpClient);
     //�����������汾������Ҫ�ͷ���������ͬ
    in.setVersion(QDataStream::Qt_4_7);


    float useTime = time.elapsed();

    if (bytesReceived <= sizeof(qint64)*2) {//�жϽ��յ������Ƿ������ֽڣ�Ҳ�����ļ��Ĵ�С��Ϣ
        if ((tcpClient->bytesAvailable()
             >= sizeof(qint64)*2) && (fileNameSize == 0)) //������򱣴浽blockSize�����У�û���򷵻أ�������������
        {
            in>>TotalBytes>>fileNameSize;  //�����յ������ݴ�ŵ�������
            bytesReceived += sizeof(qint64)*2;
        }
        if((tcpClient->bytesAvailable() >= fileNameSize) && (fileNameSize != 0)){
            in>>fileName;
            bytesReceived +=fileNameSize;

            if(!localFile->open(QFile::WriteOnly)){
                QMessageBox::warning(this,tr("application"),tr("Cannot get file %1:\n%2.")
                                     .arg(fileName).arg(localFile->errorString()));
                return;
            }
        } else {
            return;
        }
    }
    if (bytesReceived < TotalBytes) {
        bytesReceived += tcpClient->bytesAvailable();
        inBlock = tcpClient->readAll();
        localFile->write(inBlock);
        inBlock.resize(0);
    }
    ui->progressBar->setMaximum(TotalBytes);
    ui->progressBar->setValue(bytesReceived);

    double speed = bytesReceived / useTime;
    ui->tcpClientStatusLabel->setText(tr("Done %1MB (%2MB/s) "
                                         "\Total%3MB Time used:%4s\nTime left: %5s")
                                      .arg(bytesReceived / (1024*1024))
                                      .arg(speed*1000/(1024*1024),0,'f',2)
                                      .arg(TotalBytes / (1024 * 1024))
                                      .arg(useTime/1000,0,'f',0)
                                      .arg(TotalBytes/speed/1000 - useTime/1000,0,'f',0));

    if(bytesReceived == TotalBytes)
    {
        localFile->close();
        tcpClient->close();
        ui->tcpClientStatusLabel->setText(tr("FileReceived %1")
                                          .arg(fileName));
    }
}

// ������
void TcpClientFile::displayError(QAbstractSocket::SocketError socketError)
{
    switch(socketError)
    {
    case QAbstractSocket::RemoteHostClosedError : break;
    default : qDebug() << tcpClient->errorString();
    }
}

// ȡ����ť
void TcpClientFile::on_tcpClientCancleBtn_clicked()
{
    tcpClient->abort();
    if (localFile->isOpen())
        localFile->close();
}

// �رհ�ť
void TcpClientFile::on_tcpClientCloseBtn_clicked()
{
    tcpClient->abort();
    if (localFile->isOpen())
        localFile->close();
    close();
}

// �ر��¼�
void TcpClientFile::closeEvent(QCloseEvent *)
{
    on_tcpClientCloseBtn_clicked();
}


void TcpClientFile::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}


