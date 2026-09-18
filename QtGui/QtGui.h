#pragma once

#include <QtWidgets/QMainWindow>
#include <ctime>
#include <string>

#include "ui_QtGui.h"

class FeiraWorker;
class QThread;

class QtGui : public QMainWindow
{
    Q_OBJECT

public:
    QtGui(QWidget* parent = nullptr);
    ~QtGui() override;

private slots:
    void on_connect_btn_clicked();
    void on_device_connect_signal(int num_devices);

    void on_device_select_changed(int);
    void on_device_select_success_signal(
        QString d_ip, QString d_type, QString d_mac_address, QString d_firm_ver,
        QString d_cmd_port, QString d_img_port, QString d_serial_num);

    void on_device_info_update_btn_clicked();

    void on_acquisition_mode_changed(int);
    void on_mechanical_mode_changed(int);
    void on_mechanical_connect_btn_clicked();

    void on_binning_mode_changed(int);
    void on_gain_mode_changed(int);
    void on_integration_time_changed();
    void on_integration_time_signal(uint64_t integration_time);
    void on_interval_time_changed();
    void on_image_quantity_combobox_changed(int);
    void on_image_quantity_input_changed();
    void on_file_prefix_input_changed();

    time_t get_total_approximate_time();
    void set_total_approximate_time();

    void on_choose_file_path_btn_clicked();
    void on_grab_btn_clicked();
    void on_stop_grab_btn_clicked();

    void update_progress_tab(int index, int total_images, time_t starting_time, time_t remaining_time,
        QString file_path, QString file_prefix);
    void update_displayed_image(QString image_path);

    void on_operation_start_disable_all();
    void on_operation_end_enable_all();

    void testeF();
    void voltando();

private:
    Ui::QtGuiClass ui;

    void apply_mock_detector_fields();
    void apply_mock_operation_fields();
    void lock_mock_operation_controls();
    void unlock_editable_operation_controls();

    QString mock_projection_dir() const;
    int mock_projection_number(int progress_index, int total_images) const;
    QString mock_projection_path(int progress_index, int total_images) const;

    static constexpr int kMockProjectionCount = 100;

    std::string file_prefix;
    QString file_path;
    std::string save_file_name;

    bool stop_bnt_pressed = false;

    QThread* workerThread = nullptr;
    FeiraWorker* worker = nullptr;
};
