/*
 * main.c
 *
 *  Created on: Mar 18, 2013
 *      Author: vqa
 */

#include <stdio.h>
#include <stdlib.h>
#include "rs232.h"
#include "Comunicacao.h"


int main(int argc, char **argv)
{
	FILE *fin = fopen("hello-world.bin", "r");
	FILE *fout = fopen("saida.txt", "w");
	unsigned char buffer[2048];
	int n = 0;
	int COM_portNumber = 24; // /dev/ttyACM0
	//int c = 0;

	if (RS232_OpenComport(COM_portNumber, 9600))
	{
		printf("Erro ao abrir a porta serial!\n");
		exit(1);
	}
//	RS232_disableDTR(COM_portNumber);
//	RS232_disableRTS(COM_portNumber);

	if (!fin)
	{
		printf("Erro ao abrir o arquivo!\n");
		exit(1);
	}

#ifdef _WIN32
		Sleep(3000);
#else
		usleep(3000000); /* sleep for 100 milliSeconds */
#endif

	enviarComando(CMD_SERIAL_GET_INFO, COM_portNumber);
	receberDados(buffer, COM_portNumber);

	if(buffer[0] == 0){
	    printf("Nenhum Chip Detectado!\n");
		exit(1);
	}

    printf("Chip ID: ");
    printf("%X\n", buffer[0]);
    printf("Version: ");
    printf("%X\n", buffer[1]);
    printf("Flash Memory Size: ");
    switch(buffer[2]){
      case 1:
         printf("32 kB\n");
         break;
      case 2:
         printf("64 kB\n");
         break;
      case 3:
         printf("128 kB\n");
         break;
      case 4:
         printf("256 kB\n");
         break;
    }
    printf("SRAM Memory Size: %d kB\n", buffer[3]);


	while ((n = read(fileno(fin), buffer, 2048)))
	{
		if(n % 4 != 0)
		{
			while(n % 4 != 0 && n <= 2048)
			{
				buffer[n] = 0;
				n++;
			}
		}




		//TODO enviar o buffer e esperar uma confirmacao

	}

	fclose(fin);
	fclose(fout);

	return 0;
}

