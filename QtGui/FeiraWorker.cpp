#include "FeiraWorker.h"

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QSerialPortInfo>
#include <QTextStream>
#include <QThread>

#include <algorithm>
#include <cmath>

FeiraWorker::FeiraWorker(QObject* parent)
    : QObject(parent)
{
    serial = new QSerialPort(this);
}

FeiraWorker::~FeiraWorker()
{
    if (serial && serial->isOpen()) {
        serial->close();
    }
}

void FeiraWorker::w_recebido()
{
}

bool FeiraWorker::open_serial_port(const QString& port_name)
{
    if (serial->isOpen()) {
        serial->close();
    }

    serial_port_name = port_name;
    serial->setPortName(port_name);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!serial->open(QIODevice::ReadWrite)) {
        emit message_box_error(
            QStringLiteral("Erro"),
            QStringLiteral("Não foi possível abrir a porta serial %1:\n%2")
                .arg(port_name, serial->errorString()));
        return false;
    }

    return true;
}

void FeiraWorker::w_connect_detector(QString com_or_ip)
{
    w_escrever_mensagem_t(QStringLiteral("log.txt"), QStringLiteral("Conectando ao Arduino da feira"));

    QString port = com_or_ip.trimmed();
    if (port.isEmpty()) {
        port = serial_port_name;
    }

    // Aceita "COM4" ou um IP fictício; se for IP, usa COM4 por padrão
    if (port.contains('.')) {
        port = serial_port_name;
    }

    if (!open_serial_port(port)) {
        return;
    }

    emit device_conection_success(1);
    emit message_box_info(
        QStringLiteral("Status"),
        QStringLiteral("Arduino conectado em %1.\nDados do detector exibidos são apenas ilustrativos.")
            .arg(port));
}

void FeiraWorker::w_device_select(int /*index*/)
{
    // Dados do detector são apenas ilustrativos (mock). A conexão real é com o Arduino.
    emit device_select_success(
        QStringLiteral("192.168.1.50"),
        QStringLiteral("Flat Panel Detector XRD-1412"),
        QStringLiteral("00:1A:2B:3C:4D:5E"),
        QStringLiteral("FW 3.2.1"),
        QStringLiteral("1030"),
        QStringLiteral("1031"),
        QStringLiteral("XRD-FP-2024-0042"));
}

void FeiraWorker::w_arduino_connect_serial_port()
{
    if (open_serial_port(serial_port_name)) {
        emit message_box_info(
            QStringLiteral("Status"),
            QStringLiteral("Conexão estabelecida com o Arduino."));
    }
}

void FeiraWorker::w_binning_mode_change(int mode)
{
    binning_mode = mode;
    w_escrever_mensagem_t(
        QStringLiteral("log.txt"),
        QStringLiteral("Binning alterado para %1 (sem efeito no Arduino)").arg(mode));
}

void FeiraWorker::w_gain_mode_change(int mode)
{
    gain_mode = mode;
    w_escrever_mensagem_t(
        QStringLiteral("log.txt"),
        QStringLiteral("Ganho alterado para %1 (sem efeito no Arduino)").arg(mode));
}

void FeiraWorker::w_integration_time_change(int integration_time)
{
    integration_time_us = integration_time;
    emit integration_time_change_end(static_cast<uint64_t>(integration_time));
}

bool FeiraWorker::w_arduino_check_open()
{
    return serial && serial->isOpen();
}

void FeiraWorker::w_arduino_send_command(const std::string& comando)
{
    if (!serial || !serial->isOpen()) {
        emit message_box_error(
            QStringLiteral("Erro na porta serial"),
            QStringLiteral("Porta serial não está aberta"));
        return;
    }

    serial->write((comando + "\n").c_str());
    serial->waitForBytesWritten(1000);
    serial->flush();

    w_escrever_mensagem_t(
        QStringLiteral("log.txt"),
        QStringLiteral(" - Enviando comando '%1' ao Arduino")
            .arg(QString::fromStdString(comando)));
}

