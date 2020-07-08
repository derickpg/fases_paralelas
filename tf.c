#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <unistd.h>


main(int argc, char** argv) {
        
    int tam_vet = 1000000;   /* Tamanho TOTAL do VETOR  ---- Multiplo da quantidade de processos <- LEMBRAR ! */
    int debug = 1;


    int my_rank;        /* Identificador do processo */
    int np;             /* Número de processos */
    int source;         /* Identificador do proc.origem */
    int dest;           /* Identificador do proc. destino */
    int* vetor;
    int i;
    double ti,tf;
    int tam_part;       /* Tamanho das Partes do vetor */
    int ini_vetor;
    int fim_vetor;
    int recebido;
    int pronto = 0; // 0 = false e 1 = true
    int flag_erro = 0; /* 0 - sem erros ; 1 - com erros (Flag que indica que se o processo teve um erro de ordenação com o vizinho )*/
    int flag_erro_proprio = 0;
    int flag_acabou_de_ordenar = 0;
    int primeira_vez = 0;
    int positv = 0, negatv = 0;
            
    MPI_Status status;  /* Status de retorno */

    /* TAGS */
    int tag_inicio      = 1; //quando envia o intervalo dos arrays para todos os processos
    int tag_maior       = 2;
    int tag_ordenado    = 3;
    int tag_feedback    = 4;
    int tag_erro        = 5;
    int tag_broadcast   = 6;
    int tag_fim         = 9;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    tam_part = tam_vet/np; //cada array eh fragmentado em TamanhoArray / numProcessos
    vetor = malloc(sizeof *vetor * tam_vet);

    if (my_rank == 0){

        for (i=0 ; i<tam_vet; i++)
            vetor[i] = tam_vet-i;

        //https://mpitutorial.com/tutorials/mpi-broadcast-and-collective-communication/
        //MPI_Bcast(vetor, tam_vet, MPI_INT, 0, MPI_COMM_WORLD);

        for (i = 1; i < np; i++)
            MPI_Send(vetor,tam_vet,MPI_INT,i,tag_broadcast,MPI_COMM_WORLD);

        // Começa a conta o tempo
        ti = MPI_Wtime();
    }

    if(my_rank != 0)
        MPI_Recv(vetor, tam_vet, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    fim_vetor = ((my_rank + 1) * tam_part) -1;
    ini_vetor = (fim_vetor+1) - tam_part;

    MPI_Barrier(MPI_COMM_WORLD);
    if(debug == 1) printf(" pid = %d esta com array completo, seus limites sao ini= %d e fim= %d, mas pode ver todo, exemplo? 51 = %d,\n", my_rank, ini_vetor, fim_vetor, vetor[51]);

    pronto = 0;
    
    MPI_Barrier(MPI_COMM_WORLD);

    while (pronto == 0) {
        //na primeira rodada ninguem recebe nada.
        if(primeira_vez != 0 && flag_erro_proprio == 0)
            MPI_Recv(&recebido, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if((status.MPI_TAG == tag_inicio) || (status.MPI_TAG == tag_erro && flag_acabou_de_ordenar == 0) || flag_erro_proprio == 1 || primeira_vez == 0){
            if(debug == 1) printf(" DEBUG [ %d ] TAG_INICIO \n", my_rank);\
            if(status.MPI_TAG == tag_erro )
            
            primeira_vez = 1;

            // ordeno vetor local
            for (i = ini_vetor; i <= fim_vetor; i++) {
                int i_bs, troca;
                for (i_bs = i+1; i_bs <= fim_vetor; i_bs++) {
                    if (vetor[i] > vetor[i_bs]) {
                        troca       = vetor[i];
                        vetor[i]    = vetor[i_bs];
                        vetor[i_bs] = troca;
                    }
                }
            }
            if(flag_erro_proprio == 1) flag_acabou_de_ordenar = 1;
            flag_erro_proprio = 0; //ja se ordenou
            // verifico condição de parada ???

            // se não for np-1, mando o meu maior elemento para a direita
            if(my_rank < (np-1)) // Manda o último número...
                MPI_Send(&vetor[fim_vetor],1,MPI_INT,(my_rank+1),tag_maior,MPI_COMM_WORLD);
            if(debug == 1) printf(" DEBUG [ %d ] TAG_INICIO-fim \n", my_rank);
        } else if(status.MPI_TAG == tag_erro && flag_acabou_de_ordenar == 1) {
            flag_acabou_de_ordenar = 0;
        } else if(status.MPI_TAG == tag_maior) {
            if(debug == 1) printf(" DEBUG [ %d ] TAG_MAIOR \n", my_rank);
            // se não for 0, recebo o maior elemento da esquerda
            // comparo se o meu menor elemento é maior do que o maior elemento recebido (se sim, estou ordenado em relação ao meu vizinho)
            // compartilho o meu estado com todos os processos
            // se todos estiverem ordenados com seus vizinhos, a ordenação do vetor global está pronta ( pronto = TRUE, break)
            if(my_rank > 0){   
                flag_erro = 0;
                if(recebido < vetor[ini_vetor]) {
                    // Está ordenado com o da esquerda, manda broadcast positivo
                    //MPI_Bcast(1, 1, MPI_INT, tag_feedback, MPI_COMM_WORLD);
                    int feedb = 1;
                    for (i = 0; i < np; i++)
                        if(i != my_rank)
                            MPI_Send(&feedb,1,MPI_INT,i,tag_feedback,MPI_COMM_WORLD);
                } else {
                    // Nao esta ordenado com o da esquerda, manda broadcast negativo
                    //MPI_Bcast(2, 1, MPI_INT, tag_feedback, MPI_COMM_WORLD);
                    int feedb = 2;
                    for (i = 0; i < np; i++)
                        if(i != my_rank)
                            MPI_Send(&feedb,1,MPI_INT,i,tag_feedback,MPI_COMM_WORLD);
                    flag_erro = 1;
                }
                sleep(1);
            }
            if(debug == 1) printf(" DEBUG [ %d ] TAG_MAIOR-fim \n", my_rank);
        }else if(status.MPI_TAG == tag_feedback){
            if(debug == 1) printf(" DEBUG [ %d ] TAG_FEEDBACK \n", my_rank);
            if(recebido == 1) positv++;
            else negatv++;

            if((my_rank == 0 && (negatv + positv) == np-1) || (my_rank != 0 && (negatv + positv) == np-2)){ // Se todas as mensagens CHEGARAM !
                if(negatv > 0 && my_rank != 0){
                    if(flag_erro == 1) {
                        // entao teve um erro
                        // Troca pedaço do vetor com o vizinho
                        int i_aux;
                        int aux_lim_vizinho, aux_meu_lim;
                        aux_meu_lim = ((ini_vetor) + (tam_part/10));
                        aux_lim_vizinho = ((ini_vetor - 1) - (tam_part/10));
                        int vet_aux[(tam_part/10)];
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
                        int feedb = 1;
                        if(debug == 1) printf(" DEBUG p= %d alertou para %d reordenar  \n", my_rank, (my_rank-1));
                        MPI_Send(&feedb,1,MPI_INT,(my_rank-1), tag_erro, MPI_COMM_WORLD);
                        flag_erro_proprio = 1;
                        if(debug == 1) printf(" DEBUG p= %d alertou!!!!  \n", my_rank);
                    }
                }else{
                    // tudo certo pode terminar!
                    if(negatv == 0) {
                        pronto = 1;
                        if(my_rank != 0){
                            MPI_Finalize();
                            return 0;
                        }
                    }
                }
                positv = 0, negatv = 0;
            }
            if(debug == 1) printf(" DEBUG [ %d ] TAG_FEEDBACK-fim \n", my_rank);
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