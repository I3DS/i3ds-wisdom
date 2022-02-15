#include "wisdom_i3ds_wrapper.hpp"

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
#include <errno.h>

Wisdom::Wisdom(i3ds_asn1::NodeID node, unsigned int dummy_delay, std::string port) :
    Sensor(node),
    dummy_delay_(dummy_delay)
{
    set_device_name("WISDOM GPR");

    if (dummy_delay == 0) {
        int rv;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        if ((rv = getaddrinfo("127.0.0.1", port.c_str(), &hints, &wisdom_addr)) != 0) {
            throw std::runtime_error("addrinfo failed with errno: " + errno);
        }

        if ((udp_socket = socket(wisdom_addr->ai_family, wisdom_addr->ai_socktype, 
                        wisdom_addr->ai_protocol)) == -1) {
            throw std::runtime_error("socket failed with errno: " + errno);
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
}

void Wisdom::do_activate()
{
    BOOST_LOG_TRIVIAL(info) << "Activating WISDOM";
    if (dummy_delay_ == 0) {
        //BOOST_LOG_TRIVIAL(info) << "Powering on WISDOM";
        // TODO: Send POWER_ON command
        BOOST_LOG_TRIVIAL(info) << "Sending SET_TIME command";
        send_udp_command(SET_TIME);
        wait_for_ack();
        BOOST_LOG_TRIVIAL(info) << "Sending SCI_CONFIG command";
        send_udp_command(SCI_CONFIG);
        wait_for_ack();
    }
}

void Wisdom::do_start()
{
    if (worker_.joinable()) {
        worker_.join();
    }
    BOOST_LOG_TRIVIAL(info) << "Start WISDOM measurement";
    if (dummy_delay_ == 0) {
        send_udp_command(SCI_START);
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
    BOOST_LOG_TRIVIAL(info) << "Powering off WISDOM";
    // TODO: Send POWER_OFF command
}

void Wisdom::send_udp_command(const char* command)
{
    if ((sendto(udp_socket, command, CMD_LEN, 0, wisdom_addr->ai_addr, wisdom_addr->ai_addrlen)) != CMD_LEN) {
        throw std::runtime_error("sendto failed with errno: " + errno);
    }
}

void Wisdom::wait_for_ack()
{
    BOOST_LOG_TRIVIAL(info) << "Waiting for ACK";
    struct sockaddr_storage tmp_addr;
    socklen_t tmp_addr_len = sizeof(tmp_addr);
    char ack_buf[4];
    if ((recvfrom(udp_socket, ack_buf, 4 , 0, (struct sockaddr *)&tmp_addr, &tmp_addr_len)) == -1) {
        throw std::runtime_error("recvfrom failed with errno: " + errno);
    }
    BOOST_LOG_TRIVIAL(info) << "ACK received";
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
    wait_for_ack();
    BOOST_LOG_TRIVIAL(info) << "Measurement done";
    set_state(i3ds_asn1::SensorState_standby);
}