// Protocolo serial do feira_das_profissoes.ino:
// 1 + graus -> girarGraus
// 2         -> disparaFonte
// 3         -> girandoVolta
// 4         -> reiniciarContador
// 5         -> girar20Vezes

void FeiraWorker::girarGraus(float graus)
{
    if (graus <= 0.0f || (graus_acumulados + graus) > 360.0f + 0.01f) {
        emit message_box_warning(
            QStringLiteral("Movimento inválido"),
            QStringLiteral("Movimento excede 360 graus ou é zero."));
        return;
    }

    w_arduino_send_command("1");
    QThread::msleep(250);
    w_arduino_send_command(std::to_string(graus));

    graus_acumulados += graus;
    if (std::abs(graus_acumulados - 360.0f) < 0.01f) {
        graus_acumulados = 0.0f;
    }

    emit status_update(
        QStringLiteral("Giro: %1° | Total: %2°")
            .arg(graus, 0, 'f', 2)
            .arg(graus_acumulados, 0, 'f', 2));
}

void FeiraWorker::disparaFonte()
{
    // Tempo de integração (us) -> tempo com a lâmpada/relé ligado (ms)
    const int tempo_ms = std::max(1, integration_time_us / 1000);
    w_arduino_send_command("2");
    QThread::msleep(50);
    w_arduino_send_command(std::to_string(tempo_ms));
    QThread::msleep(static_cast<unsigned long>(tempo_ms + 150));
    emit status_update(
        QStringLiteral("Relé/lâmpada ligada por %1 ms").arg(tempo_ms));
}

void FeiraWorker::girandoVolta()
{
    w_arduino_send_command("3");
    graus_acumulados = 0.0f;
    emit status_update(QStringLiteral("Volta completa iniciada no Arduino"));
}

void FeiraWorker::reiniciarContador()
{
    w_arduino_send_command("4");
    graus_acumulados = 0.0f;
    emit status_update(QStringLiteral("Contador de graus zerado"));
}

void FeiraWorker::girar20Vezes()
{
    w_arduino_send_command("5");
    graus_acumulados = 0.0f;
    emit status_update(QStringLiteral("Sequência de 20 posições iniciada no Arduino"));
}

void FeiraWorker::w_grab_start_operation(
    QString acquisition_mode, QString mechanical_mode, int interval_time, int image_quantity,
    QString file_path, QString file_prefix, time_t total_time)
{
    if (!w_arduino_check_open()) {
        emit message_box_error(
            QStringLiteral("Erro"),
            QStringLiteral("Porta serial não está aberta. Conecte o Arduino antes de iniciar."));
        emit enable_all();
        return;
    }

    stopRequested.store(false);
    time_t starting_time = time(nullptr);
    time_t remaining_time = total_time;
    const float angle = 360.0f / static_cast<float>(image_quantity);

    w_escrever_inicio_log(
        QStringLiteral("log.txt"), acquisition_mode, mechanical_mode, interval_time,
        image_quantity, file_path, file_prefix);

    // Atalho de 20 posições removido: tempo de integração variável exige ciclo controlado pelo PC
    reiniciarContador();

    for (int i = 0; i < image_quantity && !stopRequested.load(); ++i) {
        emit update_tab(i, image_quantity, starting_time, remaining_time, file_path, file_prefix);

        w_escrever_mensagem_t(
            QStringLiteral("log.txt"),
            QStringLiteral(" - Iniciando ciclo %1").arg(i + 1));

        // Equivalente à operação da feira: aciona fonte e (em tomografia) gira
        disparaFonte();

        if (acquisition_mode == QStringLiteral("Tomografia")) {
            girarGraus(angle);
            w_escrever_mensagem_t(
                QStringLiteral("log.txt"),
                QStringLiteral("Rotacionando a amostra em %1 graus").arg(angle));
        }

        w_escrever_mensagem_t(
            QStringLiteral("log.txt"),
            QStringLiteral(" - Finalizando ciclo %1\n").arg(i + 1));

        QThread::msleep(static_cast<unsigned long>(interval_time));
        remaining_time = w_calcular_tempo_restante(image_quantity, i + 1, starting_time);
    }

    if (stopRequested.load()) {
        w_escrever_mensagem_t(QStringLiteral("log.txt"), QStringLiteral("Botao de parada acionado"));
    }

    emit update_tab(
        image_quantity, image_quantity, starting_time, remaining_time, file_path, file_prefix);

    stopRequested.store(false);
    emit message_box_info(QStringLiteral("Aquisição"), QStringLiteral("Operação completa."));
    emit enable_all();
}

