#ifndef TOLERANT_JSON
#include "shortjson.h"

#include <algorithm>
#include <numeric>
#include <stack>
#include <functional>
#include <regex>
#include <cmath>

#define Q2(x) #x
#define Q1(x) Q2(x)
#define JSON_ERROR(message) \
  "file: " Q1(__FILE__) "\n" \
  "line: " Q1(__LINE__) "\n" \
  "JSON parser error: " message

namespace shortjson
{
  // Convert 16 bit code point to UTF-8 string.
  // see Table 3-6 : http://www.unicode.org/versions/Unicode6.2.0/ch03.pdf#page=42
  static void append_utf16(std::string& dest, uint16_t data) noexcept
  {
    if (data <= 0x007F) // one byte value
      dest.push_back(char(data));
    else if (data <= 0x07FF) // two byte value
    {
      dest.push_back(0xC0 + ((data & 0x07C0) >> 6));
      dest.push_back(0x80 +  (data & 0x003F));
    }
    else // three byte value
    {
      dest.push_back(0xE0 + ((data & 0xF000) >> 12));
      dest.push_back(0x80 + ((data & 0x0FC0) >>  6));
      dest.push_back(0x80 +  (data & 0x003F));
    }
  }

  template <typename string_iterator>
  static inline void parse_string(const std::vector<node_t>::iterator& node,
                                  string_iterator& pos,
                                  const string_iterator& end)
  {
    std::string value;

    while(++pos, // iterate position
          pos < end && // NOT at End Of String AND
         *pos != '"') // NOT closing quote
    {
      if(*pos == '\\') // escape sequence expected
      {
        if(++pos < end) // iterate THEN check IF at End Of String
          switch (*pos)
          {
            case 'u': // unicode escape symbol \u???? - value range: 0 to 65535
            {
              auto start = ++pos;
              pos += 4; // 4 for \u
              if(pos > end || !std::all_of(start, pos, ::isxdigit)) // IF exceeds End Of String OR NOT all digits are hexadecimal
                throw JSON_ERROR("End Of String found while decoding UTF-16 OR hexadecimal escape sequence");
              append_utf16(value, std::stoi(std::string(start, pos--), nullptr, 16));
              break;
            }

            case '"':
              value.push_back('\\');
              value.push_back(*pos);
              break; // quote

            // normally escaped symbols
            case '/': value.push_back('/'); break; // slash (escaping mandatory?)
            case '\\':value.push_back('\\'); break; // backslash
            case 'b': value.push_back('\b'); break; // backspace
            case 'f': value.push_back('\f'); break; // feed
            case 'r': value.push_back('\r'); break; // return (to line start)
            case 'n': value.push_back('\n'); break; // newline
            case 't': value.push_back('\t'); break; // tab
            case 'v': value.push_back('\v'); break; // vertical tab
            case 'a': value.push_back('\a'); break; // audible bell

            default: // some other escape character or unexpected symbol
              value.push_back('\\');
              value.push_back(*pos);
              break;
          }
      }
      else
        value.push_back(*pos);
    }

    if(pos >= end)
      throw JSON_ERROR("Premature end of JSON found while processing string type.");

    node->type = Field::String;
    node->data = value;
  }

  static inline bool is_primitive_end_char(char x)
  {
    return std::iscntrl(x) ||
        x == ' ' || // only non-control character whitespace
        x == ':' ||
        x == ',' ||
        x == ']' ||
        x == '}';
  }

  static inline bool is_integer(std::string str) // only accepts base 10 integers
    { return std::regex_match(str, std::regex( "^(-?0|[1-9][[:digit:]]*)$", std::regex_constants::extended)); }

  static inline bool is_float(std::string str) // tolerates uppercase 'E' in scientific notation and values starting with '.'
    { return std::regex_match(str, std::regex("^-?([[:digit:]]+[.][[:digit:]]*|[.]?[[:digit:]]+)([eE][+-]?[[:digit:]]+)?$", std::regex_constants::extended)); }

