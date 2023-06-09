#ifndef FMM_CUBAO_TYPES_HPP
#define FMM_CUBAO_TYPES_HPP

#include <rapidjson/document.h>

// Use the CrtAllocator, because the MemoryPoolAllocator is broken on ARM
// https://github.com/miloyip/rapidjson/issues/200, 301, 388
using RapidjsonAllocator = rapidjson::CrtAllocator;
using RapidjsonDocument =
    rapidjson::GenericDocument<rapidjson::UTF8<>, RapidjsonAllocator>;
using RapidjsonValue =
    rapidjson::GenericValue<rapidjson::UTF8<>, RapidjsonAllocator>;

#endif
