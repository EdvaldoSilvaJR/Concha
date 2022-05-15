#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#define tam_comando 100

// Se flag_pos == 0 então |  pipe
// Se flag_pos == 1 então || or
// Se flag_pos == 2 então && and
// Se flag_pos == 3 então &  back


int flag_pos=3, pos_atual=0 , tam_atual=0;

// função para pegar as posições em linha de cada operador
int quantidade_comandos(int argc, char **argv)
{
	int total_comandos = 1,i;
	flag_pos = 0;
	//percorre a matriz de argumentos procurando por pipe (|), or (||)
	// and (&&) e background (&)
	for (i=0; i<argc; i++) 
	{
		if(strcmp(argv[i] , "|") == 0)
		{
			total_comandos ++;
		}
		else if(strcmp(argv[i] , "||") == 0)
		{
			total_comandos ++;
		}
		else if(strcmp(argv[i] , "&") == 0)
		{
			//posicoes[total_comandos] = i;
			//total_comandos ++;
			flag_pos = 3;
		}
		else if(strcmp(argv[i] , "&&") == 0)
		{
			total_comandos ++;
		}

	}

	//retorna o total de comandos que vão existir
	return total_comandos;

}

//monta o comando atual dado a pos atual (linha atual)
char **monta_comando(int argc, int total, char **argv)
{
	int i,cont=0;
	char **comando;
	comando = calloc(tam_comando,sizeof(char) * tam_comando);
	for(i=0;i<tam_comando;i++)
	{
		comando[i] = calloc(tam_comando,sizeof(char)*tam_comando);
	}
	tam_atual = 0 ;
			
	for(i=pos_atual+1;i<argc;i++)
	{
	
		if(strcmp(argv[i],"|")==0 || strcmp(argv[i],"||")==0 || strcmp(argv[i],"&")==0 || strcmp(argv[i],"&&")==0)
		{
			pos_atual = i;
			break;
		}
		else
		{
			comando[cont] = strcat(comando[cont],argv[i]);
		}

		tam_atual ++;
		cont++;
	}

	comando[cont] = NULL;
	return comando;
}

void operador_atual(char **argv, int argc)
{
	// Se flag_pos == 0 então |  pipe
	// Se flag_pos == 1 então || or
	// Se flag_pos == 2 então && and
	// Se flag_pos == 3 então &  back

	if(strcmp(argv[pos_atual],"|")==0)
		flag_pos = 0;
	else if (strcmp(argv[pos_atual],"||")==0)
		flag_pos = 1;
	else if (strcmp(argv[pos_atual],"&&")==0)
		flag_pos = 2;
	else if (strcmp(argv[pos_atual],"&")==0)
		flag_pos = 3;
}

//flag = indica se o comando é inicial, intermediário ou final
int executa_comando(char **input, int flag, int *status, int fd)
{
	int processo , retorno;
	int fd2[2];

	pipe(fd2);

	processo = fork();

	printf("valor de processo: %d \n",processo);

	if(processo<0)
	{
		perror("Problema com fork");
		return -1;
	}

	// processo filho 
	if(processo == 0 )
	{
		// operador pipe |
		if(flag_pos == 0)
		{
			//primeiro comando
			if(flag == 0)
			{
				close(fd2[0]);
				dup2(fd2[1],STDOUT_FILENO);
				close(fd2[1]);
			}
			//comando do meio
			else if(flag == 1)
			{
				dup2(fd,STDIN_FILENO);
				dup2(fd2[1],STDOUT_FILENO);
			}
			//comando final
			else if(flag == 2)
			{
				close(fd2[1]);
				dup2(fd,STDIN_FILENO);
				close(fd2[0]);
			}
		}

		// se for o ultimo comando fecha o pipe para leitura

		execvp(input[0],input);
	}
	//executa o comando no background &
	else if(flag_pos == 3)
	{
		waitpid(-1,status,WNOHANG);
		printf("Status do comando de background:%d \n",*status);
	}
	else
	{
		// caso seja comando || ou &&
		if(flag_pos != 0 && flag_pos != 3)
		{
			//pega o status de execução do filho para saber se deu certo
			//ou errado
			wait(status);
			if(*status == 0)
				*status = 1;
			else
				*status = 0;
		}
	}

	close(fd2[1]);
	//retorna o pipe
	return fd2[0];
}

char** novo_comando(char *entrada)
{
	int i=0,j=0,cont ;
	char **saida;
	saida = calloc(tam_comando,sizeof(char) * tam_comando);
	while(entrada[i] != '\0')
	{
		if(entrada[i] == ' ')
			cont++;

		i++;
	}

	for(i=0;i<cont;i++)
	{
		saida[i] = calloc(tam_comando,sizeof(char)*tam_comando);
		while(entrada[j] != ' ')
			saida[i][j] = entrada[j];
	}
	saida[cont] = calloc(tam_comando,sizeof(char)*tam_comando);	
	saida[cont] = NULL;

	tam_atual = cont;
	return saida;

}


int main(int argc, char **argv) 
{
	char **cmd, *cmd2;
	int qt_comandos;
	int aux=0;
	int status=0, status_aux = -1;
	int fd=0;


	//temos apenas 1 comando
	while(flag_pos == 3)
	{
		qt_comandos = quantidade_comandos(argc, argv);
		
		if(qt_comandos == 1 && flag_pos!=3)
		{
			cmd = &argv[1];
			execvp(cmd[0],cmd);
		}
		else if(flag_pos == 3)
		{
			cmd = &argv[1];
			fd = executa_comando(cmd,3,&status,fd);
			scanf("digite um novo comando : %s \n",cmd2);
			novo_comando(cmd2);
			argc = tam_atual;
		}
	}
	//temos caso de comando &
	//teremos mais de um comando ,ou um dos comandos ficou de background
	if( qt_comandos > 1)
	{
		while(aux<qt_comandos)
		{
			cmd = monta_comando(argc,qt_comandos,argv);
			//primeiro comando 
			if(aux == 0)
			{
				printf("comando primeiro \n");
				operador_atual(argv,argc);
				printf("flag_pos:%d e status_aux:%d \n",flag_pos,status_aux);
				fd = executa_comando(cmd,0,&status,fd);
			}

			//ultimo comando
			else if (aux == qt_comandos-1)
			{
				printf("comando final , pos atual : %d\n",pos_atual);
				printf("flag_pos:%d e status_aux:%d \n",flag_pos,status_aux);
				operador_atual(argv,argc);
				
				//se a operação atual for um || e resultado for negativo
				//ou se a ultima operação for um && e resultado for positivo
				if((flag_pos == 1 && status_aux == 0) || (flag_pos == 2 && status_aux == 1) || (flag_pos == 0))
				{
					fd = executa_comando(cmd,2,&status,fd);
				}
			}
			//comando intermediário
			else
			{
				printf("comando intermediario \n");
				printf("flag_pos:%d e status_aux:%d \n",flag_pos,status_aux);
				if((flag_pos == 1 && status_aux == 0) || (flag_pos == 2 && status_aux == 1) || (flag_pos == 0))
				{
					fd = executa_comando(cmd,1,&status,fd);
				}	
				operador_atual(argv,argc);
			}

			aux++;
			status_aux = status;

		}
	}
	
	
	close(fd);
	return 0;
}