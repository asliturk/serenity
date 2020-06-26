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

#include "XPMLoader.h"
#include <AK/HashMap.h>
#include <AK/LexicalPath.h>
#include <AK/MappedFile.h>
#include <AK/NetworkOrdered.h>
#include <AK/StringBuilder.h>
#include <string.h>

namespace Gfx {

static const u8 XPM2[6] = { '!', ' ', 'X', 'P', 'M', '2' };
static const u8 XPM3[9] = { '/', '*', ' ', 'X', 'P', 'M', ' ', '*', '/' };

struct XPMLoadingContext {
    enum Type {
        Unknown,
        XPM,
        XPM2,
        XPM3
    };

    enum State {
        NotDecoded = 0,
        Error,
        MagicNumber,
        Width,
        Height,
        ColorCount,
        Bitmap,
        Decoded
    };

    Type type { Type::Unknown };
    State state { State::NotDecoded };
    const u8* data { nullptr };
    size_t data_size { 0 };
    u16 width { 0 };
    u16 height { 0 };
    u16 color_count { 0 };
    u16 char_per_pixel { 0 };
    RefPtr<Gfx::Bitmap> bitmap;
};

class Streamer {
public:
    Streamer(const u8* data, size_t size)
        : m_data_ptr(data)
        , m_size_remaining(size)
    {
    }

    template<typename T>
    bool read(T& value)
    {
        if (m_size_remaining < sizeof(T))
            return false;
        value = *((const NetworkOrdered<T>*)m_data_ptr);
        m_data_ptr += sizeof(T);
        m_size_remaining -= sizeof(T);
        return true;
    }

    template<typename T>
    bool read(T* buffer, size_t count)
    {
        auto x = count * sizeof(T);
        if (m_size_remaining < x)
            return false;
        memcpy(buffer, m_data_ptr, x);
        m_data_ptr += x;
        m_size_remaining -= x;
        return true;
    }

    bool at_end() const { return !m_size_remaining; }

