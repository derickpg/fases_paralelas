#include <stdio.h>
#include "mpi.h"


void bs(int n, int * vetor)
{
    int c=0, d, troca, trocou =1;

    while (c < (n-1) & trocou )
        {
        trocou = 0;
        for (d = 0 ; d < n - c - 1; d++)
            if (vetor[d] > vetor[d+1])
                {
                troca      = vetor[d];
                vetor[d]   = vetor[d+1];
                vetor[d+1] = troca;
                trocou = 1;
                }
        c++;
        }
}

main(int argc, char** argv)
  {
  /* Parametros */    
  int pedaco_troca  =  5;        // 1/% do pedaco que deve ser trocado com o vizinho
  int tam_vet       =  10000;    // Tamanho total do Vetor a ser Ordenado

  /* Variaveis */
  int my_rank;          // Identificador deste processo
  int np;               // Numero de processos disparados pelo usuario na linha de comando (np)
  int vetor[tam_vet];   // Vetor 
  int i,j,k,l;          // Variavel Auxiliar para For
  int tam_vet_pedaco;   // Tamanho do Vetor para Cada Pedaco   
  int ultimo;           // Variavel Auxiliar para Ultima Posicao do Vetor do Vizinho
  int ordenado;         // Flag que verifica se está ordenado!
  int result_troca;     // Resultado da soma do Vetor de Troca
  MPI_Status status;    // estrutura que guarda o estado de retorno   


  MPI_Init(&argc , &argv); // funcao que inicializa o MPI, todo o codigo paralelo estah abaixo

  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); // pega pega o numero do processo atual (rank)
  MPI_Comm_size(MPI_COMM_WORLD, &np);  // pega informacao do numero de processos (quantidade total)

    ordenado = 0;
    tam_vet_pedaco = tam_vet/np;            // Tamanho do Pedaco que cada Processo vai ordenar
    int vet_aux[tam_vet_pedaco];            // Vetor auxiliar que realiza a ordenacao
    int troca[np];                          // Flag para saber se precisa trocar pedacos do vetor
    int aux[(tam_vet_pedaco/pedaco_troca)]; // Vetor Auxiliar para Envio das Trocas
    int aux1[(tam_vet_pedaco/pedaco_troca)]; // Vetor Auxiliar para Envio das Trocas
    //printf("Tam Metade = %d", tam_vet_pedaco);

  /*-*-*-*-*-*-*-*-*-*-* Inicio do Processo , o Rank 0 esta coordenando -*-*-*-*-*-*-*-*-*-* */ 
  if ( my_rank == 0 ){ // sou o primeiro?
    for(i = 0; i < tam_vet; i++){
        vetor[i] = tam_vet - i;
    }
    // Manda para todo mundo o vetor
    k = tam_vet_pedaco;
    l = 0;
    for(i = 1; i < np; i++){
        l = 0;
        for(j = k; j < (k + tam_vet_pedaco); j++){
            vet_aux[l] = vetor[j];
            l++;
        }
        k = k + tam_vet_pedaco;
        MPI_Send(&vet_aux, tam_vet_pedaco, MPI_INT, i, 1, MPI_COMM_WORLD); 
    }
  }else
    MPI_Recv(&vet_aux, tam_vet_pedaco, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // recebo primeiro vetor

    // Arrumar o vetor do Rank 0
    if(my_rank == 0)
        for(i = 0; i < tam_vet_pedaco; i++){
            vet_aux[i] = vetor[i];
        }

  /* -*-*-*-*-*-*-*-*-*-* Fim do Processo inicial -*-*-*-*-*-*-*-*-*-* */  

    while(ordenado == 0){
        bs(tam_vet_pedaco,vet_aux);
        troca[my_rank] = 0;                           // Informa que a troca nao e necessaria

        /* print  VETOR ordenado do PROCESSO 
        if(my_rank == 0) {
            printf(" \n BS PID : %d = ", my_rank);
            for(i = 0; i < tam_vet_pedaco; i++){
                printf(" %d, ", vet_aux[i]); 
            }
            printf(" \n");
        }
        fim print*/

        if(my_rank != (np-1))                         // Se eu nao for o ultimo eu mando para meu vizinho da direita
            MPI_Send(&vet_aux[tam_vet_pedaco - 1], 1, MPI_INT, (my_rank+1), 1, MPI_COMM_WORLD); 
        if(my_rank != 0){
            MPI_Recv(&ultimo, 1, MPI_INT, (my_rank -1), MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if(ultimo > vet_aux[0]){ // Precisa trocar
                troca[my_rank] = 1;
                //printf("\n Sou o PID %d e o ult é [%d] > {%d} primeiro e troca = %d  ",my_rank ,ultimo, vet_aux[0], troca[my_rank]);
            }else{         
                //printf("\n Sou o PID %d e o ult é [%d] > {%d} primeiro e troca = %d  ",my_rank ,ultimo, vet_aux[0], troca[my_rank]); // Nao precisa Trocar
                troca[my_rank] = 0;
            }
        }
        // Multicast
        for(i = 0; i < np; i++)
            MPI_Bcast(&troca[i], 1, MPI_INT, i, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        // Imprimo o Vetor de Troca 
        result_troca = 0;
        for(i = 0; i < np; i++){
            //printf("\n Sou o %d e o valor do TROCA = %d ",my_rank ,troca[i]);
            result_troca = result_troca + troca[i];
        }
        if(result_troca > 0){
            ordenado = 0;
            //printf("\n VOU ORDENAR MAIS!%d", ordenado);

        }else{
            ordenado = 1;
            //printf("\n ACABEI %d", ordenado);
        }


        /* Processo de Troca */

        if(troca[my_rank] == 1 && my_rank > 0){
            //printf(" \n DEBUG Proc1 Troca Pid: %d : ", my_rank);
            for(i = 0; i < (tam_vet_pedaco/pedaco_troca);i++){
                aux[i] = vet_aux[i];
            }
            MPI_Send(&aux, (tam_vet_pedaco/pedaco_troca), MPI_INT, (my_rank-1), 1, MPI_COMM_WORLD); 
            MPI_Recv(&aux1, (tam_vet_pedaco/pedaco_troca), MPI_INT, (my_rank-1), MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            for(i = 0; i < (tam_vet_pedaco/pedaco_troca);i++){
                vet_aux[i] = aux1[i];
            }
            /* DEBUG VETOR FINAL do PROCESSO */
            //printf(" \n DEBUG TROCOU Pid: %d : ", my_rank);
            /*for(i = 0; i < tam_vet_pedaco; i++){
                printf(" %d, ", vet_aux[i]);
            }
            printf(" \n");*/
        }

        if(troca[my_rank + 1] == 1 && my_rank < (np-1)){
            //printf(" \n DEBUG Proc2 Troca Pid: %d : ", my_rank);
            MPI_Recv(&aux, (tam_vet_pedaco/pedaco_troca), MPI_INT, (my_rank+1), MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            k = 0;
            for(i = tam_vet_pedaco - (tam_vet_pedaco/pedaco_troca); i < tam_vet_pedaco;i++){
                aux1[k] = vet_aux[i];
                k++;
            }
            k = 0;
            for(i = tam_vet_pedaco - (tam_vet_pedaco/pedaco_troca); i < tam_vet_pedaco;i++){
                vet_aux[i] = aux[k];
                k++;
            }
            MPI_Send(&aux1, (tam_vet_pedaco/pedaco_troca), MPI_INT, (my_rank+1), 1, MPI_COMM_WORLD); 
        }

    } // Fim While
     
  // processo mensagem

  /* DEBUG VETOR FINAL do PROCESSO */
  if(my_rank == 0) {
    printf(" \n VETOR FINAL Pid: %d : ", my_rank);
    for(i = 0; i < tam_vet_pedaco; i++){
        printf(" %d, ", vet_aux[i]); 
    }
    printf(" \n");
  }

  MPI_Finalize();
}