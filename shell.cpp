#define usint unsigned short int
#define uncast reinterpret_cast<unsigned short int>
#define recast reinterpret_cast<char *>
#define rechcast reinterpret_cast<const char *> 
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/time.h>

using namespace std;

struct DirEntry{
    char filename[11];
    char attributes;
    unsigned long created_time;
    usint address;
    unsigned int filesize;
    char reserved[6]; 
};

usint getFATindex(int);
void setFATindex(int, usint);
char* getDataCluster(int);
void setDataCluster(int,char*);
bool hasNextCluster(int);
DirEntry makeDirEntry(char*,char,usint,unsigned int);
char* getDirRawData(int, char*);
DirEntry* parseDirEntries(char*);
char* packDirEntries(DirEntry*);
vector<string> getTokens(string, char);

const int FAT_OFFSET = 512; // 1 sector
const int DATA_OFFSET = FAT_OFFSET + 256*1024; // 1 sector + 256kB
const int CLUSTER_SIZE = 512*8; // 4kB (8 sectors/cluster at 512B/sector) 
const usint FAT_EOF = 0xFFFF;
// DirEntry Attribute Constants 
const char ATTR_READONLY = 0x01;
const char ATTR_HIDDEN = 0x02;
const char ATTR_SYSTEMFILE = 0x04;
const char ATTR_VOLUMELABEL = 0x08;
const char ATTR_DIRECTORY = 0x10;
const char ATTR_FILE = 0x20;

fstream FAT;
string CURR_DIR;

int main(int argc, char* argv[]){
    if(argc > 1){
        FAT.open(argv[1], ios::binary | ios::in);
    }else{
        cout << "Please include filename in args" << endl;
        return 0;
    }
    if(!FAT.is_open()){
        cout << "Can't open file!" << endl;
        return 1;
    }
    cout << "File opened succesfully." << endl;
    cout << "READING BOOT SECTOR" << endl;
    char* OS_name = new char[8];
    FAT.seekg(FAT.beg+3);
    FAT.read(OS_name,8);
    cout << "OS NAME: " << OS_name << endl;
    /* Check FS Initalized status */
    cout << "Checking FS status..." << endl;
    cout << "FAT index 0: " << getFATindex(0) << endl;
    cout << "FAT index 1: " << getFATindex(1) << endl;
    cout << "FAT index 2 (ROOT): " << getFATindex(2) << endl;
    if (getFATindex(2)==0){
        cout << "FAT still uninitalized. Writing root..." << endl;
        DirEntry* root = parseDirEntries(getDataCluster(2));
        root[0]=makeDirEntry(".",ATTR_DIRECTORY | ATTR_SYSTEMFILE,2,0);
        root[1]=makeDirEntry("..",ATTR_DIRECTORY | ATTR_SYSTEMFILE,2,0);
        setDataCluster(2,packDirEntries(root));
        setFATindex(2,FAT_EOF);
        cout << "Root written." << endl;
    }
    //Welcome to the shell
    int status = 0;
	while(!status) {
		cout << OS_name << "> ";
		string line;
		getline(cin,line);
		vector<string> tokens = getTokens(line, ' ');
        if(tokens.size() > 0)
            cout << "cmd to execute: " << tokens[0] << endl;
        if(tokens.size() == 0 || tokens[0] == "exit")
            status = 1;
		//status = executeCommand(tokens);
	};
    if(status != 1){
		cerr << "ERROR " << status << ": al ejecutar el comando" << endl;
	}
    delete[] OS_name;
    FAT.close();
    return 0;
}

usint getFATindex(int index){
    FAT.seekg(FAT.beg+FAT_OFFSET+(index*2));
    usint value;
    FAT.read(recast(&value),2);
    return value;
}

void setFATindex(int index, usint value){
    FAT.seekg(FAT.beg+FAT_OFFSET+(index*2));
    FAT.write(rechcast(&value),2);
}

char* getDataCluster(int index){
    FAT.seekg(FAT.beg+DATA_OFFSET+(index*CLUSTER_SIZE));
    char* data = new char[CLUSTER_SIZE];
    FAT.read(data,CLUSTER_SIZE);
    return data;
}

void setDataCluster(int index, char* data){
    FAT.seekg(FAT.beg+DATA_OFFSET+(index*CLUSTER_SIZE));
    FAT.write(data,CLUSTER_SIZE);
}

bool hasNextCluster(int index){
    return getFATindex(index)!=FAT_EOF; 
}

DirEntry makeDirEntry(char* fn, char attr, usint addr, unsigned int size){
    DirEntry entry;
    entry.filename=fn;
    entry.attributes=attr;
    struct timeval tp;
    gettimeofday(&tp, NULL);
    entry.created_time = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    //entry.created_time=std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
    entry.address=addr;
    entry.filesize=size;
    return entry;
}

char* getDirRawData(int index, char* data){
    char* entry = new char[32];
    for(int i=0; i<32; i++){
        entry[i]=data[index*32+i];
    }
    return entry;
}

/**
    Transforms a 4kB cluster of data into 512 DirEntries.

    @param data The 4kB cluster as a char array.
    @return The array of 512 DirEntries.
*/
DirEntry* parseDirEntries(char* data){
    DirEntry* entries = new DirEntry[512];
    for(int i=0; i<512; i++){
        entries[i] = reinterpret_cast<DirEntry>(getDirRawData(i,data));
    }
    return entries;
}

/**
    The reverse of parseDirEntries. It packs 512 DirEntries into a 4kB cluster.

    @param entries Array of 512 DirEntries to pack.
    @return The 4kB cluster as a char array.
*/
char* packDirEntries(DirEntry* entries){
    char* data = new char[CLUSTER_SIZE];
    char* entry;
    for(int i=0; i<512; i++){
        entry=recast(&(entries[i]));
        for(int j=0; j<32; j++){
            data[i*32+j]=entry[j];
        }
    }
    return data;
}

/**
    Only works with one char delimiter but it works!!

    @param entries Array of 512 DirEntries to pack.
    @return The 4kB cluster as a char array.
*/
vector<string> getTokens(string toTokenize, char delimiter){
	vector<string> v;
	string::iterator stringIT = toTokenize.begin();
	for(string::iterator i = toTokenize.begin(); i != toTokenize.end(); i++){
		if(*i == delimiter){
			v.push_back(string(stringIT,i));
			stringIT=i+1;
		}
	}
	v.push_back(string(stringIT,toTokenize.end()));
	return v;
}