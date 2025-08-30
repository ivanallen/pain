#pragma once

#include <pain/base/types.h>

namespace pain::common {

class Txn {
public:
    virtual ~Txn() = default;
    virtual Status commit() = 0;
    virtual Status rollback() = 0;
};

} // namespace pain::common
