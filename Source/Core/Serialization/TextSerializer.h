#pragma once

#include <concepts>
#include <iostream>

#include <Core/Enumerate.h>
#include <Core/ScopeGuard.h>
#include <Core/Serialization/Serialize.h>
#include <Core/TypeList.h>
#include <Core/Variant.h>

RTRC_BEGIN

template<typename T>
struct TextSerializer_Custom;

class TextTokenWriter
{
public:

    TextTokenWriter &Write(std::string_view content)
    {
#if RTRC_DEBUG
        for(char c : content)
        {
            if(std::isspace(c))
            {
                throw Exception("Space character cannot be written by TextTokenWriter");
            }
        }
#endif
        writer_("{} ", content);
        return *this;
    }

    TextTokenWriter &WriteString(std::string_view content)
    {
        std::string actualContent;
        for(char c : content)
        {
            if(c == '\\')
            {
                actualContent += "\\\\";
            }
            else if(c == '"')
            {
                actualContent += "\\\"";
            }
            else
            {
                actualContent += c;
            }
        }
        writer_("\"" + actualContent + "\"");
        return *this;
    }

    SourceWriter &GetInternalWriter()
    {
        return writer_;
    }

    TextTokenWriter &NewLine()
    {
        writer_.NewLine();
        return *this;
    }

    TextTokenWriter &operator++()
    {
        ++writer_;
        return *this;
    }

    TextTokenWriter &operator--()
    {
        --writer_;
        return *this;
    }

    std::string ResolveResult(std::string_view indentUnit = "  ") const
    {
        return writer_.ResolveResult(indentUnit);
    }

private:

    SourceWriter writer_;
};

class TextTokenReader
{
public:

    void SetSource(std::string source)
    {
        source_ = std::move(source);
        pos_ = 0;
        SkipSpaces();
    }

    bool IsFinished() const
    {
        return pos_ >= source_.size();
    }

    std::string NextToken()
    {
        if(IsFinished())
        {
            return "";
        }

        RTRC_SCOPE_EXIT{ SkipSpaces(); };

        // Regular token
        if(source_[pos_] != '"')
        {
            size_t end = pos_ + 1;
            while(end < source_.size() && !std::isspace(source_[end]))
            {
                ++end;
            }
            std::string ret = source_.substr(pos_, end - pos_);
            pos_ = end;
            return ret;
        }

        // String token
        size_t end = pos_ + 1;
        while(true)
        {
            if(end >= source_.size())
            {
                throw Exception("TextTokenReader: end of string expected");
            }

            if(source_[end] == '\\')
            {
                assert(end + 1 < source_.size());
                end += 2;
                continue;
            }

            if(source_[end] == '"')
            {
                ++end;
                break;
            }

            ++end;
        }

        // [pos, end) is "..."
        std::string_view tret = std::string_view(source_).substr(pos_ + 1, end - pos_ - 2);
        pos_ = end;

        std::string ret;
        for(size_t i = 0; i < tret.size(); ++i)
        {
            if(tret[i] == '\\')
            {
                ret += tret[i + 1];
                ++i;
            }
            else
            {
                ret += tret[i];
            }
        }
        return ret;
    }

private:

    void SkipSpaces()
    {
        while(pos_ < source_.size() && std::isspace(source_[pos_]))
        {
            ++pos_;
        }
    }

    std::string source_;
    size_t pos_ = 0;
};

class TextSerializer
{
public:

    template<typename T>
    void operator()(const T &value, std::string_view name)
    {
        if(!name.empty())
        {
            writer_.GetInternalWriter()("{} = ", name);
        }
        TextSerializer_Custom<T>::Serialize(value, *this);
    }

    TextTokenWriter &GetWriter()
    {
        return writer_;
    }

    template<typename T>
    void SerializeImpl(const T &value)
    {
        if constexpr(std::is_scalar_v<T>)
        {
            static_assert(AlwaysFalse<T>, "Unsupported by Rtrc::TextSerializer");
        }
        else
        {
            cista::for_each_field(value, [&](auto &&mem) { (*this)(mem, "?"); });
        }
    }

    std::string ResolveResult(std::string_view indentUnit = "  ") const
    {
        return writer_.ResolveResult(indentUnit);
    }

private:

    TextTokenWriter writer_;
};

class TextDeserializer
{
public:

    void SetSource(std::string source)
    {
        reader_.SetSource(std::move(source));
    }

    template<typename T>
    void operator()(T &value, std::string_view name)
    {
        if(!name.empty())
        {
            const auto actualName = reader_.NextToken();
            if(actualName != name)
            {
                throw Exception(fmt::format("TextDeserializer: unmatched name. Given: {}, read: {}", name, actualName));
            }
            if(reader_.NextToken() != "=")
            {
                throw Exception("TextDeserializer: '=' expected");
            }
        }
        TextSerializer_Custom<T>::Deserialize(value, *this);
    }

