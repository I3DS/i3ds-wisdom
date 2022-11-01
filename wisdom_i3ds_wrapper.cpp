#include "wisdom_i3ds_wrapper.hpp"
#include <algorithm>
#include <i3ds/codec.hpp>
#include <i3ds_asn1/Common.hpp>
#include <iterator>
#include <string>
#include <unistd.h>

#ifndef BOOST_LOG_DYN_LINK
#define BOOST_LOG_DYN_LINK
#endif

#include <boost/log/trivial.hpp>

#include <i3ds/exception.hpp>

#include <thread>
#include <chrono>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>

Wisdom::Wisdom(i3ds_asn1::NodeID node, unsigned int dummy_delay,  std::string uart_dev, std::string port, std::string ip) :
    Sensor(node),
    dummy_delay_(dummy_delay),
    running_(true)
{
    set_device_name("WISDOM GPR");

    if (dummy_delay == 0) {
        int rv;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        if ((rv = getaddrinfo(ip.c_str(), port.c_str(), &hints, &wisdom_addr_)) != 0) {
            throw std::runtime_error("addrinfo failed with errno: " + std::to_string(errno));
        }

        if ((udp_socket_ = socket(wisdom_addr_->ai_family, wisdom_addr_->ai_socktype, 
                        wisdom_addr_->ai_protocol)) == -1) {
            throw std::runtime_error("socket failed with errno: " + std::to_string(errno));
        }
    }
    if (uart_dev != "") {
        serial_port_ = open(uart_dev.c_str(), O_RDWR);
        if (serial_port_ < 0) {
            BOOST_LOG_TRIVIAL(error) << "Error when opening serial port: " << std::to_string(errno)
                                     << ": " << strerror(errno);
        }
        else {
            struct termios tty;
            if(tcgetattr(serial_port_, &tty) != 0) {
                BOOST_LOG_TRIVIAL(error) << "Error from tcgetattr: " <<  std::to_string(errno) 
                                         << ": " << strerror(errno);
                close(serial_port_);
                serial_port_ = -1;
            }
            else {
                tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
                tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
                tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size 
                tty.c_cflag |= CS8; // 8 bits per byte (most common)
                tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
                tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

                tty.c_lflag &= ~ICANON;
                tty.c_lflag &= ~ECHO; // Disable echo
                tty.c_lflag &= ~ECHOE; // Disable erasure
                tty.c_lflag &= ~ECHONL; // Disable new-line echo
                tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
                tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
                tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

                tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
                tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

                tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
                tty.c_cc[VMIN] = 0;

                cfsetispeed(&tty, B115200);
                cfsetospeed(&tty, B115200);

                // Save tty settings, also checking for error
                if (tcsetattr(serial_port_, TCSANOW, &tty) != 0) {
                    BOOST_LOG_TRIVIAL(error) << "Error from tcsetattr: " <<  std::to_string(errno) 
                                             << ": " << strerror(errno);
                    close(serial_port_);
                    serial_port_ = -1;
                }
            }
        }
        
    }
    else {
        serial_port_ = -1;
    }
}

Wisdom::~Wisdom()
{
    if (dummy_delay_ == 0) {
        freeaddrinfo(wisdom_addr_);
        close(udp_socket_);
    }
    if (serial_port_ > 0) {
        close(serial_port_);
    }
}

bool Wisdom::is_sampling_supported(i3ds_asn1::SampleCommand sample)
{
    // Setting sampling rate will never be relevant so we always return false
    return false;
}

void Wisdom::Attach(i3ds::Server& server)
{
    Sensor::Attach(server);
    server.Attach<SetTimeService>(node(), [this](SetTimeService::Data d){handle_set_time(d);});
    server.Attach<LoadTablesService>(node(), [this](LoadTablesService::Data d){handle_load_tables(d);});
    server.Attach<TableSelectService>(node(), [this](TableSelectService::Data d){handle_table_select(d);});
}

void Wisdom::Stop()
{
    running_ = false;
}

