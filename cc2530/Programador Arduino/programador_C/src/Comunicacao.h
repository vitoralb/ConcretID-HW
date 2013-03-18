/*
 * Comunicacao.h
 *
 *  Created on: Mar 18, 2013
 *      Author: vqa
 */

#ifndef COMUNICACAO_H_
#define COMUNICACAO_H_

#include "rs232.h"

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

void enviarChecksum(unsigned char const  *buffer, unsigned short len, int COM_portNumber){
  unsigned int crc = uiCrc16Cal(buffer, len);

  RS232_SendByte(COM_portNumber, LOBYTE(crc));
  RS232_SendByte(COM_portNumber, HIBYTE(crc));
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

void enviarComando(unsigned char comando, int COM_portNumber)
{
	RS232_SendByte(COM_portNumber, 0x01);
	RS232_SendByte(COM_portNumber, comando);
}

unsigned char receberComando(int COM_portNumber)
{
	unsigned char tipo, comando;
	tipo = RS232_ReadByte(COM_portNumber);
	if(tipo != 0x01)
	{
		printf("Erro na comunicacao! Cabecalho de comando invalido\n");
		exit(1);
	}
	comando = RS232_ReadByte(COM_portNumber);

	return comando;
}

void enviarDados(unsigned char *buffer, unsigned short len, int COM_portNumber){
	RS232_SendByte(COM_portNumber, 0x00);
	RS232_SendByte(COM_portNumber, LOBYTE(len));
	RS232_SendByte(COM_portNumber, HIBYTE(len));
	if(RS232_SendBuf(COM_portNumber, buffer, len) < len)
	{
		printf("Erro na comunicacao! Erro ao enviar dados\n");
		exit(1);
	}
	enviarChecksum(buffer, len, COM_portNumber);
}

int receberDados(unsigned char *buffer, int COM_portNumber)
{
	unsigned char tipo;
	unsigned char len_MSB, len_LSB;
	unsigned int len, i;
	unsigned char CRC_LSB, CRC_MSB;
	tipo = RS232_ReadByte(COM_portNumber);
	if(tipo != 0x00)
	{
		printf("Erro na comunicacao! Cabecalho de dados invalido\n");
		exit(1);
	}
	len_LSB = RS232_ReadByte(COM_portNumber);
	len_MSB = RS232_ReadByte(COM_portNumber);
	len = (len_MSB | len_LSB);


	RS232_ReadBuffer(COM_portNumber, buffer, len);

	CRC_LSB = RS232_ReadByte(COM_portNumber);
	CRC_MSB = RS232_ReadByte(COM_portNumber);

	if(!checarChecksum(buffer, len, CRC_LSB, CRC_MSB))
	{
		printf("Erro na comunicacao! Erro Checksum de dados\n");
		exit(1);
	}

	return len;
}




#endif /* COMUNICACAO_H_ */
