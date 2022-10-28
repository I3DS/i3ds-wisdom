///////////////////////////////////////////////////////////////////////////\file
///
///   Copyright 2022 SINTEF AS
///
///   This Source Code Form is subject to the terms of the Mozilla
///   Public License, v. 2.0. If a copy of the MPL was not distributed
///   with this file, You can obtain one at https://mozilla.org/MPL/2.0/
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __WISDOM_CLIENT_HPP
#define __WISDOM_CLIENT_HPP

#include <i3ds/sensor_client.hpp>
#include <vector>
#include "wisdom_i3ds_wrapper.hpp"


class WisdomClient : public i3ds::SensorClient
{
public:

    typedef std::shared_ptr<WisdomClient> Ptr;
    static Ptr Create(i3ds::Context::Ptr context, i3ds_asn1::NodeID id)
    {
        return std::make_shared<WisdomClient>(context, id);
    }

    WisdomClient(i3ds::Context::Ptr context, i3ds_asn1::NodeID sensor);

    void set_time();
    void load_tables();
    void table_select(const std::vector<bool>& tables);
};


#endif