    TextTokenReader &GetReader()
    {
        return reader_;
    }

    template<typename T>
    void SerializeImpl(T &value)
    {
        if constexpr(std::is_scalar_v<T>)
        {
            static_assert(AlwaysFalse<T>, "Unsupported by Rtrc::TextDeserializer");
        }
        else
        {
            cista::for_each_field(value, [&](auto &&mem) { (*this)(mem, "?"); });
        }
    }

private:

    TextTokenReader reader_;
};

template<typename T>
struct TextSerializer_Custom
{
    static void Serialize(const T &value, TextSerializer &ser)
    {
        auto &writer = ser.GetWriter();
        writer.Write("{").NewLine();
        ++writer;
        if constexpr(requires{ value.Serialize(ser); })
        {
            value.Serialize(ser);
        }
        else
        {
            ser.SerializeImpl(value);
        }
        --writer;
        writer.Write("}").NewLine();
    }

    static void Deserialize(T &value, TextDeserializer &ser)
    {
        auto &reader = ser.GetReader();
        [[maybe_unused]] std::string t = reader.NextToken();
        assert(t == "{");
        if constexpr(requires{ value.Serialize(ser); })
        {
            value.Serialize(ser);
        }
        else
        {
            ser.SerializeImpl(value);
        }
        t = reader.NextToken();
        assert(t == "}");
    }
};

template<typename T> requires std::integral<T> || std::floating_point<T>
struct TextSerializer_Custom<T>
{
    static void Serialize(T value, TextSerializer &ser)
    {
        if constexpr(std::floating_point<T>)
        {
            constexpr std::string_view HEX_MAP = "0123456789abcdef";
            char result[sizeof(T) * 2 + 1];
            for(size_t i = 0; i < sizeof(T); ++i)
            {
                const unsigned int byte = *(reinterpret_cast<const unsigned char *>(&value) + i);
                result[2 * i + 0] = HEX_MAP[byte & 0x0f];
                result[2 * i + 1] = HEX_MAP[byte >> 4];
            }
            result[sizeof(T) * 2] = '\0';
            ser.GetWriter().Write(result).NewLine();
        }
        else
        {
            ser.GetWriter().Write(std::to_string(value)).NewLine();
        }
    }

    static void Deserialize(T &value, TextDeserializer &ser)
    {
        const std::string token(ser.GetReader().NextToken());
        if constexpr(std::floating_point<T>) // Endianness is assumed to be consistent
        {
            assert(token.size() == 2 * sizeof(T));
            auto ConvertHalfByte = [](char c) { return c >= 'a' ? (10 + c - 'a') : (c - '0'); };
            for(size_t i = 0; i < sizeof(T); ++i)
            {
                const unsigned int byteLower = ConvertHalfByte(token[2 * i]);
                const unsigned int byteUpper = ConvertHalfByte(token[2 * i + 1]);
                const unsigned int byte = byteLower | (byteUpper << 4);
                unsigned char *outByte = reinterpret_cast<unsigned char *>(&value) + i;
                *outByte = byte;
            }
        }
        else if constexpr(std::signed_integral<T>)
        {
            value = static_cast<T>(std::stoll(token));
        }
        else // unsigned integral
        {
            static_assert(std::unsigned_integral<T>);
            value = std::stoull(token);
        }
    }
};

template<typename T>
struct TextSerializer_Custom<std::vector<T>>
{
    static void Serialize(const std::vector<T> &value, TextSerializer &ser)
    {
        auto &writer = ser.GetWriter();
        writer.Write(std::to_string(value.size()));
        writer.Write("[").NewLine();
        ++writer;
        if constexpr(std::is_same_v<T, bool>)
        {
            for(bool elem : value)
            {
                ser(elem, "");
            }
        }
        else
        {
            for(auto &elem : value)
            {
                ser(elem, "");
            }
        }
        --writer;
        writer.Write("]").NewLine();
    }

    static void Deserialize(std::vector<T> &value, TextDeserializer &ser)
    {
        auto &reader = ser.GetReader();
        const size_t size = std::stoull(std::string(reader.NextToken()));
        value.resize(size);
        reader.NextToken(); // Skip [
        if constexpr(std::is_same_v<T, bool>)
        {
            for(size_t i = 0; i < size; ++i)
            {
                bool elem = false;
                ser(elem, "");
                value[i] = elem;
            }
        }
        else
        {
            for(auto &elem : value)
            {
                ser(elem, "");
            }
        }
        reader.NextToken(); // Skip ]
    }
};

template<>
struct TextSerializer_Custom<std::string>
{
    static void Serialize(const std::string &value, TextSerializer &ser)
    {
        ser.GetWriter().WriteString(value).NewLine();
    }

    static void Deserialize(std::string &value, TextDeserializer &ser)
    {
        value = std::string(ser.GetReader().NextToken());
    }
};

