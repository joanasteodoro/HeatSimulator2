//Projeto SO - exercicio 1
//----- Grupo 97 -----
//Joana Teodoro, 86440
//Taíssa Ribeiro, 86514

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "matrix2d.h"
#include "mplib3.h"


//Estrutura de uma trabalhadora
typedef struct { 
  int id;
  int N;
  int trab;
  int iter;
} args_mythread_t;


/*--------------------------------------------------------------------
| Function: simul
  Novos argumentos: trab, id
---------------------------------------------------------------------*/

DoubleMatrix2D *simul(DoubleMatrix2D *matrix, DoubleMatrix2D *matrix_aux, int linhas, int colunas, int numIteracoes, int trab, int id) {

  DoubleMatrix2D *m, *aux, *tmp;
  int iter, i, j;
  double value;
  int k = linhas - 2;


  if(linhas < 2 || colunas < 2)
    return NULL;

  m = matrix;
  aux = matrix_aux;


  for (iter = 0; iter < numIteracoes; iter++) {
  
    for (i = 1; i < linhas - 1; i++){
      for (j = 1; j < colunas - 1; j++) {
        value = ( dm2dGetEntry(m, i-1, j) + dm2dGetEntry(m, i+1, j) +
        dm2dGetEntry(m, i, j-1) + dm2dGetEntry(m, i, j+1) ) / 4.0;
        dm2dSetEntry(aux, i, j, value);
      }
    }  

    tmp = aux;
    aux = m;
    m = tmp;


    //Troca de mensagens: 
    //Quando uma thread i envia uma mensagem para um thread j, a primeira coisa que j faz é receber o conteúdo de i
    //de forma a que, quando o armazenamento é nulo, não haja perda de informação

    if (trab > 1){
        if (id != 0){
          if((receberMensagem(id - 1, id, dm2dGetLine(m, 0), sizeof(double)*(colunas))) == -1){
            printf("Erro na receção da mensagem\n");
            pthread_exit((void*)-1);
          }
          if((enviarMensagem(id, id - 1, dm2dGetLine(m, 1), sizeof(double)*(colunas))) == -1){
            printf("Erro no envio da mensagem\n");
            pthread_exit((void*)-1);
          }
        }
        if (id != trab - 1){
          if ((enviarMensagem(id, id + 1, dm2dGetLine(m, k), sizeof(double)*(colunas))) == -1){
            printf("Erro no envio da mensagem\n");
            pthread_exit((void*)-1);
          }
          if((receberMensagem(id + 1, id, dm2dGetLine(m, k + 1), sizeof(double)*(colunas))) == -1){
            printf("Erro na receção da mensagem\n");
            pthread_exit((void*)-1);
          }
        }
    }
  }
  return m;
}

/*--------------------------------------------------------------------
| Function: myThread
---------------------------------------------------------------------*/

void *myThread(void *a) {

  args_mythread_t *args = (args_mythread_t *) a;

  int N = args->N;
  int trab = args->trab;
  int iter = args->iter;
  int id = args->id;
  int j;
  int k = (N/trab);

  //Matrizes auxiliares para cada thread executar simul
  DoubleMatrix2D *buffer = dm2dNew(k+2, N+2);
  DoubleMatrix2D *buffer_aux = dm2dNew(k+2, N+2);
  DoubleMatrix2D *res;
     
  //Recebe mensagem da tarefa mestre
  for (j = 0; j < k+2; j++){
    if((receberMensagem(trab, id, dm2dGetLine(buffer, j), sizeof(double)*(N+2))) == -1){
      printf("Erro na receção da mensagem\n");
      pthread_exit((void*)-1);
    }
  }

  dm2dCopy(buffer_aux, buffer);

  res = simul(buffer, buffer_aux, k + 2, N + 2, iter, trab, id);

  //Envia mensagem para tarefa mestre
  for (j = 0; j <= k + 1; j++){
    if ((enviarMensagem(id, trab, dm2dGetLine(res, j), sizeof(double)*(N+2))) == -1){
      printf("Erro no envio da mensagem\n");
      pthread_exit((void*)-1);
    }
  }

  dm2dFree(buffer);
  dm2dFree(buffer_aux);
  
  return 0;
}


