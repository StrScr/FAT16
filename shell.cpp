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
usint getNextAvailableDATAIndex();
usint getNextAvailableIndex();
usint getFATindex(int);
void setFATindex(int, usint);
char* getDataCluster(int);
void setDataCluster(int,char*);
bool hasNextCluster(int);
DirEntry makeDirEntry(string, char, usint, unsigned int);
char* getDirRawData(int, char*);
DirEntry* parseDirEntries(char*);
char* packDirEntries(DirEntry*);
string filenameToString(char*);
void deleteEntry(DirEntry);
void clearFATindex(usint);
vector<string> getTokens(string, char);
//use for cat
void createFile(string);

const int FAT_SIZE = 65536; // 65536 entries. 2B per entry.
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
          cout << myDir[i].filesize << endl;
        }
      }
    }else if(tokens[0]=="cat"){
      if(tokens.size() == 2){

      }
      if(tokens.size() == 3 && tokens[1] == ">"){
        createFile(tokens[2]);
      }
    }else if(tokens[0]=="mkdir"){
      if(tokens.size()>1){
        if(tokens[1].length()>11 || tokens[1].length()<1){
          cout << "mkdir: Wrong length for specified name! Must be 1 to 11 characters long." << endl;
        }else{
          bool cont=true;
          DirEntry* myDir = parseDirEntries(getDataCluster(currentIndex));
          for(int i=0; i<128; i++){//Check if name already exists in DirEntries
            if(filenameToString(myDir[i].filename)==tokens[1]){
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
              setDataCluster(currentIndex,packDirEntries(myDir));
              //Initialize directory (Create . and .. DirEntries)
              myDir = parseDirEntries(getDataCluster(ind));
              myDir[0]=makeDirEntry(".",ATTR_DIRECTORY | ATTR_SYSTEMFILE,ind,4096);
              myDir[1]=makeDirEntry("..",ATTR_DIRECTORY | ATTR_SYSTEMFILE,currentIndex,4096);
              setDataCluster(ind,packDirEntries(myDir));
              cout << "Directory created." << endl;
            }else{
              cout << "mkdir: No more space available for entries in current directory!" << endl;
            }
          }
        }
      }else{
        cout << "mkdir: Need to specify directory name!" << endl;
      }
    }else if(tokens[0]=="cd"){
      if(tokens.size()>1){
        if(tokens[1].length()>11 || tokens[1].length()<1){
          cout << "cd: Wrong length for specified name! Must be 1 to 11 characters long." << endl;
        }else{
          //Find element in directory with same name
          DirEntry* myDir = parseDirEntries(getDataCluster(currentIndex));
          bool found=false;
          for(int i=0; i<128; i++){//Check if name already exists in DirEntries
            if(filenameToString(myDir[i].filename)==tokens[1]){
              found=true;
              //Check if DirEntry is Directory
              if(myDir[i].attributes & ATTR_DIRECTORY){
                currentIndex = myDir[i].address;
                cout << "Changed current directory to '" << tokens[1] << "'." << endl;
              }else{
                cout << "'" << tokens[1] << "' is not a directory!" << endl;
              }
              break;
            }
          }
          if(!found){
            cout << "cd: Directory '" << tokens[1] << "' not found!" << endl;
          }
        }
      }else{
        cout << "cd: Need to specify directory name!" << endl;
      }
    }else if(tokens[0]=="chmod"){
      if(tokens.size()>1){
        //First of all, check if the element exists.
        DirEntry* myDir = parseDirEntries(getDataCluster(currentIndex));
        int found=-1;
        for(int i=0; i<128; i++){
          if(filenameToString(myDir[i].filename)==tokens[1]){
            found=i;
            break;
          }
        }
        if(found>=0){
          if(tokens.size()==2){
            //Just print the file's attributes.
            cout << "'" << tokens[1] << "' is a " << ((myDir[found].attributes & ATTR_DIRECTORY)?"directory.":"file.") << endl;
            cout << "It's " << ((myDir[found].attributes & ATTR_READONLY)?"":"NOT ") << "Read-Only." << endl;
            cout << "It's " << ((myDir[found].attributes & ATTR_HIDDEN)?"":"NOT ") << "Hidden." << endl;
            cout << "It's " << ((myDir[found].attributes & ATTR_SYSTEMFILE)?"":"NOT ") << "a System File." << endl;
            cout << "It's " << ((myDir[found].attributes & ATTR_VOLUMELABEL)?"":"NOT ") << "a Volume Label." << endl;
          }else if(tokens.size()==3){
            if(tokens[2].length()==4){
              string attr = tokens[2];
              char nattr=0;
              attr+=(myDir[found].attributes & ATTR_FILE);
              attr+=(myDir[found].attributes & ATTR_DIRECTORY);
              if(attr[0]=='V'){
                nattr+=ATTR_VOLUMELABEL;
              }else if(attr[0]=='-'){
                //Do nothing
              }else if(attr[0]=='.'){
                nattr+=(myDir[found].attributes & ATTR_VOLUMELABEL);
              }else{
                cout << "Unknown value for Volume Label. Keeping old value." << endl;
                nattr+=(myDir[found].attributes & ATTR_VOLUMELABEL);
              }
              if(attr[1]=='S'){
                nattr+=ATTR_SYSTEMFILE;
              }else if(attr[1]=='-'){
                //Do nothing
              }else if(attr[1]=='.'){
                nattr+=(myDir[found].attributes & ATTR_SYSTEMFILE);
              }else{
                cout << "Unknown value for System File. Keeping old value." << endl;
                nattr+=(myDir[found].attributes & ATTR_SYSTEMFILE);
              }
              if(attr[2]=='H'){
                nattr+=ATTR_HIDDEN;
              }else if(attr[2]=='-'){
                //Do nothing
              }else if(attr[2]=='.'){
                nattr+=(myDir[found].attributes & ATTR_HIDDEN);
              }else{
                cout << "Unknown value for Hidden. Keeping old value." << endl;
                nattr+=(myDir[found].attributes & ATTR_HIDDEN);
              }
              if(attr[3]=='R'){
                nattr+=ATTR_READONLY;
              }else if(attr[3]=='-'){
                //Do nothing
              }else if(attr[3]=='.'){
                nattr+=(myDir[found].attributes & ATTR_READONLY);
              }else{
                cout << "Unknown value for Read-Only. Keeping old value." << endl;
                nattr+=(myDir[found].attributes & ATTR_READONLY);
              }
              myDir[found].attributes = nattr;
              setDataCluster(currentIndex,packDirEntries(myDir));
            }else{
              cout << "Atrribute string is the wrong lenght! Must be 4 characters long." << endl;
            }
          }else{
            cout << "Too many arguments specified!" << endl;
          }
        }else{
          cout << "chmod: Element '" << tokens[1] << "' not found in directory!" << endl;
        }
      }else{
        cout << "chmod: Need to specify file/direcotry and, optionally, attributes!" << endl << endl;
        cout << "Usage: chmod filename [attributes]" << endl;
        cout << "If attributes are not specified, the file's attributes are displayed." << endl;
        cout << "If they are specified, the file's attributes are changed accordingly." << endl;
        cout << "Attributes should be specified as a 4-length string 'VSHR', where a letter would set the attribute," << endl;
        cout << "a hyphen '-' would unset it, or a period '.' would not change it. (E.g.: -.HR)" << endl;
        cout << "[V]olume Label, [S]ystem File, [H]idden, [R]ead-Only." << endl << endl;
      }
    }else if (tokens[0]=="rmdir") {
        if(tokens.size()>1){
            if(tokens[1].length()>11 || tokens[1].length()<1){
                cout << "rmdir: Wrong length for specified name! Must be 1 to 11 characters long." << endl;
            }else{
                if(tokens[1]=="." || tokens[1]==".."){
                    cout << "rmdir: Can't delete special directory!" << endl;
                }else{
                    int found=-1;
                    bool isdir=true;
                    DirEntry* myDir = parseDirEntries(getDataCluster(currentIndex));
                    for(int i=0; i<128; i++){//Check if name exists in DirEntries
                        if(filenameToString(myDir[i].filename)==tokens[1]){
                            if(myDir[i].attributes & ATTR_DIRECTORY){
                                found=i;
                            }else{
                                cout << "rmdir: '" << tokens[1] << "' isn't a directory!" << endl;
                                isdir=false;
                            }
                            break;
                        }
                    }
                    if(found>=0){
                        deleteEntry(myDir[found]);
                        myDir[found].filename[0]='\0';
                        setDataCluster(currentIndex, packDirEntries(myDir));
                        cout << "Directory removed." << endl;
                    }else if(isdir){
                        cout << "rmdir: Element doesn't exist in current directory." << endl;
                    }
                }
            }
      }else{
            cout << "rmdir: Need to specify directory name!" << endl;
      }
    }else if(tokens[0]=="rm"){
        if(tokens.size()>1){
            if(tokens[1].length()>11 || tokens[1].length()<1){
                cout << "rm: Wrong length for specified name! Must be 1 to 11 characters long." << endl;
            }else{
                int found=-1;
                bool isfile=true;
                DirEntry* myDir = parseDirEntries(getDataCluster(currentIndex));
                for(int i=0; i<128; i++){//Check if name exists in DirEntries
                    if(filenameToString(myDir[i].filename)==tokens[1]){
                        if(myDir[i].attributes & ATTR_FILE){
                            found=i;
                        }else{
                            cout << "rm: '" << tokens[1] << "' isn't a file!" << endl;
                            isfile=false;
                        }
                        break;
                    }
                }
                if(found>=0){
                    deleteEntry(myDir[found]);
                    myDir[found].filename[0]='\0';
                    setDataCluster(currentIndex, packDirEntries(myDir));
                    cout << "File removed." << endl;
                }else if(isfile){
                    cout << "rm: Element doesn't exist in current directory." << endl;
                }
            }
        }else{
            cout << "rm: Need to specify filename!" << endl;
        }
    }else if (tokens[0]=="pwd") {
      cout << currentDir <<endl;
    }
    //status = executeCommand(tokens);
  };
  if(status != 1){
    cerr << "ERROR " << status << ": al ejecutar el comando" << endl;
  }
  FAT.close();
  return 0;
}

