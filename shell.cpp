#define usint unsigned short int
#define uncast reinterpret_cast<unsigned short int>
#define recast reinterpret_cast<char *>
#define rechcast reinterpret_cast<const char *> 
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>

using namespace std;

struct DirEntry{
    char filename[11];
    char attributes;
    time_t created_time;
    usint address;
    unsigned int filesize;
    char reserved[6]; 
};

usint getFATindex(int);
void setFATindex(int, usint);
char* getDataCluster(int);
void setDataCluster(int,char*);
bool hasNextCluster(int);
DirEntry makeDirEntry(string, char, usint, unsigned int);
char* getDirRawData(int, char*);
DirEntry* parseDirEntries(char*);
char* packDirEntries(DirEntry*);
int getNextAvailableIndex();
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
string currentDir="/";
int currentIndex=2;

int main(int argc, char* argv[]){
    if(argc > 1){
        FAT.open(argv[1], ios::out | ios::in);
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
    char OS_name[9];
    FAT.seekg(FAT.beg+3);
    FAT.read(OS_name,8);
    OS_name[8] = '\0';
    cout << "OS NAME: " << OS_name << endl;
    /* Check FS Initalized status */
    cout << "Checking FS status..." << endl;
    cout << "FAT index 0: " << getFATindex(0) << endl;
    cout << "FAT index 1: " << getFATindex(1) << endl;
    cout << "FAT index 2 (ROOT): " << getFATindex(2) << endl;
    if (getFATindex(2)==0){
        cout << "FAT still uninitalized. Writing root..." << endl;
        DirEntry* root = parseDirEntries(getDataCluster(2));
        root[0]=makeDirEntry(".",ATTR_DIRECTORY | ATTR_SYSTEMFILE,2,4096);
        root[1]=makeDirEntry("..",ATTR_DIRECTORY | ATTR_SYSTEMFILE,2,4096);
        setDataCluster(2,packDirEntries(root));
        setFATindex(2,FAT_EOF);
        FAT.flush();
        cout << "Root written." << endl;
    }
    //Welcome to the shell
    int status = 0;
	while(!status) {
		cout << OS_name << "> ";
		string line;
		getline(cin,line);
		vector<string> tokens = getTokens(line, ' ');
        if(tokens.size() == 0 || tokens[0] == "exit"){
            status = 1;
        }else if(tokens[0]=="ls"){
            DirEntry* myDir = parseDirEntries(getDataCluster(currentIndex));
            cout << "Filename   |Type |Date                   |Size" << endl;
            cout << "======================================================" << endl;
            for(int i=0; i<128; i++){
                if(myDir[i].filename[0]!='\0'){//Check if valid DirEntry
                    int j=0;
                    for(j=0; j<11; j++){
                        if(myDir[i].filename[j]=='\0'){
                            break;
                        }
                        cout << myDir[i].filename[j];
                    }
                    for(; j<11; j++){
                        cout << " ";
                    }
                    cout << "|";
                    if(myDir[i].attributes & ATTR_DIRECTORY){
                        cout << "DIR  |";
                    }else{
                        cout << "FILE |";
                    }
                    //cout << myDir[i].created_time << "|"; //UNFORMATTED!!!
                    char mytime[24];
                    int mys = strftime(mytime, 24, "%b %d, %Y; %H:%M:%S", localtime(&(myDir[i].created_time)));
                    cout << mytime;
                    cout << " |";
                    cout << myDir[i].filesize << "B" << endl;
                }
            }
        }else if(tokens[0]=="mkdir"){
            if(tokens.size()>1){
                if(tokens[1].length()>11 || tokens[1].length()<1){
                    cout << "mkdir: Wrong length for specified name! Must be 1 to 11 characters long." << endl;
                }else{
                    bool cont=true;
                    DirEntry* myDir = parseDirEntries(getDataCluster(currentIndex));
                    for(int i=0; i<128; i++){//Check if name already exists in DirEntries
                        // BROKEN. ALLOWS REPEATS
                        if(myDir[i].filename == tokens[1].c_str()){
                            cout << "mkdir: Element with same name exists in directory!" << endl;
                            cont=false;
                            break;
                        }
                    }
                    if(cont){
                        int avail=0;
                        for(int i=0; i<128; i++){//Find available DirEntry
                            if(myDir[i].filename[0]=='\0'){
                                avail=i;
                                break;
                            }
                        }
                        if(avail>0){
                            //get available index in FAT
                            int ind = getNextAvailableIndex();
                            myDir[avail]=makeDirEntry(tokens[1].c_str(),ATTR_DIRECTORY,ind,4096);
                            setFATindex(ind,FAT_EOF);
                            setDataCluster(2,packDirEntries(myDir));
                            cout << "Directory created." << endl;
                        }else{
                            cout << "mkdir: No more space available for entries in current directory!" << endl;
                        }
                    }
                }
            }else{
                cout << "mkdir: Need to specify directory name!" << endl;
            }
        }
        //status = executeCommand(tokens);
	};
    if(status != 1){
		cerr << "ERROR " << status << ": al ejecutar el comando" << endl;
	}
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

DirEntry makeDirEntry(string fn, char attr, usint addr, unsigned int size){
    DirEntry entry;
    //entry.filename=fn;
    for(int i=0; i<11; i++){
        entry.filename[i]=fn[i];
        if(fn[i]=='\0'){
            break;
        }
    }
    entry.attributes=attr;
    time_t now;
    time(&now);
    entry.created_time = now;
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
    Transforms a 4kB cluster of data into 128 DirEntries.

    @param data The 4kB cluster as a char array.
    @return The array of 128 DirEntries.
*/
DirEntry* parseDirEntries(char* data){
    DirEntry* entries = new DirEntry[128];
    for(int i=0; i<128; i++){
        entries[i] = *(reinterpret_cast<DirEntry*>(getDirRawData(i,data)));
    }
    return entries;
}

/**
    The reverse of parseDirEntries. It packs 128 DirEntries into a 4kB cluster.

    @param entries Array of 128 DirEntries to pack.
    @return The 4kB cluster as a char array.
*/
char* packDirEntries(DirEntry* entries){
    char* data = new char[CLUSTER_SIZE];
    char* entry;
    for(int i=0; i<128; i++){
        entry=recast(&entries[i]);
        for(int j=0; j<32; j++){
            data[i*32+j]=entry[j];
        }
    }
    return data;
}

int getNextAvailableIndex(){
    FAT.seekg(FAT.beg+FAT_OFFSET+6);
    usint available;
    do{
        FAT.read(recast(&available),2);
    }while(available!=0);
    return available;
}

/**
    Only works with one char delimiter but it works!!
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