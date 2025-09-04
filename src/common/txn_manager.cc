#include "common/txn_manager.h"

namespace pain::common {

thread_local TxnStore* TxnManager::g_txn_store = nullptr;

} // namespace pain::common
