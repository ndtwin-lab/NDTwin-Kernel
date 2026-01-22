#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

/**
 * @brief Operation type for a flow rule update.
 */
enum class FlowOp : uint8_t
{
    Install,
    Modify,
    Delete
};

/**
 * @brief A unit of work for OpenFlow rule updates.
 *
 * FlowJob represents one requested change to a switch flow table (install/modify/delete).
 * It carries the original JSON match/actions payload to send southbound, and also caches
 * parsed IPv4 destination information for fast lookups and “affected-flow” recomputation.
 *
 * Fields:
 *  - dpid: Target switch datapath ID.
 *  - op: Operation type (Install / Modify / Delete).
 *  - priority: Flow priority (OpenFlow rule priority).
 *  - match: Match fields in JSON form (e.g., eth_type, ipv4_dst).
 *  - actions: Actions in JSON form (e.g., OUTPUT port).
 *  - idleTimeout: Optional idle timeout in seconds (0 means no idle timeout unless your controller
 *    interprets it differently).
 */

struct FlowJob {
    uint64_t dpid;
    FlowOp op;
    int priority;
    nlohmann::json match;
    nlohmann::json actions;

    int idleTimeout = 0;
};

