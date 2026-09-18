#include "QtGui.h"
#include "FeiraWorker.h"
#include "Utils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QImage>
#include <QPixmap>

#include <algorithm>
#include <string>

using namespace std;

QtGui::QtGui(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    // Campo de conexão real: porta serial do Arduino (ex.: COM4)
    ui.hostIpLabel->setText(QStringLiteral("Porta serial do Arduino"));
    ui.hostIpInput->setPlaceholderText(QStringLiteral("COM4"));
    ui.hostIpInput->setText(QStringLiteral("COM4"));

    // Dados ilustrativos do detector (mock) — preenchidos na conexão
    apply_mock_detector_fields();
    apply_mock_operation_fields();

    workerThread = new QThread(this);
    worker = new FeiraWorker();
    worker->moveToThread(workerThread);

    connect(ui.hostIpInput, SIGNAL(editingFinished()), this, SLOT(testeF()));
    connect(worker, &FeiraWorker::retornando, this, &QtGui::voltando);

    connect(ui.hostIpConnectBtn, SIGNAL(clicked()), this, SLOT(on_connect_btn_clicked()));
    connect(worker, &FeiraWorker::device_conection_success, this, &QtGui::on_device_connect_signal);

    connect(worker, &FeiraWorker::message_box_error, this, [this](const QString& titulo, const QString& mensagem) {
        QMessageBox::critical(this, titulo, mensagem);
    });

    connect(worker, &FeiraWorker::message_box_warning, this, [this](const QString& titulo, const QString& mensagem) {
        QMessageBox::warning(this, titulo, mensagem);
    });

    connect(worker, &FeiraWorker::message_box_info, this, [this](const QString& titulo, const QString& mensagem) {
        QMessageBox::information(this, titulo, mensagem);
    });

    connect(ui.deviceSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(on_device_select_changed(int)));
    connect(worker, &FeiraWorker::device_select_success, this, &QtGui::on_device_select_success_signal);

    connect(ui.deviceInfoUpdateBtn, SIGNAL(clicked()), this, SLOT(on_device_info_update_btn_clicked()));

    connect(ui.acquisitionModeInput, SIGNAL(currentIndexChanged(int)), this, SLOT(on_acquisition_mode_changed(int)));
    connect(ui.mechanicalModeInput, SIGNAL(currentIndexChanged(int)), this, SLOT(on_mechanical_mode_changed(int)));
    connect(ui.mechanicalConnectBtn, SIGNAL(clicked()), this, SLOT(on_mechanical_connect_btn_clicked()));

    connect(ui.binningModeInput, SIGNAL(currentIndexChanged(int)), this, SLOT(on_binning_mode_changed(int)));
    connect(ui.gainModeInput, SIGNAL(currentIndexChanged(int)), this, SLOT(on_gain_mode_changed(int)));
    connect(ui.integrationTimeInput, SIGNAL(editingFinished()), this, SLOT(on_integration_time_changed()));
    connect(worker, &FeiraWorker::integration_time_change_end, this, &QtGui::on_integration_time_signal);
    connect(ui.intervalTimeInput, SIGNAL(editingFinished()), this, SLOT(on_interval_time_changed()));

    connect(ui.imageQuantityComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(on_image_quantity_combobox_changed(int)));
    connect(ui.imageQuantityInput, SIGNAL(editingFinished()), this, SLOT(on_image_quantity_input_changed()));

    connect(ui.filePrefixInput, SIGNAL(editingFinished()), this, SLOT(on_file_prefix_input_changed()));
    connect(ui.chooseFilePathBtn, SIGNAL(clicked()), this, SLOT(on_choose_file_path_btn_clicked()));
    connect(ui.grabBtn, SIGNAL(clicked()), this, SLOT(on_grab_btn_clicked()));
    connect(ui.stopGrabBtn, SIGNAL(clicked()), this, SLOT(on_stop_grab_btn_clicked()));

    connect(worker, &FeiraWorker::enable_all, this, &QtGui::on_operation_end_enable_all);
    connect(worker, &FeiraWorker::disable_all, this, &QtGui::on_operation_start_disable_all);
    connect(worker, &FeiraWorker::update_tab, this, &QtGui::update_progress_tab);

    // Apenas quantidade de imagens e tempo de integração são editáveis
    lock_mock_operation_controls();

    workerThread->start();
}

void QtGui::testeF()
{
    QMetaObject::invokeMethod(worker, "w_recebido", Qt::QueuedConnection);
}

void QtGui::voltando()
{
}

