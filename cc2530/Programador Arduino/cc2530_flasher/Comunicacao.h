/*
 * Comunicacao.h
 *
 *  Created on: Mar 18, 2013
 *      Author: vqa
 */

#ifndef COMUNICACAO_H_
#define COMUNICACAO_H_

#include "Arduino.h"

#define PRESET_VALUE 0xFFFF
#define POLYNOMIAL  0x8408

//! Low nibble of 16bit variable
#define LOBYTE(w)           ((unsigned char)(w))
//! High nibble of 16bit variable
#define HIBYTE(w)           ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))


#define CMD_SERIAL_GET_INFO 0x01
#define CMD_SERIAL_SET_WRITE_ADDR 0x02
#define CMD_SERIAL_SET_READ_ADDR 0x03
#define CMD_SERIAL_START_WRITE 0x04
#define CMD_SERIAL_START_READ 0x05


void exit(int i )
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

unsigned int uiCrc16Cal(unsigned char const  * pucY, unsigned short ucX)
{
	unsigned short ucI,ucJ;
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

void enviarChecksum(unsigned char const  *buffer, unsigned short len,  HardwareSerial *porta){
  unsigned int crc = uiCrc16Cal(buffer, len);

  porta->write(LOBYTE(crc));
  porta->write(HIBYTE(crc));
}

int checarChecksum(unsigned char const  *buffer, unsigned short len, unsigned char CRC_LSB, unsigned char CRC_MSB){
  unsigned int crc = uiCrc16Cal(buffer, len);

  int retorno = 0;

  if(CRC_MSB == HIBYTE(crc) && CRC_LSB == LOBYTE(crc))
  {
	  retorno = 1;
  }

  return retorno;
}

void enviarComando(unsigned char comando, HardwareSerial *porta)
{
	porta->write(0x01);
	porta->write(comando);
}

unsigned char receberComando(HardwareSerial *porta)
{
	char tipo = 0, comando = 0;
	porta->readBytes(&tipo, 1);

	if(tipo != 0x01)
	{
		exit(1);
	}
	porta->readBytes(&comando, 1);

	return comando;
}

void enviarDados(unsigned char *buffer, unsigned short len, HardwareSerial *porta){
	porta->write((byte) 0x00);
	porta->write(LOBYTE(len));
	porta->write(HIBYTE(len));
	if(porta->write(buffer, len) < len)
	{
		exit(1);
	}
	enviarChecksum(buffer, len, porta);

}

int receberDados(char *buffer, HardwareSerial *porta)
{
	char tipo;
	char len_MSB, len_LSB;
	int len;
	char CRC_LSB, CRC_MSB;
	porta->readBytes(&tipo, 1);
	if(tipo != 0x00)
	{
		exit(1);
	}
	porta->readBytes(&len_LSB, 1);
	porta->readBytes(&len_MSB, 1);
	len = (len_MSB | len_LSB);
	porta->readBytes(buffer, len);
	porta->readBytes(&CRC_LSB, 1);
	porta->readBytes(&CRC_MSB, 1);


	if(!checarChecksum((unsigned char*)buffer, len, CRC_LSB, CRC_MSB))
	{
		exit(1);
	}

	return len;
}




#endif /* COMUNICACAO_H_ */
