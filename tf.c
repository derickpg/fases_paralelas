#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"


main(int argc, char** argv) {
    
    printf("♥♥♥ LA NO COMECAO! \n");
    
    int tam_vet = 10000;   /* Tamanho TOTAL do VETOR  ---- Multiplo da quantidade de processos <- LEMBRAR ! */


    int my_rank;        /* Identificador do processo */
    int np;             /* Número de processos */
    int source;         /* Identificador do proc.origem */
    int dest;           /* Identificador do proc. destino */
    int *vetor;
    int i;
    double ti,tf;
    int tam_part;       /* Tamanho das Partes do vetor */
    int ini_vetor;
    int fim_vetor;
    int ordenado = 0;   /* 0 = nao ordenado  - 1 = ordenado */
    int recebido;
    int pronto = 0; 
    int flag_erro = 0; /* 0 - sem erros ; 1 - com erros (Flag que indica que se o processo teve um erro de ordenação com o vizinho )*/
    MPI_Status status;  /* Status de retorno */

    /* TAGS */
    int tag_inicio      = 1; //quando envia o intervalo dos arrays para todos os processos
    int tag_maior       = 2;
    int tag_ordenado    = 3;
    int tag_feedback    = 4;
    int tag_erro        = 5;
    int tag_fim         = 9;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    tam_part = tam_vet/np; //cada array eh fragmentado em TamanhoArray / numProcessos

    printf("♥♥♥ aNTES DO MY 0 \n");
    if (my_rank == 0){  
        printf("Comecando a ordenacao! \n");
        vetor = malloc(tam_vet*sizeof(int));

        for (i=0 ; i<tam_vet; i++)
            vetor[i] = tam_vet-i;

        printf("♥♥♥ aNTES DO broadcast \n");

        //https://mpitutorial.com/tutorials/mpi-broadcast-and-collective-communication/
        //exemplo: MPI_Bcast(data, num_elements, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(vetor, tam_vet, MPI_INT, 0, MPI_COMM_WORLD);

        printf("♥♥♥ depois DO broadcast \n");
        // Começa a conta o tempo
        ti = MPI_Wtime();
    }

    //if(my_rank != 0)
    //    MPI_Recv(vetor, tam_vet, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);


    fim_vetor = ((my_rank + 1) * tam_part) -1;
    ini_vetor = (fim_vetor+1) - tam_part;

    MPI_Barrier(MPI_COMM_WORLD);

    printf("♥♥♥ pid = %d recebeu array, ini= %d  e fim= %d \n", my_rank, ini_vetor, fim_vetor);

    MPI_Finalize();
    return 0;
}