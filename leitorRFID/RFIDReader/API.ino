const unsigned char getReaderInfo[] = {0x04, 0x00, 0x21};
const unsigned char inventory[] = {0x04, 0x00, 0x01};
const unsigned char buzzer[] = {0x07, 0x00, 0x33, 0, 0, 0};



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

