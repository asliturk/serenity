/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
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

#pragma once

#include <AK/CircularQueue.h>
#include <AK/Vector.h>
#include <Kernel/Devices/CharacterDevice.h>

class Console final : public Kernel::CharacterDevice {
    AK_MAKE_ETERNAL
public:
    static Console& the();
    static void initialize();
    static bool is_initialized();

    Console();
    virtual ~Console() override;

    // ^CharacterDevice
    virtual bool can_read(const Kernel::FileDescription&, size_t) const override;
    virtual bool can_write(const Kernel::FileDescription&, size_t) const override { return true; }
    virtual Kernel::KResultOr<size_t> read(Kernel::FileDescription&, size_t, Kernel::UserOrKernelBuffer&, size_t) override;
    virtual Kernel::KResultOr<size_t> write(Kernel::FileDescription&, size_t, const Kernel::UserOrKernelBuffer&, size_t) override;
    virtual const char* class_name() const override { return "Console"; }

    void put_char(char);

    const CircularQueue<char, 16384>& logbuffer() const { return m_logbuffer; }

    // ^Device
    virtual mode_t required_mode() const override { return 0666; }

private:
    CircularQueue<char, 16384> m_logbuffer;
};
