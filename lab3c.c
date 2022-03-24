#include <sys/mman.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>


int* board[9];
int board_data[81];

bool column();

void itoa();

void* column_wrapper(void* args)
{
	char* msg = column() ? "La solución validada por columnas es correcta\n" 
	: "La solución validada por columnas es incorrecta\n";
	printf(msg);
	pthread_exit(0);
}


bool check_flags(bool* flags)
{
	for(int i = 0; i < 9; i++)
		if(!flags[i])
			return false;
	return true;
}

bool column()
{
	bool flags[10];
	omp_set_num_threads(9);
	omp_set_nested(true);
	
	#pragma omp parallel for //schedule(dynamic)
	for (int i = 0; i < 9; i++)
	{
		printf("Thread de column(): %d\n", (int)syscall(SYS_gettid));
		for(int j = 0; j < 9; j++)
			flags[board[j][i]] = true;
	}

	return check_flags(&flags[1]);
}


bool row()
{
	bool flags[10];
	omp_set_num_threads(9);
	
	omp_set_nested(true);
	#pragma omp parallel for //schedule(dynamic)
	for(int j = 0; j < 9; j++)
		for (int i = 0; i < 9; i++)
			flags[board[j][i]] = true;

	return check_flags(&flags[1]);
}

bool subarray(int i_, int j_)
{
	bool flags[10];

	for(int i = i_; i < i_ + 3; i++)
		for(int j = j_; j < j_ + 3; j++)
			flags[board[j][i]] = true;


	return check_flags(&flags[1]);
}

int main(int argc, char** argv)
{
	omp_set_num_threads(1);
	if (argc != 2)
		return -1;
	int i = 0;
	int fd = open(argv[1], O_RDONLY);
	if (fd == -1)
		return -1;
	//board_data = malloc(sizeof(char)*128);

	char rd_buff;
	while (read(fd, &rd_buff, sizeof(rd_buff)))
	{
		if(rd_buff == 0)
			break;
		board_data[i] = rd_buff - 48;		// -48 to convert char single digit numbers into int single digit numbers
		i++;
	}


	//mapear un arreglo de dos dimensiones a los datos del arreglo de una hacer esto asegura 
	//que toda la memoria sea alocada contiguamente
	//board = malloc(sizeof(int*) * 16);
	for(i = 0; i < 9; i++)
		board[i] = &board_data[i * 9];

	bool flags[9];
	int indexes[3] = {0, 3, 6};
	omp_set_nested(true);
	#pragma omp parallel for //schedule(dynamic)
	for(i = 0; i < 9; i++)
		flags[i] = subarray(indexes[i%3], indexes[i/3]);
	
	int pid = getpid();
	char process_id[33];
	itoa(pid, process_id, 10);

	if (fork()) // padre
	{
		//Compile and link with -pthread.
		pthread_t thread_id;
		pthread_create(&thread_id, 0, column_wrapper, (void*)NULL);
		pthread_join(thread_id, NULL);
		printf("Thread de main(): %d\n", (int)syscall(SYS_gettid));

		char* msg = row() ? "La solución validada por filas es correcta\n" 
		: "La solución validada por filas es incorrecta\n";
		printf(msg);
		wait(NULL);
	}
	else 	// hijo
	{
		execlp("ps", "ps", "-p", process_id, "-lLf", NULL);
	}

	if (fork()) // padre
	{
		wait(NULL);
	}
	else 	// hijo
	{
		execlp("ps", "ps", "-p", process_id, "-lLf", NULL);
	}
	
	return 0;
}	






//https://stackoverflow.com/questions/190229/where-is-the-itoa-function-in-linux
 /* reverse:  reverse string s in place */
 void reverse(char s[])
 {
     int i, j;
     char c;

     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
}  



 /* itoa:  convert n to characters in s */
 void itoa(int n, char s[])
 {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
}  