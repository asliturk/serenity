/*
 * Copyright (c) 2020, Hüseyin Aslıtürk <asliturk@hotmail.com>
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

#include "KeyboardMap.h"
#include <AK/JsonArray.h>
#include <AK/JsonObject.h>
#include <AK/JsonValue.h>
#include <LibCore/File.h>

KeyboardMapper::KeyboardMap KeyboardMapper::load_map()
{
    String file_name = "/res/keyboard-layout.json";

    auto file = Core::File::construct(file_name);
    file->open(Core::IODevice::ReadOnly);
    if (!file->is_open()) {
        dbg() << "Failed to open " << file_name << ":" << file->error_string();
        ASSERT(file->is_open());
    }

    auto file_contents = file->read_all();
    auto json_result = JsonValue::from_string(file_contents);
    ASSERT(json_result.has_value());
    auto json_map_object = json_result.value().as_object();

    KeyboardMap keyboard_map;
    keyboard_map.name = json_map_object.get("Name").as_string();
    keyboard_map.width = json_map_object.get("Width").as_u32();
    keyboard_map.height = json_map_object.get("Height").as_u32();

    auto json_keys_array = json_map_object.get("Keys").as_array();

    json_keys_array.for_each([&](auto& value) {
        auto fields = value.as_array();

        KeyPosition key_position;
        key_position.scancode = fields[0].as_string().to_uint().value_or(0);
        key_position.x = fields[1].as_u32();
        key_position.y = fields[2].as_u32();
        key_position.width = fields[3].as_u32();
        key_position.height = fields[4].as_u32();
        key_position.enabled = fields[5].as_bool();
        key_position.map_index = fields[6].as_u32();
        key_position.name = fields[7].as_string();

        keyboard_map.keys.append(key_position);
    });

    return keyboard_map;
}