    void step_back(size_t size = 1)
    {
        m_data_ptr -= size;
        m_size_remaining += size;
    }

private:
    const u8* m_data_ptr { nullptr };
    size_t m_size_remaining { 0 };
};

ALWAYS_INLINE static Color adjust_color(u16 max_val, Color& color)
{
    color.set_red((color.red() * 255) / max_val);
    color.set_green((color.green() * 255) / max_val);
    color.set_blue((color.blue() * 255) / max_val);

    return color;
}

ALWAYS_INLINE static bool is_wite_space(u8 byte)
{
    if (byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r') {
        return true;
    }
    return false;
}

static bool read_text(Streamer& streamer, String& value)
{
    u8 byte;
    StringBuilder sb;

    while (streamer.read(byte)) {
        if (is_wite_space(byte)) {
            streamer.step_back();
            break;
        }

        sb.append(byte);
    }

    value = sb.to_string();

    return true;
}

static bool read_text_and_skip_it(Streamer& streamer, const String value)
{
    String buffer;
    if (read_text(streamer, buffer)) {
        if (buffer == value)
            return true;
    }

    dbg() << "READ_TEXT_AND_SKIP_IT failed  buffer:[" << buffer << "] value:[" << value << "]";

    return false;
}

static bool read_number(Streamer& streamer, u16* value)
{
    // FIXME use read_text

    u8 byte;
    StringBuilder sb;

    while (streamer.read(byte)) {
        if (is_wite_space(byte)) {
            streamer.step_back();
            break;
        }

        sb.append(byte);
    }

    auto opt_value = sb.to_string().to_uint();
    if (!opt_value.has_value()) {
        return false;
    }

    *value = (u16)opt_value.value();
    return true;
}

static bool read_comment(Streamer& streamer)
{
    bool exist = false;
    u8 byte;

    while (streamer.read(byte)) {
        switch (byte) {
        case '*': {
            streamer.read(byte);
            if (byte == '/') {
                exist = true;
                return exist;
            }
            break;
        }
        default:
            break;
        }
    }

    return exist;
}

static bool read_magic_number(XPMLoadingContext& context, Streamer& streamer)
{
    if (context.state >= XPMLoadingContext::MagicNumber)
        return true;

    if (!context.data || context.data_size < 10) {
        dbg() << "There is no enough data.";
        context.state = XPMLoadingContext::State::Error;
        return false;
    }

    u8 magic_number[9];
    if (!streamer.read(magic_number, 9)) {
        dbg() << "We can't read magic number.";
        context.state = XPMLoadingContext::State::Error;
        return false;
    }

    if (memcmp(context.data, XPM3, sizeof(XPM3)) == 0) {
        context.type = XPMLoadingContext::XPM3;
        context.state = XPMLoadingContext::MagicNumber;
        return true;
    }

    if (memcmp(context.data, XPM2, sizeof(XPM2)) == 0) {
        streamer.step_back(3);

        context.type = XPMLoadingContext::XPM2;
        context.state = XPMLoadingContext::MagicNumber;
        return true;
    }

    if (false) {               // FIXME
        streamer.step_back(9); // FIXME en başa döne bu böyle olmaz :)

        context.type = XPMLoadingContext::XPM;
        context.state = XPMLoadingContext::MagicNumber;
        return true;
    }

    dbg() << "Magic number is not valid:" << (char)magic_number[0] << (char)magic_number[1];
    context.state = XPMLoadingContext::State::Error;
    return false;
}

static bool read_pixel(XPMLoadingContext& context, Streamer& streamer, String& value)
{
    auto count = context.char_per_pixel;
    u8 byte;
    StringBuilder sb;

    while (count != 0) {
        if (!streamer.read(byte)) {
            dbg() << "Stream don't have enought byte for pixel data." << count;
            return false;
        }

        if (is_wite_space(byte)) {
            continue;
        }

        count--;
        sb.append(byte);
    }

    if (count != 0) {
        dbg() << "Yeterince byte yok" << count;
        return false;
    }

    value = sb.to_string();
    return true;
}

static bool read_white_space(Streamer& streamer)
{
    bool exist = false;
    u8 byte;

    dbg() << "WS-----------------------------------------------";
    while (streamer.read(byte)) {
        dbg() << "BYTE:[" << (char)byte << "]";
        if (is_wite_space(byte)) {
            exist = true;
            continue;
        }

        if (byte == '/') {
            streamer.read(byte);
            if (byte == '*') {
                streamer.step_back(2);
                read_comment(streamer);
            }
            break;
        }

        streamer.step_back();
        return exist;
    }

    return exist;
}

static bool read_width(XPMLoadingContext& context, Streamer& streamer)
{
    bool result = read_number(streamer, &context.width);
    if (!result || context.width == 0) {
        return false;
    }

    context.state = XPMLoadingContext::Width;
    return true;
}

static bool read_height(XPMLoadingContext& context, Streamer& streamer)
{
    bool result = read_number(streamer, &context.height);
    if (!result || context.height == 0) {
        return false;
    }

    context.state = XPMLoadingContext::Height;
    return true;
}

static bool read_color_count(XPMLoadingContext& context, Streamer& streamer)
{
    bool result = read_number(streamer, &context.color_count);
    if (!result || context.color_count == 0) {
        dbg() << "We can't parse color count.";
        context.state = XPMLoadingContext::Error;
        return false;
    }

    context.state = XPMLoadingContext::ColorCount;
    return true;
}

static bool read_char_per_pixel(XPMLoadingContext& context, Streamer& streamer)
{
    bool result = read_number(streamer, &context.char_per_pixel);
    if (!result || context.char_per_pixel == 0) {
        dbg() << "We can't parse color count.";
        context.state = XPMLoadingContext::Error;
        return false;
    }

    //    context.state = XPMLoadingContext::ColorCount;    //FIXME:
    return true;
}

static bool read_image_data(XPMLoadingContext& context, Streamer& streamer)
{
    Vector<Gfx::Color> color_data;

    if (context.type == XPMLoadingContext::XPM) {
        ASSERT(false);
    } else if (context.type == XPMLoadingContext::XPM2) {
        // Read color palette.
        String color_key;
        String c;
        String color_name;
        size_t color_index = context.color_count;
        HashMap<String, Gfx::Color> color_palette;

        while (color_index--) {
            read_text(streamer, color_key);
            read_white_space(streamer);
            read_text(streamer, c);
            read_white_space(streamer);
            read_text(streamer, color_name);
            read_white_space(streamer);

            auto color = Color::from_string(color_name);
            if (!color.has_value()) {
                dbg() << "Unknown color name:" << color_name;
                context.state = XPMLoadingContext::State::Error;
                return false;
            }

            color_palette.set(color_key, color.value());
        }

        // Read pixels
        String pixel_key;
        size_t pixel_count = context.width * context.height;
        while (pixel_count > 0) {
            if (!read_pixel(context, streamer, pixel_key)) {
                dbg() << "There is not enought pixel:" << pixel_count;
                context.state = XPMLoadingContext::State::Error;
                return false;
            }

            auto pixel_color = color_palette.get(pixel_key);
            if (!pixel_color.has_value()) {
                dbg() << "Unknown pixel key:" << pixel_key;
                context.state = XPMLoadingContext::State::Error;
                return false;
            }

            color_data.append(pixel_color.value());
            --pixel_count;
        }
        ASSERT(pixel_count == 0);

    } else if (context.type == XPMLoadingContext::XPM3) {
        ASSERT(false);
    }

    context.bitmap = Bitmap::create_purgeable(BitmapFormat::RGB32, { context.width, context.height });

    size_t index = 0;
    for (int y = 0; y < context.height; ++y) {
        for (int x = 0; x < context.width; ++x) {
            Color color = color_data.at(index);
            context.bitmap->set_pixel(x, y, color);
            index++;
        }
    }

    context.state = XPMLoadingContext::State::Bitmap;
    return true;
}

static bool decode_xpm(XPMLoadingContext& context)
{
    if (context.state >= XPMLoadingContext::State::Decoded)
        return true;

    Streamer streamer(context.data, context.data_size);

    if (!read_magic_number(context, streamer))
        return false;

    if (context.type == XPMLoadingContext::Type::XPM2) {
        if (!read_white_space(streamer))
            return false;

        if (!read_width(context, streamer))
            return false;

        if (!read_white_space(streamer))
            return false;

        if (!read_height(context, streamer))
            return false;

        if (!read_white_space(streamer))
            return false;

        if (!read_color_count(context, streamer))
            return false;

        if (!read_white_space(streamer))
            return false;

        if (!read_char_per_pixel(context, streamer))
            return false;

        if (!read_white_space(streamer))
            return false;

    } else if (context.type == XPMLoadingContext::Type::XPM3) {
        dbg() << "------------------------------------------------------------------------------------PM3";

        if (!read_white_space(streamer)) {
            return false;
        }

        if (!read_text_and_skip_it(streamer, "static")) {
            return false;
        }

        if (!read_white_space(streamer)) {
            return false;
        }

        if (!read_text_and_skip_it(streamer, "char")) {
            return false;
        }

        if (!read_white_space(streamer)) {
            return false;
        }

        String buffer;
        if (read_text(streamer, buffer)) {
            if (!buffer.ends_with("[]")) {
                return false;
            }
        }

        read_white_space(streamer); // skip if exist

        if (!read_text_and_skip_it(streamer, "=")) {
            return false;
        }

        read_white_space(streamer); // skip if exist

        if (!read_text_and_skip_it(streamer, "{")) {
            return false;
        }




        dbg() << "3_____________________________________________________DONE";
    } else {
        dbg() << "Unknown XPM file type.";
        return false;
    }

    if (!read_image_data(context, streamer))
        return false;

    context.state = XPMLoadingContext::State::Decoded;
    return true;
}

static RefPtr<Gfx::Bitmap> load_xpm_impl(const u8* data, size_t data_size)
{
    XPMLoadingContext context;
    context.data = data;
    context.data_size = data_size;

    if (!decode_xpm(context))
        return nullptr;

    return context.bitmap;
}

RefPtr<Gfx::Bitmap> load_xpm(const StringView& path)
{
    MappedFile mapped_file(path);
    if (!mapped_file.is_valid()) {
        return nullptr;
    }

    auto bitmap = load_xpm_impl((const u8*)mapped_file.data(), mapped_file.size());
    if (bitmap)
        bitmap->set_mmap_name(String::format("Gfx::Bitmap [%dx%d] - Decoded XPM: %s", bitmap->width(), bitmap->height(), LexicalPath::canonicalized_path(path).characters()));
    return bitmap;
}

RefPtr<Gfx::Bitmap> load_xpm_from_memory(const u8* data, size_t length)
{
    auto bitmap = load_xpm_impl(data, length);
    if (bitmap)
        bitmap->set_mmap_name(String::format("Gfx::Bitmap [%dx%d] - Decoded XPM: <memory>", bitmap->width(), bitmap->height()));
    return bitmap;
}

XPMImageDecoderPlugin::XPMImageDecoderPlugin(const u8* data, size_t size)
{
    m_context = make<XPMLoadingContext>();
    m_context->data = data;
    m_context->data_size = size;
}

XPMImageDecoderPlugin::~XPMImageDecoderPlugin()
{
}

IntSize XPMImageDecoderPlugin::size()
{
    if (m_context->state == XPMLoadingContext::State::Error)
        return {};

    if (m_context->state < XPMLoadingContext::State::Decoded) {
        bool success = decode_xpm(*m_context);
        if (!success)
            return {};
    }

    return { m_context->width, m_context->height };
}

RefPtr<Gfx::Bitmap> XPMImageDecoderPlugin::bitmap()
{
    if (m_context->state == XPMLoadingContext::State::Error)
        return nullptr;

    if (m_context->state < XPMLoadingContext::State::Decoded) {
        bool success = decode_xpm(*m_context);
        if (!success)
            return nullptr;
    }

    ASSERT(m_context->bitmap);
    return m_context->bitmap;
}

void XPMImageDecoderPlugin::set_volatile()
{
    if (m_context->bitmap)
        m_context->bitmap->set_volatile();
}

bool XPMImageDecoderPlugin::set_nonvolatile()
{
    if (!m_context->bitmap)
        return false;

    return m_context->bitmap->set_nonvolatile();
}

bool XPMImageDecoderPlugin::sniff()
{
    ASSERT(false);

    if (m_context->data_size < 2)
        return false;

    if (m_context->data[0] == 'P' && m_context->data[1] == '2')
        return true;

    if (m_context->data[0] == 'P' && m_context->data[1] == '5')
        return true;

    return false;
}

bool XPMImageDecoderPlugin::is_animated()
{
    return false;
}

size_t XPMImageDecoderPlugin::loop_count()
{
    return 0;
}

size_t XPMImageDecoderPlugin::frame_count()
{
    return 1;
}

ImageFrameDescriptor XPMImageDecoderPlugin::frame(size_t i)
{
    if (i > 0) {
        return { bitmap(), 0 };
    }

    return {};
}
}
