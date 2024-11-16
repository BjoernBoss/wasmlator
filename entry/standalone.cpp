#include "../interface/host.h"
#include "../interface/interface.h"

int main() {
	host::SetLogLevel(host::LogLevel::debug);

	StartupProcess();
	return 0;
}
