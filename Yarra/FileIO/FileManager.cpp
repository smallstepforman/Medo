/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Yarra package file format
*/

#include <vector>
#include <cstring>
#include <cassert>

#include "Yarra/Platform.h"
#include "Yarra/FileManager.h"


/**************************************************
	SimpleFastHash written by Paul Larson.
	http://research.microsoft.com/~PALARSON/
***************************************************/
static unsigned int SimpleFastHash(const char* s, unsigned int seed)
{
	unsigned hash = seed;
	while (*s)
		hash = hash * 101  +  *s++;
	return hash;
}

/**********************************************
	Abstract base class
***********************************************/
class YFileSystem
{
public:
				YFileSystem() {}
	virtual		~YFileSystem() {}

	virtual const bool	AddResource(const char *file) = 0;

	virtual YFILE		*Open(const char *filename, const char *mode) = 0;
	virtual int			Close(YFILE *stream) = 0;
	virtual size_t		Read(void *ptr, size_t size, size_t count, YFILE *stream) = 0;
	virtual int			Seek(YFILE *stream, size_t offset, int origin) = 0;
	virtual size_t		Tell(YFILE *stream) = 0;
	virtual int			Eof(YFILE *stream) = 0;
	virtual void		Rewind(YFILE *stream) = 0;
};

/********************************************************
	YFileSystem_Dir reads from a directory
*********************************************************/
class YFileSystem_Dir : public YFileSystem
{
	std::vector<std::string> fFilePaths;
public:
	//	AddResource() interface
	const bool AddResource(const char *file)
	{
		fFilePaths.push_back(std::string(file));
		return true;
	}
	//	Yfopen() interface
	YFILE * Open(const char *filename, const char *mode)
	{
		assert(!fFilePaths.empty());

		//	Check virtual file system path(s)
		for (auto resource_path : fFilePaths)
		{
			std::string full_path = resource_path;
			full_path.append(filename);
		
			YFILE *f = new YFILE;
			f->mFile = fopen(full_path.c_str(), mode);
			if (f->mFile)
			{
				f->mRawPath = 0;
				return f;
			}
			else
				delete f;
		}

		//	Try RAW path
		YFILE *f = new YFILE;
		f->mFile = fopen(filename, mode);
		if (!f->mFile)
		{
			delete f;
			return NULL;
		}
		else
		{
			f->mRawPath = 1;
			return f;
		}
	}
	//	Yfclose() interface
	int Close(YFILE *stream)
	{
		int ret = fclose(stream->mFile);
		delete stream;
		stream = NULL;
		return ret;
	}
	//	Yfread() interface
	size_t Read(void *ptr, size_t size, size_t count, YFILE *stream)
	{
		return fread(ptr, size, count, stream->mFile);
	}
	//	Yfseek() interface
	int Seek(YFILE *stream, size_t offset, int origin)
	{
		return fseek(stream->mFile, (long)offset, origin);
	}
	//	Yftell() interface
	size_t Tell(YFILE *stream)
	{
		return ftell(stream->mFile);
	}
	//	Yfeof() interface
	int Eof(YFILE *stream)
	{
		return feof(stream->mFile);
	}
	//	Yrewind() interface
	void Rewind(YFILE *stream)
	{
		fseek(stream->mFile, 0L, SEEK_SET);
	}
};

/********************************************************
	YFileSystem_Packed reads from a packed resource file
*********************************************************/
struct YPackageItem
{
	unsigned int	hash;
	std::string		filename;
	unsigned int	size;
	unsigned int	location;

	inline bool operator ()(const YPackageItem &a,  const YPackageItem &b) 
	{
		return (a.hash < b.hash);
	}
} SORT_PACKAGE_ITEM;

