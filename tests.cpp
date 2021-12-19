#include <cassert>
#include <string>
#include <iostream>

#include "shortjson.h"


template<typename T> std::string_view get_value(shortjson::node_t&, T&) { assert(false); return "this shouldn't be reached"; }

template<> std::string_view get_value<std::errc>(shortjson::node_t&, std::errc&) { return "parser error"; }

template<> std::string_view get_value<bool>(shortjson::node_t& node, bool& value)
{
  value = node.toBool();
  return "Boolean";
}

template<> std::string_view get_value<intmax_t>(shortjson::node_t& node, intmax_t& value)
{
  value = node.toNumber();
  return "Integer";
}


template<> std::string_view get_value<double>(shortjson::node_t& node, double& value)
{
  value = node.toFloat();
  return "Float";
}

template<> std::string_view get_value<std::string>(shortjson::node_t& node, std::string& value)
{
  value = node.toString();
  return "String";
}

template<> std::string_view get_value<std::nullptr_t>(shortjson::node_t&, std::nullptr_t&)
{
  return "Null";
}


template<typename T>
void parse_test(std::string test, T expected_value)
{
  std::cout << std::endl;

  shortjson::node_t root = shortjson::Parse(test);
  shortjson::node_t& node = root.toObject().front();

  std::cout << "test identifier: " << node.identifier << std::endl;
  T value;
  std::string_view type = get_value(node, value);

  if(value != expected_value)
  {
    std::cout << "Parsing string:" << std::endl
              << test << std::endl;

    std::cout << "Type found: " << type << std::endl;
    std::cout << "Value   : " << value << std::endl;
    std::cout << "Expected: " << expected_value << std::endl;

    std::cout << "Test: FAILED" << std::endl;
    throw "test failed";
  }
  std::cout << "Test: PASSED" << std::endl;
}

template<>
void parse_test<const char*>(std::string test, const char* expected_value)
  { parse_test(test, std::string(expected_value)); }

template<>
void parse_test<int>(std::string test, int expected_value)
  { parse_test(test, intmax_t(expected_value)); }

template<>
void parse_test<float>(std::string test, float expected_value)
  { parse_test(test, double(expected_value)); }


void parser_error_test(std::string test, std::string test_id)
{
  std::cout << std::endl;

  std::cout << "test identifier: " << test_id << std::endl;
  try
  {
    shortjson::node_t root = shortjson::Parse(test);
    std::cout << "Test: FAILED" << std::endl;
  }
  catch(const char* message)
  {
    std::cout << "Test: PASSED" << std::endl;
  }
}



