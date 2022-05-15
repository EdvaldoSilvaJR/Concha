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
pid_t id_back ;


int quantidade_comandos(int argc, char **argv)
{
	int total_comandos = 1, i;
	flag_pos = 0;

	// Percorre a matriz de argumentos procurando por pipe (|), or (||)
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
			flag_pos = 3; // Quando "&" é encontrado significa que não temos 
					      // mais comandos além dele, então somente marcamos 
					      // flag_pos com 3 para depois sabermos que ali havia 
					      // um parâmetro de background. 
		}
		else if(strcmp(argv[i] , "&&") == 0)
		{
			total_comandos ++;
		}
	}
	return total_comandos;
}


char **monta_comando(int argc, int total, char **argv)
{
	int i, cont=0;
	char **comando;
	tam_atual = 0;

	comando = calloc(tam_comando,sizeof(char) * tam_comando);
	for(i=0;i<tam_comando;i++)
	{
		comando[i] = calloc(tam_comando,sizeof(char)*tam_comando);
	}

			
	for(i=pos_atual+1; i<argc; i++)
	{
	
		if(strcmp(argv[i],"|")==0 || strcmp(argv[i],"||")==0 || strcmp(argv[i],"&")==0 || strcmp(argv[i],"&&")==0)
		{
			pos_atual = i; // Identifica a última linha do subcomando antes do parâmetro
			break;
		}
		else
		{
			comando[cont] = strcat(comando[cont],argv[i]); 
		}

		tam_atual++; // Identifica o tamanho do comando atual
		cont++; 
	}

	comando[cont] = NULL; // Execvp precisa que a última linha da matriz seja sempre NULL
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

	if(processo<0)
	{
		perror("Problema com fork");
		return -1;
	}

	// Processo filho 
	if(processo == 0 )
	{
		// Operador pipe |
		if(flag_pos == 0)
		{
			// Primeiro comando
			if(flag == 0)
			{
				close(fd2[0]); // Fecha a posição zero do pipe porque o primeiro comando não lê valores de lá 
				dup2(fd2[1],STDOUT_FILENO); // Escreve o file descriptor do processo filho na posição 1 do pipe 
				close(fd2[1]); // Fecha a posição de escrita
			}

			// Comando do meio
			else if(flag == 1)
			{
				dup2(fd,STDIN_FILENO);
				dup2(fd2[1],STDOUT_FILENO);
				// Como o comando intermediário lê e escreve do pipe, então não devemos fechar nenhuma posição do pipe 
			}

			// Último comando
			else if(flag == 2)
			{
				close(fd2[1]); // O último comando não escreve no pipe, logo podemos fechar a posição 1
				dup2(fd, STDIN_FILENO); // Lê o file descriptor 
				close(fd2[0]); // Fecha a posição de leitura
			}
		}

		id_back = getpid(); 

		execvp(input[0],input); // Executa o comando por chamada de sistema
	}

	// Operador background &
	else if(flag_pos == 3)
		waitpid(-1, status, WNOHANG); // Espera um processo filho executar 
									  // WNOHANG continua mesmo que nenhum filho tenha terminado
									  // -1 indica que pode ser o PID de qualquer filho
									  // status: recebe o resultado do processo filho

	// Operdores OR || e AND &&
	else if(flag_pos != 0 && flag_pos != 3)
	{
		wait(status); // Verifica se o filho deu certo 
		if(*status == 0)
			*status = 1;
		else
			*status = 0;
	}

	close(fd2[1]); 
	return fd2[0]; // Retorna o conteúdo do pipe de escrita 
}


// Função utilizada para montar comandos quando o primeiro comando foi background &
char** novo_comando(char *entrada) // Entrada: novo comando digitado, ele é lido como uma string, em apenas uma linha 
{
	int i=0, j=0, cont=1, k=0 ;
	char **saida;
	saida = calloc(tam_comando,sizeof(char) * tam_comando);
	

	for(i=0;i<strlen(entrada);i++)
	{
		if(entrada[i] == ' ') // Conta quantas palavras o comando possui
			cont++;
	}


	// Faz a matriz de saída receber cada palavra do comando em uma linha, 
	// passando a cada letra do comando armazenado em "entrada"
	for(i=0;i<cont;i++)
	{
		saida[i] = calloc(tam_comando,sizeof(char)*tam_comando);
		j=0;
		while(entrada[k] != ' ')
		{
			saida[i][j] = entrada[k];
			
			j++;
			k++;
		}
		k++;
	}
	saida[cont] = calloc(tam_comando,sizeof(char)*tam_comando);	
	saida[cont] = NULL; // Finaliza a matriz de saída com NULL necessário ao comando execvp

	tam_atual = cont; // Atualiza a variável global com o tamanho do nosso comando para, posteriormente, atualizar argc 

	return saida;
}


