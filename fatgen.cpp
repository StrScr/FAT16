// Generador de Archivos FAT16
#define usint unsigned short int
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
    FATfile.write(0,3); // Bootstrap Jump Code, Unused here
    FATfile.write("MyFS PWN",8); // OEM Name, 8 char, Doesn't really matter
    /* BIOS PARAMETER BLOCK */
    FATfile.write((usint)512,2); //Bytes Per Sector
    FATfile.write(8,1); // Sectors Per Cluster, 8 Sectors * 512 Bytes/Sector = 4kB Clusters
    FATfile.write((usint)1,2); // # Reserved Sectors, FAT16 uses 1
    FATfile.write((char)2,1); // # FAT Copies, Using 2
    FATfile.write((usint)512,2); // # Root Dir Entries, Using 512
    FATfile.write(0,2); // # Sectors in FS (Small), Unused here
    FATfile.write((char)0xF8,1); // Media Descriptor, Using Fixed Disk
    FATfile.write((usint)256,2); // # Sectors per FAT, 256 Sectors/FAT * 512 Bytes/Sector = 128kB per FAT
    FATfile.write(0,2); // # Sectors per Track, Unused here
    FATfile.write(0,2); // # Heads, Unused here
    FATfile.write(0,4); // # Hidden Sectors, Unused here
    FATfile.write(524288,4); // # Sectors in FS (Big), 524288 Sectors * 512 Bytes/Sector = 256MB FS
    /* EXTENDED BIOS PARAMETER BLOCK */
    FATfile.write(0,1); // Drive Number, Unused
    FATfile.write(0,1); // Reserved
    FATfile.write(0,1); // Extended Boot Signature, Unused
    FATfile.write(0,4); // Volume Serial Number, Unused
    FATfile.write("MyFATVolume",11); // Volume Label, Should be same as ROOT
    FATfile.write("'FAT16 '",8); // FS Type
    /* BOOTSTRAP CODE */
    FATfile.write(0,448); // Bootstrap Code, Unused
    FATfile.write((usint)0xAA55,2); // Boot Sector Signature
    cout << "Done writing Reserved Region." << endl;
    FATfile.close();
    return 0;
}