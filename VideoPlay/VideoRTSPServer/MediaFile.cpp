#include "MediaFile.h"

MediaFile::MediaFile() : m_file(NULL), m_type(-1) {
}

MediaFile::~MediaFile() {
	Close();
}

int MediaFile::Open(const HBuffer& path, int nType) {
	m_file = fopen(path, "rb");
	if (m_file == NULL) {
		return -1;
	}
	m_type = nType;
	//先将文件指针移动到尾部，然后通过ftell计算出偏移量得到长度
	fseek(m_file, 0, SEEK_END);
	m_size = ftell(m_file);
	Reset();

	return 0;
}

HBuffer MediaFile::ReadOneFrame() {
	switch (m_type) {
		case 96:
			return ReadH264Frame();
			break;
		default:break;
	}
	return HBuffer();
}

void MediaFile::Close() {
	m_type = -1;
	if (m_file != NULL) {
		FILE* pfile = m_file;
		m_file = NULL;
		fclose(pfile);
	}
}

void MediaFile::Reset() {
	if (m_file) {
		fseek(m_file, 0, SEEK_SET);
	}
}

long MediaFile::FindH264Head(int& headsize) {
	while (!feof(m_file)) {
		char c = 0x7F;
		//先判断是否读到结尾
		while (!feof(m_file)) {
			c = fgetc(m_file);
			if (c == 0) break;
		}

		//不为空，然后读到0，再读到1，默认就当00 00 01
		//或者不为空，读到0，再读到0，再读到1，视作00 00 00 01
		if (!feof(m_file)) {
			c = fgetc(m_file);
			if (c == 0) {
				c = fgetc(m_file);
				if (c == 1) {
					headsize = 3;
					return ftell(m_file) - 3;
				}
				else if (c == 0) {
					c = fgetc(m_file);
					if (c == 1) {
						headsize = 4;
						return ftell(m_file) - 4;
					}
				}
			}
		}
	}

	return -1;
}

HBuffer MediaFile::ReadH264Frame() {
	if (m_file) {
		int headsize = 0;
		long off = FindH264Head(headsize);
		if (off == -1)return HBuffer();
		fseek(m_file, off + headsize, SEEK_SET);	//读到头之后就移动文件指针位置
		long tail = FindH264Head(headsize);
		//若是没有下一帧了，就让taile=文件总长度
		if (tail == -1) tail = m_size;
		//否则就是两种情况
		//读到的这一帧和下一帧之间从差得到size
		//文件总长度-这一帧长度得到size
		long size = tail - off;
		fseek(m_file, off, SEEK_SET);
		HBuffer result(size);
		fread(result, 1, size, m_file);
		return result;
	}
	return HBuffer();
}