/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name)
{
  int value;
 
  if(sscanf(str, "%d", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name)
{
  double value;

  if(sscanf(str, "%lf", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}


/*--------------------------------------------------------------------
| Function: main
---------------------------------------------------------------------*/

int main (int argc, char** argv) {

  args_mythread_t *slave_args;
  int              i,j;
  pthread_t       *slaves;

  if(argc != 9) {
    fprintf(stderr, "\nNumero invalido de argumentos.\n");
    fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes trab csz\n\n");
    return 1;
  }

  /* argv[0] = program name */
  int N = parse_integer_or_exit(argv[1], "N");
  double tEsq = parse_double_or_exit(argv[2], "tEsq");
  double tSup = parse_double_or_exit(argv[3], "tSup");
  double tDir = parse_double_or_exit(argv[4], "tDir");
  double tInf = parse_double_or_exit(argv[5], "tInf");
  int iteracoes = parse_integer_or_exit(argv[6], "iteracoes");
  int trab = parse_integer_or_exit(argv[7], "trab");
  int csz = parse_integer_or_exit(argv[8], "csz");


  fprintf(stderr, "\nArgumentos:\n"
	" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d trab=%d csz=%d\n",
	N, tEsq, tSup, tDir, tInf, iteracoes, trab, csz);

  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iteracoes < 1 || trab <=0 || N%trab !=0 || csz<0) {
    fprintf(stderr, "\nErro: Argumentos invalidos.\n"
	" Lembrar que N >= 1, temperaturas >= 0, iteracoes >= 1, trab>=1 e tem de ser multiplo de N, e csz>=0\n\n");
    return 1;
  }

  int k = (N/trab); //numero de linhas interiores de cada thread

  DoubleMatrix2D *matrix, *matrix_aux, *result;

  slave_args = (args_mythread_t*)malloc(trab*sizeof(args_mythread_t));
  slaves     = (pthread_t*)malloc(trab*sizeof(pthread_t));

  matrix = dm2dNew(N+2, N+2);
  matrix_aux = dm2dNew(N+2, N+2);

  if (matrix == NULL || matrix_aux == NULL || slave_args == NULL || slaves == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para as matrizes ou para as trabalhadoras\n\n");
    return -1;
  }


  for(i=0; i<N+2; i++)
    dm2dSetLineTo(matrix, i, 0);

  dm2dSetLineTo (matrix, 0, tSup);
  dm2dSetLineTo (matrix, N+1, tInf);
  dm2dSetColumnTo (matrix, 0, tEsq);
  dm2dSetColumnTo (matrix, N+1, tDir);

  dm2dCopy (matrix_aux, matrix);

  if(trab > 1){
    if (inicializarMPlib(csz,trab+1) == -1) {
      printf("Erro ao inicializar MPLib.\n"); return 1;
    }


    //Criação de threads e envio, por parte da tarefa mestre, linha a linha da informação 
    //com a qual as threads terão de trabalhar
    for (i=0; i<trab; i++) {
      slave_args[i].id = i;
      slave_args[i].N = N;
      slave_args[i].trab = trab;
      slave_args[i].iter = iteracoes;
      printf("Creating thread %d\n", i);
      pthread_create(&slaves[i], NULL, myThread, &slave_args[i]);
      for (j = k*i; j <= k*(i+1)+1; j++) {
        if((enviarMensagem (trab, i, dm2dGetLine(matrix, j), sizeof(double)*(N+2))) == -1){
          printf("Erro no envio da mensagem\n");
          return -1;
        }
      }
    }

    //Receção, a partir de cada trabalhadora, da informção simulada para a mestre
    for (i = 0; i < trab; i++) {
      for (j = k*i; j <= k*(i + 1) + 1; j++) {
        if((receberMensagem (i, trab, dm2dGetLine(matrix, j), (N+2)*sizeof(double))) == -1){
          printf("Erro na receção da mensagem\n");
          return -1;
        }
      }
    }

    for (i=0; i<trab; i++) {
      if (pthread_join(slaves[i], NULL)) {
        fprintf(stderr, "\nErro ao esperar por um escravo.\n");    
        return -1;
      }
    }

    dm2dPrint(matrix);
  }


  //Caso de simulação sequencial
  else if(trab == 1){
    result = simul(matrix, matrix_aux, N+2, N+2, iteracoes, trab, 0);
    if (result == NULL) {
      printf("\nErro na simulacao.\n\n");
      return -1;
    }
    dm2dPrint(result);
  }

  libertarMPlib();
  dm2dFree(matrix);
  dm2dFree(matrix_aux);
  free(slaves);
  free(slave_args);

  return 0;
}
