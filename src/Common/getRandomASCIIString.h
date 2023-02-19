#pragma once

#include <Core/Types.h>


namespace DB
{

/// Slow random string. Useful for random names and things like this. Not for generating data.
String getRandomASCIIString(size_t length);

}
