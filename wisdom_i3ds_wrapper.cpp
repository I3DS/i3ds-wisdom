#include "wisdom_i3ds_wrapper.hpp"

#ifndef BOOST_LOG_DYN_LINK
#define BOOST_LOG_DYN_LINK
#endif

#include <boost/log/trivial.hpp>

#include <i3ds/exception.hpp>

#include <thread>
#include <chrono>

Wisdom::Wisdom(i3ds_asn1::NodeID node, unsigned int dummy_delay) :
    Sensor(node),
    dummy_delay_(dummy_delay)
{
    set_device_name("WISDOM GPR");
}

// Setting sampling rate will never be relevant so we always return true
bool Wisdom::is_sampling_supported(i3ds_asn1::SampleCommand sample)
{
    return true;
}

void Wisdom::Attach(i3ds::Server& server)
{
    Sensor::Attach(server);
}

void Wisdom::do_activate()
{
    BOOST_LOG_TRIVIAL(info) << "Powering on WISDOM";
    // TODO: Send POWER_ON command
}

void Wisdom::do_start()
{
    if (worker_.joinable()) {
        worker_.join();
    }
    BOOST_LOG_TRIVIAL(info) << "Start WISDOM measurement";
    if (dummy_delay_ == 0) {
        // TODO: Send START_MEASUREMENT command
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

void Wisdom::dummy_wait_for_measurement_to_finish()
{
    BOOST_LOG_TRIVIAL(info) << "Starting dummy measurement";
    std::this_thread::sleep_for(std::chrono::seconds(dummy_delay_));
    BOOST_LOG_TRIVIAL(info) << "Dummy measurement done";
    set_state(i3ds_asn1::SensorState_standby);
}

void Wisdom::wait_for_measurement_to_finish()
{
    while(!measurement_finished())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    BOOST_LOG_TRIVIAL(info) << "Measurement done";
    set_state(i3ds_asn1::SensorState_standby);
}

bool Wisdom::measurement_finished()
{
    // TODO: Ask GPR if measurement is finished
    return true;
}
