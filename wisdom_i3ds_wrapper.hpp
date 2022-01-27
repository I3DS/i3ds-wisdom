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
        Wisdom(i3ds_asn1::NodeID node, unsigned int dummy_delay = 0);

        // Destructor
        virtual ~Wisdom() = default;

        // Returns true if sample configuration is supported.
        virtual bool is_sampling_supported(i3ds_asn1::SampleCommand sample);

        virtual void Attach(i3ds::Server& server);

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

        void dummy_wait_for_measurement_to_finish();
        void wait_for_measurement_to_finish();
        bool measurement_finished();

        const unsigned int dummy_delay_;

        // Worker thread.
        std::thread worker_;

};


#endif