usint getNextAvailableIndex(){
  for(int i = 2; i < FAT_SIZE; i++){
    if(getFATindex(i) == 0){
      return i;
    }
  }
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
  FAT.flush();
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

string filenameToString(char* filename){
  char longer[12];
  longer[11]='\0';
  for(int i=0; i<11; i++){
    longer[i]=filename[i];
  }
  return string(longer);
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

/**
cat > filemae
*/
void createFile(string filename){
  /*Do cat>file logic here*/
  /*string fileData = cin.get()*/

  DirEntry* myDir = parseDirEntries(getDataCluster(currentIndex));
  for(int i=0; i<128; i++){
    if(myDir[i].filename[0] == '\0'){
      int index = getNextAvailableIndex();
      myDir[i] = makeDirEntry(filename, ATTR_FILE, index,0);
      char fileData;
      char* cluster = getDataCluster(index);
      usint clusterIndex = 0;
      bool addNewCluster = false;
      while(cin.get(fileData)){//end of file
        /*FAT LOGIC HERE*/
        if(addNewCluster){
          myDir[i].filesize+=CLUSTER_SIZE;
          int next_index = getNextAvailableIndex();

          //Saving cluster and fat
          setDataCluster(index,cluster);
          setFATindex(index,next_index);

          //Now the new cluster and index
          cluster = getDataCluster(next_index);
          index = next_index;
        }
        if(fileData == 4){
          cluster[clusterIndex] = fileData;
          break;
        }
        cluster[clusterIndex++] = fileData;
        addNewCluster = (clusterIndex == 0);
      }
      //save cluster to data
      setDataCluster(index,cluster);
      //save fat
      setFATindex(index,FAT_EOF);
      myDir[i].filesize+=clusterIndex;
      break;
    }
  }
  setDataCluster(currentIndex,packDirEntries(myDir));
  //remember to setFATindex(currentIndex,nextcluster|FAT_EOF)
  /*Maybe flush to disk?*/
}

void deleteEntry(DirEntry entry){
  if(entry.attributes & ATTR_FILE){
    //Just a file.
    clearFATindex(entry.address);
  }else{
    if(filenameToString(entry.filename)=="." || filenameToString(entry.filename)==".."){
      //Special Directory. Don't do anything.
    }else{
      //Directory. Must recurse.
      DirEntry* myDir = parseDirEntries(getDataCluster(entry.address));
      for(int i=0; i<128; i++){
        if(myDir[i].filename[0]!='\0'){
          deleteEntry(myDir[i]);
          myDir[i].filename[0]='\0';
        }
      }
      setDataCluster(entry.address,packDirEntries(myDir));
      clearFATindex(entry.address);
    }
  }
}

void clearFATindex(usint index){
  if(hasNextCluster(index)){
    clearFATindex(getFATindex(index));
  }
  setFATindex(index, 0x0000);
}
