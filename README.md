# shortJSON
shortJSON is for when you just need a simple JSON parser for your project and not a monster with thousands of lines of code and/or spread across so many files you'll never be able to understand it.

It's neither optimized for speed or binary size but instead for brevity by using C++17 STL features.  In total it's about 300 lines comes in _Strict_ and _Tolerant_ flavors.  _Strict_ should be conformant to JSON while _Tolerant_ will read most anything valid in EMCAScript.

Note: comments are **completely** unsupported and will cause it to throw a `const char*` error message.

Feel free to use shortJSON in your project without attribution.  Just copy shortjson.h and shortjson_xxxxxxx.cpp into your source directory.
