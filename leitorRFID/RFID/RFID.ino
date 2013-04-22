/* 
    RFIDReaderAPI.h
    Autor: Felipe Salgueiro Oliveira e Silva
    19/03/2013
    
*/

#include "RFIDReaderAPI.h"

void setup()  
{
  
  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }


  Serial.println("InicioSerial0 ");

  Serial1.begin(57600);
  while (!Serial1) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
}

void loop(){

  
    // Example of: CMD inventory
    ResponseInventoryCMD respInventory;
    inventory_EPCC1G2CMD (&Serial1, &respInventory);
    printfResponseInventoryCMD (&respInventory);   
    delay (500); 


    /*
    // Example of: CMD readData
    byte tag[16] = {0xE2,0x00,0x10,0x01,0x83,0x18,0x02,0x70,0x20,0x50,0x43,0x53};
    ResponseReadDataCMD respReadData;
    readData_EPCC1G2CMD (&Serial1, &respReadData, 6, tag, EPC_MEMORY, 0x00, 8, 0x00, 0x00, 0x00);
    printfResponseReadDataCMD (&respReadData);
    delay (500); 

    // Example of: CMD writeData
    ResponseWriteDataCMD respWriteData;
    writeData_EPCC1G2CMD (&Serial1, &respWriteData, byte WNum, byte ENum, byte EPC [16], byte Mem, byte WordPtr, byte Wdt [240], int accessPassword, byte MaskAdr, byte MaskLen);
    printfResponseSimpleCMD (&respWriteData);
    delay (500);

    // Example of: CMD killTag
    byte tag[16] = {0xE2,0x00,0x10,0x01,0x83,0x18,0x02,0x70,0x20,0x50,0x43,0x53};
    ResponseKillTagCMD respKillTag;
    killTag_EPCC1G2CMD (&Serial1, &respKillTag, 6, tag, 0, 0x00, 0x00);
    printfResponseSimpleCMD (&respKillTag);
    delay (500);

    */



  // Example of: CMD rdProtectWEPC
    byte tag[16] = {0xE2,0x00,0x10,0x01,0x83,0x18,0x02,0x70,0x20,0x50,0x43,0x53};
    ResponseRdProtectWEPC respRdProtectWEPC;
    rdProtectWEPC_EPCC1G2CMD (&Serial1, &respRdProtectWEPC, 6, tag, 836644864, 0x00, 0x00);
    printfResponseSimpleCMD (&respRdProtectWEPC);
    delay (500);

  /* 

    // Example of: CMD resetRdProtect
    ResponseRstRProtectCMD respRstRProtect;
    resetRdProtect_EPCC1G2CMD (&Serial1, &respRstRProtect, 0);
    printfResponseSimpleCMD (&respRstRProtect);
    delay (500);

    
    // Example of: CMD checkRdProtect
    ResponseCheckRdProtectCMD respCheckRdProtect;
    checkRdProtect_EPCC1G2CMD (&Serial1, &respCheckRdProtect);
    printfCheckRdProtectCMD (&respCheckRdProtect);
    delay (500);*/

    
/*

  //Funcoes dos comandos Read-Defined
    // Example of: CMD getReaderInfo
    ResponseReaderInfoCMD respReaderInfo;
    getReaderInfo_ReadDefCMD (&Serial1, &respReaderInfo);
    printfResponseGetReaderInfoCMD (&respReaderInfo);
    delay (500);


    // Example of: CMD setAdr
    ResponseSetAdrCMD respSetAdr;
    setAdr_ReadDefCMD (&Serial1, &respSetAdr, 0x00);  // Setando endereco para 0x00. Range: 0x00 - 0xFE
    printfResponseSetAdrCMD (&respSetAdr);
    delay (500);


    // Example of: CMD setBRate
    ResponseSetBRateCMD respSetBRate;
    setBRate_ReadDefCMD (&Serial1, &respSetBRate, 57600); // Valores possiveis: 9600, 19200,38400,43000,56000,57600,115200 bps;
    printfResponseSetBRateCMD (&respSetBRate);
    delay (500);
   
    // Example of: CMD setPower
    ResponseSetPowerCMD respSetPower;
    setPower_ReadDefCMD (&Serial1, &respSetPower, 20);  //Max é 30, min é 0
    printfResponseSetPowerCMD (&respSetPower);
    delay (500);

    // Example of: CMD setScanTime
    ResponseSetScanTimeCMD respSetScanTime;
    setScanTime_ReadDefCMD (&Serial1, &respSetScanTime, 10);  // Setando para 10 * 100ms
    printfResponseSimpleCMD (&respSetScanTime);
    delay (500);
    // Example of: CMD getWorkMode
    ResponseWorkModeCMD respWorkMode;
    getWorkMode_ReadDefCMD (&Serial1, &respWorkMode);
    printfResponseGetWorkModeCMD (&respWorkMode);
    delay (500);
 
*/  }



