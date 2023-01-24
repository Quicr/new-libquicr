#pragma once

#include "quicr_name.h"

namespace quicr
{
class Namespace
{
public:
    Namespace() = delete;
    Namespace(const Name& name);
    Namespace(Name&& name);

    bool contains(const Name& name) const;
    bool contains(const Namespace& name_space) const;

private:
    Name _name;
    uint64_t _sig_bits{120};
};
}
