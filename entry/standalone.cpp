#include "../util/util-logger.h"
#include "../host/interface.h"

int main() {
	util::SetLogLevel(util::LogLevel::trace);

	while (true) {
		std::string cmd;
		std::getline(std::cin, cmd);
		HandleCommand(str::u8::To(cmd));
	}
	return 0;
}
