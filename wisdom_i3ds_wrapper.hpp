///////////////////////////////////////////////////////////////////////////\file
///
///   Copyright 2022 SINTEF AS
///
///   This Source Code Form is subject to the terms of the Mozilla
///   Public License, v. 2.0. If a copy of the MPL was not distributed
///   with this file, You can obtain one at https://mozilla.org/MPL/2.0/
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __WISDOM_WRAPPER_HPP
#define __WISDOM_WRAPPER_HPP

#include <i3ds/sensor.hpp>
#include <i3ds/server.hpp>
#include <i3ds/service.hpp>
#include <i3ds/codec.hpp>

class Wisdom : public i3ds::Sensor
{

    public:

        typedef i3ds::Command<16, i3ds::NullCodec> SetTimeService;
        typedef i3ds::Command<17, i3ds::NullCodec> LoadTablesService;
        typedef i3ds::Command<19, i3ds::T_StringCodec> TableSelectService; 

        // Constructor
        Wisdom(i3ds_asn1::NodeID node, unsigned int dummy_delay = 0, std::string uart_dev = "", 
               std::string port = "", std::string ip = "127.0.0.1");

        // Destructor
        virtual ~Wisdom();

        // Returns true if sample configuration is supported.
        virtual bool is_sampling_supported(i3ds_asn1::SampleCommand sample);

        virtual void Attach(i3ds::Server& server);

        // Stop listening for ACK. Call this before stopping the attached server.
        void Stop();

    protected:

        // Action when activated.
        virtual void do_activate();

        // Action when started.
        virtual void do_start();

        // Action when activated.
        virtual void do_stop();

        // Action when activated.
        virtual void do_deactivate();

    private:

        // UDP communication functions
        void make_sci_config_cmd(char* buf, unsigned char table_number);
        void make_sci_start_cmd(char* buf, unsigned char table_number);
        void send_udp_command(const char* command);
        void wait_for_ack(const char expected_byte);

        void dummy_wait_for_measurement_to_finish();
        void wait_for_measurement_to_finish();

        // Command handlers
        void handle_set_time(SetTimeService::Data);
        void handle_load_tables(LoadTablesService::Data);
        void handle_table_select(TableSelectService::Data);

        void set_time();
        void load_tables();

        bool send_serial_cmd(const char* cmd, const size_t cmd_len, const char* expected_ack);
        bool power_on();
        bool power_off();

        // Number of seconds to wait for dummy measurement. Will be 0 if 
        // real commands are to be sent
        const unsigned int dummy_delay_;

        // Worker thread.
        std::thread worker_;

        // Networking structs
        int udp_socket_;
        struct addrinfo *wisdom_addr_;

        // UDP message commands
        static const unsigned int CMD_LEN = 4;
        const char SCI_CONFIG[CMD_LEN] = {1, 0, 0, 0};
        const char HK_REQUEST[CMD_LEN] = {2, 0, 0, 0};
        const char SCI_START[CMD_LEN] = {3, 0, 0, 3};
        const char SCI_REQUEST[CMD_LEN] = {4, 0, 0, 0};
        const char SET_TIME[CMD_LEN] = {7, 0, 0, 0};

        // Flags for which parameter tables to use
        static const unsigned int N_TABLES = 4;
        bool active_tables_[N_TABLES] = {true, true, true, true};

        std::atomic<bool> running_;

        // Serial communication
        int serial_port_;
        const unsigned int serial_retries_ = 5;
        const char power_on_cmd_ = '1';
        const char power_on_ack_[6] = "0x01\n";
        const char power_off_cmd_ = '1';
        const char power_off_ack_[6] = "0x00\n";

};


#endif