class YFileSystem_Packed : public YFileSystem
{
	std::vector	<YPackageItem *>	fFiles;
	size_t							fFileSize;
	std::string						fPackageFile;
	bool							fEncrypted;
public:
	//======================
	//	destructor
	~YFileSystem_Packed()
	{
		for (std::vector<YPackageItem *>::iterator i = fFiles.begin(); i != fFiles.end(); i++)
			delete *i;
		fFiles.clear();
	}
	//======================
	//	AddResource() interface
	const bool AddResource(const char *file)
	{
		FILE *f = fopen(file, "rb");
		if (!f)
			yplatform::Exit("YPackageManager::AddPackage(%s) - file not found\n", file);
	
		fPackageFile.assign(file);

		//	Validate size for header
		fseek(f, 0, SEEK_END);
		fFileSize = ftell(f);	
		fseek(f, 0, SEEK_SET);
		if (fFileSize < 12)
			yplatform::Exit("YPackageManager::AddPackage(%s) - file size too small to parse header\n", file);

		size_t ret;

		//	Validate signature
		const char *kSignature = "ypkg";
		char buffer[4];
		ret = fread(buffer, 4, 1, f);
		if ((ret != 1) || (strncmp(buffer, kSignature, 4) != 0))
			yplatform::Exit("YPackageManager::AddPackage(%s) - file signature incorrect\n", file);

		//	Load header size
		unsigned int header_size;
		fread(&header_size, sizeof(unsigned int), 1, f);
		fEncrypted = (header_size & 0x80000000) ? true : false;
		header_size &= 0x7fffffff;
		if ((ret != 1) || (header_size > fFileSize - 8))
			yplatform::Exit("YPackageManager::AddPackage(%s) - file header corrupt (1)\n", file);
		unsigned char *header_buffer = new unsigned char [header_size];
		ret = fread(header_buffer, header_size, 1, f);
		if (ret != 1)
			yplatform::Exit("YPackageManager::AddPackage(%s) - file header corrupt (2)\n", file);
		fclose(f);

		//	Decrypt header
		if (fEncrypted)
		{
			unsigned char *hp = header_buffer;
			for (unsigned int i = 0; i < header_size; i++)
				*hp++ ^= (unsigned char)(header_size^i);
		}

		//	Count items
		unsigned char *p = header_buffer;
		unsigned int count_items = *((unsigned int *)p);
		p += sizeof(unsigned int);
		fFiles.reserve(count_items);
		for (unsigned int i=0; i < count_items; i++)
		{
			YPackageItem *item = new YPackageItem;
			fFiles.push_back(item);

			//	hash
			item->hash = *((unsigned int *)p);
			p += sizeof(unsigned int);
			//	filename length
			unsigned int len = *((unsigned int *)p);
			p += sizeof(unsigned int);
			//	filename
			item->filename.assign((const char *)p, len);
			p += len;
			//	file size
			item->size = *p | (*(p+1)<<8) | (*(p+2)<<16) | (*(p+3)<<24);
			p += sizeof(unsigned int);
			//	location
			item->location = *p | (*(p+1)<<8) | (*(p+2)<<16) | (*(p+3)<<24);
			p += sizeof(unsigned int);
		}
		assert(p - header_buffer == header_size);
		delete [] header_buffer;
	
		#if 0
		for (std::vector<YPackageItem *>::iterator i = fFiles.begin(); i != fFiles.end(); i++)
			YPlatform_Debug("Hash = %lu, size = %lu,  filename = %s\n", (*i)->hash, (*i)->size, (*i)->filename.c_str());
		YPlatform_Debug("Count files = %d\n", fFiles.size());
		#endif
		
		return true;
	}
	//======================
	//	Yfopen() interface
	YFILE * Open(const char *filename, const char *mode)
	{
		assert((filename != NULL) && (*filename != 0));
		unsigned int hash = SimpleFastHash(filename, 0);
#if 0
		//	Linear search
		YFILE *f1;
		for (std::vector<YPackageItem *>::iterator i = fFiles.begin(); i != fFiles.end(); i++)
		{
			if ((hash == (*i)->hash) && ((*i)->filename.compare(filename) == 0))
			{
				YFILE *f = new YFILE;
				f->mFile = fopen(fPackageFile.c_str(), mode);
				f->mItem = (void *)*i;
				f->mOffset = 0;
				fseek(f->mFile, ((YPackageItem *)f->mItem)->location, SEEK_SET);
				return f;
			}
		}
		return NULL;
#else
		//	Binary search
		int start = 0;
		int end = (int)fFiles.size() - 1;
		int index = (int)fFiles.size()/2;
		bool found = false;
		while (end >= start && !found)
		{
			index = start + (end - start)/2;
			if (hash < fFiles[index]->hash)
				end = index - 1;
			else if (hash > fFiles[index]->hash)
				start = index + 1;
			else if (fFiles[index]->filename.compare(filename) == 0)
			{
				found = true;
				break;
			}
			else
			{
				// scenario where same hash, but different filename.  Do linear search
				while ((index > 0) && (hash == fFiles[index]->hash))
					--index;
				while (index < (int)fFiles.size())
				{
					if (fFiles[index]->filename.compare(filename) == 0)
					{
						found = true;
						break;
					}
					index++;
				}
				assert(0);
			}
		}
		if (found)
		{
			YFILE *f = new YFILE;
			f->mFile = fopen(fPackageFile.c_str(), mode);
			f->mItem = (void *)fFiles[index];
			f->mOffset = 0;
			f->mRawPath = 0;
			fseek(f->mFile, (long)(((YPackageItem *)f->mItem)->location), SEEK_SET);
			return f;
		}
		else
		{
			//	Try RAW path
			YFILE *f = new YFILE;
			f->mFile = fopen(filename, mode);
			if (!f->mFile)
			{
				delete f;
				return NULL;
			}
			else
			{
				f->mRawPath = 1;
				return f;
			}
		}
#endif
	}

