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
#include <AK/HashMap.h>
#include <AK/URL.h>
#include <DialogServer/DialogClientEndpoint.h>
#include <LibGUI/AboutDialog.h>
#include <LibGfx/Bitmap.h>

namespace DialogServer {

static HashMap<int, RefPtr<ClientConnection>> s_connections;
ClientConnection::ClientConnection(Core::LocalSocket& client_socket, int client_id)
    : IPC::ClientConnection<DialogServerEndpoint>(*this, client_socket, client_id)
{
    s_connections.set(client_id, *this);
}

ClientConnection::~ClientConnection()
{
}

void ClientConnection::die()
{
    s_connections.remove(client_id());
}

OwnPtr<Messages::DialogServer::GreetResponse> ClientConnection::handle(const Messages::DialogServer::Greet&)
{
    return make<Messages::DialogServer::GreetResponse>(client_id());
}

OwnPtr<Messages::DialogServer::AboutDialogResponse> ClientConnection::handle(const Messages::DialogServer::AboutDialog& message)
{
    dbg() << "AboutDialog !!!!!! Name:" << message.name() << ", icon_path" << message.icon_path();

    GUI::AboutDialog::show(message.name(), nullptr, nullptr, Gfx::Bitmap::load_from_file(message.icon_path()));
    return make<Messages::DialogServer::AboutDialogResponse>(client_id());
}

}
