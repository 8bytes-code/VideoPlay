#pragma once
#include <string>
typedef unsigned char BYTE;

/*自定义类继承string，额外封装一些运算符重载和功能*/
class HBuffer :public std::string {
public:
	HBuffer(const char* str) {
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}
	HBuffer(size_t size = 0) :std::string() {
		if (size > 0) {
			resize(size);
			memset(*this, 0, this->size());
		}
	}
	HBuffer(void* buffer, size_t size) :std::string() {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	~HBuffer() {
		std::string::~basic_string();
	}

	operator char* () const { return (char*)c_str(); }
	operator const char* () const { return c_str(); }
	operator BYTE* () const { return (BYTE*)c_str(); }
	operator void* () const { return (void*)c_str(); }

	void Update(void* buffer, size_t size) {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}

	void Zero() {
		if (size() > 0) memset((void*)c_str(), 0, size());
	}

	HBuffer& operator<<(const HBuffer& str) {
		if (this != str) {
			*this += str;
		}
		else {
			HBuffer tmp = str;
			*this += tmp;
		}
		return *this;
	}

	HBuffer& operator<<(const std::string& str) {
		*this += str;
		return *this;
	}

	HBuffer& operator<<(const char* str) {
		*this += HBuffer(str);
		return *this;
	}

	HBuffer& operator<<(int data) {
		char s[16] = "";
		snprintf(s, sizeof(s), "%d", data);
		*this += s;
		return *this;
	}

	const HBuffer& operator>>(int& data) const {
		data = atoi(c_str());
		return *this;
	}

	const HBuffer& operator>>(short& data) const {
		data = (short)atoi(c_str());
		return *this;
	}
};