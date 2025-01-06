#include "../interface/logger.h"
#include "../interface/interface.h"

int main() {
	host::SetLogLevel(host::LogLevel::trace);

	while (true) {
		std::string cmd;
		std::getline(std::cin, cmd);
		HandleCommand(str::u8::To(cmd));
	}
	return 0;
}
