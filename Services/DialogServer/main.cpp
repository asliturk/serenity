/*
 * Copyright (c) 2020, Hüseyin ASLITÜRK <asliturk@hotmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ClientConnection.h"
#include <LibCore/ConfigFile.h>
#include <LibCore/EventLoop.h>
#include <LibCore/LocalServer.h>
#include <stdio.h>
#include <unistd.h>
#include <LibGUI/Application.h>

int main(int argc, char** argv)
{


    dbg() << "MAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    GUI::Application app(argc, argv);

    Core::EventLoop event_loop;
    auto server = Core::LocalServer::construct();

    if (pledge("stdio shared_buffer accept rpath wpath cpath unix fattr", nullptr) < 0) {
        perror("pledge");
        return 1;
    }

    bool ok = server->take_over_from_system_server();
    ASSERT(ok);
    server->on_ready_to_accept = [&] {
        auto client_socket = server->accept();
        if (!client_socket) {
            dbg() << "LaunchServer: accept failed.";
            return;
        }
        static int s_next_client_id = 0;
        int client_id = ++s_next_client_id;
        dbg() << "Received connection";
        IPC::new_client_connection<DialogServer::ClientConnection>(*client_socket, client_id);
    };

    return event_loop.exec();
}