void Wisdom::do_activate()
{
    BOOST_LOG_TRIVIAL(info) << "Activating WISDOM";
    if (serial_port_ > 0) {
        BOOST_LOG_TRIVIAL(info) << "Powering on";
        if (power_on()) {
            BOOST_LOG_TRIVIAL(info) << "Successfully powered on";
        }
        else {
            BOOST_LOG_TRIVIAL(error) << "Unable to power on";
             throw i3ds::CommandError(i3ds_asn1::ResultCode_error_other, 
                                 "Power on failed");
        }
    }
    if (dummy_delay_ == 0) {
        BOOST_LOG_TRIVIAL(info) << "Sending SET_TIME command";
        set_time();
        BOOST_LOG_TRIVIAL(info) << "Sending SCI_CONFIG command";
        load_tables();
    }

}

void Wisdom::do_start()
{
    if (worker_.joinable()) {
        worker_.join();
    }
    BOOST_LOG_TRIVIAL(info) << "Start WISDOM measurement";
    if (dummy_delay_ == 0) {
        worker_ = std::thread(&Wisdom::wait_for_measurement_to_finish, this);
    }
    else {
        worker_ = std::thread(&Wisdom::dummy_wait_for_measurement_to_finish, this);
    }
}

void Wisdom::do_stop()
{
    BOOST_LOG_TRIVIAL(warning) << "WISDOM does not support stopping measurement in progress";
    throw i3ds::CommandError(i3ds_asn1::ResultCode_error_unsupported , "Wisdom cannot stop active measurement");
}

void Wisdom::do_deactivate()
{
    BOOST_LOG_TRIVIAL(info) << "Deactivating WISDOM";
    if (serial_port_ > 0) {
        BOOST_LOG_TRIVIAL(info) << "Powering off";
        if (power_off()) {
            BOOST_LOG_TRIVIAL(info) << "Successfully powered off";
        }
        else {
            BOOST_LOG_TRIVIAL(error) << "Unable to power off";
             throw i3ds::CommandError(i3ds_asn1::ResultCode_error_other, 
                                 "Power off failed");
        }
    }
}

void Wisdom::make_sci_config_cmd(char* buf, unsigned char table_number)
{
    buf[0] = 1;
    buf[1] = table_number;
    buf[2] = 0;
    buf[3] = 0;
}

void Wisdom::make_sci_start_cmd(char* buf, unsigned char table_number)
{
    buf[0] = 3;
    buf[1] = table_number;
    buf[2] = 0;
    buf[3] = 3;
}

void Wisdom::send_udp_command(const char* command)
{
    if ((sendto(udp_socket_, command, CMD_LEN, 0, wisdom_addr_->ai_addr, wisdom_addr_->ai_addrlen)) != CMD_LEN) {
        throw std::runtime_error("sendto failed with errno: " + std::to_string(errno));
    }
}

void Wisdom::wait_for_ack(const char expected_byte)
{
    BOOST_LOG_TRIVIAL(info) << "Waiting for ACK";
    struct sockaddr_storage tmp_addr;
    socklen_t tmp_addr_len = sizeof(tmp_addr);
    struct pollfd pfds[1];
    pfds[0].fd = udp_socket_;
    pfds[0].events = POLLIN;
    char ack_buf;
    int n_events = 0;
    while (n_events == 0 && running_) {
        n_events = poll(pfds, 1, 1000);
    }
    if (pfds[0].revents & POLLIN) {
        // We have a message to receive
        if ((recvfrom(udp_socket_, &ack_buf, 1 , 0, (struct sockaddr *)&tmp_addr, &tmp_addr_len)) == -1) {
            throw std::runtime_error("recvfrom failed with errno: " + std::to_string(errno));
        }
        BOOST_LOG_TRIVIAL(info) << "ACK received: " << (int)ack_buf;
        if (ack_buf != expected_byte) {
            BOOST_LOG_TRIVIAL(warning) << "WARNING: incorrect ack byte received, expected " 
                                       << (int)expected_byte;
        }
    }
}

