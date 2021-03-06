/*
	IMPLEMENTACION DE FUNCIONES PARA EL MANEJADOR DE HEBRAS (thread_handler.h)
*/

#include "thread_handler.h"
/*
int loadWordsOnThreads(FILE *file, WSThread * threads , int threads_count, int wordCount){

}
*/

void WSThread_init(WSThread* wsthread, int id, int rows, int cols, int words, pthread_mutex_t* mutex, char** matrix, int dflag)
{
	int i, j;

	wsthread->id = id;
	wsthread->rows = rows;
	wsthread->cols = cols;
	wsthread->wordCount = words;

	wsthread->threadMutex = malloc(rows * sizeof(*(wsthread->threadMutex)));
	for (int i = 0; i < rows; ++i)	{
		wsthread->threadMutex[i] = &(mutex[i]);
	}

	wsthread->matrix = malloc(rows * sizeof(*wsthread->matrix));
	for (i = 0; i < rows; ++i)	{
		wsthread->matrix[i] = malloc(cols * sizeof(*(wsthread->matrix[i])));
	}
	for (i = 0; i < rows; i++)	{
		for (j = 0; j < cols; j++) {
			wsthread->matrix[i][j] = &(matrix[i][j]);
		}
	}


	wsthread->posX = malloc(words * sizeof(*(wsthread->posX)));
	wsthread->posY = malloc(words * sizeof(*(wsthread->posY)));
	wsthread->words = malloc(words * sizeof(*(wsthread->words)));

	wsthread->dflag = dflag;
}

void WSThread_add_word(WSThread* wsthread, char* word, int wordIndex)
{
	wsthread->words[wordIndex] = word;
}

void WSThread_set_coordinates(WSThread* wsthread, int x, int y, int word)
{
	wsthread->posX[word] = x;
	wsthread->posY[word] = y;
}

void WSThread_free(WSThread* wsthread)
{
	int i, j;
	free(wsthread->posX);
	free(wsthread->posY);
	for (i = 0; i < wsthread->wordCount; ++i)
	{
		free(wsthread->words[i]);
	}
	free(wsthread->words);

	free(wsthread->threadMutex);
	for (int i = 0; i < wsthread->rows; ++i){
		free(wsthread->matrix[i]);
	}
	free(wsthread->matrix);
}

void * locate(void * args) {
	WSThread * t = (WSThread *) args;
	int i;

	// Inicializar semilla random
	srand(time(NULL) * t->id);

	for (i = 0; i < t->wordCount; ++i)
	{
		char* word = t->words[i];
		int lenWord = (int)strlen(word);
		// Este ciclo revisa si es que una palabra puede insertarse sin que sobreescriba una palabra
		// que ya se encuntre posicionada, en caso de que la fila se encuntre ocupada por otro hilo,
		// no se debe utilizar la posicion obtenida para evitar condiciones de carrera (el caso en que
		// se seleccione una posicion y que la palabra que entro anteriormente a la fila aun no haya
		// terminado de anotar las posiciones que ocupa, lo que causaria que erroneamente la otra
		// hebra piense que puede escribirse y termine sobreescribiendo a la hebra que entro antes).
		do {
			int rc = get_locatable_coordinates(t->rows, t->cols, &(t->posX[i]), &(t->posY[i]), lenWord);
			if(rc == 1) {
				printf("Error: no es posible posicionar todas las palabras en la matriz, por favor intente denuevo con una matriz mas grande\n");
				exit(0);
			}
			//printf("%d %d %d\n", t->id, t->posX[i], t->posY[i]);
			// Revisar si es que es posible poner esta palabra en algun lugar para que
			// no se produzca un ciclo infinito.
		} while(pthread_mutex_trylock(t->threadMutex[t->posY[i]]) != 0);
		if(t->dflag)
			printf("enterSC(): hilo-%d con la palabra: '%s' en la posicion (%d, %d) \n", t->id, word, t->posX[i], t->posY[i]);

		set_positions(t->rows, t->cols, t->posX[i], t->posY[i], lenWord);
		write_word_matrix(t->matrix, t->posX[i], t->posY[i], word);
		
		if(t->dflag)
			printf("exitSC(): hilo-%d inserto '%s' en la posicion (%d, %d)\n", t->id, word, t->posX[i], t->posY[i]);
		pthread_mutex_unlock(t->threadMutex[t->posY[i]]);
	}
}