#include <iostream>
#include <fstream>
using namespace std;

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
    delete[] OS_name;
    file.close();
    return 0;
}