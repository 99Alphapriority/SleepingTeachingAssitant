#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

void* studentFunc( void* student_id );
void* taFunc();

#define NUM_CHAIRS 3
#define DEFAULT_NUM_STUDENTS 5

sem_t sem_students;
sem_t sem_ta;
pthread_mutex_t mutex_thread;

int waiting_room_chairs[3];
int num_students_waiting = 0;
int nextEmptyChair = 0;
int currentStudentChair = 0;
int isTASleeping = 0;

int isWaiting(int student_id);

int main( int argc, char **argv )
{
	int i;
	int student_num;

	if (argc > 1 ) 
	{
		student_num = atoi( argv[1] );
	}
	else 
	{
		student_num = DEFAULT_NUM_STUDENTS;
	}

	int studentIDs[student_num];
	pthread_t students[student_num];
	pthread_t ta;

	sem_init( &sem_students, 0, 0 );
	sem_init( &sem_ta, 0, 1 );

	//Create mutex and threads
	pthread_mutex_init( &mutex_thread, NULL );

	pthread_create( &ta, NULL, taFunc, NULL );

	for( i=0; i<student_num; i++ )
	{
		studentIDs[i] = i + 1;
		pthread_create( &students[i], NULL, studentFunc, (void*) &studentIDs[i] );
	}

	//Wait for threads to terminate
	for( i=0; i < student_num; i++ )
	{
		pthread_join( students[i],NULL );
	}

	pthread_cancel(ta);

	pthread_join(ta, NULL);


	return 0;
}

void* taFunc() {

	printf( "Checking for students.\n" );

	while(1) {
		pthread_testcancel();
		//students are waiting
		if ( num_students_waiting > 0 ) {

			isTASleeping = 0;
			sem_wait( &sem_students );
			pthread_mutex_lock( &mutex_thread );

			int helpTime = (rand() % 10) + 1;

			//TA helping student.
			printf( "Helping a student for %d seconds. Students waiting for help = %d.\n", helpTime, --num_students_waiting);
			printf( "Student %d receiving help.\n",waiting_room_chairs[currentStudentChair] );

			waiting_room_chairs[currentStudentChair]=0;
			currentStudentChair = (currentStudentChair + 1) % NUM_CHAIRS;

			sleep( helpTime );

			pthread_mutex_unlock( &mutex_thread );
			sem_post( &sem_ta );

		}
		//no students are waiting
		else 
		{
			if ( isTASleeping == 0 ) 
			{
				printf( "No students waiting. TA going to Sleep.\n" );
				isTASleeping = 1;
			}
		}
	}
}

void* studentFunc( void* student_id ) {

	int id_student = *(int*)student_id;

	while(1) 
	{
		//if student is waiting, continue waiting
		if (isWaiting(id_student) == 1) 
			continue;

		pthread_mutex_lock( &mutex_thread );

		if( num_students_waiting < NUM_CHAIRS ) 
		{

			waiting_room_chairs[nextEmptyChair] = id_student;
			num_students_waiting++;

			//student takes a seat in the hallway.
			printf( "\tStudent %d takes a seat. Students waiting = %d.\n", id_student, num_students_waiting );
			nextEmptyChair = ( nextEmptyChair + 1 ) % NUM_CHAIRS;

			pthread_mutex_unlock( &mutex_thread );

			//wake TA if sleeping
			sem_post( &sem_students );
			sem_wait( &sem_ta );
			pthread_exit(0);

		}
		else 
		{
			pthread_mutex_unlock( &mutex_thread );
			//No chairs available. Student will try later.
			int time = (rand() % 10)+1;
			printf( "\tStudent %d is programming for %d seconds.\n", id_student, time );
			printf( "\tStudent %d will try later.\n",id_student );
			sleep(time);
		}
	}
}

int isWaiting( int student_id ) {
	int i;
	for ( i = 0; i < 3; i++ ) {
		if ( waiting_room_chairs[i] == student_id ) { return 1; }
	}
	return 0;
}
