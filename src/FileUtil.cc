#include "FileUtil.h"

#include <cstdio>

FileWriter::FileWriter(std::string filename)
	: fp_(fopen(filename.c_str(),"ae"))
{
	setbuffer(fp_,buf,sizeof buf);	// 设置应用层缓冲区
}

FileWriter::~FileWriter()
{
	fclose(fp_);
}

void FileWriter::append(const char *logline, const size_t len)
{
	size_t n = write(logline,len);
	size_t remain = len - n;
	while(remain > 0){
		size_t x = write(logline + n,remain);
		if(x == 0){
			int err = ferror(fp_);
			if (err)
                fprintf(stderr, "AppendFile::append() failed !\n");
            break;
		}
		n += x;
		remain = len - n;
	}
}

void FileWriter::flush()
{
	fflush(fp_);
}

size_t FileWriter::write(const char *logline, const size_t len)
{
	return fwrite_unlocked(logline,1,len,fp_);
	// 无锁，非线程安全，但由于只在单线程运行，没有问题，且提高了效率
}