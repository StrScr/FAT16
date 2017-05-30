#define usint unsigned short int
#define uncast reinterpret_cast<unsigned short int>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define rechcast reinterpret_cast<const char *> 
#define reconstchcast reinterpret_cast<const char *> 

using namespace std;

usint getFATindex(int);
vector<string> getTokens(string, char);

const int FAToffset = 512;
const usint FAT_EOF = 0xFFFF;

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
    cout << "Ignoring first 3 bytes" << endl;
    FAT.seekg(FAT.beg+3);
    FAT.read(OS_name,8);
    cout << "OS NAME: " << OS_name << endl;
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
    delete[] OS_name;
    /* Check FS Initalized status */
    if (getFATindex(2)==0){
        cout << "FAT still uninitalized. Writing root..." << endl;
    }else{
        cout << "Root value: " << getFATindex(2) << endl;
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
    FAT.close();
    return 0;
}

usint getFATindex(int index){
    FAT.seekg(FAT.beg+FAToffset+(index*2));
    char* value = new char[2];
    FAT.read(value,2);
    return uncast(value);
}

//Only works with one char delimiter but it works!!
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