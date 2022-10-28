///////////////////////////////////////////////////////////////////////////\file
///
///   Copyright 2022 SINTEF AS
///
///   This Source Code Form is subject to the terms of the Mozilla
///   Public License, v. 2.0. If a copy of the MPL was not distributed
///   with this file, You can obtain one at https://mozilla.org/MPL/2.0/
///
////////////////////////////////////////////////////////////////////////////////

#include <boost/program_options/value_semantic.hpp>
#include <iostream>
#include <cstdlib>

#include "wisdom_client.hpp"
#include <i3ds/configurator.hpp>

#include <boost/program_options.hpp>
#include <vector>

#ifndef BOOST_LOG_DYN_LINK
#define BOOST_LOG_DYN_LINK
#endif


#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    i3ds::SensorConfigurator configurator;

    po::options_description desc("Allowed Wisdom GPR control options");

    std::vector<bool> tables;

    configurator.add_common_options(desc);
    desc.add_options()
    ("print,p", "Print the ToF camera configuration")
    ("set-time,s", "Send SET_TIME command")
    ("load-tables,l", "Load parameter tables into Wisdom")
    ("set-tables", po::value<std::vector<bool>>(&tables)->multitoken(), "Set which tables to use. Ex 1 0 1 1")
    ;

    po::variables_map vm = configurator.parse_common_options(desc, argc, argv);

    i3ds::Context::Ptr context(i3ds::Context::Create());

    BOOST_LOG_TRIVIAL(info) << "Connecting to Wisdom with node ID: " << configurator.node_id;
    WisdomClient wisdom(context, configurator.node_id);
    BOOST_LOG_TRIVIAL(trace) << "---> [OK]";

    configurator.handle_sensor_commands(vm, wisdom);

    if (vm.count("set-time")) {
        wisdom.set_time();
    }
    
    if (vm.count("load-tables")) {
        wisdom.load_tables();
    }
    
    if (vm.count("print")) {
      configurator.print_sensor_status(wisdom);
    }

    if (vm.count("set-tables")) {
      wisdom.table_select(tables);
    }

  return 0;
}