void QtGui::on_connect_btn_clicked()
{
    const QString host = ui.hostIpInput->text().trimmed();

    if (!isValidConnectionTarget(host.toStdString())) {
        QMessageBox::warning(
            this,
            QStringLiteral("Aviso"),
            QStringLiteral("Informe uma porta serial válida (ex.: COM4) ou um IP."));
        return;
    }

    QMetaObject::invokeMethod(
        worker, "w_connect_detector",
        Qt::QueuedConnection, Q_ARG(QString, host));
}

void QtGui::on_device_connect_signal(int /*num_devices*/)
{
    ui.hostIpInput->setDisabled(true);
    ui.hostIpConnectBtn->setDisabled(true);

    // Lista ilustrativa de detector (mock); conexão real já feita com o Arduino
    ui.deviceSelect->blockSignals(true);
    ui.deviceSelect->clear();
    ui.deviceSelect->addItem(QStringLiteral("Detector XRD-1412 (mock)"));
    ui.deviceSelect->setCurrentIndex(0);
    ui.deviceSelect->blockSignals(false);
    ui.deviceSelect->setDisabled(true);

    QMetaObject::invokeMethod(
        worker, "w_device_select",
        Qt::QueuedConnection, Q_ARG(int, 0));
}

void QtGui::on_device_select_changed(int index)
{
    if (index < 0) {
        return;
    }

    QMetaObject::invokeMethod(
        worker, "w_device_select",
        Qt::QueuedConnection, Q_ARG(int, index));
}

void QtGui::on_device_select_success_signal(
    QString d_ip, QString d_type, QString d_mac_address, QString d_firm_ver,
    QString d_cmd_port, QString d_img_port, QString d_serial_num)
{
    ui.deviceIpInput->setText(d_ip);
    ui.deviceTypeInput->setText(d_type);
    ui.deviceMacInput->setText(d_mac_address);
    ui.deviceFirmwareInput->setText(d_firm_ver);
    ui.deviceCmdPortInput->setText(d_cmd_port);
    ui.deviceImgPortInput->setText(d_img_port);
    ui.deviceSerialInput->setText(d_serial_num);

    apply_mock_operation_fields();
    unlock_editable_operation_controls();
    set_total_approximate_time();
}

void QtGui::on_device_info_update_btn_clicked()
{
    QMessageBox::information(
        this,
        QStringLiteral("Info"),
        QStringLiteral("Neste projeto a conexão é via serial Arduino; não há reconfiguração de IP/portas de detector."));
}

void QtGui::on_acquisition_mode_changed(int /*index*/)
{
    // Modo de aquisição é mockado (sempre Tomografia)
    ui.acquisitionModeInput->setCurrentIndex(0);
    ui.imageQuantityStackedWidget->setCurrentIndex(0);
    set_total_approximate_time();
}

void QtGui::on_mechanical_mode_changed(int /*index*/)
{
}

void QtGui::on_mechanical_connect_btn_clicked()
{
    // Conexão mecânica é ilustrativa; a serial do Arduino já foi aberta na aba Conexão
    QMessageBox::information(
        this,
        QStringLiteral("Info"),
        QStringLiteral("Conexão com o Arduino já estabelecida na aba Conexão e dispositivos."));
}

void QtGui::on_binning_mode_changed(int /*index*/)
{
    // Campo mockado — sem efeito real
}

void QtGui::on_gain_mode_changed(int /*index*/)
{
    // Campo mockado — sem efeito real
}

void QtGui::on_integration_time_changed()
{
    int integration_time = ui.integrationTimeInput->text().toInt();

    if (integration_time < 1) {
        QMessageBox::warning(this, QStringLiteral("Aviso"), QStringLiteral("O tempo de integração deve ser maior que 0s."));
        ui.integrationTimeInput->setText(QStringLiteral("1000000"));
        integration_time = 1000000;
    }

    QMetaObject::invokeMethod(
        worker, "w_integration_time_change",
        Qt::QueuedConnection, Q_ARG(int, integration_time));
}

void QtGui::on_integration_time_signal(uint64_t integration_time)
{
    ui.integrationTimeInput->setText(QString::number(integration_time));
    set_total_approximate_time();
}

void QtGui::on_interval_time_changed()
{
    const int interval_time = ui.intervalTimeInput->text().toInt();

    if (interval_time < 0) {
        QMessageBox::warning(this, QStringLiteral("Aviso"), QStringLiteral("O tempo de intervalo deve ser maior que ou igual a 0s."));
        ui.intervalTimeInput->setText(QStringLiteral("1500"));
    }

    set_total_approximate_time();
}

