#include <string>
#include <iostream>

#include "shortjson.h"

void parse_test(std::string test, std::string expected_value)
{
  std::cout << std::endl;

  shortjson::node_t root = shortjson::Parse(test);
  shortjson::node_t& node = root.toObject().front();

  std::cout << "test identifier: " << node.identifier << std::endl;
  std::string type;
  std::string value;
  switch(node.type)
  {
    case shortjson::Field::Undefined: type = "Undefined"; break;
    case shortjson::Field::Boolean  : type = "Boolean"; value = node.toBool() ? "true" : "false"; break;
    case shortjson::Field::Array    : type = "Array"; break;
    case shortjson::Field::Object   : type = "Object"; break;
    case shortjson::Field::Float    : type = "Float"; value = std::to_string(node.toFloat()); break;
    case shortjson::Field::Integer  : type = "Integer"; value = std::to_string(node.toNumber()); break;
    case shortjson::Field::Null     : type = "Null"; break;
    case shortjson::Field::String   : type = "String"; value = node.toString(); break;
  }
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


int main(int argc, char* argv[])
{
  // NOTE: unicode/hex/octal escape sequences generated with https://onlineunicodetools.com/escape-unicode
  try
  {
    parse_test("{\"normal quoted string\" : \"string\" }", "string");
    parse_test("{\"EMCAScript quoted string\" : 'string' }", "string");

    parse_test("{'bad boolean true 1' : TRUE }", "true");
    parse_test("{'bad boolean true 2' : True }", "true");
    parse_test("{'good boolean true' : true }", "true");

    parse_test("{'bad boolean false 1' : FALSE }", "false");
    parse_test("{'bad boolean false 2' : False }", "false");
    parse_test("{'good boolean false' : false }", "false");

    parse_test("{'bad null 1' : NULL }", "");
    parse_test("{'bad null 2' : Null }", "");
    parse_test("{'good null' : null }", "");

    parse_test("{'unescaped UTF-16' : 'Hello World! ☺'}", "Hello World! ☺");
    parse_test("{'escaped UTF-16' : '\\u0048\\u0065\\u006c\\u006c\\u006f\\u0020\\u0057\\u006f\\u0072\\u006c\\u0064\\u0021\\u0020\\u263a'}", "Hello World! ☺");
    parse_test("{'escaped hexadecimal' : '\\x48\\x65\\x6c\\x6c\\x6f\\x20\\x57\\x6f\\x72\\x6c\\x64\\x21\\x20☺'}", "Hello World! ☺");
    parse_test("{'unpadded octal' : '\\110\\145\\154\\154\\157\\40\\127\\157\\162\\154\\144\\41\\40☺' }", "Hello World! ☺");
    parse_test("{'padded octal' : '\\110\\145\\154\\154\\157\\040\\127\\157\\162\\154\\144\\041\\040☺' }", "Hello World! ☺");

    parse_test("{'signless integer' : 4096 }", "4096");
    parse_test("{'positive integer' : +4096 }", "4096");
    parse_test("{'negative integer' : -4096 }", "-4096");
    parse_test("{'signless hexadecimal integer' : 0x1000 }", "4096");
    parse_test("{'positive hexadecimal integer' : +0x1000 }", "4096");
    parse_test("{'negative hexadecimal integer' : -0x1000 }", "-4096");

    parse_test("{'signless float' : 409600000.004096 }", "409600000.004096");
    parse_test("{'positive float' : +409600000.004096 }", "409600000.004096");
    parse_test("{'negative float' : -409600000.004096 }", "-409600000.004096");

    parse_test("{'signless scientific large float' : 4.096e+10 }", "40960000000.000000");
    parse_test("{'signless scientific normal float' : 4.096e+3 }", "4096.000000");
    parse_test("{'signless scientific small float' : 4.096e-3 }", "0.004096");

    parse_test("{'positive scientific large float' : +4.096e+10 }",  "40960000000.000000");
    parse_test("{'positive scientific normal float' : +4.096e+3 }", "4096.000000");
    parse_test("{'positive scientific small float' : +4.096e-3 }", "0.004096");

    parse_test("{'negative scientific large float' : -4.096e+10 }", "-40960000000.000000");
    parse_test("{'negative scientific normal float' : -4.096e+3 }", "-4096.000000");
    parse_test("{'negative scientific small float' : -4.096e-3 }", "-0.004096");

  }
  catch(const char* error)
  {
    std::cout << error << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