void imprime_argv(int argc, char **argv)
{
	int i,j,tamanho_linha,tamanho_coluna;
	tamanho_linha = strlen(*argv);
	for(i=0;i<argc;i++)
	{
		printf("%s \n",argv[i]);
		//printf("tamanho da linha %d \n",strlen(argv[i]));
	}

}


int main(int argc, char **argv) 
{
	char **cmd , cmd2[10000]; //Recebe o comando pronto para ser executado
	int qt_comandos; // Armazena a quantidade de comandos, desconsiderando parâmetros
	int aux=0; // Variável auxiliar para percorrer os comandos
	int status=0, status_aux=-1; // Status: resultado do comando executado
				 				 // Status_aux: Resultado do comando anterior ao que está em execução (utilizado para || e &&)
	int fd=0; // Recebe o valor do pipe de leitura


	while(flag_pos == 3) // flag_pos sempre é inicializado com 3, o que pode indicar um background no comando ou ser apenas uma inicialização 
	{
		qt_comandos = quantidade_comandos(argc, argv); // Recebe a quantidade de subcomando presentes no comando 
		

		// Na função quantidade_comandos a flag_pos é atualizada caso o comando não seja um background, e executa apenas um comando
		if(qt_comandos == 1 && flag_pos != 3) 
		{
			cmd = &argv[1]; // argv na posição zero é o nome do executável 
			execvp(cmd[0], cmd); 
		}

		else if(flag_pos == 3) // Se a função não atualizou a flag, então se trata de um background
		{
			argv[argc-1] = NULL; // Substitui o & por NULL porque ali marca o final do comando 
			cmd = &argv[1]; 
			fd = executa_comando(cmd, 3, &status, fd); // Passa o comando para a função. 3 marcaria que este é um comando final mas
													   // aqui ele é enviado apenas porque a função exige
			
			fgets(cmd2,100,stdin); // Pega o novo comando que o usuário digita 
			argv = novo_comando(cmd2); // Manda o comando para a função que devolve ele em uma matriz igual a estrutura do argv
			argc = tam_atual;
		}
	}

	if(qt_comandos > 1)
	{
		while(aux < qt_comandos)
		{
			cmd = monta_comando(argc, qt_comandos, argv); // Recebe o comando em apenas uma linha

			// Primeiro comando 
			if(aux == 0)
			{
				operador_atual(argv,argc); // Atualiza a flag_pos sobre o tipo de operador presente no comando 
				fd = executa_comando(cmd, 0, &status, fd); // Recebe o descritor de arquivos presente na posição zero do pipe
			}

			// Último comando
			else if (aux == qt_comandos-1)
			{
				operador_atual(argv,argc);
				
				// Se o operador for um || e o resultado do comando anterior apresentou erro
				// ou se o último operador for um && e o resultado do comando anterior for de êxito, o comando atual é executado 
				if((flag_pos == 1 && status_aux == 0) || (flag_pos == 2 && status_aux == 1) || (flag_pos == 0))
					fd = executa_comando(cmd, 2, &status, fd); // 2 indica para o executa_comando que este se trata de um comando final
			}

			// Comando intermediário
			else
			{
				if((flag_pos == 1 && status_aux == 0) || (flag_pos == 2 && status_aux == 1) || (flag_pos == 0))
					fd = executa_comando(cmd, 1, &status, fd); // 1 indica para o executa_comando que este se trata de um comando intermediário

				operador_atual(argv,argc); 
			}

			aux++;
			status_aux = status; // Atualiza o status atual do comando que acabou de ser executado
		}
	}
	
	close(fd); 
	//kill(id_back,SIGSEGV);
	return 0;
}