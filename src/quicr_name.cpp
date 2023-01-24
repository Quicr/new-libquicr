#include "quicr_name.h"

#include <string>

namespace quicr
{
Name::Name(uint64_t value)
{
    
}

Name::Name(const std::string& hex_value)
{
    
}

Name::Name(uint8_t* data, size_t length)
{
    
}

Name::Name(const std::vector<uint8_t>& data)
{
    
}

Name::Name(const Name& other)
{
    
}

Name::Name(Name&& other)
{
    
}

std::vector<uint8_t> Name::data() const
{
    
}

size_t Name::size() const
{
    
}

std::string Name::to_hex() const
{
    
}

Name Name::operator>>(uint16_t value)
{
    
}

Name Name::operator<<(uint16_t value)
{
    
}

Name Name::operator+(uint64_t value)
{
    
}

Name Name::operator-(uint64_t value)
{
    
}

Name Name::operator&(uint64_t value)
{
    
}

Name Name::operator|(uint64_t value)
{
    
}

Name Name::operator&(const Name& other)
{
    _hi &= other._hi;
    _low &= other._low;
    return *this;
}

Name Name::operator|(const Name& other)
{
    
}

Name& Name::operator=(const Name& other)
{
    _hi = other._hi;
    _low = other._low;
    return *this;
}

Name& Name::operator=(Name&& other)
{
    _hi = std::move(other._hi);
    _low = std::move(other._low);
    return *this;
}

bool operator==(const Name& a, const Name& b)
{
    return !(a._hi ^ b._hi | a._low ^ b._low);
}

bool operator!=(const Name& a, const Name& b)
{
    return !(a == b);
}

bool operator>(const Name& a, const Name& b)
{
    return std::tie(a._hi, a._low) > std::tie(b._hi, b._low);
}

bool operator<(const Name& a, const Name& b)
{
    return std::tie(a._hi, b._low) < std::tie(b._hi, b._low);
}
}
