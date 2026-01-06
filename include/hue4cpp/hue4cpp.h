#pragma once

/**
 * @file hue4cpp.h
 * @brief Main header file for hue4cpp library
 * 
 * Include this header to access all hue4cpp functionality.
 */

#include "hue4cpp/types.h"
#include "hue4cpp/exceptions.h"
#include "hue4cpp/http_client.h"
#include "hue4cpp/json_utils.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/light.h"
#include "hue4cpp/state.h"

/**
 * @namespace hue4cpp
 * @brief Main namespace for the hue4cpp library
 */
namespace hue4cpp {

/**
 * @brief Library version information
 */
struct Version {
    static constexpr int MAJOR = 0;
    static constexpr int MINOR = 1;
    static constexpr int PATCH = 0;
    
    static constexpr const char* STRING = "0.1.0";
};

} // namespace hue4cpp
