/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 *
 */

#include "anbox/cmds/wait_ready.h"

#ifdef USE_OLD_DBUS
#include "anbox/dbus-old/stub/application_manager.h"
#else
#include "anbox/dbus/application_manager_client.h"
#include "anbox/dbus/bus.h"
#include "anbox/dbus/interface.h"
#endif

namespace {
constexpr const unsigned int max_wait_attempts{30};
}

anbox::cmds::WaitReady::WaitReady()
    : CommandWithFlagsAndAction{
          cli::Name{"wait-ready"}, cli::Usage{"wait-ready"},
          cli::Description{"Wait until the Android system has successfully booted"}} {

  flag(cli::make_flag(cli::Name{"use-system-dbus"},
                      cli::Description{"Use system instead of session DBus"},
                      use_system_dbus_));

  action([this](const cli::Command::Context&) {
    auto bus_type = anbox::dbus::Bus::Type::Session;
    if (use_system_dbus_)
      bus_type = anbox::dbus::Bus::Type::System;
    auto bus = std::make_shared<anbox::dbus::Bus>(bus_type);

#ifdef USE_OLD_DBUS
    auto stub = dbus::stub::ApplicationManager::create_for_bus(bus);
#else
    auto connection = use_system_dbus_
                          ? sdbus::createSystemBusConnection()
                          : sdbus::createSessionBusConnection();
    ApplicationManagerClient client(*connection, dbus::interface::Service::name(), dbus::interface::Service::path());
#endif

    unsigned int n = 0;
    while (n < max_wait_attempts) {
#ifdef USE_OLD_DBUS
    stub->update_properties();
      if (stub->ready().get())
#else
      if (client.Ready())
#endif
        return EXIT_SUCCESS;

      std::this_thread::sleep_for(std::chrono::seconds{1});
      n++;
    }

    return EXIT_FAILURE;
  });
}
