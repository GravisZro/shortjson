#ifndef SHORTJSON_H
#define SHORTJSON_H

#if __cplusplus < 201703L
# error This JSON parser utilizes C++17 features.
#endif

#include <cstdint>
#include <variant>
#include <string>
#include <string_view>
#include <vector>

namespace shortjson
{
  enum class Field : uint8_t
  {
    Undefined = 0,
    Null,
    Array,
    Object,
    Boolean,
    String,
    Integer,
    Float,
  };

  struct node_t
  {
    node_t(void) noexcept : type(Field::Undefined) { }

    std::string identifier;
    Field       type;
    std::variant<bool, intmax_t, double, std::string, std::vector<node_t>> data;

    constexpr bool&      toBool  (void) noexcept { return std::get<bool       >(data); }
    constexpr intmax_t&  toNumber(void) noexcept { return std::get<intmax_t   >(data); }
    constexpr double&    toFloat (void) noexcept { return std::get<double     >(data); }
    inline std::string&  toString(void) noexcept { return std::get<std::string>(data); }

    inline       std::vector<node_t>& toArray  (void)       noexcept { return std::get<std::vector<node_t>>(data); }
    inline const std::vector<node_t>& toArray  (void) const noexcept { return std::get<std::vector<node_t>>(data); }
#define toObject toArray // simple alias
  };

  node_t Parse(const std::string& json_data);

  bool FindNode(const node_t& parent, node_t& output, const std::string_view& identifier) noexcept;

  bool FindString(const node_t& parent, std::string& output, const std::string_view& identifier) noexcept;
  bool FindNumber(const node_t& parent, intmax_t& output, const std::string_view& identifier) noexcept;
  bool FindFloat(const node_t& parent, double& output, const std::string_view& identifier) noexcept;
  bool FindBoolean(const node_t& parent, bool& output, const std::string_view& identifier) noexcept;
}

#endif // SHORTJSON_H
