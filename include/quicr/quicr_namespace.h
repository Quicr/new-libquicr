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

    friend bool operator==(const Namespace& a, const Namespace& b);
    friend bool operator!=(const Namespace& a, const Namespace& b);
    friend bool operator>(const Namespace& a, const Namespace& b);
    friend bool operator<(const Namespace& a, const Namespace& b);

private:
    Name _name;
    uint64_t _sig_bits{120};
};
}
