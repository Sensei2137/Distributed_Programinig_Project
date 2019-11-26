#include "mpi.h"
#include <stdio.h>
#include <time.h>  
#include <pthread.h>
#define MSG_SIZE 3
#define NUMBER_OF_HELIPADS 3
#define NUMBER_OF_HANGARS 5
#define NUMBER_OF_HELICOPTERS 8

#define Hangar_Request_Msg 1
#define Hangar_Release_Msg 0

#define Landing_Request_Msg 3
#define Landing_Release_Msg 2


pthread_mutex_t hangar_lock;
pthread_mutex_t landing_lock;
pthread_cond_t hangar_condition;
pthread_cond_t landing_condition;


typedef struct message{
	int rank;
	double time;
};
//struktury kolejkujace procesy
struct message landing_pad_queue[NUMBER_OF_HELICOPTERS];
struct message hangar_queue[NUMBER_OF_HELICOPTERS];


//Inicjalizacja kolejek
void init_queues()
{
    struct message msg;
    msg.time = 0;
    msg.rank = -1;
    for(int i = 0; i < NUMBER_OF_HELICOPTERS ; i++)
    {
        hangar_queue[i] = msg;
        landing_pad_queue[i] = msg;
    }
    
}
//Wstawianie zadania do kolejki
void insert_msg(struct message *tab, struct message msg)
{
    for(int i = 0; i < NUMBER_OF_HELICOPTERS ; i++)
    {
        if (tab[i].rank == -1)
        {
            tab[i] = msg;
            break;
        }
        
        
        if(tab[i].time == msg.time && tab[i].rank > msg.rank || tab[i].time > msg.time) 
        {
            for(int j = NUMBER_OF_HELICOPTERS -1; j > i ; j--)
                tab[j] = tab[j-1];
            tab[i] = msg;
            break;
        }
            
            
    }
}
//Szukanie komunikatu w kolejce
int find_msg(struct message *tab, struct message msg)
{
    for(int i=0 ; i < NUMBER_OF_HELICOPTERS ; i++)
    {
        if(tab[i].rank == msg.rank && tab[i].time == msg.time)
        {
            return i;
        }
    }
    return -1;
}
//Szukanie jedynie po ranku - miejsce od 1
int find_rank_msg(struct message *tab, int rank)
{
    for(int i=0 ; i < NUMBER_OF_HELICOPTERS ; i++)
    {
        if(tab[i].rank == rank)
        {
            return i;
        }
    }
    return -1;
}
//Wypisywanie kolejki
void print_queue(struct message *tab)
{
    for (int i = 0; i < NUMBER_OF_HELICOPTERS ; i++)
    {
        printf("%d: %d, %f\n",i, tab[i].rank, tab[i].time);
    }
}
//Usuwanie z kolejki - zmienione na usuwanie jedynie po ranku
void delete_msg(struct message *tab, int rank)
{
    //int i = find_msg(tab, msg);
    int i = find_rank_msg(tab, rank);
    if(i != -1)
    {
        for(i ; i < NUMBER_OF_HELICOPTERS - 1 ; ++i)
            tab[i] = tab[i+1];
        struct message msg = { -1, 0};
        tab[NUMBER_OF_HELICOPTERS - 1] = msg; 
    }
}

