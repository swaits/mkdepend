/**
 * $Id: //depot/mkdepend/mkdepend.cpp#2 $
 */

#if defined(_MSC_VER)
	#pragma warning( disable : 4786 )
#endif

#include <stdio.h>
#include <unistd.h>
#include <map>
#include <set>
#include <vector>
#include <string>

#define MAKEFILETAGSHORT "# DO NOT DELETE"
#define MAKEFILETAGLONG  "# DO NOT DELETE THIS LINE -- make  depend  depends  on it."

// types
typedef std::vector<std::string> stringvec;
typedef struct _FileInfo
{
	bool      scanned;
	stringvec includes;
} FileInfo;

// globals
stringvec gSourceFiles;
stringvec gIncludeDirs;
std::map<std::string,FileInfo> gScanList;
FILE* gOutputFile;

// determine if a file can be found
bool FileExists(std::string filename)
{
	FILE* f = fopen(filename.c_str(),"rt");
	if ( f )
	{
		fclose(f);
		return true;
	}
	return false;
}

// reset all data
void Clear()
{
	gSourceFiles.clear();
	gIncludeDirs.clear();
	gScanList.clear();
}

// add a directory to out global list
void AddIncludeDirectory(std::string dir)
{
	gIncludeDirs.push_back(dir);
}

// add a source file to our global list
void AddSourceFile(std::string source)
{
	gSourceFiles.push_back(source);
}

// search through all include dirs until we find a file or fail
std::string FindIncludeFile(std::string filename,bool check_current=true)
{
	// check filename alone (might be absolute)
	if ( FileExists( filename ) )
	{
		return filename;
	}

	// check current directory
	if ( check_current )
	{
		if ( FileExists( std::string("./") + filename ) )
		{
			return std::string("./") + filename;
		}
	}

	// now check each include dir
	for ( unsigned int i=0;i<gIncludeDirs.size();i++ )
	{
		if ( FileExists( gIncludeDirs[i] + std::string("/") + filename ) )
		{
			return gIncludeDirs[i] + std::string("/") + filename;
		}
	}

	// if we get here the file was never found
	return std::string("");
}

// add a new "unscanned" file to the scan list
bool AddFileToScanList(std::string filename)
{
	if ( FileExists(filename) )
	{
		// determine if the file is in the map yet
		if ( gScanList.count(filename) == 0 )
		{
			// setup temp structure
			FileInfo fi;
			fi.scanned = false;
			fi.includes.clear();

			// finally, add it to the list!
			gScanList[filename] = fi;
		}
		return true;
	}
	return false;
}

// return number of "unscanned" files in the scan list
int FilesToScan()
{
	int unscanned = 0;
	std::map<std::string,FileInfo>::iterator it;
	for ( it=gScanList.begin();it!=gScanList.end();it++ )
	{
		if ( !(*it).second.scanned )
		{
			unscanned++;
		}
	}
	return unscanned;
}

// return true if 'str' begins with 'sub', else false
bool StrBegins(const char* str,const char* sub)
{
	int i = 0;
	while (str[i] && sub[i])
	{
		if (str[i] != sub[i])
		{
			return false;
		}
		i++;
	}

	// if mathced all the way to the end of sub[i] then success
	if (!sub[i])
	{
		return true;
	}
	return false;
}

// grab all include directories and source files from the command line
void ParseCommandLine(int argc,char** argv)
{
	for (int i=1;i<argc;i++)
	{
		if ( StrBegins(argv[i],"-I") )
		{
			//printf("Got Include Directory: '%s'\n",argv[i]+2);
			AddIncludeDirectory(std::string(argv[i]+2));
		}
		else if ( StrBegins(argv[i],"-") )
		{
			fprintf(stderr,"Warning: I don't understand the switch: '%s'\n",argv[i]);
		}
		else
		{
			//printf("Got Source File: '%s'\n",argv[i]);
			AddSourceFile(argv[i]);
		}
	}
}

bool CharInStr(const char* str,const char c)
{
	char* cptr = (char*)str;
	while ( *cptr )
	{
		if ( *cptr == c )
		{
			return true;
		}
		cptr++;
	}
	return false;
}

char* StrSeekChar(const char* str,const char* chars)
{
	char* c = (char*)str;
	while ( *c )
	{
		if ( CharInStr(chars,*c) )
			break;
		c++;
	}

	return c;
}

char* StrSkipWhitespace(const char* str)
{
	char* c = (char*)str;
	while ( *c == ' ' || *c =='\t' || *c == '\n' || *c == '\r' )
	{
		c++;
	}
	return c;
}

char* StrSeekCharSkipWhitespace(char* str,const char* chars)
{
	char* c = str;
	c = StrSkipWhitespace(c);
	if ( *c )
		return StrSeekChar(c,chars);
	else
		return c;
}

void StrTrim(char* str)
{
	// ***TODO***
}

