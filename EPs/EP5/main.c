/* -----------------------------------------------------------------------------
 * Title:       main
 * File:        main.c
 * Author:      Gabriel Crabbé
 * Version:     0.0 (2017-06-21)
 * Date:        2017-06-21
 * Description: Exercício 5 de PSI2653.
 * -----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "lumiar.h"


/**
 * Main thread.
 */
int main(void)
{
	/* Parse configuration file */
	struct lumiarConfig sconf;
	initConfig(&sconf);
	parseConfig(&sconf);

	/* Webserver driver */
	struct lumiarState lstate;
	struct lumiarState ldriver;
	struct webDriver wdriver;
	initDriver(&wdriver, &ldriver, &sconf.web);
	//wiringPiSetup() ;

	/* Create threads */
	pthread_t pwmThread, ldrThread, webThread;
	printf("Launching services\n");
	pthread_create(&webThread, NULL, webService, (void *) &wdriver);
	/* UNCOMMENT LINES BELOW FOR DEPLOYMENT */
	 pthread_create(&ldrThread, NULL, ldrService, (void *) &sconf.ldr);
	 pthread_create(&pwmThread, NULL, pwmService, (void *) &sconf.pwm);

	/* Local flags and variables */
	int hasChanged = 0;
	int readLuminosity;
	const struct timespec slp = {0, 100000000L};  /* 100 ms */

	/* Main loop */
	for(;;)
	{
		/* Get request from server */
		sem_wait(&wdriver.mutex);
		if(memcmp(&lstate, wdriver.current, sizeof(lstate)))
		{	
			printf("b1");
			memcpy(&lstate, wdriver.current, sizeof(lstate));
			sem_post(&wdriver.mutex);
			hasChanged = 1;
		}
		else
		{
			sem_post(&wdriver.mutex);
			hasChanged = 0;
		}

		/* Get luminosity */
		/* UNCOMMENT LINE BELOW FOR DEPLOYMENT */
		readLuminosity = getLuminosity();
		/* DELETE LINE BELOW FOR DEPLOYMENT */
		//readLuminosity = (int) time(NULL) % 101;
		if(lstate.luminosity != readLuminosity)
		{
			lstate.luminosity = readLuminosity;
			hasChanged = 1;
		}

		/* Send command */
		if(hasChanged)
		{	
			printf("b2");
			if(lstate.state == LUMIAR_STATE_ON)
			{
				if(lstate.mode == LUMIAR_MODE_AUTO)
					lstate.pwmValue = LUMIAR_VALUE_MAX - readLuminosity;
				else
					lstate.pwmValue = lstate.userValue;
			}
			else
			{
				lstate.pwmValue = LUMIAR_VALUE_MIN;
			}

			/* UNCOMMENT LINE BELOW FOR DEPLOYMENT */
			setOperatingPoint(lstate.pwmValue,&sconf.pwm);
		}

		/* Send back to server */
		if(hasChanged)
		{
			sem_wait(&wdriver.mutex);
			memcpy(wdriver.current, &lstate, sizeof(lstate));
			sem_post(&wdriver.mutex);
		}

		/* Sleep */
		//nanosleep(&slp, (struct timespec *) NULL);
		sleep(1);
	}

	return 0;
}
