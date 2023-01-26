#include <quicr/quicr_namespace.h>

#include <iostream>
#include <sstream>

namespace quicr {

const size_t max_uint_type_bit_size = sizeof(Name::uint_type) * 8 * 2;

Namespace::Namespace(const Name& name, uint16_t sig_bits)
  : _name{ name }
  , _sig_bits{ sig_bits }
{
}

Namespace::Namespace(Name&& name, uint16_t sig_bits)
  : _name{ std::move(name) }
  , _sig_bits{ sig_bits }
{
}

bool
Namespace::contains(const Name& name) const
{
  auto insig_bits = max_uint_type_bit_size - _sig_bits;
  return (name >> insig_bits) == (_name >> insig_bits);
}

bool
Namespace::contains(const Namespace& name_space) const
{
  return contains(name_space._name);
}

bool
operator==(const Namespace& a, const Namespace& b)
{
  return a._name == b._name && a._sig_bits == b._sig_bits;
}

bool
operator!=(const Namespace& a, const Namespace& b)
{
  return !(a == b);
}

bool
operator>(const Namespace& a, const Namespace& b)
{
  return a._name > b._name;
}

bool
operator<(const Namespace& a, const Namespace& b)
{
  return a._name < b._name;
}
}