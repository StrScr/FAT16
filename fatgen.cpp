// Generador de Archivos FAT16
#define usint unsigned short int
#define rechcast reinterpret_cast<const char *> 
#include <iostream>
#include <fstream>
using namespace std;

int main(int argc, char* argv[]){
    cout << "Input filename: ";
    string fn;
    cin >> fn;
    fn = fn + ".FAT";
    cout << "Filename: " << fn << endl;
    ofstream FATfile(fn.c_str(), ios::binary | ios::out);
    if(!FATfile.is_open()){
        cout << "Can't open file!" << endl;
        return 1;
    }
    cout << "Writing Reserved Region... (512 Bytes)" << endl;
    char* buffer = new char[448];
    for(int i = 0; i < 448;i++)
        buffer[i] = 0;
    /* BOOT START BLOCK */ 
    FATfile.write(buffer,3); // Bootstrap Jump Code, Unused here
    FATfile.write("MyFS PWN",8); // OEM Name, 8 char, Doesn't really matter
    /* BIOS PARAMETER BLOCK */
    int values = 512;
    FATfile.write(rechcast(&values),2); //Bytes Per Sector
    values = 8;
    FATfile.write(rechcast(&values),1); // Sectors Per Cluster, 8 Sectors * 512 Bytes/Sector = 4kB Clusters
    values = 1;
    FATfile.write(rechcast(&values),2); // # Reserved Sectors, FAT16 uses 1
    values = 2;
    FATfile.write(rechcast(&values),1); // # FAT Copies, Using 2
    values = 512;
    FATfile.write(rechcast(&values),2); // # Root Dir Entries, Using 512
    FATfile.write(buffer,2); // # Sectors in FS (Small), Unused here
    values = 0xF8;
    FATfile.write(rechcast(&values),1); // Media Descriptor, Using Fixed Disk
    values = 256;
    FATfile.write(rechcast(&values),2); // # Sectors per FAT, 256 Sectors/FAT * 512 Bytes/Sector = 128kB per FAT
    FATfile.write(buffer,2); // # Sectors per Track, Unused here
    FATfile.write(buffer,2); // # Heads, Unused here
    FATfile.write(buffer,4); // # Hidden Sectors, Unused here
    values = 524288;
    FATfile.write(rechcast(&values),4); // # Sectors in FS (Big), 524288 Sectors * 512 Bytes/Sector = 256MB FS
    /* EXTENDED BIOS PARAMETER BLOCK */
    FATfile.write(buffer,1); // Drive Number, Unused
    FATfile.write(buffer,1); // Reserved
    FATfile.write(buffer,1); // Extended Boot Signature, Unused
    FATfile.write(buffer,4); // Volume Serial Number, Unused
    FATfile.write("MyFATVolume",11); // Volume Label, Should be same as ROOT
    FATfile.write("'FAT16 '",8); // FS Type
    /* BOOTSTRAP CODE */
    FATfile.write(buffer,448); // Bootstrap Code, Unused
    values = 0xAA55;
    FATfile.write(rechcast(&values),2); // Boot Sector Signature
    cout << "Done writing Reserved Region." << endl;
    delete[] buffer;
    FATfile.close();
    return 0;
}