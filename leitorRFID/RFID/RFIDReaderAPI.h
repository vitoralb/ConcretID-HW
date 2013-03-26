/* 
    RFIDReaderAPI.h
    Autor: Felipe Salgueiro
    19/03/2013
    
*/

/******         Lista das funcoes da RFIDReaderAPI     ******/ 
/*
//Funcoes dos comandos EOC C1G2
  void inventory_EPCC1G2CMD (HardwareSerial * porta, ResponseInventoryCMD * resposta);
  void printfResponseInventoryCMD (ResponseInventoryCMD * resposta);

//Funcoes dos comandos Read-Defined
  void getReaderInfo_ReadDefCMD (HardwareSerial * porta, ResponseReaderInfoCMD * resposta);
  void printfResponseGetReaderInfo (ResponseReaderInfoCMD * resposta);

//Funcoes de checksum
  unsigned int uiCrc16Cal(unsigned char const  * pucY, unsigned char ucX);
  void enviarChecksum(unsigned char const  * buffer, unsigned char len, HardwareSerial *porta);
  int checarChecksum(unsigned char const  *buffer, unsigned short len, unsigned char CRC_LSB, unsigned char CRC_MSB);

// Funcoes de uso geral (utils)
  void print_serial();
  void exit(int i);
*/
/************************************************************/


#ifndef RFIDREADERAPI_H_
#define RFIDREADERAPI_H_

#include "Arduino.h"
#include <String.h>
#define PRESET_VALUE 0xFFFF
#define POLYNOMIAL  0x8408

const byte getReaderInfo[] = {0x04, 0x00, 0x21};
const byte inventory[] = {0x04, 0x00, 0x01};

byte bufferCmd [262];

typedef struct EPC {
  byte len;
  byte data [16];

} EPC;


typedef struct retornoInventoryCmd {
  byte flagCRC;
  byte flagTimeout;
  byte len;
  byte adr;
  byte reCmd;
  byte status_;
  byte numTags;
  EPC tags [16];
  byte LSB_CRC16;
  byte MSB_CRC16;  
} ResponseInventoryCMD;

typedef struct retornoReaderInfoCmd {
  byte flagCRC;
  byte flagTimeout;
  byte len;
  byte adr;
  byte reCmd;
  byte status_;
  byte version_;
  byte subversion;
  byte type;
  byte Tr_Type;
  byte DMaxFre;
  byte DMinFre;
  byte Power;
  byte Scntm;
  byte LSB_CRC16;
  byte MSB_CRC16;  
} ResponseReaderInfoCMD;


unsigned int uiCrc16Cal(unsigned char const  * pucY, unsigned char ucX)
{
	unsigned char ucI,ucJ;
	unsigned short int  uiCrcValue = PRESET_VALUE;

   	for(ucI = 0; ucI < ucX; ucI++)
	   {
		   uiCrcValue = uiCrcValue ^ *(pucY + ucI);
	  	   for(ucJ = 0; ucJ < 8; ucJ++)
	   	  {
		 	if(uiCrcValue & 0x0001)
		   	{
		    	uiCrcValue = (uiCrcValue >> 1) ^ POLYNOMIAL;
		   	}
		 	else
		   	{
		    	uiCrcValue = (uiCrcValue >> 1);
		   	}
		}
 	}
return uiCrcValue;
}

void enviarChecksum(unsigned char const  * buffer, unsigned char len, HardwareSerial *porta){
  unsigned int crc = uiCrc16Cal(buffer, len);
  unsigned char CRC_MSB = 0;
  unsigned char CRC_LSB = 0;

  CRC_MSB = (crc & 0x0000FF00) >> 8;
  CRC_LSB = crc & 0x000000FF;

  (*porta).write(CRC_LSB);
  (*porta).write(CRC_MSB);
}

int checarChecksum(unsigned char const  *buffer, unsigned short len, unsigned char CRC_LSB, unsigned char CRC_MSB){
  unsigned int crc = uiCrc16Cal(buffer, len);
  unsigned char NEW_CRC_MSB = 0;
  unsigned char NEW_CRC_LSB = 0;
  int retorno = 0;

  NEW_CRC_MSB = (crc & 0x0000FF00) >> 8;
  NEW_CRC_LSB = crc & 0x000000FF;
  
  if(CRC_MSB == NEW_CRC_MSB && CRC_LSB == NEW_CRC_LSB)
  {
	  retorno = 1;
  }

  return retorno;
}

void print_serial()
{  
  int i = 0;
  while(!Serial1.available());
  while(Serial1.available()){
    if (Serial1.available()){
      Serial.print(i++);
      Serial.print(" - Serial2: ");
      Serial.print(Serial1.read(), HEX);
      Serial.println("");
    }
  }
}

