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


int flag_pos=0, pos_atual=0 , tam_atual=0;
// status=0, status_aux=0;

// vetor que será utilizado para criar pipe
//int fd[2];

// função para pegar as posições em linha de cada operador
int posicoes_comandos(int argc, char **argv, int *posicoes)
{
	posicoes[0] = 0 ;
	int total_comandos = 1,i;

	//percorre a matriz de argumentos procurando por pipe (|), or (||)
	// and (&&) e background (&)
	for (i=0; i<argc; i++) 
	{
		if(strcmp(argv[i] , "|") == 0)
		{
			posicoes[total_comandos] = i;
			total_comandos ++;
		}
		else if(strcmp(argv[i] , "||") == 0)
		{
			posicoes[total_comandos] = i;
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
			posicoes[total_comandos] = i;
			total_comandos ++;
		}

	}

	//retorna o total de comandos que vão existir
	return total_comandos;

}

//monta o comando atual dado a pos atual (linha atual)
char **monta_comando(int argc, int total, int *posicoes, char **argv)
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

void operador_atual(char **argv, int argc,int *posicoes)
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
				printf("entrou no pipe primeiro \n" );
				
				close(fd2[0]);
				dup2(fd2[1],STDOUT_FILENO);
				close(fd2[1]);
			}
			//comando do meio
			else if(flag == 1)
			{
				printf("entrou no pipe segundo \n" );
				
				//close(fd[1]);
				dup2(fd,STDIN_FILENO);
				//close(fd[0]);
				dup2(fd2[1],STDOUT_FILENO);
			}
			//comando final
			else if(flag == 2)
			{
				printf("entrou no pipe terceiro \n" );
				
				close(fd2[1]);
				dup2(fd,STDIN_FILENO);
				close(fd2[0]);
			}
		}

		//close(fd[0]);
		//close(fd[1]);

		// se for o ultimo comando fecha o pipe para leitura

		execvp(input[0],input);
	}
	else
	{

		//retorna o descritor de arquivos na posição de leitura
		//return fd[0];

		//waitpid(processo, &retorno,0 );
		//testar aqui !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		if(flag_pos != 0 && flag_pos != 3)
		{
			wait(status);
			if(*status == 0)
				*status = 1;
			else
				*status = 0;
			//printf("Valor de status : %d \n",*status);		
		}

		//close(fd[0]);
	}

	close(fd2[1]);
	return fd2[0];
}


int main(int argc, char **argv) 
{
	char **cmd, **cmd2;
	int *posicoes, qt_comandos;
	int saida = 0;
	int i,aux=0;
	int status=0, status_aux = -1;
	
	int fd=0;

	posicoes = malloc( sizeof (int) * argc);
	qt_comandos = posicoes_comandos(argc, argv, posicoes);

	//printf("quantidade de comandos : %d \n", qt_comandos);

	tam_atual = argc;
	printf("qt_comandos %d\n",qt_comandos);
	

	//temos apenas 1 comando
	if(qt_comandos == 1)
	{
		cmd = &argv[1];
		execvp(cmd[0],cmd);
	}
	//teremos mais de um comando
	else
	{
		while(aux<qt_comandos)
		{
			//printf("aux :%d\n", aux);
			cmd = monta_comando(argc,qt_comandos,posicoes,argv);
			//close(fd[0]);
			//close(fd[1]);
			//primeiro comando 
			if(aux == 0)
			{
				printf("comando primeiro \n");
				operador_atual(argv,argc,posicoes);
				printf("flag_pos:%d e status_aux:%d \n",flag_pos,status_aux);
				fd = executa_comando(cmd,0,&status,fd);
			}

			//ultimo comando
			else if (aux == qt_comandos-1)
			{
				printf("comando final , pos atual : %d\n",pos_atual);
				printf("flag_pos:%d e status_aux:%d \n",flag_pos,status_aux);
				operador_atual(argv,argc,posicoes);
				
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
				operador_atual(argv,argc,posicoes);
			}
			
			//verifica se o comando é && e caso primeiro comando falhe
			//não executa o segundo
			//if(status != 0 && flag_pos == 2)
			//	aux++;
			// caso comando seja || e resultado do primeiro seja positivo,
			//imprime o primeiro apenas
			//else if(status == 0 && flag_pos == 1 )
			//	aux++;



			aux++;
			status_aux = status;

		}
	}
	
	
	//close(fd[0]);
	//close(fd[1]);
	return 0;
}