int main(int argc, char* argv[])
{
  // NOTE: unicode/hex/octal escape sequences generated with https://onlineunicodetools.com/escape-unicode
  try
  {
#ifndef TOLERANT_JSON
    parse_test("{\"normal quoted string\" : \"string\" }", "string");
    parser_error_test("{\"EMCAScript quoted string\" : 'string' }", "EMCAScript quoted string");

    parser_error_test("{\"bad boolean true 1\" : TRUE }", "bad boolean true 1");
    parser_error_test("{\"bad boolean true 2\" : True }", "bad boolean true 2");
    parse_test("{\"good boolean true\" : true }", true);

    parser_error_test("{\"bad boolean false 1\" : FALSE }", "bad boolean false 1");
    parser_error_test("{\"bad boolean false 2\" : False }", "bad boolean false 2");
    parse_test("{\"good boolean false\" : false }", false);

    parser_error_test("{\"bad null 1\" : NULL }", "bad null 1");
    parser_error_test("{\"bad null 2\" : Null }", "bad null 2");
    parse_test("{\"good null\" : null }", nullptr);

    parse_test("{\"unescaped UTF-16\" : \"Hello World! ☺\"}", "Hello World! ☺");
    parse_test("{\"escaped UTF-16\" : \"\\u0048\\u0065\\u006c\\u006c\\u006f\\u0020\\u0057\\u006f\\u0072\\u006c\\u0064\\u0021\\u0020\\u263a\"}", "Hello World! ☺");
    parse_test("{\"escaped hexadecimal\" : \"\\x48\\x65\\x6c\\x6c\\x6f\\x20\\x57\\x6f\\x72\\x6c\\x64\\x21\\x20☺\"}", "\\x48\\x65\\x6c\\x6c\\x6f\\x20\\x57\\x6f\\x72\\x6c\\x64\\x21\\x20☺");
    parse_test("{\"unpadded octal\" : \"\\110\\145\\154\\154\\157\\40\\127\\157\\162\\154\\144\\41\\40☺\" }", "\\110\\145\\154\\154\\157\\40\\127\\157\\162\\154\\144\\41\\40☺");
    parse_test("{\"padded octal\" : \"\\110\\145\\154\\154\\157\\040\\127\\157\\162\\154\\144\\041\\040☺\" }", "\\110\\145\\154\\154\\157\\040\\127\\157\\162\\154\\144\\041\\040☺");

    parse_test("{\"signless integer\" : 4096 }", 4096);
    parser_error_test("{\"explicitly positive integer\" : +4096 }", "explicitly positive integer");
    parse_test("{\"negative integer\" : -4096 }", -4096);
    parser_error_test("{\"signless hexadecimal integer\" : 0x1000 }", "signless hexadecimal integer");
    parser_error_test("{\"explicitly positive hexadecimal integer\" : +0x1000 }", "explicitly positive hexadecimal integer");
    parser_error_test("{\"negative hexadecimal integer\" : -0x1000 }", "negative hexadecimal integer");

    parse_test("{\"signless float\" : 409600000.004096 }", 409600000.004096);
    parser_error_test("{\"explicitly positive float\" : +409600000.004096 }", "explicitly positive float");
    parse_test("{\"negative float\" : -409600000.004096 }", -409600000.004096);

    parse_test("{\"signless scientific large float\" : 4.096e+10 }", 40960000000.000000);
    parse_test("{\"signless scientific normal float\" : 4.096e+3 }", 4096.000000);
    parse_test("{\"signless scientific small float\" : 4.096e-3 }", 0.004096);

    parser_error_test("{\"explicitly positive scientific large float\" : +4.096e+10 }",  "explicitly positive scientific large float");
    parser_error_test("{\"explicitly positive scientific normal float\" : +4.096e+3 }", "explicitly positive scientific normal float");
    parser_error_test("{\"explicitly positive scientific small float\" : +4.096e-3 }", "explicitly positive scientific small float");

    parse_test("{\"negative scientific large float\" : -4.096e+10 }", -40960000000.000000);
    parse_test("{\"negative scientific normal float\" : -4.096e+3 }", -4096.000000);
    parse_test("{\"negative scientific small float\" : -4.096e-3 }", -0.004096);
#else
    parse_test("{\"normal quoted string\" : \"string\" }", "string");
    parse_test("{\"EMCAScript quoted string\" : 'string' }", "string");

    parse_test("{'bad boolean true 1' : TRUE }", true);
    parse_test("{'bad boolean true 2' : True }", true);
    parse_test("{'good boolean true' : true }", true);

    parse_test("{'bad boolean false 1' : FALSE }", false);
    parse_test("{'bad boolean false 2' : False }", false);
    parse_test("{'good boolean false' : false }", false);

    parse_test("{'bad null 1' : NULL }", nullptr);
    parse_test("{'bad null 2' : Null }", nullptr);
    parse_test("{'good null' : null }", nullptr);

    parse_test("{'unescaped UTF-16' : 'Hello World! ☺'}", "Hello World! ☺");
    parse_test("{'escaped UTF-16' : '\\u0048\\u0065\\u006c\\u006c\\u006f\\u0020\\u0057\\u006f\\u0072\\u006c\\u0064\\u0021\\u0020\\u263a'}", "Hello World! ☺");
    parse_test("{'escaped hexadecimal' : '\\x48\\x65\\x6c\\x6c\\x6f\\x20\\x57\\x6f\\x72\\x6c\\x64\\x21\\x20☺'}", "Hello World! ☺");
    parse_test("{'unpadded octal' : '\\110\\145\\154\\154\\157\\40\\127\\157\\162\\154\\144\\41\\40☺' }", "Hello World! ☺");
    parse_test("{'padded octal' : '\\110\\145\\154\\154\\157\\040\\127\\157\\162\\154\\144\\041\\040☺' }", "Hello World! ☺");

    parse_test("{'signless integer' : 4096 }", 4096);
    parse_test("{'explicitly positive integer' : +4096 }", 4096);
    parse_test("{'negative integer' : -4096 }", -4096);
    parse_test("{'signless hexadecimal integer' : 0x1000 }", 4096);
    parse_test("{'explicitly positive hexadecimal integer' : +0x1000 }", 4096);
    parse_test("{'negative hexadecimal integer' : -0x1000 }", -4096);

    parse_test("{'signless float' : 409600000.004096 }", 409600000.004096);
    parse_test("{'explicitly positive float' : +409600000.004096 }", 409600000.004096);
    parse_test("{'negative float' : -409600000.004096 }", -409600000.004096);

    parse_test("{'signless scientific large float' : 4.096e+10 }", 40960000000.000000);
    parse_test("{'signless scientific normal float' : 4.096e+3 }", 4096.000000);
    parse_test("{'signless scientific small float' : 4.096e-3 }", 0.004096);

    parse_test("{'explicitly positive scientific large float' : +4.096e+10 }",  40960000000.000000);
    parse_test("{'explicitly positive scientific normal float' : +4.096e+3 }", 4096.000000);
    parse_test("{'explicitly positive scientific small float' : +4.096e-3 }", 0.004096);

    parse_test("{'negative scientific large float' : -4.096e+10 }", -40960000000.000000);
    parse_test("{'negative scientific normal float' : -4.096e+3 }", -4096.000000);
    parse_test("{'negative scientific small float' : -4.096e-3 }", -0.004096);
#endif

  }
  catch(const char* error)
  {
    std::cout << error << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