void exit(int i)
{
	if(i != 0)
	{
            while(1)
            {	digitalWrite(13, HIGH);
		delay(1000);
        	digitalWrite(13, LOW);
                delay(1000);
            }
	}
}


/*************************************************************************************/
/****  Comandos   EPCC1G2 ****/

void inventory_EPCC1G2CMD (HardwareSerial * porta, ResponseInventoryCMD * resposta)
{  
    int i = 0;
    int contEPC = 0;
    int indiceAtual = 0;

    Serial.println("Pedindo inventorio do leitor...");
    (*porta).write(inventory, 3);
    enviarChecksum(inventory, 3, porta);
    
    while(!(*porta).available());
    while((*porta).available()){
      if ((*porta).available()){      
        bufferCmd [i] = (*porta).read();
        
        Serial.print(" - Serial 01: ");
        Serial.print(bufferCmd [i], HEX);
        Serial.println("");
        i++;
      }
    }
    
    Serial.print ("Tamanho do buffer: ");
    Serial.print(i);
    Serial.println ("");
    
    resposta->flagTimeout = 0;               // TO-DO: Ainda falta fazer logica do timeout
    if (resposta->flagTimeout == 1)
    {
       return;
    }
    
    resposta->flagCRC = !checarChecksum ((unsigned char *) bufferCmd, (i - 2), (unsigned char) bufferCmd [i-2], (unsigned char)  bufferCmd [i-1]);   // 0 se tudo OK, 1 se tiver erro
    if (resposta->flagCRC == 1)
    {
       return;
    }
    
    resposta->len = bufferCmd [0];
    resposta->adr = bufferCmd [1];
    resposta->reCmd = bufferCmd [2];
    resposta->status_ = bufferCmd [3];
    
    if (resposta->len > 5)
    {
      resposta->numTags = bufferCmd [4];
      
      indiceAtual = 5;
      
      if (resposta->numTags > 16)
      {
        Serial.print("Leitor detectou muitas tags no mesmo momento (mais de 16). Informacoes sobre algumas tags serao descartadas.");
        resposta->numTags = 16;
      }
      
      for (contEPC = 0; contEPC < resposta->numTags; contEPC++)
      {
        resposta->tags [contEPC].len = bufferCmd [indiceAtual];
        
        if (resposta->tags [contEPC].len > 16)
         {
           Serial.print("Codigo EPC identificado eh maior do que o permitido. Erro.");
           exit(1);
         }
         
        memcpy (&(resposta->tags [contEPC].data), bufferCmd + indiceAtual + 1, (resposta->tags [contEPC].len));
        indiceAtual = indiceAtual + (resposta->tags [contEPC].len) + 1;
      }
    }
    else {
      resposta->numTags = 0;
      
    }
    
    
    resposta->LSB_CRC16 = bufferCmd [(i-2)];
    resposta->MSB_CRC16 = bufferCmd [(i-1)];;  
  
}

void printfResponseInventoryCMD (ResponseInventoryCMD * resposta){
  int contTag = 0;
  int contData = 0;
  
  Serial.print("**** Resposta do CMD ****");
  Serial.println ("");
  
  Serial.print("flagTimeout: ");
  Serial.print(resposta->flagTimeout, HEX);
  Serial.println ("");
  if (resposta->flagTimeout == 1)
  {
     Serial.print("Nao houve resposta. Timeout. ");
     return; 
  }
  
  Serial.print("flagCRC: ");
  Serial.print(resposta->flagCRC, HEX);
  Serial.println ("");
  if (resposta->flagCRC == 1)
  {  
     Serial.print("Resposta do comando corrompida. CRC nao eh valido. ");
     return; 
  }
  
  Serial.print("len: ");
  Serial.print(resposta->len, HEX);
  Serial.println("");
  Serial.print("adr: ");
  Serial.print(resposta->adr, HEX);
  Serial.println("");
  Serial.print("reCmd: ");
  Serial.print(resposta->reCmd, HEX);
  Serial.println("");
  Serial.print("status: ");
  Serial.print(resposta->status_, HEX);
  Serial.println("");
  Serial.print("Numtags: ");
  Serial.print(resposta->numTags, HEX);
  Serial.println("");
  
  for (contTag = 0; contTag < (resposta->numTags); contTag++)
  {
    Serial.print("******   TAG #");
    Serial.print(contTag + 1);
    Serial.print("    ******");
    Serial.println("");
    Serial.print(" - LENGTH: ");
    Serial.print(resposta->tags [contTag].len, HEX);
    Serial.println("");
    
    for (contData = 0; contData < resposta->tags[contTag].len; contData++)
    {
      Serial.print(" - DATA ");
      Serial.print(contData);
      Serial.print(": ");
      Serial.print(resposta->tags[contTag].data[contData], HEX);
      Serial.println("");
    }
    
  }
  
  Serial.print("LSB_CRC16: ");
  Serial.print(resposta->LSB_CRC16, HEX);
  Serial.println("");
  Serial.print("MSB_CRC16: ");
  Serial.print(resposta->MSB_CRC16, HEX);
  Serial.println("");
  
  Serial.println("**** FIM ****");
}


