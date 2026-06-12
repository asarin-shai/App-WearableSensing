// Wearable Sensing LSL GUI
// Copyright (C) 2014-2020 Syntrogi Inc dba Intheon.

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

#ifndef WIN32
#include <unistd.h>
#endif

#ifdef WIN32
const QString program = "dsi2lsl.exe";
#else
const QString program = "./dsi2lsl";
#endif

const QString port = "--port=";
const QString lslStream = "--lsl-stream-name=";
const QString montage = "--montage=";
const QString reference = "--reference=";
const QString defaultValule = "(use default)";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    counter(0)
{
    ui->setupUi(this);
#ifndef WIN32
    char cdir[256];
    setenv("LD_LIBRARY_PATH", getcwd(cdir, 256), 1);
#endif

    this->loadConfiguration();

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Start");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("Stop");
    ui->statusBar->setVisible(false);
    this->ui->statusBar->showMessage("Streaming...");
    this->progressBar = new QProgressBar(this);
    ui->statusBar->addPermanentWidget(this->progressBar, 1);
    this->progressBar->setTextVisible(false);

    this->streamer = new QProcess(this);
    this->streamer->setProcessChannelMode(QProcess::MergedChannels);
}

MainWindow::~MainWindow()
{
    this->saveConfiguration();
    this->on_buttonBox_rejected();
    delete ui;
}

void MainWindow::loadConfiguration()
{
    QSettings settings("dsi2lsl", "dsi2lslgui");
    QString cfgPort = settings.value("port", "").toString();
    QString cfgName = settings.value("lsl_stream_name", "WS-default").toString();
    QString cfgMontage = settings.value("montage", defaultValule).toString();
    QString cfgReference = settings.value("reference", defaultValule).toString();

    QMap<QString, QString> cli;
    QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.size(); ++i) {
        QString arg = args[i];
        if (arg.startsWith("--") && arg.contains("=")) {
            int split = arg.indexOf('=');
            cli[arg.mid(2, split - 2)] = arg.mid(split + 1);
        }
    }

    QString finalPort = cli.value("port", qEnvironmentVariable("DSI2LSL_PORT", cfgPort.toUtf8().constData()));
    QString finalName = cli.value("lsl-stream-name", qEnvironmentVariable("DSI2LSL_STREAM_NAME", cfgName.toUtf8().constData()));
    QString finalMontage = cli.value("montage", qEnvironmentVariable("DSI2LSL_MONTAGE", cfgMontage.toUtf8().constData()));
    QString finalReference = cli.value("reference", qEnvironmentVariable("DSI2LSL_REFERENCE", cfgReference.toUtf8().constData()));

    ui->portLineEdit->setText(finalPort);
    ui->nameLineEdit->setText(finalName);
    ui->montageLineEdit->setText(finalMontage.isEmpty() ? defaultValule : finalMontage);
    ui->referenceLineEdit->setText(finalReference.isEmpty() ? defaultValule : finalReference);
}

void MainWindow::saveConfiguration()
{
    QSettings settings("dsi2lsl", "dsi2lslgui");
    settings.setValue("port", ui->portLineEdit->text().simplified());
    settings.setValue("lsl_stream_name", ui->nameLineEdit->text().simplified());
    settings.setValue("montage", ui->montageLineEdit->text().simplified());
    settings.setValue("reference", ui->referenceLineEdit->text().simplified());
}

void MainWindow::on_buttonBox_accepted()
{
    if(this->streamer != NULL)
        this->on_buttonBox_rejected();

    QStringList arguments = this->parseArguments();
    this->streamer->start(program, arguments);
    connect(this->streamer, SIGNAL(readyReadStandardOutput()), this, SLOT(writeToConsole()));
    this->counter = 0;
    this->timerId = this->startTimer(1000);
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
    if(this->streamer->state()==QProcess::NotRunning)
    {
        this->on_buttonBox_rejected();
        return;
    }

    if(!this->ui->statusBar->isVisible())
        this->ui->statusBar->setVisible(true);
    this->counter += 33;
    if(this->counter > 100)
        this->counter = 0;
    this->progressBar->setValue(this->counter);
    this->ui->statusBar->showMessage("Streaming...");
}

void MainWindow::writeToConsole()
{
    while(this->streamer->canReadLine()){
        this->ui->console->append(this->streamer->readLine());
    }
}

void MainWindow::on_buttonBox_rejected()
{
    if(this->streamer != NULL){
        this->streamer->close();
        this->ui->console->append("Streamer will exit now. Good bye!");
        this->killTimer(this->timerId);
        this->counter = 0;
        this->ui->statusBar->setVisible(false);
    }
}

QStringList MainWindow::parseArguments()
{
    QStringList arguments;

    QString configuredPort = this->ui->portLineEdit->text().simplified();
    if (!configuredPort.isEmpty())
        arguments << (port+configuredPort).toStdString().c_str();

    arguments << (lslStream+this->ui->nameLineEdit->text().simplified()).toStdString().c_str();

    if(ui->montageLineEdit->text().simplified().compare(defaultValule))
        arguments << (montage+this->ui->montageLineEdit->text().simplified()).toStdString().c_str();

    if(ui->referenceLineEdit->text().simplified().compare(defaultValule))
        arguments << (reference+this->ui->referenceLineEdit->text().simplified()).toStdString().c_str();

    return arguments;
}