void Wisdom::dummy_wait_for_measurement_to_finish()
{
    for (int i = 0; i < N_TABLES; i++) {
        if (active_tables_[i]) {
            BOOST_LOG_TRIVIAL(info) << "Starting dummy measurement with table " << std::to_string(i);
            std::this_thread::sleep_for(std::chrono::seconds(dummy_delay_));
            BOOST_LOG_TRIVIAL(info) << "Measurement done, retrieving data";
            std::this_thread::sleep_for(std::chrono::seconds(dummy_delay_));
            BOOST_LOG_TRIVIAL(info) << "Data retreived";
        }
    }
    BOOST_LOG_TRIVIAL(info) << "Dummy measurement done";
    set_state(i3ds_asn1::SensorState_standby);
}

void Wisdom::wait_for_measurement_to_finish()
{
    char cmd[CMD_LEN];
    for (int i = 0; i < N_TABLES; i++) {
        if (active_tables_[i]) {
            BOOST_LOG_TRIVIAL(info) << "Starting measurement with table " << std::to_string(i);
            make_sci_start_cmd(cmd, 1);
            send_udp_command(cmd);
            wait_for_ack(cmd[0]);
            BOOST_LOG_TRIVIAL(info) << "Measurement done, retrieving data";
            send_udp_command(SCI_REQUEST);
            wait_for_ack(SCI_REQUEST[0]);
            BOOST_LOG_TRIVIAL(info) << "Data retreived";
        }
    }
    set_state(i3ds_asn1::SensorState_standby);
}

void Wisdom::handle_set_time(SetTimeService::Data)
{
    check_standby();
    BOOST_LOG_TRIVIAL(info) << "Sending SET_TIME";
    set_time();
}

void Wisdom::handle_load_tables(LoadTablesService::Data)
{
    check_standby();
    BOOST_LOG_TRIVIAL(info) << "Loading tables";
    load_tables();
}

void Wisdom::handle_table_select(TableSelectService::Data d)
{
    check_standby();
    BOOST_LOG_TRIVIAL(info) << "Got new table setting";
    if (d.request.nCount != N_TABLES) {
        throw i3ds::CommandError(i3ds_asn1::ResultCode_error_value, 
                                 "Requires " + std::to_string(N_TABLES) + " byte message");

    }

    // Check that not all tables are set to false.
    if (std::none_of(std::begin(d.request.arr), std::end(d.request.arr), [](bool b){return b;})) {
        throw i3ds::CommandError(i3ds_asn1::ResultCode_error_value,
                                  "At least one table must be enabled");
        return;
    }

    std::string current_setting = "";
    for (int i = 0; i < N_TABLES; i++) {
        active_tables_[i] = d.request.arr[i];
        if (active_tables_[i]) {
            current_setting += "1 ";
        }
        else {
            current_setting += "0 ";
        }
    }
    BOOST_LOG_TRIVIAL(info) << "Setting tables: " + current_setting;
}

void Wisdom::set_time()
{
    if (dummy_delay_ == 0) {
        send_udp_command(SET_TIME);
        wait_for_ack(SET_TIME[0]);
    }
}

void Wisdom::load_tables()
{
    if (dummy_delay_ == 0) {
        char cmd[CMD_LEN];
        for (unsigned int table = 1; table <= 4; table++) {
            make_sci_config_cmd(cmd, table);
            send_udp_command(cmd);
            wait_for_ack(cmd[0]);
        }
    }
}

bool Wisdom::send_serial_cmd(const char* cmd, const size_t cmd_len, const char* expected_ack)
{
    bool success = false;
    if (serial_port_ > 0) {
        char read_buffer[16];
        for (int i = 0; i < serial_retries_; i++) {
            write(serial_port_, &cmd, cmd_len);
            int n = read(serial_port_, &read_buffer, sizeof(read_buffer));
            if (n == 0) {
                BOOST_LOG_TRIVIAL(warning) << "Got no ack";
            }
            else if (!strcmp(read_buffer, power_on_ack_)) {
                success = true;
                break;
            }
            else {
                BOOST_LOG_TRIVIAL(warning) << "Got unexpected ack: " << read_buffer;
            }
        }
    }
    return success;
}

bool Wisdom::power_on()
{
    return send_serial_cmd(&power_on_cmd_, 1, power_on_ack_);
}

bool Wisdom::power_off()
{
    return send_serial_cmd(&power_off_cmd_, 1, power_off_ack_);
}