  template <typename string_iterator>
  static inline void parse_primitive(const std::vector<node_t>::iterator& node,
                                     string_iterator& pos,
                                     const string_iterator& end)
  {
    string_iterator start = pos;

    pos = std::find_if(pos, end, is_primitive_end_char); // finds the character that terminates the primitive

    if(pos >= end) // parsing error occured
      throw JSON_ERROR("Premature end of JSON found while processing primitive.");

    if(!std::isspace(*pos) && std::iscntrl(*pos))
      throw JSON_ERROR("Non-space control character found in primitive. Possibly an unquoted string.");

    std::string value(start, pos); // copy the primitive

    if(value == "true") // if boolean true
    {
      node->type = Field::Boolean;
      node->data = true;
    }
    else if(value == "false") // if boolean false
    {
      node->type = Field::Boolean;
      node->data = false;
    }
    else if(value == "null") // if value is null
      node->type = Field::Null;
    else // numeric value is the only type left
    {
      size_t offset = 0;
      while((offset = value.find('_', offset)) != std::string::npos) // while "search for a '_' seperator" succeeds
        value.erase(offset, 1); // erase the seperator from the copied primitive

      if(is_integer(value)) // if the primitive only uses characters valid in an integer
      {
        node->type = Field::Integer;
        node->data = std::stoll(value, nullptr, 10); // convert
      }
      else if(is_float(value)) // if the primitive only uses characters valid in a float
      {
        node->type = Field::Float;
        node->data = std::stod(value); // convert
      }
      else // Unexpected character for an integer or float primitive.  Maybe it's neither of those.
        throw JSON_ERROR("Unrecognized primitive type.\n"
                         "Strict Mode:\n"
                         "  * Strings must use quotes.\n"
                         "  * Hexadecimal numbers are invalid.\n"
                         "  * Boolean and null values must be lowercase.\n"
                         "  * Numbers cannot be explicitly positive.");
    }
  }

  node_t Parse(const std::string& json_data)
  {
    node_t root;
    root.data = std::vector<node_t> { node_t() };

    auto pos = json_data.cbegin();
    const auto end = json_data.cend();
    std::vector<node_t>::iterator iter = root.toArray().begin(); // create new node and get iterator
    std::stack<std::vector<node_t>::iterator> lineage;
    lineage.push(iter); // this stack MUST NEVER be empty

    while(pos < end) // NOT at End Of String
    {
      if(!std::isspace(*pos)) // ignore spaces
        switch(*pos)
        {
          case '[': // beginning of new node
          case '{':
          {
            lineage.push(iter); // remember the current position so that we can return
            iter->type = *pos == '[' ? Field::Array : Field::Object;
            iter->data = std::vector<node_t>();
            iter = iter->toArray().emplace(iter->toArray().cend()); // create new node and get iterator
            break;
          }

          case '}': // end of current node
          case ']':
            if(iter->type == Field::Undefined) // if node is unfilled (can happen with a trailing comma)
              lineage.top()->toArray().erase(iter); // delete the unfilled node
            lineage.pop();
            iter = lineage.top();
            break;

            // open quote
          case '"':
            parse_string(iter, pos, end);
            break;

          case '\'':
            throw JSON_ERROR("Strings must use quotes, not apostrophes.");

          case ':': // indicates current value is a name
            if(iter->type != Field::String)
              throw JSON_ERROR("Only a string can be a label.");
            iter->type = Field::Undefined;
            iter->identifier = iter->toString();
            break;

          case ',':
            if(iter->type != Field::Array &&
               iter->type != Field::Object)
              iter = lineage.top();
            iter = iter->toArray().emplace(iter->toArray().cend()); // create new node and get iterator
            break;

          default:
            parse_primitive(iter, pos, end);
            continue; // immediate jump to start of loop (avoid iterating)
        }
      ++pos;
    }
    return std::move(root.toArray().front()); // root Object always contains a single node.  explicitly move node_t
  }

  bool FindNode(const node_t& parent, node_t& output, const std::string_view& identifier) noexcept
  {
    if(parent.identifier == identifier) // if this node_t has the correct identifier
      output = parent; // save this node! ('type' MUST not be be Field::Undefined or it will keep searching)
    else if(parent.type == Field::Array ||
            parent.type == Field::Object)
    {
      std::any_of(parent.toArray().begin(), parent.toArray().end(), // check if any of the child elements return true from predicate
                  [&output, &identifier](const node_t& child)       // predicate to check if child node returns true from FindNodeByIdentifier()
                  { return FindNode(child, output, identifier); }); // return value from any_of() is ignored because output.type is tested instead
    }

    return output.type != Field::Undefined; // will be true when found
  }

  bool FindString(const node_t& parent, std::string& output, const std::string_view& identifier) noexcept
  {
    node_t child;
    return FindNode(parent, child, identifier) &&
        child.type == Field::String &&
        (output = child.toString(), true);
  }

  bool FindNumber(const node_t& parent, intmax_t& output, const std::string_view& identifier) noexcept
  {
    node_t child;
    return FindNode(parent, child, identifier) &&
        child.type == Field::Integer &&
        (output = child.toNumber(), true);
  }

  bool FindFloat(const node_t& parent, double& output, const std::string_view& identifier) noexcept
  {
    node_t child;
    return FindNode(parent, child, identifier) &&
        child.type == Field::Float &&
        (output = child.toFloat(), true);
  }

  bool FindBoolean(const node_t& parent, bool& output, const std::string_view& identifier) noexcept
  {
    node_t child;
    return FindNode(parent, child, identifier) &&
        child.type == Field::Boolean &&
        (output = child.toBool(), true);
  }
}
#endif