// scan a single file
void ScanFile(std::string filename)
{
	if ( gScanList[filename].scanned )
		return;
	gScanList[filename].scanned = true;

	// open file
	FILE* f = fopen(filename.c_str(),"rt");
	if ( !f )
	{
		// error!
		return;
	}

	// process each line of the file
	char linebuf[4096];
	while ( fgets(linebuf,4096,f) )
	{
		char* c = linebuf;

		c = StrSkipWhitespace(c);
		if ( !(*c) )
			continue;

		if ( !StrBegins(c,"#") )
			continue;

		// move to next char
		c++;
		if ( !(*c) )
			continue;

		c = StrSkipWhitespace(c);
		if ( !(*c) )
			continue;

		if ( StrBegins(c,"include") )
			c += 7;
		else
			continue;

		c = StrSeekCharSkipWhitespace(c,"<\"");
		if ( !(*c) )
			break;
		char quote_type = *c;

		// move to next char
		c++;
		if ( !(*c) )
			break;
		char* fn_begin = c;

		c = StrSeekChar(c,quote_type == '<' ? ">" : "\"" );
		if ( !(*c) )
			break;
		*c = '\0';

		// we can trim it if it was a <> type
		if ( quote_type == '<' )
			StrTrim(fn_begin);

		std::string incfile = FindIncludeFile(std::string(fn_begin));
		if ( incfile != "" )
		{
			gScanList[filename].includes.push_back(incfile);
			AddFileToScanList(incfile);
		}
		else
			if ( quote_type == '\"' )
			fprintf(stderr,"Warning: Unable to find include file '%s' included from '%s'\n",fn_begin,filename.c_str());
	}

	// close file
	fclose(f);
}

// replace .cpp with .o, etc.
std::string ExtSubst(const std::string filename)
{
	std::string ret = filename;

	// find the last '.'
	ret.resize( ret.find('.') );

	// add a ".o"
	ret += ".o";

	return ret;
}

// this does the actual work
void DiscoverDependencies()
{
	// add all the source files to the scan list
	for (unsigned int i=0;i<gSourceFiles.size();i++)
	{
		if ( !AddFileToScanList(gSourceFiles[i]) )
			fprintf(stderr,"Warning: Unable to find source file '%s'\n",gSourceFiles[i].c_str());
	}

	// iterate through the scan list until all files have been scanned
	while ( FilesToScan() )
	{
		std::map<std::string,FileInfo>::iterator it;
		for ( it=gScanList.begin();it!=gScanList.end();it++ )
		{
			ScanFile((*it).first);
		}
	}
}

void WriteDependencies()
{
	fprintf(gOutputFile,"\n");

	// output each source file
	for ( unsigned int i=0;i<gSourceFiles.size();i++ )
	{
		std::set<std::string> dumped;
		std::set<std::string> dependencies;

		// add our includes for this file
		for ( unsigned int j=0;j<gScanList[gSourceFiles[i]].includes.size();j++ )
			dependencies.insert(gScanList[gSourceFiles[i]].includes[j]);

		// now "recurse" without recursing
		while ( dumped != dependencies )
		{
			std::set<std::string>::iterator it;
			for ( it = dependencies.begin(); it != dependencies.end(); it++ )
			{
				if ( dumped.find(*it) == dumped.end() )
				{
					dumped.insert(*it);
					for ( unsigned int j=0;j<gScanList[*it].includes.size();j++ )
					{
						dependencies.insert(gScanList[*it].includes[j]);
					}
				}
			}
		}

		// and output
		if ( !dependencies.empty() )
		{
			std::string objfile = ExtSubst(gSourceFiles[i]);
			std::set<std::string>::iterator it;
			fprintf(gOutputFile,"%s: ",objfile.c_str());
			for ( it = dependencies.begin(); it != dependencies.end(); it++ )
				fprintf(gOutputFile,"\\\n\t%s ",(*it).c_str());
			fprintf(gOutputFile,"\n\n");
		}
	}
}

void OpenOutputFile()
{
	FILE* oldmakefile = fopen("Makefile","r");
	if ( !oldmakefile )
	{
		gOutputFile = stdout;
		return;
	}

	gOutputFile = fopen("Makefile.tmp","w");
	if ( !gOutputFile )
	{
		// **TODO** should warn user
		gOutputFile = stdout;
		fclose(oldmakefile);
		return;
	}

	// look for our tag line while copying to a temp file
	char instr[4096];
	while ( fgets(instr,4096,oldmakefile) )
	{
		fputs(instr,gOutputFile);
		if ( StrBegins(instr,MAKEFILETAGSHORT) )
		{
			fclose(oldmakefile);
			return;
		}
	}

	// got to the end without finding it, so let's add the tag
	fprintf(gOutputFile,"\n" MAKEFILETAGLONG "\n");
	fclose(oldmakefile);
}

void CloseOutputFile()
{
	fflush(gOutputFile);
	if ( gOutputFile != stdout )
	{
		// close temp file
		fclose(gOutputFile);

		// copy to actual Makefile
		FILE* dest = fopen("Makefile","w");
		if ( !dest )
		{
			// **TODO** warn user
			return;
		}

		FILE* src = fopen("Makefile.tmp","r");
		if ( !src )
		{
			// **TODO** warn user
			fclose(dest);
			return;
		}

		// do copy
		char instr[4096];
		while ( fgets(instr,4096,src) )
		{
			fputs(instr,dest);
		}
		fclose(src);
		fclose(dest);
	}
}

int main(int argc,char** argv)
{
	Clear();
	ParseCommandLine(argc,argv);
	DiscoverDependencies();
	OpenOutputFile();
	WriteDependencies();
	CloseOutputFile();
	return 0;
}

