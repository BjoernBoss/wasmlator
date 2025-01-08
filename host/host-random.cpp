#include "host-random.h"
#include "../host/interface.h"

uint32_t host::Random() {
	return host_random();
}