	//======================
	//	Yfclose() interface
	int Close(YFILE *stream)
	{
		assert(stream != NULL);
		int ret = fclose(stream->mFile);
		delete stream;
		return ret;
	}
	//======================
	//	Yfread() interface
	size_t Read(void *ptr, size_t size, size_t count, YFILE *stream)
	{
		assert(stream != NULL);
		if (count*size == 0)
			return 0;
		assert(ptr != NULL);
		assert(stream != NULL);

		if (stream->mRawPath)
			return fread(ptr, size, count, stream->mFile);

		//	Size checks
		if (stream->mOffset + size*count > ((YPackageItem *)stream->mItem)->size)
		{
			size = 1;
			count = ((YPackageItem *)stream->mItem)->size - stream->mOffset;
		}

		size_t ret = fread(ptr, size, count, stream->mFile);

		//	Decryption loop
		if (fEncrypted)
		{
			unsigned char *bfr = (unsigned char *)ptr;
			size_t loc = ((YPackageItem *)stream->mItem)->location + stream->mOffset;
			size_t file_size = ((YPackageItem *)stream->mItem)->size;
			for (size_t i=0; i < size*count; i++)
			{
				*bfr++ ^= (unsigned char)(loc^file_size);
				loc++;
			}
		}

		stream->mOffset += size*count;
		return ret;
	}
	//======================
	//	Yfseek() interface
	int Seek(YFILE *stream, size_t offset, int origin)
	{
		assert(stream != NULL);

		if (stream->mRawPath)
			return fseek(stream->mFile, (long)offset, origin);
	
		if (origin == SEEK_SET)
			stream->mOffset = offset;
		else if (origin == SEEK_CUR)
			stream->mOffset += offset;
		else if (origin == SEEK_END)
			stream->mOffset = ((YPackageItem *)stream->mItem)->size;
		else
			assert(0);

		if (stream->mOffset > ((YPackageItem *)stream->mItem)->size)
			stream->mOffset = ((YPackageItem *)stream->mItem)->size;
		int ret = fseek(stream->mFile, (long)(((YPackageItem *)stream->mItem)->location + stream->mOffset), SEEK_SET);
		return ret;
	}
	//======================
	//	Yftell() interface
	size_t Tell(YFILE *stream)
	{
		assert(stream != NULL);
		assert(stream != NULL);
		if (stream->mRawPath)
			return ftell(stream->mFile);
		else
			return stream->mOffset;
	}
	//======================
	//	Yfeof() interface
	int Eof(YFILE *stream)
	{
		assert(stream != NULL);
		assert(stream != NULL);
		if (stream->mRawPath)
			return feof(stream->mFile);
		
		if (stream->mOffset >= ((YPackageItem *)stream->mItem)->size)
			return 1;
		else
			return 0;
	}
	//======================
	//	Yrewind() interface
	void Rewind(YFILE *stream)
	{
		assert(stream);
		if (stream->mRawPath)
			fseek(stream->mFile, 0L, SEEK_SET);
		else
		{
			stream->mOffset = 0;
			fseek(stream->mFile, (long)(((YPackageItem *)stream->mItem)->location + stream->mOffset), SEEK_SET);
		}
	}
};

