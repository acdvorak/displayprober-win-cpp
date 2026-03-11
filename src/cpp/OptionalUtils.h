#pragma once

#include <optional>

template <typename TMap, typename TKey>
const std::optional<typename TMap::mapped_type> TryGetOptionalValue(
    const TMap& map, const TKey& key) {
  const auto it = map.find(key);
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}
