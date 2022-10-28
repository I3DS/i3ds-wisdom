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
#include "wisdom_i3ds_wrapper.hpp"
#include <algorithm>
#include <stdexcept>

WisdomClient::WisdomClient(i3ds::Context::Ptr context, i3ds_asn1::NodeID sensor)
    : i3ds::SensorClient(context, sensor) {}

void WisdomClient::set_time()
{
    Wisdom::SetTimeService::Data d;
    Wisdom::SetTimeService::Initialize(d);
    Call<Wisdom::SetTimeService>(d);
}

void WisdomClient::load_tables()
{
    Wisdom::LoadTablesService::Data d;
    Wisdom::LoadTablesService::Initialize(d);
    Call<Wisdom::LoadTablesService>(d);
}

void WisdomClient::table_select(const std::vector<bool>& tables)
{
    if (tables.size() > 40) {
        throw std::invalid_argument("Cannot send more than 40 values");
    }
    Wisdom::TableSelectService::Data d;
    Wisdom::TableSelectService::Initialize(d);
    for (int i = 0; i < tables.size(); i++) {
        d.request.arr[i] = tables[i];
    }
    d.request.nCount = tables.size();
    Call<Wisdom::TableSelectService>(d);
}