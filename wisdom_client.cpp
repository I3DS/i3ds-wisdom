///////////////////////////////////////////////////////////////////////////\file
///
///   Copyright 2022 SINTEF AS
///
///   This Source Code Form is subject to the terms of the Mozilla
///   Public License, v. 2.0. If a copy of the MPL was not distributed
///   with this file, You can obtain one at https://mozilla.org/MPL/2.0/
///
////////////////////////////////////////////////////////////////////////////////

#include "wisdom_client.hpp"

WisdomClient::WisdomClient(i3ds::Context::Ptr context, i3ds_asn1::NodeID sensor)
    : i3ds::SensorClient(context, sensor) {}

void WisdomClient::set_time()
{
    Wisdom::SetTimeService::Data d;
    Call<Wisdom::SetTimeService>(d);
}

void WisdomClient::load_tables()
{
    Wisdom::LoadTablesService::Data d;
    Call<Wisdom::LoadTablesService>(d);
}