/*

Manual da RFIDReader - API
Autor: Felipe Salgueiro Oliveira e Silva
Data: 22/04/2013

*/

TO-DO:
1. Para aumentar a robustez, fazer a checagem de timeout da porta serial. Nas situações em que o leitor parar de responder, o arduino não ficará travado em loop na espera da resposta. Note que já tem um campo em cada struct de retorno para sinalizar casos de timeout. Basta inserir um contador e sair do laço caso o limite máximo de tempo de espera seja ultrapassado.

2. Na atual implementação, considera-se que o endereço ("ADR_READER") de cada leitor é fixo em tempo de compilação. Acredito que isso é suficiente para nossos propósitos. Caso futuramente seja necessário tornar o endereço dos leitores dinâmico, em tempo de execução, basta modificar os "Código dos comandos", no arquivo "RFIDReaderAPI.h". No momento, esses "Códigos dos comandos" são estáticos (constantes na memória flash). Será necessário torná-los dinâmicos (memória RAM).


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