/*************************************************************************************/
/****  Comandos   EPCC1G2 ****/




/*************************************************************************************/
/****  Comandos   READ-DEFINED  ****/

void getReaderInfo_ReadDefCMD (HardwareSerial * porta, ResponseReaderInfoCMD * resposta)
{  
    int i = 0;
    int contEPC = 0;
    int indiceAtual = 0;

    Serial.println("Pedindo informacoes do leitor...");
    (*porta).write(getReaderInfo, 3);
    enviarChecksum(getReaderInfo, 3, porta);
    
    while(!(*porta).available());
    while((*porta).available()){
      if ((*porta).available()){      
        bufferCmd [i] = (*porta).read();
        
        Serial.print(" - Serial 01: ");
        Serial.print(bufferCmd [i], HEX);
        Serial.println("");
        i++;
      }
    }
    
    Serial.print ("Tamanho do buffer: ");
    Serial.print(i);
    Serial.println ("");
    
    resposta->flagTimeout = 0;               // TO-DO: Ainda falta fazer logica do timeout
    if (resposta->flagTimeout == 1)
    {
       return;
    }
    
    resposta->flagCRC = !checarChecksum ((unsigned char *) bufferCmd, (i - 2), (unsigned char) bufferCmd [i-2], (unsigned char)  bufferCmd [i-1]);   // 0 se tudo OK, 1 se tiver erro
    if (resposta->flagCRC == 1)
    {
       return;
    }
    
    resposta->len = bufferCmd [0];
    resposta->adr = bufferCmd [1];
    resposta->reCmd = bufferCmd [2];
    resposta->status_ = bufferCmd [3];
    resposta->version_ = bufferCmd [4];
    resposta->subversion = bufferCmd [5];
    resposta->type = bufferCmd [6];
    resposta->Tr_Type = bufferCmd [7];
    resposta->DMaxFre = bufferCmd [8];
    resposta->DMinFre = bufferCmd [9];
    resposta->Power = bufferCmd [10];
    resposta->Scntm = bufferCmd [11];   
    resposta->LSB_CRC16 = bufferCmd [(i-2)];
    resposta->MSB_CRC16 = bufferCmd [(i-1)];;  
  
}

void printfResponseGetReaderInfo (ResponseReaderInfoCMD * resposta){
  
  Serial.print("**** Resposta do CMD ****");
  Serial.println ("");
  
  Serial.print("flagTimeout: ");
  Serial.print(resposta->flagTimeout, HEX);
  Serial.println ("");
  if (resposta->flagTimeout == 1)
  {
     Serial.print("Nao houve resposta. Timeout. ");
     return; 
  }
  
  Serial.print("flagCRC: ");
  Serial.print(resposta->flagCRC, HEX);
  Serial.println ("");
  if (resposta->flagCRC == 1)
  {  
     Serial.print("Resposta do comando corrompida. CRC nao eh valido. ");
     return; 
  }
  
  Serial.print("len: ");
  Serial.print(resposta->len, HEX);
  Serial.println("");
  Serial.print("adr: ");
  Serial.print(resposta->adr, HEX);
  Serial.println("");
  Serial.print("reCmd: ");
  Serial.print(resposta->reCmd, HEX);
  Serial.println("");
  Serial.print("status: ");
  Serial.print(resposta->status_, HEX);
  Serial.println("");
  Serial.print("Version: ");
  Serial.print(resposta->version_, HEX);
  Serial.println("");
  Serial.print("Subversion: ");
  Serial.print(resposta->subversion, HEX);
  Serial.println("");
  Serial.print("Type: ");
  Serial.print(resposta->type, HEX);
  Serial.println("");
  Serial.print("Tr_Type: ");
  Serial.print(resposta->Tr_Type, HEX);
  Serial.println("");
  Serial.print("DMaxFre: ");
  Serial.print(resposta->DMaxFre, HEX);
  Serial.println("");
  Serial.print("DMinFre: ");
  Serial.print(resposta->DMinFre, HEX);
  Serial.println("");
  Serial.print("Power: ");
  Serial.print(resposta->Power, HEX);
  Serial.println("");
  Serial.print("Scntm: ");
  Serial.print(resposta->Scntm, HEX);
  Serial.println("");
  Serial.print("LSB_CRC16: ");
  Serial.print(resposta->LSB_CRC16, HEX);
  Serial.println("");
  Serial.print("MSB_CRC16: ");
  Serial.print(resposta->MSB_CRC16, HEX);
  Serial.println("");
  
  Serial.println("**** FIM ****");
  
}

#endif 