void QtGui::on_image_quantity_combobox_changed(int /*index*/)
{
    set_total_approximate_time();
}

void QtGui::on_image_quantity_input_changed()
{
    const int image_quantity = ui.imageQuantityInput->text().toInt();

    if (image_quantity < 1) {
        QMessageBox::warning(this, QStringLiteral("Aviso"), QStringLiteral("A quantidade de imagens deve maior que 0."));
        ui.imageQuantityInput->setText(QStringLiteral("5"));
    }

    set_total_approximate_time();
}

void QtGui::on_file_prefix_input_changed()
{
    if (ui.filePrefixInput->text().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Aviso"), QStringLiteral("O prefixo não pode ser vazio."));
        ui.filePrefixInput->setText(QStringLiteral("img"));
    }
}

void QtGui::on_choose_file_path_btn_clicked()
{
    this->file_path = QFileDialog::getExistingDirectory(this, QStringLiteral("Escolha um diretório"), QStringLiteral("C:/"));
    ui.filePathInput->setText(this->file_path);
}

void QtGui::on_grab_btn_clicked()
{
    const string file_path = ui.filePathInput->text().toStdString();
    const string file_prefix = ui.filePrefixInput->text().toStdString();
    const time_t total_approximate_time = get_total_approximate_time();
    const int image_quantity = ui.imageQuantityComboBox->currentText().toInt();
    const int interval_time = ui.intervalTimeInput->text().toInt();

    if (image_quantity < 1 || ui.integrationTimeInput->text().toInt() < 1) {
        QMessageBox::warning(
            this,
            QStringLiteral("Aviso"),
            QStringLiteral("Informe quantidade de imagens e tempo de integração válidos."));
        return;
    }

    bool arduino_is_open = false;
    QMetaObject::invokeMethod(
        worker, "w_arduino_check_open",
        Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, arduino_is_open));

    if (!arduino_is_open) {
        QMessageBox::warning(this, QStringLiteral("Erro"), QStringLiteral("Porta serial não está aberta. Conecte o Arduino primeiro."));
        return;
    }

    QMessageBox::information(this, QStringLiteral("Dispositivo pronto"), QStringLiteral("Clique em OK para iniciar a operação."));
    on_operation_start_disable_all();

    // Aquisição/mecânica/caminho são mockados; só quantidade e integração são reais
    QMetaObject::invokeMethod(
        worker, "w_grab_start_operation",
        Qt::QueuedConnection,
        Q_ARG(QString, QStringLiteral("Tomografia")),
        Q_ARG(QString, QStringLiteral("Arduino")),
        Q_ARG(int, interval_time),
        Q_ARG(int, image_quantity),
        Q_ARG(QString, QString::fromStdString(file_path.empty() ? string("C:/feira_aquisicoes") : file_path)),
        Q_ARG(QString, QString::fromStdString(file_prefix.empty() ? string("img") : file_prefix)),
        Q_ARG(time_t, total_approximate_time > 0 ? total_approximate_time : static_cast<time_t>(image_quantity)));
}

void QtGui::on_operation_start_disable_all()
{
    ui.integrationTimeInput->setDisabled(true);
    ui.imageQuantityComboBox->setDisabled(true);
    ui.imageQuantityStackedWidget->setDisabled(true);
    ui.grabBtn->setDisabled(true);
    ui.stopGrabBtn->setDisabled(false);
}

void QtGui::on_operation_end_enable_all()
{
    unlock_editable_operation_controls();
    ui.stopGrabBtn->setDisabled(true);
    this->stop_bnt_pressed = false;
}

void QtGui::on_stop_grab_btn_clicked()
{
    this->stop_bnt_pressed = true;
    worker->stopRequested.store(true);
    QMetaObject::invokeMethod(worker, "w_grab_stop_operation", Qt::QueuedConnection);
}