//Symulacja lotu oraz postoju - wykorzystanie losowego czasu
void sekcja(int n){

srand (time(NULL));
int delay = rand() % 10 + 1;

if(n==1){
	printf("Sekcja lokalna\n"); 
   
    	sleep(delay);                 
    	printf("Koniec sekcji lokalnej\n"); 
	}
else{
	printf("Sekcja krytyczna\n"); 
   	 
    	sleep(delay);                 
    	printf("Koniec sekcji krytycznej\n"); 
	}

}
//Odbior wiadomosci przez watek| 0-usuniecie z hangar_queue,1-wstawienie do hangar_queue,2-usuniecie z landing_queue,3-wstawienie do landing_queue
void *recive(void *arg) 
{
	MPI_Status status;
    int type;
	struct message msg2;
	double msg[MSG_SIZE];
	while(1)
	{
			MPI_Recv( msg, MSG_SIZE, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			//printf("%d\n",status.MPI_TAG);
			//printf("%f\n",msg[2]);
			type = status.MPI_TAG;
			switch(type)
				{
				case 0:
					pthread_mutex_lock(&hangar_lock);
					delete_msg(hangar_queue,(int) msg[0]);
					pthread_cond_broadcast(&hangar_condition);
					pthread_mutex_unlock(&hangar_lock);
					break;
				case 1:
					pthread_mutex_lock(&hangar_lock);
					msg2.rank = msg[0];	
					msg2.time = msg[1];
					insert_msg(hangar_queue,msg2);
					pthread_cond_broadcast(&hangar_condition);
					pthread_mutex_unlock(&hangar_lock);
					break;
				case 2:
					pthread_mutex_lock(&landing_lock);
					delete_msg(landing_pad_queue,(int) msg[0]);
					pthread_cond_broadcast(&landing_condition);
					pthread_mutex_unlock(&landing_lock);
					break;
				case 3:
					pthread_mutex_lock(&landing_lock);
					msg2.rank = msg[0];	
					msg2.time = msg[1];
					insert_msg(landing_pad_queue,msg2);
					pthread_cond_broadcast(&landing_condition);
					pthread_mutex_unlock(&landing_lock);
					break;
				default:
					break;
		}
	
	}
    
}


int main(int argc, char **argv)
{
//Deklaracja zmiennych
	int rank,size,sender,i;
//MPI things
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
//Deklaracja wiadomosci - zdefiniowany rozmiar 2 (rank, time)
	double msg[MSG_SIZE];
	double msg_del[MSG_SIZE];
//Inicjalizacja kolejek
	init_queues();
//Uruchomienie watku odbierania
	pthread_t reciver_thread;
 	pthread_mutex_init(&hangar_lock, NULL);
 	pthread_mutex_init(&landing_lock, NULL);
	pthread_cond_init(&hangar_condition, NULL); 
	pthread_cond_init(&landing_condition, NULL);

//Sekcja lokalna
//	sekcja(1);
//Zgloszenie checi na ladowanie oraz szeregowanie zadan w kolejce dotyczacej miejsc w hangarze
	
	int result =	pthread_create(&reciver_thread, NULL, recive, (void *) argv[0]);

		msg[0] = rank;
		msg[2] = 1;
	while(1){
		
		sleep(rank);
		/*
		lot
		*/


		//utworzenie wiadomosci
		msg[1] = MPI_Wtime();
		for(i = 0; i< NUMBER_OF_HELICOPTERS; ++i){ //wysłanie wiadomości zadania hangaru
			MPI_Send(msg, MSG_SIZE, MPI_DOUBLE, i, Hangar_Request_Msg, MPI_COMM_WORLD );

		}
		

		// //debug
		// pthread_mutex_lock(&hangar_lock);
		// if(rank == 1)
		// 	print_queue(hangar_queue);
		// pthread_mutex_unlock(&hangar_lock);
		sleep(1);

		//sprawdzenie czy mogę wejsc do sekcji krytycznej
		pthread_mutex_lock(&hangar_lock);
		while(find_rank_msg(hangar_queue,rank) >= NUMBER_OF_HANGARS){  //critical section check
			pthread_cond_wait(&hangar_condition, &hangar_lock);
		}
		pthread_mutex_unlock(&hangar_lock);

		//wyslanie zadania pola startowego
		msg[1] = MPI_Wtime();
		for(i = 0; i< NUMBER_OF_HELICOPTERS; ++i){ //wysłanie wiadomości zadania miejsca do ladowania
			MPI_Send(msg, MSG_SIZE, MPI_DOUBLE, i, Landing_Request_Msg, MPI_COMM_WORLD );

		}

		sleep(1);

		//sprawdzenie czy mogę wyladowac
		pthread_mutex_unlock(&landing_lock);
		while(find_rank_msg(landing_pad_queue,rank) >= NUMBER_OF_HELIPADS){  //critical section check
			pthread_cond_wait(&landing_condition, &landing_lock);
		}
		pthread_mutex_unlock(&landing_lock);

		printf("Helikopter %d laduje\n",rank);

		//wyslanie wiadomosci o zwolnieniu pola startowego
		for(i = 0; i< NUMBER_OF_HELICOPTERS; ++i){ //hangar release

			MPI_Send(msg, MSG_SIZE, MPI_DOUBLE, i, Landing_Release_Msg, MPI_COMM_WORLD);

		}
		sleep(1);
		printf("Helikopter %d zwalnia pole startowe\n",rank);
		printf("Helikopter %d zajmuje hangar\n",rank);

		/*
		postoj
		*/
		sleep(rank);
		

		//wyslanie zadania pola startowego
		msg[1] = MPI_Wtime();
		for(i = 0; i< NUMBER_OF_HELICOPTERS; ++i){ //wysłanie wiadomości zadania miejsca do ladowania
			MPI_Send(msg, MSG_SIZE, MPI_DOUBLE, i, Landing_Request_Msg, MPI_COMM_WORLD );

		}
		sleep(1);


		//sprawdzenie czy mogę wyladowac
		pthread_mutex_unlock(&landing_lock);
		while(find_rank_msg(landing_pad_queue,rank) >= NUMBER_OF_HELIPADS){  //critical section check
			pthread_cond_wait(&landing_condition, &landing_lock);
		}
		pthread_mutex_unlock(&landing_lock);

		printf("Helikopter %d zajmuje pole startowe\n",rank);

		//wyslanie wiadomosci o zwolnieniu miejsca w hangarze
		for(i = 0; i< NUMBER_OF_HELICOPTERS; ++i){ //hangar relase

			MPI_Send(msg, MSG_SIZE, MPI_DOUBLE, i, Hangar_Release_Msg, MPI_COMM_WORLD );

		}

		printf("Helikopter %d zwalnia hangar\n",rank);
		sleep(1); //przejazd na pole startowe
		

		//wyslanie wiadomosci o zwolnieniu miejsca postojowego
		for(i = 0; i< NUMBER_OF_HELICOPTERS; ++i){ //hangar release

			MPI_Send(msg, MSG_SIZE, MPI_DOUBLE, i, Landing_Release_Msg, MPI_COMM_WORLD);

		}

		printf("Helikopter %d startuje\n",rank);
		sleep(1); //start

		// //debug
		// pthread_mutex_lock(&hangar_lock); 
		// if(rank == 1)
		// 	print_queue(hangar_queue);
		// pthread_mutex_unlock(&hangar_lock);


	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
}
