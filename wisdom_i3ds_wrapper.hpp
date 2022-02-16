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

class Wisdom : public i3ds::Sensor
{

    public:

        // Constructor
        Wisdom(i3ds_asn1::NodeID node, unsigned int dummy_delay = 0, std::string port = "");

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
        void send_udp_command(const char* command);
        void wait_for_ack();

        void dummy_wait_for_measurement_to_finish();
        void wait_for_measurement_to_finish();

        const unsigned int dummy_delay_;

        // Worker thread.
        std::thread worker_;

        // Networking structs
        int udp_socket;
        struct addrinfo *wisdom_addr;

        // UDP message commands
        static const unsigned int CMD_LEN = 4;
        const char SCI_CONFIG[CMD_LEN] = {1, 0, 0, 0};
        const char HK_REQUEST[CMD_LEN] = {2, 0, 0, 0};
        const char SCI_START[CMD_LEN] = {3, 0, 0, 3};
        const char SCI_REQUEST[CMD_LEN] = {4, 0, 0, 0};
        const char SET_TIME[CMD_LEN] = {7, 0, 0, 0};

        std::atomic<bool> running_;
};


#endif
