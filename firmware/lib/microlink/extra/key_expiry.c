// What the robot needs from microlink beyond its public header: the node
// key's expiry, which microlink parses from the control server's answer but
// keeps in its internal state. Declared in src/remote_access/tailscale.cpp.

#include <stdbool.h>
#include <stdint.h>

#include "microlink_internal.h"

// Unix time (seconds) the node key expires at, 0 = never or unknown yet.
int64_t cwc_microlink_key_expiry(const microlink_t *ml) { return ml ? ml->key_expiry_epoch : 0; }

// Whether the control server said the node key has expired.
bool cwc_microlink_key_expired(const microlink_t *ml) { return ml && ml->key_expired; }
