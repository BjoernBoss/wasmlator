#include "../interface/host.h"
#include "../interface/interface.h"

int main() {
	host::SetLogLevel(host::LogLevel::trace);

	StartupProcess();
	return 0;
}
