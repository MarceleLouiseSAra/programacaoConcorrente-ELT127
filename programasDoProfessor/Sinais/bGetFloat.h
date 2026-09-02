//
//	Programação Multithreaded em ambiente Windows NT© - uma visão de  Automação
// 
//	Autores: Constantino Seixas Filho/ Marcelo Szuster
//
//	Função bGetFloat: Entrada de dados de valor em Floating Point
//
//	Versão: 1.1	20/01/1999
//
//  Adaptada para WSL (Windows Subsystem for Linux) por Luiz T. S. Mendes
//  em 24/03/2020

#include <ctype.h>		// _isdigit
#include "conio.h"      // Simula <conio.h> do Windows e implementa _getch()
#include <stdio.h>

//Adaptações para compatibilidade com linguagem C do Visual Studio
typedef int BOOL;
#define FALSE 0
#define TRUE  !FALSE

#define	ESC			0x1B
#define CR			0x0D
#define LF			0x0A
#define BS			0x08
#define BS_WSL      0x7F

BOOL bGetFloat(double *result, int iMaxDig)
{	
	int dwNum = 0;
	int dwDivFrac = 1;
	int iNumDig = 0;
	int ch;
	int Estado = 0; // Estado = 0: ponto não entrou.

	do {
		ch=_getch();
		if (isdigit(ch)) {
			printf("%c", ch); // ecoa
			dwNum = dwNum*10 + (ch - 0x30);
			iNumDig++;  // incrementa número de dígitos
			if (Estado == 1) dwDivFrac *= 10;
		} // if

		else {
			if (ch == ESC) return FALSE;
			//if (ch == CR) {   /* No WSL, <Enter> retorna LF e não CR!
			if (ch == LF) {
				if (iNumDig > 0) break;
				else return FALSE;  // Não digitou nada
			}

			if ((ch == BS_WSL) && (iNumDig>=1)) {
				printf("%c%c%c", BS_WSL, ' ', BS_WSL); // apaga último caracter
				
				if (Estado == 0) {
					dwNum /= 10;
					iNumDig--;
				}
				else if (dwDivFrac >1) {
							dwNum /= 10;
							iNumDig--;
							dwDivFrac /= 10;
						}
					  else Estado = 0;
			}
			
			if ((ch == '.') && (Estado == 0)) {
				Estado = 1;
				printf("%c", '.');
			}
		}	 
	} while (iNumDig<=iMaxDig);

	*result = (double)dwNum / dwDivFrac;
	return TRUE;
	
}	// bGetFloat


