#include <stdio.h>
#include "mpi.h"


main(int argc, char** argv)
{ 
    /* Parametros */
    int tam_vet = 50;   /* Tamanho TOTAL do VETOR  ---- Multiplo da quantidade de processos <- LEMBRAR ! */

    /* Variaveis */
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
    int tag_inicio      = 1;
    int tag_maior       = 2;
    int tag_ordenado    = 3;
    int tag_feedback    = 4;
    int tag_erro        = 5;
    int tag_fim         = 9;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    tam_part = tam_vet/np;

    if (my_rank == 0){  
        vetor = malloc(tam_vet*sizeof(int));

        for (i=0 ; i<tam_vet; i++)
            vetor[i] = tam_vet-i;

        MPI_Bcast(&vetor, tam_vet, MPI_INT, tag_inicio, MPI_COMM_WORLD);

        // Começa a conta o tempo
        ti = MPI_Wtime();
    }

    // Processo de Ordenação

    if(my_rank != 0)
        MPI_Recv(vetor, tam_vet, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    fim_vetor = ((my_rank + 1) * tam_part) -1;
    ini_vetor = (fim_vetor+1) - tam_part;

    while(ordenado == 0){
        
        MPI_Recv(recebido, 1, MPI_INT, my_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if((status.MPI_TAG == tag_inicio) || (status.MPI_TAG = tag_erro) || (flag_erro == 1)){

            // BS 
            int c=ini_vetor, d, troca, trocou =1;
            int n = fim_vetor;
            while (c < (n-1) & trocou )
            {
                trocou = 0;
                for (d = ini_vetor ; d < n - c - 1; d++)
                    if (vetor[d] > vetor[d+1])
                        {
                        troca      = vetor[d];
                        vetor[d]   = vetor[d+1];
                        vetor[d+1] = troca;
                        trocou = 1;
                        }
                c++;
            }
        
            if(my_rank < (np-1)) // Manda o último número... 
                MPI_Send(vetor[fim_vetor],1,MPI_INT,(my_rank+1),tag_maior,MPI_COMM_WORLD);

        }else if(status.MPI_TAG == tag_maior){

            if(my_rank > 0){   
                flag_erro = 0;
                if(recebido < vetor[ini_vetor]) {

                    // Está ordenado com o da esquerda, manda broadcast positivo
                    MPI_Bcast(1, 1, MPI_INT, tag_feedback, MPI_COMM_WORLD);
                } else {
                    // Nao esta ordenado com o da esquerda, manda broadcast negativo
                    MPI_Bcast(2, 1, MPI_INT, tag_feedback, MPI_COMM_WORLD);
                    flag_erro = 1;
                }
            }
        }else if(status.MPI_TAG == tag_feedback){
            int positv = 0, negatv = 0;
            if(recebido == 1) positv++;
            else negatv++;
            
            if(((negatv + positv) == np)){ // Se todas as mensagens CHEGARAM !
                if(negatv > 0){
                    // entao teve um erro
                    // Troca pedaço do vetor com o vizinho
                    int i_aux;
                    int aux_lim_vizinho, aux_meu_lim;
                    aux_meu_lim = ((ini_vetor) + (tam_part/10));
                    aux_lim_vizinho = ((ini_vetor - 1) - (tam_part/10));
                    int[(tam_part/10)] vet_aux;
                    // Faco minha copia
                    for(i = ini_vetor; i < aux_meu_lim; i++)
                        vet_aux[i] = vetor[i];
                    // Pego os dados do vizinhos (my_rank - 1)
                    i_aux = ini_vetor;
                    for(i = aux_lim_vizinho; i < (ini_vetor); i++){
                        vetor[i_aux] = vetor[i];
                        i_aux++;
                    }
                    // Manda meus dados de Copia para o vizinho
                    i_aux = aux_lim_vizinho;
                    for(i = 0; i < (tam_part/10);i++){
                        vetor[i_aux] = vet_aux[i];
                        i_aux++;
                    }
                    // Avisa o vizinho que ele tem que ordenar novamente por que deu erro
                    MPI_Send(1,1,MPI_INT,(my_rank -1), tag_erro, MPI_COMM_WORLD);
                }else{
                    // tudo certo pode terminar!
                    ordenado = 1;
                    if(my_rank != 0){
                        MPI_Finalize();
                        return 0;
                    }
                }
            }
        }
    }


    if(my_rank == 0){
        // Fim do Tempo
		tf = MPI_Wtime();
		double total_time;
		total_time = tf - ti;
		printf("\n TEMPO TOTAL = %f \n", total_time);
        MPI_Finalize();
        return 0;
    }
    
    MPI_Finalize();
    return 0;
}