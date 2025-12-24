/*
 * Copyright (c) 2025-present
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * The NDTwin Authors and Contributors:
 *     Prof. Shie-Yuan Wang <National Yang Ming Chiao Tung University; CITI, Academia Sinica>
 *     Ms. Xiang-Ling Lin <CITI, Academia Sinica>
 *     Mr. Po-Yu Juan <CITI, Academia Sinica>
 */

// utils/Utils.hpp
#pragma once

#include "utils/Logger.hpp"
#include <arpa/inet.h>
#include <array>
#include <boost/asio/connect.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/url.hpp>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @brief Common utility helpers used across NDTwin.
 *
 * This header provides small, header-only helpers for:
 *  - deployment mode flags (MININET vs TESTBED),
 *  - IPv4 and MAC address conversions,
 *  - shell command execution (popen),
 *  - HTTPS POST helper (Boost.Beast),
 *  - timestamp helpers and formatting.
 *
 * @warning Some functions (execCommand, httpsPost) perform I/O and may throw.
 * @warning Functions using inet_ntoa/localtime rely on non-thread-safe libc APIs.
 *          Prefer thread-safe alternatives if called from multiple threads.
 */
namespace utils
{

/**
 * @brief Indicates where NDTwin is running.
 *
 * MININET: simulated environment (Mininet/OVS).
 * TESTBED: physical hardware testbed.
 */
enum DeploymentMode
{
    MININET = 1,
    TESTBED = 2
};

/**
 * @brief Convert an IPv4 address to dotted-decimal string.
 *
 * @param ip IPv4 address (expected in network byte order, i.e., in_addr.s_addr format).
 * @return Dotted string (e.g., "10.10.10.12").
 * @throws std::runtime_error if conversion fails.
 *
 * @warning Uses inet_ntoa(), which is not thread-safe.
 */
inline std::string
ipToString(uint32_t ip)
{
    struct in_addr addr;
    addr.s_addr = ip;
    const char* s = inet_ntoa(addr);
    if (!s)
    {
        throw std::runtime_error("inet_ntoa failed");
    }
    return std::string(s);
}

/**
 * @brief Convert a vector of IPv4 addresses to dotted-decimal strings.
 *
 * @param ipVec IPv4 addresses (expected in network byte order).
 * @return Vector of dotted strings.
 * @throws std::runtime_error on conversion failure.
 *
 * @warning Uses inet_ntoa(), which is not thread-safe.
 */
inline std::vector<std::string>
ipToString(std::vector<uint32_t> ipVec)
{
    std::vector<std::string> res;
    for (const auto& ip : ipVec)
    {
        struct in_addr addr;
        addr.s_addr = ip;
        const char* s = inet_ntoa(addr);
        if (!s)
        {
            throw std::runtime_error("inet_ntoa failed");
        }
        res.push_back(std::string(s));
    }
    return res;
}

/**
 * @brief Parse dotted IPv4 string into uint32_t.
 *
 * @param ipStr Dotted IPv4 string (e.g., "10.10.10.12").
 * @return IPv4 address in network byte order (in_addr.s_addr).
 * @throws std::invalid_argument if the string is not a valid IPv4 address.
 */
inline uint32_t
ipStringToUint32(const std::string& ipStr)
{
    struct in_addr addr;
    if (inet_aton(ipStr.c_str(), &addr) == 0)
    {
        throw std::invalid_argument("Invalid IP address: " + ipStr);
    }
    return addr.s_addr;
}

/**
 * @brief Parse a vector of dotted IPv4 strings into uint32_t addresses.
 *
 * @return IPv4 addresses in network byte order.
 * @throws std::invalid_argument if any entry is invalid.
 */
inline std::vector<uint32_t>
ipStringVecToUint32Vec(std::vector<std::string> ipStringVec)
{
    std::vector<uint32_t> res;
    for (const auto& ipStr : ipStringVec)
    {
        struct in_addr addr;
        if (inet_aton(ipStr.c_str(), &addr) == 0)
        {
            throw std::invalid_argument("Invalid IP address: " + ipStr);
        }
        res.push_back(addr.s_addr);
    }
    return res;
}

inline uint32_t
portStringToUint(const std::string& portStr)
{
    try
    {
        return std::stoul(portStr, nullptr, 16);
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "Invalid input: " << e.what() << std::endl;
        return 0; // Handle appropriately (e.g., return a default value)
    }
    catch (const std::out_of_range& e)
    {
        std::cerr << "Out of range: " << e.what() << std::endl;
        return 0; // Handle appropriately
    }
}

/**
 * @brief Parse a hex string into uint64_t.
 *
 * Accepts strings like "0x1a2b" or "1A2B".
 *
 * @throws std::invalid_argument on parse failure.
 */
inline uint64_t
hexStringToUint64(const std::string& hexStr)
{
    std::stringstream ss(hexStr);
    ss >> std::hex >> std::ws;
    uint64_t value;
    if (!(ss >> value))
    {
        throw std::invalid_argument("Invalid hex string: " + hexStr);
    }
    return value;
}

/**
 * @brief Execute a shell command and capture its stdout.
 *
 * @param cmd Shell command string passed to popen().
 * @return Captured stdout output.
 * @throws std::runtime_error if popen() fails.
 *
 * @warning This function executes via the shell. Do not pass untrusted input
 *          into @p cmd unless properly escaped/sanitized.
 */
inline std::string
execCommand(const std::string& cmd)
{
    std::array<char, 256> buffer;
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe))
    {
        result += buffer.data();
    }
    int rc = pclose(pipe);
    if (rc != 0)
    {
        std::cerr << "Command exited with code " << rc << "\n";
    }
    return result;
}