void FeiraWorker::w_grab_stop_operation()
{
    stopRequested.store(true);
    w_escrever_mensagem_t(QStringLiteral("log.txt"), QStringLiteral("Botao de parada acionado"));
}

void FeiraWorker::w_escrever_mensagem(const QString& caminhoArquivo, const QString& mensagem)
{
    QFile arquivo(caminhoArquivo);
    if (arquivo.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&arquivo);
        out << mensagem << "\n";
        arquivo.close();
    } else {
        qDebug() << "Erro ao abrir arquivo de log!";
    }
}

void FeiraWorker::w_escrever_mensagem_t(const QString& caminhoArquivo, const QString& mensagem)
{
    const QString mensagemFormatada =
        QDateTime::currentDateTime().toString(QStringLiteral("[yyyy/MM/dd hh:mm:ss]")) + mensagem;
    w_escrever_mensagem(caminhoArquivo, mensagemFormatada);
}

void FeiraWorker::w_escrever_inicio_log(
    const QString& caminho_arquivo, QString acquisition_mode, QString mechanical_mode,
    int interval_time, int image_quantity, QString file_path, QString file_prefix)
{
    w_escrever_mensagem_t(caminho_arquivo, QStringLiteral("Iniciando operacao ") + file_prefix);

    const QString binning = (binning_mode == 0 ? QStringLiteral("Normal") : QStringLiteral("2x2"));
    const QString gain = (gain_mode == 1 ? QStringLiteral("Baixo") : QStringLiteral("Alto"));

    QString logText =
        QStringLiteral("Parametros da Operacao:\n")
        + QStringLiteral("Modo de Aquisicao: ") + acquisition_mode + QStringLiteral("\n")
        + (acquisition_mode == QStringLiteral("Tomografia")
               ? (QStringLiteral("Mecanismo de Rotacao: ") + mechanical_mode + QStringLiteral("\n"))
               : QString())
        + QStringLiteral("Modo de Binning: ") + binning + QStringLiteral("\n")
        + QStringLiteral("Modo de Ganho: ") + gain + QStringLiteral("\n")
        + QStringLiteral("Tempo de Integracao (us): ") + QString::number(integration_time_us)
        + QStringLiteral("\n")
        + QStringLiteral("Tempo de Intervalo (ms): ") + QString::number(interval_time)
        + QStringLiteral("\n")
        + QStringLiteral("Quantidade de Imagens/Posicoes: ") + QString::number(image_quantity)
        + QStringLiteral("\n")
        + QStringLiteral("Prefixo: ") + file_prefix + QStringLiteral("\n")
        + QStringLiteral("Diretorio: ") + file_path + QStringLiteral("\n")
        + QStringLiteral("Backend: feira_das_profissoes.ino (serial)\n");

    w_escrever_mensagem(caminho_arquivo, logText);
}

time_t FeiraWorker::w_calcular_tempo_restante(int img_total, int img_processadas, time_t starting_time)
{
    if (img_processadas <= 0) {
        return 0;
    }

    const time_t elapsed_time_t = time(nullptr) - starting_time;
    const time_t total_time_t = (elapsed_time_t * img_total) / img_processadas;
    return total_time_t - elapsed_time_t;
}
