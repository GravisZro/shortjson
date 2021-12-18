#include "shortjson.h"

#include <algorithm>
#include <numeric>
#include <stack>
#include <functional>
#include <cmath>

#include <iostream>

#define Q2(x) #x
#define Q1(x) Q2(x)
#define JSON_ERROR(message) \
  "file: " Q1(__FILE__) "\n" \
  "line: " Q1(__LINE__) "\n" \
  "JSON parser error: " message

namespace shortjson
{
  constexpr bool is_octal_digit(const uint8_t data) noexcept
    { return data >= '0' && data <= '7'; }

  constexpr uint8_t hex_value(char x) noexcept
    { return x <= '9' ? x - '0' : (10 + ::tolower(x) - 'a'); }

  template <uint32_t base, typename string_iterator> // turns character sequence into a number
  constexpr uint32_t reconstitute_number(string_iterator& pos,
                                         const string_iterator& end)
    { return std::accumulate(pos, end, 0, [](uint32_t t, char x) { return (t * base) + hex_value(x); }); }

  template <typename string_iterator>  // grabs 'num_digits' hexadecimal digits and returns it as a value
  static uint32_t get_hex_digits(string_iterator& pos,
                                 const string_iterator& end,
                                 uint8_t num_digits)
  {
    auto start = pos;
    pos += num_digits;
    if(pos > end || !std::all_of(start, pos, ::isxdigit)) // all digits are hexadecimal
      throw JSON_ERROR("End Of String found while decoding UTF-16 OR hexadecimal escape sequence");
    return reconstitute_number<16>(start, pos--);
  }

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
    const char quote_char = *pos;
    std::string value;

    while(++pos, // iterate position
          pos < end && // NOT at End Of String AND
         *pos != quote_char) // NOT closing quote
    {
      if(*pos == '\\') // escape sequence expected
      {
        if(++pos < end) // iterate THEN check IF at End Of String
          switch (*pos)
          {
            // hexadecimal encoded
            case 'u': // unicode escape symbol \u???? - value range: 0 to 65535
              append_utf16(value, get_hex_digits(++pos, end, 4)); // read digits
              break;

            case 'x': // unicode escape symbol \x?? - value range: 0 to 255
              value.push_back(char(get_hex_digits(++pos, end, 2)));
              break;

            // octal sequence
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
            { // value range: 0 to 511
              auto oct_end = std::find_if_not(pos, pos + 3 < end ? pos + 3 : end, is_octal_digit);  // find last octal digit (max of 3 digits)
              append_utf16(value, reconstitute_number<8>(pos, oct_end));
              pos = oct_end - 1;
              break;
            }

            case '"':
            case '\'':
              if(quote_char != *pos)
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

  static inline bool is_integer(char x) noexcept
  {
    return std::isxdigit(x) ||
        x == '+' ||
        x == '-' ||
        x == 'x';
  }

  static inline bool is_float(char x) noexcept
  {
    return std::isdigit(x) ||
        x == '+' ||
        x == '-' ||
        x == 'e' ||
        x == '.';
  }

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
    std::transform(value.begin(), value.end(), value.begin(), ::tolower); // transform the copy to lowercase

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
      size_t offset;
      while((offset = value.find('_')) != std::string::npos) // while search for a '_' seperator succeeds
        value.erase(offset, 1); // erase the seperator from the copied primitive

      if(std::all_of(value.begin(), value.end(), is_integer)) // if the primitive only uses characters valid in an integer
      {
        node->type = Field::Integer;
        node->data = std::strtoll(&value.front(), nullptr, 0); // convert and detect the base
      }
      else if(std::all_of(value.begin(), value.end(), is_float)) // if the primitive only uses characters valid in a float
      {
        node->type = Field::Float;
        node->data = std::strtod(&value.front(), nullptr);
      }
      else // Unexpected character for an integer or float primitive.  Maybe it's neither of those.
        throw JSON_ERROR("Unrecognized primitive type. Possibly an unquoted string.");
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
            lineage.pop();
            if(iter->type == Field::Undefined) // if node is unfilled (can happen with a trailing comma)
              lineage.top()->toArray().erase(iter); // delete the unfilled node
            iter = lineage.top();
            break;

            // open quote
          case '"':
          case '\'':
            parse_string(iter, pos, end);
            break;

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
}
