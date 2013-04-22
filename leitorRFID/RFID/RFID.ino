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
    ResponseReadDataCMD respReadData;
    readData_EPCC1G2CMD (&Serial1, &respReadData, byte ENum, byte EPC [16], byte Mem, byte WordPtr, byte Num, int accessPassword, byte MaskAdr, byte MaskLen);
    printfResponseReadDataCMD (&respReadData);
    delay (500); 

    // Example of: CMD writeData
    ResponseWriteDataCMD respWriteData;
    writeData_EPCC1G2CMD (&Serial1, &respWriteData, byte WNum, byte ENum, byte EPC [16], byte Mem, byte WordPtr, byte Wdt [240], int accessPassword, byte MaskAdr, byte MaskLen);
    printfResponseSimpleCMD (&respWriteData);
    delay (500);

    // Example of: CMD killTag
    ResponseKillTagCMD respKillTag;
    killTag_EPCC1G2CMD (&Serial1, &respKillTag, byte ENum, byte EPC [16], int accessPassword, byte MaskAdr, byte MaskLen);
    printfResponseSimpleCMD (&respKillTag);
    delay (500);

    // Example of: CMD rdProtectWEPC
    ResponseRdProtectWEPC respRdProtectWEPC;
    rdProtectWEPC_EPCC1G2CMD (&Serial1, &respRdProtectWEPC, byte ENum, byte EPC [16], int accessPassword, byte MaskAdr, byte MaskLen);
    printfResponseSimpleCMD (&respRdProtectWEPC);
    delay (500);

    // Example of: CMD checkRdProtect
    ResponseCheckRdProtectCMD respCheckRdProtect;
    checkRdProtect_EPCC1G2CMD (&respCheckRdProtect);
    printfCheckRdProtectCMD (&respCheckRdProtect);
    delay (500);

    // Example of: CMD resetRdProtect
    ResponseRstRProtectCMD respRstRProtect;
    resetRdProtect_EPCC1G2CMD (&Serial1, &respRstRProtect, int accessPassword);
    printfResponseSimpleCMD (&respRstRProtect);
    delay (500);*/

/*  //Funcoes dos comandos Read-Defined
    // Example of: CMD getReaderInfo
    ResponseReaderInfoCMD respReaderInfo;
    getReaderInfo_ReadDefCMD (&Serial1, &respReaderInfo);
    printfResponseGetReaderInfoCMD2 (&respReaderInfo);
    delay (500);

    // Example of: CMD setAdr
    ResponseSetAdrCMD respSetAdr;
    setAdr_ReadDefCMD (&Serial1, &respSetAdr, byte adr);
    printfResponseSetAdrCMD (&respSetAdr);
    delay (500);

    // Example of: CMD setBRate
    ResponseSetBRateCMD respSetBRate;
    setBRate_ReadDefCMD (&Serial1, &respSetBRate, int baudrate);
    printfResponseSetBRateCMD (&respSetBRate);
    delay (500);

    // Example of: CMD setPower
    ResponseSetPowerCMD respSetPower;
    setPower_ReadDefCMD (&Serial1, &respSetPower, byte power);
    printfResponseSetPowerCMD ( struct retornoSimplesCmd * resposta);
    delay (500);

    // Example of: CMD setScanTime
    ResponseSetScanTimeCMD resoSetScanTime;
    setScanTime_ReadDefCMD (&Serial1, ResponseSetScanTimeCMD * resposta, int scantime_x100ms);
    printfResponseSimpleCMD ( struct retornoSimplesCmd * resposta);
    delay (500);

    // Example of: CMD getWorkMode
    ResponseWorkModeCMD respWorkMode;
    getWorkMode_ReadDefCMD (&Serial1, ResponseWorkModeCMD * resposta);
    printfResponseGetWorkModeCMD (ResponseWorkModeCMD * resposta);
    delay (500);
 
*/}