/**
 * @brief Perform an HTTPS POST request and return the response body.
 *
 * Uses Boost.URL for parsing and Boost.Beast for TLS + HTTP.
 *
 * @param url HTTPS URL (must start with "https://").
 * @param payload Request body payload.
 * @param ctype Content-Type header value (default: "application/json").
 * @param authorization Authorization header value (default: empty).
 * @return Response body as a string.
 *
 * @throws std::invalid_argument for invalid URLs (non-https, missing host, etc.).
 * @throws std::runtime_error if the server returns >= 400 or on TLS/IO failures.
 *
 * @note This helper uses system default verify paths; certificate validation behavior
 *       depends on platform configuration.
 */
inline std::string
httpsPost(const std::string& url,
          const std::string& payload,
          const std::string& ctype = "application/json",
          const std::string& authorization = "")
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace asio = boost::asio;
    namespace urls = boost::urls;

    urls::url_view u{url};
    if (u.scheme() != "https")
    {
        throw std::invalid_argument("Only https:// URLs are supported: " + url);
    }
    if (u.host().empty())
    {
        throw std::invalid_argument("URL missing host: " + url);
    }

    const std::string host = static_cast<std::string>(u.host());
    const std::string port = u.port().empty() ? "443" : static_cast<std::string>(u.port());

    std::string target =
        u.encoded_path().empty() ? "/" : static_cast<std::string>(u.encoded_path());

    if (!u.encoded_query().empty())
    {
        target += "?" + static_cast<std::string>(u.encoded_query());
    }

    asio::io_context ioc;
    asio::ssl::context sslCtx{asio::ssl::context::tls_client};
    sslCtx.set_default_verify_paths();

    beast::ssl_stream<beast::tcp_stream> stream{ioc, sslCtx};
    asio::ip::tcp::resolver resolver{ioc};

    auto results = resolver.resolve(host, port);
    beast::get_lowest_layer(stream).connect(results);

    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
    {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()),
            "Failed to set SNI");
    }

    stream.handshake(asio::ssl::stream_base::client);

    http::request<http::string_body> req{http::verb::post, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::content_type, ctype);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::authorization, authorization);
    req.body() = payload;
    req.prepare_payload();

    http::write(stream, req);

    beast::flat_buffer buf;
    http::response<http::string_body> res;
    http::read(stream, buf, res);

    beast::error_code ec;
    stream.shutdown(ec);

    if (res.result() >= http::status::bad_request)
    {
        // SPDLOG_LOGGER_ERROR(Logger::instance(),
        //                     "HTTP request failed: {} {}", res.result(), res.reason());
        // SPDLOG_LOGGER_DEBUG(Logger::instance(), "HTTP Response: {}", res.body());
        std::cerr << "HTTP Response: " << res << std::endl;
        throw std::runtime_error("Server returned HTTP " + std::to_string(res.result_int()));
    }

    return std::move(res.body());
}

/**
 * @brief Current time in milliseconds since epoch (system_clock).
 *
 * Suitable for wall-clock timestamps and logging.
 */
inline int64_t
getCurrentTimeMillisSystemClock()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/**
 * @brief Monotonic time in milliseconds (steady_clock).
 *
 * Suitable for measuring durations; not tied to wall-clock time.
 */
inline int64_t
getCurrentTimeMillisSteadyClock()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

/**
 * @brief Monotonic time in milliseconds (steady_clock).
 *
 * Suitable for measuring durations; not tied to wall-clock time.
 */
inline std::string
formatTime(int64_t timestamp_ms)
{
    time_t timestamp_s = timestamp_ms / 1000;
    struct tm* localTime = localtime(&timestamp_s);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);
    return std::string(buffer);
}

/**
 * @brief Log the current local time at DEBUG level using the global Logger.
 *
 * @warning Uses localtime(), which is not thread-safe.
 */
inline void
logCurrentTimeSystemClock()
{
    int64_t now_ms = utils::getCurrentTimeMillisSystemClock();
    time_t now_s = now_ms / 1000;
    struct tm* localTime = localtime(&now_s);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);
    SPDLOG_LOGGER_DEBUG(Logger::instance(), "Local Time: {}", buffer);
}

/**
 * @brief Convert MAC address string ("aa:bb:cc:dd:ee:ff") to a 48-bit integer.
 *
 * @param mac MAC string in colon-separated hex format.
 * @return MAC as uint64_t (lower 48 bits used).
 * @throws std::invalid_argument if parsing fails.
 */
inline uint64_t
macToUint64(const std::string& mac)
{
    uint64_t result = 0;
    auto parse_hex_byte = [&](const char* ptr) {
        uint8_t byte = 0;
        auto [p, ec] = std::from_chars(ptr, ptr + 2, byte, 16);
        if (ec != std::errc())
        {
            throw std::invalid_argument("Invalid hex digit");
        }
        return byte;
    };

    const char* p = mac.data();
    for (int i = 0; i < 6; ++i)
    {
        result <<= 8;
        result |= parse_hex_byte(p + i * 3);
    }
    return result;
}

/**
 * @brief Convert a 48-bit MAC stored in uint64_t to string form.
 *
 * @param mac MAC value (lower 48 bits used).
 * @return MAC string ("aa:bb:cc:dd:ee:ff").
 */
inline std::string
macToString(uint64_t mac)
{
    std::ostringstream oss;

    // Extract 6 bytes from the 48-bit MAC
    for (int i = 5; i >= 0; --i)
    {
        uint8_t byte = (mac >> (i * 8)) & 0xFF;
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        if (i > 0)
        {
            oss << ":";
        }
    }

    return oss.str();
}
} // namespace utils
