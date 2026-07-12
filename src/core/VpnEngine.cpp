#include "vpnofmes/core/VpnEngine.h"

namespace vpnofmes::core {

// Out-of-line destructor: pins VpnEngine's vtable to this translation unit
// instead of duplicating it in every file that includes the header.
VpnEngine::~VpnEngine() = default;

} // namespace vpnofmes::core
