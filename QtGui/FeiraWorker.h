#pragma once

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <atomic>
#include <ctime>
#include <string>

class FeiraWorker : public QObject
{
    Q_OBJECT
public:
    explicit FeiraWorker(QObject* parent = nullptr);
    ~FeiraWorker() override;

    std::atomic_bool stopRequested{ false };

public slots:
    void w_connect_detector(QString com_or_ip);
    void w_device_select(int index);
    void w_arduino_connect_serial_port();
    void w_binning_mode_change(int binning_mode);
    void w_gain_mode_change(int gain_mode);
    void w_integration_time_change(int integration_time);
    void w_recebido();

    void w_grab_start_operation(
        QString acquisition_mode, QString mechanical_mode, int interval_time, int image_quantity,
        QString file_path, QString file_prefix, time_t total_time);

    void w_grab_stop_operation();

    bool w_arduino_check_open();
    void w_arduino_send_command(const std::string& comando);

    // Funções equivalentes ao sketch feira_das_profissoes.ino
    void girarGraus(float graus);
    void disparaFonte();
    void girandoVolta();
    void reiniciarContador();
    void girar20Vezes();

signals:
    void message_box_error(QString title, QString message);
    void message_box_warning(QString title, QString message);
    void message_box_info(QString title, QString message);

    void device_conection_success(int num_devices);
    void device_select_success(
        QString d_ip, QString d_type, QString d_mac_address, QString d_firm_ver,
        QString d_cmd_port, QString d_img_port, QString d_serial_num);

    void integration_time_change_end(uint64_t frame_period);

    void enable_all();
    void disable_all();
    void update_tab(int index, int total_images, time_t starting_time, time_t remaining_time,
        QString file_path, QString file_prefix);

    void retornando();
    void status_update(QString message);

private:
    bool open_serial_port(const QString& port_name);
    void w_escrever_mensagem(const QString& caminhoArquivo, const QString& mensagem);
    void w_escrever_mensagem_t(const QString& caminhoArquivo, const QString& mensagem);
    void w_escrever_inicio_log(
        const QString& caminho_arquivo, QString acquisition_mode, QString mechanical_mode,
        int interval_time, int image_quantity, QString file_path, QString file_prefix);
    time_t w_calcular_tempo_restante(int img_total, int img_processadas, time_t starting_time);

    QSerialPort* serial = nullptr;
    QString serial_port_name = QStringLiteral("COM4");
    float graus_acumulados = 0.0f;
    int integration_time_us = 1000000;
    int binning_mode = 0;
    int gain_mode = 1;
};
