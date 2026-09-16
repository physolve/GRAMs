#pragma once

// Коды ошибок контракта grams.sim/1 (раздел 2.3).

namespace sim::RpcError {

inline constexpr int ParseError = -32700;
inline constexpr int InvalidRequest = -32600;
inline constexpr int MethodNotFound = -32601;
inline constexpr int InvalidParams = -32602;

inline constexpr int SimNotAllowed = 1001;
inline constexpr int InvalidProfile = 1002;
inline constexpr int NoActiveRun = 1003;
inline constexpr int UnknownPhase = 1004;
inline constexpr int Busy = 1005;
inline constexpr int Unauthorized = 1006;

} // namespace sim::RpcError