template<typename T> requires std::is_enum_v<T>
struct TextSerializer_Custom<T>
{
    static void Serialize(T value, TextSerializer &ser)
    {
        ser.GetWriter().Write(std::to_string(std::to_underlying(value))).NewLine();
    }

    static void Deserialize(T &value, TextDeserializer &ser)
    {
        if constexpr(std::is_signed_v<std::underlying_type_t<T>>)
        {
            value = static_cast<T>(std::stoll(std::string(ser.GetReader().NextToken())));
        }
        else
        {
            value = static_cast<T>(std::stoull(std::string(ser.GetReader().NextToken())));
        }
    }
};

template<typename T>
struct TextSerializer_Custom<std::optional<T>>
{
    static void Serialize(const std::optional<T> &value, TextSerializer &ser)
    {
        if(value.has_value())
        {
            ser.GetWriter().Write("+");
            ser(*value, "std::optional::internal");
        }
        else
        {
            ser.GetWriter().Write("-").NewLine();
        }
    }

    static void Deserialize(std::optional<T> &value, TextDeserializer &ser)
    {
        if(ser.GetReader().NextToken() == "+")
        {
            T actualValue;
            ser(actualValue, "std::optional::internal");
            value = std::move(actualValue);
        }
        else
        {
            value = std::nullopt;
        }
    }
};

template<typename K, typename V, typename L>
struct TextSerializer_Custom<std::map<K, V, L>>
{
    static void Serialize(const std::map<K, V, L> &value, TextSerializer &ser)
    {
        auto &writer = ser.GetWriter();
        writer.Write(std::to_string(value.size()));
        writer.Write("{").NewLine();
        ++writer;
        for(auto &[k, v] : value)
        {
            writer.Write("{").NewLine();
            ++writer;
            ser(k, "key");
            ser(v, "value");
            --writer;
            writer.Write("}").NewLine();
        }
        --writer;
        writer.Write("}").NewLine();
    }

    static void Deserialize(std::map<K, V, L> &value, TextDeserializer &ser)
    {
        value.clear();
        auto &reader = ser.GetReader();
        const size_t size = std::stoul(reader.NextToken());
        reader.NextToken();
        for(size_t i = 0; i < size; ++i)
        {
            reader.NextToken();
            K k; V v;
            ser(k, "key");
            ser(v, "value");
            value.insert({ std::move(k), std::move(v) });
            reader.NextToken();
        }
        reader.NextToken();
    }
};

template<typename...Types>
struct TextSerializer_Custom<Variant<Types...>>
{
    static void Serialize(const Variant<Types...> &value, TextSerializer &ser)
    {
        ser.GetWriter().Write(std::to_string(value.index()));
        TextSerializer_Custom::WriteElem(value, ser, std::make_integer_sequence<int, TypeList<Types...>::Size>());
    }

    static void Deserialize(Variant<Types...> &value, TextDeserializer &ser)
    {
        const int index = std::stoi(ser.GetReader().NextToken());
        TextSerializer_Custom::ReadElem(index, value, ser, std::make_integer_sequence<int, TypeList<Types...>::Size>());
    }

    template<int...Is>
    static void WriteElem(const Variant<Types...> &value, TextSerializer &ser, std::integer_sequence<int, Is...>)
    {
        (TextSerializer_Custom::WriteElem<Is>(value, ser), ...);
    }

    template<int I>
    static void WriteElem(const Variant<Types...> &value, TextSerializer &ser)
    {
        if(I == value.index())
        {
            auto &internalValue = std::get<I>(value);
            ser(internalValue, "internalValue");
        }
    }

    template<int...Is>
    static void ReadElem(int index, Variant<Types...> &value, TextDeserializer &ser, std::integer_sequence<int, Is...>)
    {
        (TextSerializer_Custom::ReadElem<Is>(index, value, ser), ...);
    }

    template<int I>
    static void ReadElem(int index, Variant<Types...> &value, TextDeserializer &ser)
    {
        if(I == index)
        {
            using Type = typename TypeList<Types...>::template At<I>;
            value = Type();
            Type &internalValue = std::get<I>(value);
            ser(internalValue, "internalValue");
        }
    }
};

template<bool PrintWhenOk = false, bool PrintWhenFail = true, typename T>
bool CheckTextSerializationAndDeserialization(const T &value)
{
    TextSerializer ser;
    ser(value, "");
    const std::string valueStr = ser.ResolveResult();

    TextDeserializer deser;
    deser.SetSource(valueStr);

    T newValue;
    deser(newValue, "");

    TextSerializer ser2;
    ser2(newValue, "");
    const std::string valueStr2 = ser.ResolveResult();

    const bool ret = valueStr == valueStr2;
    if((ret && PrintWhenOk) || (!ret && PrintWhenFail))
    {
        std::cerr << valueStr;
        std::cerr << valueStr2;
        return false;
    }
    return true;
}

RTRC_END
