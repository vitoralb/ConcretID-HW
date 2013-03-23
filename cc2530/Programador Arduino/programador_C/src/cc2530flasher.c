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

#define BUFFER_SIZE 1536

extern char comports[][16];


int main(int argc, char **argv)
{
	FILE *fin;


	fpos_t fpos;
	int file_size = 0;
	int count = 0;
	static unsigned char buffer[BUFFER_SIZE], buffer2[BUFFER_SIZE];
	unsigned char buf_aux[4];
	int n = 0, i, n_aux;
	int COM_portNumber = 0; // /dev/ttyACM0


	if(argc != 3)
	{
		printf("Uso: cc2530flasher porta arquivo.bin\n");
		exit(1);
	}

	while (strcmp(argv[1], comports[COM_portNumber]) && COM_portNumber < 30)
	{
		COM_portNumber++;
	}

	if (COM_portNumber >= 30)
	{
		printf("Erro ao abrir a porta serial!\n");
		exit(1);
	}

	fin = fopen(argv[2], "rb");
	if (!fin)
	{
		printf("Erro ao abrir o arquivo!\n");
		exit(1);
	}

	fgetpos(fin, &fpos);
	fseek(fin, 0L, SEEK_END);
	file_size = ftell(fin);
	fsetpos(fin, &fpos);

	printf("Tamanho do arquivo: %d bytes\n", file_size);

	if (RS232_OpenComport(COM_portNumber, 115200))
	{
		printf("Erro ao abrir a porta serial!\n");
		exit(1);
	}
//	RS232_disableDTR(COM_portNumber);
//	RS232_disableRTS(COM_portNumber);

//	if (!fin)
//	{
//		printf("Erro ao abrir o arquivo!\n");
//		exit(1);
//	}

#ifdef _WIN32
	Sleep(3000);
#else
	usleep(3000000); /* sleep for 100 milliSeconds */
#endif

	enviarComando(CMD_SERIAL_GET_INFO, COM_portNumber);
	receberDados(buffer, COM_portNumber);

	if (buffer[0] == 0)
	{
		printf("Nenhum Chip Detectado!\n");
		exit(1);
	}

	printf("Chip ID: ");
	printf("%X\n", buffer[0]);
	printf("Version: ");
	printf("%X\n", buffer[1]);
	printf("Flash Memory Size: ");
	switch (buffer[2])
	{
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

	enviarComando(CMD_SERIAL_ERASE_CHIP, COM_portNumber);
	if (receberComando(COM_portNumber) != CMD_SERIAL_ACK)
	{
		printf("Erro de comunicacao! Ack nao recebido CMD_SERIAL_ERASE_CHIP\n");
	}
	else
	{
		printf("Chip Apagado Completamente!\n");
	}

	enviarComando(CMD_SERIAL_ENABLE_DMA, COM_portNumber);
	if (receberComando(COM_portNumber) != CMD_SERIAL_ACK)
	{
		printf("Erro de comunicacao! Ack nao recebido CMD_SERIAL_ENABLE_DMA\n");
	}

	while ((n = fread(buffer, sizeof(unsigned char), BUFFER_SIZE, fin)))
	{
		enviarComando(CMD_SERIAL_SET_WRITE_ADDR, COM_portNumber);
		buf_aux[0] = (unsigned char) (count & 0x000000FF);
		buf_aux[1] = (unsigned char) ((count & 0x0000FF00) >> 8);
		buf_aux[2] = (unsigned char) ((count & 0x00030000) >> 16);
		enviarDados(buf_aux, 3, COM_portNumber);
		if (receberComando(COM_portNumber) != CMD_SERIAL_ACK)
		{
			printf(
					"Erro de comunicacao! Ack nao recebido CMD_SERIAL_SET_WRITE_ADDR\n");
		}
		// realmente necessario?
		if (n % 4 != 0)
		{
			while (n % 4 != 0 && n <= BUFFER_SIZE)
			{
				buffer[n] = 0;
				n++;
			}
		}
		enviarComando(CMD_SERIAL_SET_WRITE_BUFFER, COM_portNumber);
		enviarDados(buffer, n, COM_portNumber);
		if (receberComando(COM_portNumber) != CMD_SERIAL_ACK)
		{
			printf(
					"Erro de comunicacao! Ack nao recebido CMD_SERIAL_SET_WRITE_BUFFER\n");
		}

		enviarComando(CMD_SERIAL_START_WRITE, COM_portNumber);
		if (receberComando(COM_portNumber) != CMD_SERIAL_ACK)
		{
			printf(
					"Erro de comunicacao! Ack nao recebido CMD_SERIAL_START_WRITE\n");
		}

		printf("%d%%\r", (int) ((((double) count) / file_size) * 100));
		fflush(stdout);
		count += n;

	}

	printf("100%%");
	printf("\n%d bytes escritos.\n", count);

	printf("Deseja fazer a verificacao dos dados escritos? (s/n) ");
	scanf("%c", buf_aux);

	if ((buf_aux[0] == 's') | (buf_aux[0] == 'S') | (buf_aux[0] == 'y')
			| (buf_aux[0] == 'Y'))
	{

		fsetpos(fin, &fpos);
		count = 0;
		while ((n = fread(buffer, sizeof(unsigned char), BUFFER_SIZE, fin)))
		{
			enviarComando(CMD_SERIAL_SET_READ_ADDR, COM_portNumber);
			buf_aux[0] = (unsigned char) (count & 0x000000FF);
			buf_aux[1] = (unsigned char) ((count & 0x0000FF00) >> 8);
			buf_aux[2] = (unsigned char) ((count & 0x00030000) >> 16);
			enviarDados(buf_aux, 3, COM_portNumber);
			if (receberComando(COM_portNumber) != CMD_SERIAL_ACK)
			{
				printf(
						"Erro de comunicacao! Ack nao recebido CMD_SERIAL_SET_READ_ADDR\n");
			}
			// realmente necessario?
			if (n % 4 != 0)
			{
				while (n % 4 != 0 && n <= BUFFER_SIZE)
				{
					buffer[n] = 0;
					n++;
				}
			}
			enviarComando(CMD_SERIAL_START_READ, COM_portNumber);
			buf_aux[0] = (unsigned char) (n & 0x000000FF);
			buf_aux[1] = (unsigned char) ((n & 0x0000FF00) >> 8);
			enviarDados(buf_aux, 2, COM_portNumber);
			if ((n_aux = receberDados(buffer2, COM_portNumber)) != n)
			{
				printf(
						"Erro de comunicacao! Esperado %d bytes, chegaram %d bytes\n",
						n, n_aux);
			}
			for (i = 0; i < n; ++i)
				if (buffer[i] != buffer2[i])
				{
					printf(
							"Erro de verificacao! Conteudo recebido apos escrita diferente do esperado\n");
					break;
				}

			if (receberComando(COM_portNumber) != CMD_SERIAL_ACK)
			{
				printf(
						"Erro de comunicacao! Ack nao recebido CMD_SERIAL_START_READ\n");
			}

			printf("%d%%\r", (int) ((((double) count) / file_size) * 100));
			fflush(stdout);
			count += n;

		}

		printf("100%%");
		printf("\n%d bytes verificados.\n", count);
	}
	fclose(fin);

	return 0;
}
