#ifndef CUSTOM_TYPES_H
#define CUSTOM_TYPES_H

template <typename T>
    concept QueryType = std::same_as<T, std::string> ||
    std::same_as<T, std::vector<uint8_t>>;

template <typename L>
    concept MapType = std::same_as<L, std::string> ||
    std::same_as<L, int64_t> ||
    std::same_as<L, uint64_t> ||
    std::same_as<L, float> ||
    std::same_as<L, time_t> ||
    std::same_as<L, uint32_t>;

template <typename D>
    concept FstQueryType = std::same_as<D, std::string> ||
    std::same_as<D, std::vector<uint8_t>>;
#endif