void QtGui::update_progress_tab(
    int index, int total_images, time_t starting_time, time_t remaining_time,
    QString /*file_path*/, QString /*file_prefix*/)
{
    const int displayed_proj = index > 0
        ? mock_projection_number(index, total_images)
        : 0;
    const int processing_proj = index < total_images
        ? mock_projection_number(index + 1, total_images)
        : 0;

    QString current_displayed_string = QStringLiteral("Exibindo: ---");
    if (displayed_proj > 0) {
        current_displayed_string = QStringLiteral("Exibindo: img%1.dat").arg(displayed_proj);
    }
    ui.currentDisplayedImageLabel->setText(current_displayed_string);

    QString processing_string = QStringLiteral("Processando: ---");
    if (processing_proj > 0) {
        processing_string = QStringLiteral("Processando: img%1.dat").arg(processing_proj);
    }
    ui.currentProcessingImageLabel->setText(processing_string);

    const int progress_percent = total_images > 0 ? (index * 100) / total_images : 0;
    ui.currentProgressLabel->setText(
        QStringLiteral("Progresso: %1/%2 (%3%)")
            .arg(index, 1, 10, QChar('0'))
            .arg(total_images, 1, 10, QChar('0'))
            .arg(progress_percent, 1, 10, QChar('0')));

    const time_t elapsed_time_t = time(nullptr) - starting_time;
    ui.elapsedTimeLabel->setText(
        QStringLiteral("Tempo decorrido: %1:%2:%3")
            .arg(elapsed_time_t / 3600, 3, 10, QChar('0'))
            .arg((elapsed_time_t % 3600) / 60, 2, 10, QChar('0'))
            .arg(elapsed_time_t % 60, 2, 10, QChar('0')));

    ui.remainingTimeLabel->setText(
        QStringLiteral("Tempo restante: %1:%2:%3")
            .arg(remaining_time / 3600, 3, 10, QChar('0'))
            .arg((remaining_time % 3600) / 60, 2, 10, QChar('0'))
            .arg(remaining_time % 60, 2, 10, QChar('0')));

    QString status_string = QStringLiteral("Status: Em operação");
    if (this->stop_bnt_pressed) {
        status_string = QStringLiteral("Status: Parado");
    }
    if (index == total_images) {
        status_string = QStringLiteral("Status: Concluído");
    }
    ui.currentStatusLabel->setText(status_string);

    ui.progressBar->setMaximum(total_images);
    ui.progressBar->setValue(index);

    if (index > 0) {
        update_displayed_image(mock_projection_path(index, total_images));
    }
}

