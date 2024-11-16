#include "../env-process.h"

template <class Type>
void ReadAll(uint32_t offset, uint8_t* data, uint32_t size, uint32_t& i) {
	while (size - i >= uint32_t(sizeof(Type))) {
		*(Type*)(data + i) = Type(env::detail::ContextBridge::Read(offset + i, sizeof(Type)));
		i += sizeof(Type);
	}
}
template <class Type>
void WriteAll(uint32_t offset, const uint8_t* data, uint32_t size, uint32_t& i) {
	while (size - i >= uint32_t(sizeof(Type))) {
		env::detail::ContextBridge::Write(offset + i, sizeof(Type), *(Type*)(data + i));
		i += sizeof(Type);
	}
}

void env::Context::fRead(uint32_t offset, uint8_t* data, uint32_t size) const {
	if (offset + size >= pSize)
		host::Fatal(u8"Cannot read [", offset + size, u8"] bytes from context of size [", pSize, u8']');

	uint32_t i = 0;
	ReadAll<uint64_t>(offset, data, size, i);
	ReadAll<uint32_t>(offset, data, size, i);
	ReadAll<uint16_t>(offset, data, size, i);
	ReadAll<uint8_t>(offset, data, size, i);
}
void env::Context::fWrite(uint32_t offset, const uint8_t* data, uint32_t size) const {
	if (offset + size >= pSize)
		host::Fatal(u8"Cannot write [", offset + size, u8"] bytes from context of size [", pSize, u8']');

	uint32_t i = 0;
	WriteAll<uint64_t>(offset, data, size, i);
	WriteAll<uint32_t>(offset, data, size, i);
	WriteAll<uint16_t>(offset, data, size, i);
	WriteAll<uint8_t>(offset, data, size, i);
}
void env::Context::fTerminate(int32_t code) {
	throw env::Termianted{ code };
}
void env::Context::fNotDecodable(env::guest_t address) {
	throw env::NotDecodable{ address };
}