/*************************************************************************************************
	fstream replacement functions
**************************************************************************************************/

static YFileSystem	*sFileSystem = NULL;
static bool sFileSystemIsPacked = false;

/*	FUNCTION:		YInitFileManager
	ARGUMENTS:		filename
	RETURN:			n/a
	DESCRIPTION:	Init file manager
					If the filename contains the substring ".ypkg", then this is a packed resource
					Otherwise, it's a raw file directory
*/
void	YInitFileManager(const char *filename)
{
	assert(sFileSystem == NULL);
	
	if (strstr(filename, ".ypkg"))
	{
		sFileSystem = new YFileSystem_Packed;
		sFileSystemIsPacked = true;
	}
	else
		sFileSystem = new YFileSystem_Dir;

	sFileSystem->AddResource(filename);
}

/*	FUNCTION:		YAddFileSystem
	ARGUMENTS:		filename
	RETURN:			n/a
	DESCRIPTION:	Add directory to virtual file system
*/
void	YAddFileSystem(const char *filename)
{
	assert(sFileSystem != nullptr);
	assert(sFileSystemIsPacked == false);
	sFileSystem->AddResource(filename);
}

/*	FUNCTION:		YDestroyFileManager
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Destroy file manager
*/
void	YDestroyFileManager()
{
	assert(sFileSystem != NULL);
	delete sFileSystem;
	sFileSystem = NULL;
}

/*	FUNCTION:		Yfopen
	ARGUMENTS:		filename
					mode
	RETURN:			file handle
	DESCRIPTION:	Wrapper for fopen()
*/
YFILE * Yfopen(const char *filename, const char *mode)
{
	assert((mode != 0) && (mode[0] == 'r'));
	if (mode[1] != 0)
		assert(mode[1] == 'b');
	return sFileSystem->Open(filename, mode);
}

/*	FUNCTION:		Yfclose
	ARGUMENTS:		stream
	RETURN:			0 on success, EOF on failure
	DESCRIPTION:	Wrapper for fclose()
*/
int Yfclose(YFILE * stream)
{
	return sFileSystem->Close(stream);
}

/*	FUNCTION:		Yfread
	ARGUMENTS:		ptr
					size
					count
					stream
	RETURN:			count of items read
	DESCRIPTION:	Wrapper for fread()
*/
size_t Yfread(void *ptr, size_t size, size_t count, YFILE *stream)
{
	return sFileSystem->Read(ptr, size, count, stream);
}

/*	FUNCTION:		Yfseek
	ARGUMENTS:		stream
					offset
					origin
	RETURN:			zero for success, nonzero for error
	DESCRIPTION:	Wrapper for fseek()
*/
int Yfseek(YFILE *stream, size_t offset, int origin)
{
	return sFileSystem->Seek(stream, offset, origin);
}

/*	FUNCTION:		Yftell
	ARGUMENTS:		stream
	RETURN:			current position in stream
	DESCRIPTION:	Wrapper for ftell()
*/
size_t Yftell(YFILE *stream)
{
	return sFileSystem->Tell(stream);
}

/*	FUNCTION:		Yfeof
	ARGUMENTS:		stream
	RETURN:			non-zero if end of file indicator set, zero if not end of file
	DESCRIPTION:	Wrapper for feof()
*/
int Yfeof(YFILE *stream)
{
	return sFileSystem->Eof(stream);
}

/*	FUNCTION:		Yrewind
	ARGUMENTS:		stream
	RETURN:			n/a
	DESCRIPTION:	Wrapper for rewind()
*/
void Yrewind(YFILE *stream)
{
	sFileSystem->Rewind(stream);
}

