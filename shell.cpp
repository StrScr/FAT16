#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define rechcast reinterpret_cast<const char *> 
#define reconstchcast reinterpret_cast<const char *> 

using namespace std;

string CURR_DIR;

vector<string> getTokens(string, char);

int main(int argc, char* argv[]){
    fstream file;
    if(argc > 1){
        file.open(argv[1], ios::binary | ios::in);
    }else{
        cout << "Please include filename in args" << endl;
        return 0;
    }
    if(!file.is_open()){
        cout << "Can't open file!" << endl;
        return 1;
    }
	
    cout << "file opened succesfully" << endl;
    cout << "READING BOOT SECTOR" << endl;
    char* OS_name = new char[8];
    cout << "Ignoring first 3 bytes" << endl;
    file.seekg(file.beg+3);
    file.read(OS_name,8);
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
    file.close();
	if(status != 1){
		cerr << "ERROR " << status << ": al ejecutar el comando" << endl;
	}
    return 0;
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