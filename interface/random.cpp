#include "random.h"
#include "../interface/interface.h"

uint32_t host::Random() {
	return host_random();
}