void QtGui::update_displayed_image(QString image_path)
{
    // Projeções mockadas de assets/caracol/100prjs (uint16 1400x1200)
    QFile file(image_path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return;
    }

    const int width = 1400;
    const int height = 1200;
    const int total_size = width * height;

    QByteArray data_u16_bits = file.readAll();
    file.close();
    if (data_u16_bits.size() < total_size * 2) {
        return;
    }

    QByteArray data_u8_bits(total_size, 0);
    uint16_t min_val = 65535;
    uint16_t max_val = 0;

    for (int i = 0; i < total_size; i++) {
        const uint16_t val =
            static_cast<unsigned char>(data_u16_bits[i * 2]) |
            (static_cast<unsigned char>(data_u16_bits[i * 2 + 1]) << 8);
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }

    for (int i = 0; i < total_size; i++) {
        const uint16_t val =
            static_cast<unsigned char>(data_u16_bits[i * 2]) |
            (static_cast<unsigned char>(data_u16_bits[i * 2 + 1]) << 8);
        unsigned char normalized = 0;
        if (max_val > min_val) {
            normalized = static_cast<unsigned char>((val - min_val) * 255 / (max_val - min_val));
        }
        data_u8_bits[i] = static_cast<char>(normalized);
    }

    QImage img(reinterpret_cast<uchar*>(data_u8_bits.data()), width, height, QImage::Format_Grayscale8);
    ui.imageLabel->setPixmap(
        QPixmap::fromImage(img.copy()).scaled(420, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

time_t QtGui::get_total_approximate_time()
{
    const int integration_time = ui.integrationTimeInput->text().toInt();
    const int interval_time = ui.intervalTimeInput->text().toInt();
    const int image_quantity = ui.imageQuantityComboBox->currentText().toInt();
    const float mechanical_signal_time = 1.25f;

    if (integration_time == 0 || image_quantity == 0) {
        return 0;
    }

    return static_cast<time_t>(
        image_quantity * ((integration_time * 1.0) / 1000000.0 + (interval_time * 1.0) / 1000.0 + mechanical_signal_time));
}

void QtGui::set_total_approximate_time()
{
    const time_t total_time = get_total_approximate_time();
    if (total_time != 0) {
        ui.totalTimeInput->setText(
            QStringLiteral("%1:%2:%3")
                .arg(total_time / 3600, 3, 10, QChar('0'))
                .arg((total_time % 3600) / 60, 2, 10, QChar('0'))
                .arg(total_time % 60, 2, 10, QChar('0')));
    }
}

void QtGui::apply_mock_detector_fields()
{
    ui.deviceIpInput->setText(QStringLiteral("192.168.1.50"));
    ui.deviceTypeInput->setText(QStringLiteral("Flat Panel Detector XRD-1412"));
    ui.deviceMacInput->setText(QStringLiteral("00:1A:2B:3C:4D:5E"));
    ui.deviceFirmwareInput->setText(QStringLiteral("FW 3.2.1"));
    ui.deviceCmdPortInput->setText(QStringLiteral("1030"));
    ui.deviceImgPortInput->setText(QStringLiteral("1031"));
    ui.deviceSerialInput->setText(QStringLiteral("XRD-FP-2024-0042"));
    ui.deviceInfoUpdateBtn->setDisabled(true);
}

void QtGui::apply_mock_operation_fields()
{
    ui.acquisitionModeInput->setCurrentIndex(0); // Tomografia
    ui.mechanicalModeInput->setCurrentIndex(0);  // Arduino
    ui.binningModeInput->setCurrentIndex(0);     // Normal
    ui.gainModeInput->setCurrentIndex(0);        // Baixo
    ui.intervalTimeInput->setText(QStringLiteral("1500"));
    ui.filePrefixInput->setText(QStringLiteral("img"));
    const QString mock_dir = mock_projection_dir();
    ui.filePathInput->setText(mock_dir);
    this->file_path = mock_dir;
    ui.imageQuantityStackedWidget->setCurrentIndex(0);

    if (ui.integrationTimeInput->text().trimmed().isEmpty()) {
        ui.integrationTimeInput->setText(QStringLiteral("1000000"));
    }

    // Padrão: 20 posições (18° cada) para completar 360°
    const int idx20 = ui.imageQuantityComboBox->findText(QStringLiteral("20"));
    ui.imageQuantityComboBox->setCurrentIndex(idx20 >= 0 ? idx20 : 0);
}

void QtGui::lock_mock_operation_controls()
{
    ui.deviceSelect->setDisabled(true);
    ui.deviceInfoUpdateBtn->setDisabled(true);

    ui.acquisitionModeInput->setDisabled(true);
    ui.mechanicalModeInput->setDisabled(true);
    ui.mechanicalConnectBtn->setDisabled(true);
    ui.binningModeInput->setDisabled(true);
    ui.gainModeInput->setDisabled(true);
    ui.intervalTimeInput->setDisabled(true);
    ui.filePrefixInput->setDisabled(true);
    ui.filePathInput->setDisabled(true);
    ui.chooseFilePathBtn->setDisabled(true);
    ui.imageQuantityInput->setDisabled(true);

    ui.integrationTimeInput->setDisabled(true);
    ui.imageQuantityComboBox->setDisabled(true);
    ui.imageQuantityStackedWidget->setDisabled(true);
    ui.grabBtn->setDisabled(true);
}

void QtGui::unlock_editable_operation_controls()
{
    lock_mock_operation_controls();

    ui.integrationTimeInput->setDisabled(false);
    ui.imageQuantityStackedWidget->setDisabled(false);
    ui.imageQuantityComboBox->setDisabled(false);
    ui.grabBtn->setDisabled(false);
}

QString QtGui::mock_projection_dir() const
{
    const QString app_dir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(app_dir).absoluteFilePath(QStringLiteral("assets/caracol/100prjs")),
        QDir(app_dir).absoluteFilePath(QStringLiteral("../../../assets/caracol/100prjs")),
        QDir(app_dir).absoluteFilePath(QStringLiteral("../../../../QtGui/assets/caracol/100prjs")),
    };

    for (const QString& candidate : candidates) {
        if (QDir(candidate).exists()) {
            return QDir(candidate).absolutePath();
        }
    }

    // Fallback: pasta do projeto relativa ao cwd
    return QDir(QStringLiteral("assets/caracol/100prjs")).absolutePath();
}

int QtGui::mock_projection_number(int progress_index, int total_images) const
{
    if (progress_index <= 0) {
        return 1;
    }

    if (total_images <= 1) {
        return 1;
    }

    // Distribui o progresso pelas 100 projeções (img1..img100)
    const int clamped = std::min(progress_index, total_images);
    const int proj = 1 + ((clamped - 1) * (kMockProjectionCount - 1)) / (total_images - 1);
    return std::min(std::max(proj, 1), kMockProjectionCount);
}

QString QtGui::mock_projection_path(int progress_index, int total_images) const
{
    const int proj = mock_projection_number(progress_index, total_images);
    return QDir(mock_projection_dir()).filePath(QStringLiteral("img%1.dat").arg(proj));
}

QtGui::~QtGui()
{
    if (workerThread) {
        workerThread->quit();
        workerThread->wait(3000);
    }
    delete worker;
}
