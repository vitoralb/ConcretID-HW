/*

Manual da RFIDReader - API
Autor: Felipe Salgueiro Oliveira e Silva
Data: 22/04/2013

*/

/*

=> RFIDReaderAPI.h
Este arquivo possui a implementação em C da API.

=> RFID.ino
Este arquivo é um exemplo de código para arduino mostrando como utilizar todas as funções da RFIDReader API.

Segue a baixo a listagem de todas as funções implementadas. Os nomes sãoauto-explicativos. Qualquer dúvida, vide manual "UHF RFID Reader User's Manual v1.8"

*/


 void inventory_EPCC1G2CMD (HardwareSerial * porta, ResponseInventoryCMD * resposta);
  void printfResponseInventoryCMD (ResponseInventoryCMD * resposta);

  void readData_EPCC1G2CMD (HardwareSerial * porta, ResponseReadDataCMD * resposta, byte ENum, byte EPC [16], byte Mem, byte WordPtr, byte Num, int accessPassword, byte MaskAdr, byte MaskLen);
  void printfResponseReadDataCMD (ResponseReadDataCMD * resposta);

  void writeData_EPCC1G2CMD (HardwareSerial * porta, ResponseWriteDataCMD * resposta, byte WNum, byte ENum, byte EPC [16], byte Mem, byte WordPtr, byte Wdt [240], int accessPassword, byte MaskAdr, byte MaskLen);
  void printfResponseSimpleCMD ( struct retornoSimplesCmd * resposta);

  void killTag_EPCC1G2CMD (HardwareSerial * porta, ResponseKillTagCMD * resposta, byte ENum, byte EPC [16], int accessPassword, byte MaskAdr, byte MaskLen);
  void printfResponseSimpleCMD ( struct retornoSimplesCmd * resposta);

  void rdProtectWEPC_EPCC1G2CMD (HardwareSerial * porta, ResponseRdProtectWEPC * resposta, byte ENum, byte EPC [16], int accessPassword, byte MaskAdr, byte MaskLen);
  void printfResponseSimpleCMD ( struct retornoSimplesCmd * resposta);

  void checkRdProtect_EPCC1G2CMD (ResponseCheckRdProtectCMD * resposta);
  void printfCheckRdProtectCMD (ResponseCheckRdProtectCMD * resposta);

  void resetRdProtect_EPCC1G2CMD (HardwareSerial * porta, ResponseRstRProtectCMD * resposta, int accessPassword);
  void printfResponseSimpleCMD ( struct retornoSimplesCmd * resposta);

//Funcoes dos comandos Read-Defined
  void getReaderInfo_ReadDefCMD (HardwareSerial * porta, ResponseReaderInfoCMD * resposta);
  void printfResponseGetReaderInfo (ResponseReaderInfoCMD * resposta);
  
  void setAdr_ReadDefCMD (HardwareSerial * porta, ResponseSetAdrCMD * resposta, byte adr);
  void printfResponseSetAdrCMD ( struct retornoSimplesCmd * resposta);

  void setBRate_ReadDefCMD (HardwareSerial * porta, ResponseSetBRateCMD * resposta, int baudrate);
  void printfResponseSetBRateCMD ( struct retornoSimplesCmd * resposta);

  void setPower_ReadDefCMD (HardwareSerial * porta, ResponseSetPowerCMD * resposta, byte power);
  void printfResponseSetPowerCMD ( struct retornoSimplesCmd * resposta);

  void setScanTime_ReadDefCMD (HardwareSerial * porta, ResponseSetScanTimeCMD * resposta, int scantime_x100ms);
  void printfResponseSimpleCMD ( struct retornoSimplesCmd * resposta);

  void getWorkMode_ReadDefCMD (HardwareSerial * porta, ResponseWorkModeCMD * resposta);
  void printfResponseGetWorkModeCMD (ResponseWorkModeCMD * resposta);