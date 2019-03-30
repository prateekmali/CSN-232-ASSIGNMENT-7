#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <errno.h>   
#include <unistd.h>

sem_t a_ready;

sem_t smoker_sema[3];

char* smoker_types[3] = { "matches & tobacco", "matches & paper", "tobacco & paper" };

bool items_on_table[3] = { false, false, false };

sem_t pusher_sema[3];

void* smoker(void* arg)
{
	int smoker_id = *(int*) arg;
	int type_id   = smoker_id % 3;

	for (int i = 0; i < 3; ++i)
	{
		printf("\033[0;37mSmoker %d \033[0;31m>>\033[0m Waiting for %s\n",
			smoker_id, smoker_types[type_id]);

		sem_wait(&smoker_sema[type_id]);

		printf("\033[0;37mSmoker %d \033[0;32m<<\033[0m Now making the a cigarette\n", smoker_id);
		usleep(rand() % 50000);
		sem_post(&a_ready);

		printf("\033[0;37mSmoker %d \033[0;37m--\033[0m Now smoking\n", smoker_id);
		usleep(rand() % 50000);
	}

	return NULL;
}

sem_t pusher_lock;

void* pusher(void* arg)
{
	int pusher_id = *(int*) arg;

	for (int i = 0; i < 12; ++i)
	{
		sem_wait(&pusher_sema[pusher_id]);
		sem_wait(&pusher_lock);

		if (items_on_table[(pusher_id + 1) % 3])
		{
			items_on_table[(pusher_id + 1) % 3] = false;
			sem_post(&smoker_sema[(pusher_id + 2) % 3]);
		}
		else if (items_on_table[(pusher_id + 2) % 3])
		{
			items_on_table[(pusher_id + 2) % 3] = false;
			sem_post(&smoker_sema[(pusher_id + 1) % 3]);
		}
		else
		{
			items_on_table[pusher_id] = true;
		}

		sem_post(&pusher_lock);
	}

	return NULL;
}

void* agent(void* arg)
{
	int agent_id = *(int*) arg;

	for (int i = 0; i < 6; ++i)
	{
		usleep(rand() % 200000);

		sem_wait(&a_ready);

		sem_post(&pusher_sema[agent_id]);
		sem_post(&pusher_sema[(agent_id + 1) % 3]);

		printf("\033[0;35m==> \033[0;33mAgent %d giving out %s\033[0;0m\n",
			agent_id, smoker_types[(agent_id + 2) % 3]);
	}

	return NULL;
}

int main(int argc, char* arvg[])
{
	srand(time(NULL));

	sem_init(&a_ready, 0, 1);

	sem_init(&pusher_lock, 0, 1);

	for (int i = 0; i < 3; ++i)
	{
		sem_init(&smoker_sema[i], 0, 0);
		sem_init(&pusher_sema[i], 0, 0);
	}


	int smoker_ids[6];

	pthread_t smoker_threads[6];

	for (int i = 0; i < 6; ++i)
	{
		smoker_ids[i] = i;

		if (pthread_create(&smoker_threads[i], NULL, smoker, &smoker_ids[i]) == EAGAIN)
		{
			perror("Insufficient resources to create thread");
			return 0;
		}
	}

	int pusher_ids[6];

	pthread_t pusher_threads[6];

	for (int i = 0; i < 3; ++i)
	{
		pusher_ids[i] = i;

		if (pthread_create(&pusher_threads[i], NULL, pusher, &pusher_ids[i]) == EAGAIN)
		{
			perror("Insufficient resources to create thread");
			return 0;
		}
	}

	int agent_ids[6];

	pthread_t agent_threads[6];

	for (int i = 0; i < 3; ++i)
	{
		agent_ids[i] =i;

		if (pthread_create(&agent_threads[i], NULL, agent, &agent_ids[i]) == EAGAIN)
		{
			perror("Insufficient resources to create thread");
			return 0;
		}
	}

	for (int i = 0; i < 6; ++i)
	{
		pthread_join(smoker_threads[i], NULL);
	}

	return 0;
}
