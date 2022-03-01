#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <ncurses.h>
#include <optional>
#include <string>

#include "Utils.hpp"

#include "Protocol.hpp"

typedef unsigned int uint;

// Contains a couple of functions for reading and writing
// arbitrary messages, as well as a get latest message
// function that iterates over a list of messages and gets
// the desired structs with the highest message IDs
class Messenger {
  public:
    template <class T>
    static std::optional<T> readMessage(const std::string &buffer) {
        uint offset = 0;
        uint id = Utils::readUintFromBuf(buffer, offset);
        std::string rest = buffer.substr(offset);
        if (id == T::id) {
            return T(rest);
        } else {
            return {};
        }
    }

    //this function feels out of place in this class 
    //in reality it feels like it belongs inside of the connection method
    //the issue is this method bridges the gap between receiving up to date
    //messages and finding app data, so it can really only exist here
    template <class... Types>
    static void getLatest(const std::vector<Protocol::AppData> &messages,
                          std::optional<Types> &...out) {
        std::map<uint, uint> highestIds;
        // initialize everything with zeros
        (
            [&](auto &input) {
                using t = typename std::remove_reference<
                    decltype(input)>::type::value_type;
                uint typeId = t::id;
                if (highestIds.find(typeId) != highestIds.end()) {
                    throw "Expected unique getLatest call";
                }
                highestIds[typeId] = 0;
            }(out),
            ...);
        for (auto message : messages) {
            if (message.message.length()) {
                (
                    [&](auto &input) {
                        using t = typename std::remove_reference<
                            decltype(input)>::type::value_type;
                        uint typeId = t::id;
                        auto castedType = readMessage<t>(message.message);
                        if (castedType.has_value()) {
                            if (message.id >= highestIds[typeId]) {
                                input = *castedType;
                            }
                        }
                    }(out),
                    ...);
            }
        }
    }

    template <class T> static std::string writeMessage(const T &object) {
        std::string buffer;
        Utils::writeUintToBuf(buffer, T::id);
        buffer += object.toBuffer();
        return buffer;
    }
};
