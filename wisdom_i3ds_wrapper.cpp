#include "wisdom_i3ds_wrapper.hpp"
#include <algorithm>
#include <i3ds/codec.hpp>
#include <i3ds_asn1/Common.hpp>
#include <iterator>
#include <string>

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

Wisdom::Wisdom(i3ds_asn1::NodeID node, unsigned int dummy_delay, std::string port, std::string ip) :
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

        if ((rv = getaddrinfo(ip.c_str(), port.c_str(), &hints, &wisdom_addr)) != 0) {
            throw std::runtime_error("addrinfo failed with errno: " + std::to_string(errno));
        }

        if ((udp_socket = socket(wisdom_addr->ai_family, wisdom_addr->ai_socktype, 
                        wisdom_addr->ai_protocol)) == -1) {
            throw std::runtime_error("socket failed with errno: " + std::to_string(errno));
        }
    }
}

Wisdom::~Wisdom()
{
    if (dummy_delay_ == 0) {
        freeaddrinfo(wisdom_addr);
        close(udp_socket);
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
    server.Attach<SetTimeService>(node(), [this](SetTimeService::Data d){set_time_handler(d);});
    server.Attach<LoadTablesService>(node(), [this](LoadTablesService::Data d){load_tables_handler(d);});
    server.Attach<TableSelectService>(node(), [this](TableSelectService::Data d){table_select_handler(d);});
}

void Wisdom::set_time_handler(SetTimeService::Data)
{
    check_standby();
    BOOST_LOG_TRIVIAL(info) << "Sending SET_TIME";
    set_time();
}

void Wisdom::load_tables_handler(LoadTablesService::Data)
{
    check_standby();
    BOOST_LOG_TRIVIAL(info) << "Loading tables";
    load_tables();
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

void Wisdom::do_activate()
{
    BOOST_LOG_TRIVIAL(info) << "Activating WISDOM";
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
}

void Wisdom::send_udp_command(const char* command)
{
    if ((sendto(udp_socket, command, CMD_LEN, 0, wisdom_addr->ai_addr, wisdom_addr->ai_addrlen)) != CMD_LEN) {
        throw std::runtime_error("sendto failed with errno: " + std::to_string(errno));
    }
}

void Wisdom::wait_for_ack(const char expected_byte)
{
    BOOST_LOG_TRIVIAL(info) << "Waiting for ACK";
    struct sockaddr_storage tmp_addr;
    socklen_t tmp_addr_len = sizeof(tmp_addr);
    struct pollfd pfds[1];
    pfds[0].fd = udp_socket;
    pfds[0].events = POLLIN;
    char ack_buf;
    int n_events = 0;
    while (n_events == 0 && running_) {
        n_events = poll(pfds, 1, 1000);
    }
    if (pfds[0].revents & POLLIN) {
        // We have a message to receive
        if ((recvfrom(udp_socket, &ack_buf, 1 , 0, (struct sockaddr *)&tmp_addr, &tmp_addr_len)) == -1) {
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
    BOOST_LOG_TRIVIAL(info) << "Starting dummy measurement";
    std::this_thread::sleep_for(std::chrono::seconds(dummy_delay_));
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

void Wisdom::Stop()
{
    running_ = false;
}

void Wisdom::table_select_handler(TableSelectService::Data d)
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