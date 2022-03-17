
///////////////////////////////////////////////////////////////////////////\file
///
///   Copyright 2022 SINTEF AS
///
///   This Source Code Form is subject to the terms of the Mozilla
///   Public License, v. 2.0. If a copy of the MPL was not distributed
///   with this file, You can obtain one at https://mozilla.org/MPL/2.0/
///
////////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <signal.h>

#include "wisdom_i3ds_wrapper.hpp"

#include <boost/program_options.hpp>

#include <i3ds/configurator.hpp>
#include "i3ds/communication.hpp"

#ifndef BOOST_LOG_DYN_LINK
#define BOOST_LOG_DYN_LINK
#endif

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

namespace po = boost::program_options;

std::atomic<bool> running;

void signal_handler(int)
{
    running = false;
}

int main(int argc, char *argv[])
{
    unsigned int node_id;
    unsigned int dummy_delay;
    std::string port;
    std::string ip;
    i3ds::Configurator configurator;

    po::options_description desc("Allowed WISDOM options");
    configurator.add_common_options(desc);

    desc.add_options()
    ("node,n", po::value<unsigned int>(&node_id)->default_value(10), "Node ID of sensor")
    ("dummy-delay,d", po::value<unsigned int>(&dummy_delay)->default_value(0), "Set to a value > 0 to run in dummy mode.")
    ("port,p", po::value<std::string>(&port)->default_value(""), "Port number of Wisdom server. Ignored if run in dummy mode")
    ("ip,i", po::value<std::string>(&ip)->default_value("127.0.0.1"), "IP address of WISDOM server");
    po::variables_map vm = configurator.parse_common_options(desc, argc, argv);

    if (dummy_delay != 0) {
        BOOST_LOG_TRIVIAL(info) << "Running in dummy mode";
    }
    BOOST_LOG_TRIVIAL(info) << "Node ID: " << node_id;
    i3ds::Context::Ptr context(i3ds::Context::Create());
    i3ds::Server server(context);
    Wisdom wisdom(node_id, dummy_delay, port);

    running = true;
    signal(SIGINT, signal_handler);

    wisdom.Attach(server);
    server.Start();

    while(running)
    {
        sleep(1);
    }

    wisdom.Stop();
    server.Stop();
    return 0;
}
