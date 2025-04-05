#pragma once

#include <sw/redis++/redis++.h>
#include "common.pb.h"
#include <stdexcept>
#include <string>

// #include <unordered_map>
// #include <vector>
// #include <optional>

namespace iot::parser {

class StreamParserException : public std::runtime_error{
public:
    using std::runtime_error::runtime_error;
};

class RedisStreamParser{
public:
    //https://github.com/sewenew/redis-plus-plus
    using FieldMap = std::unordered_map<std::string, std::string>;
    using StreamResult = std::unordered_map<std::string, std::vector<std::pair<std::string, FieldMap>>>;

    static iot::v1::SensorData parse_sensor_data(
        const std::string& stream_entry_id,
        const FieldMap& fields) {
            iot::v1::SensorData data;
            try{
                data.set_device_id(get_field(fields, "device_id"));
                data.set_temperature(parse_double(fields, "temperature"));
                data.set_humidity(parse_double(fields, "humidity"));
                data.mutable_timestamp()->set_seconds(parse_int64(fields, "timestamp"));
            }catch(const std::exception& e){
                throw StreamParserException(
                    "Failed to parse stream entry " + stream_entry_id + ": " + e.what());
            }
            return data;
        }

private:
    static std::string get_field(const FieldMap& fields, const std::string& name){
        if(auto it = fields.find(name); it != fields.end()){
            return it->second;
        }
        throw StreamParserException("Missing required field: " + name);
    }

    static double parse_double(const FieldMap& fields, const std::string& name){
        try{
            return std::stod(get_field(fields,name));
        }catch(const std::invalid_argument&){
            throw StreamParserException("Invalid double value from field: " + name);
        }
    }

    static int64_t parse_int64(const FieldMap& fields, const std::string& name){
        try{
            return std::stoll(get_field(fields,name));
        }catch(const std::invalid_argument&){
            throw StreamParserException("Invalid integer value from field: " + name);
        }
    }
};

}//namespace iot::parser
