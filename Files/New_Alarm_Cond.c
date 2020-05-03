/* Working Version WITH SEMAPHORES
 *
 * New_Alarm_Cond.c
 *
 *
 * This is an enhancement to the alarm_mutex.c program, which
 * used only a mutex to synchronize access to the shared alarm
 * list. This version adds a condition variable. The alarm
 * thread waits on this condition variable, with a timeout that
 * corresponds to the earliest timer request. If the main thread
 * enters an earlier timeout, it signals the condition variable
 * so that the alarm thread will wake up and process the earlier
 * timeout first, requeueing the later request.
 */
#include <pthread.h>
#include <time.h>
#include "errors.h"
#include <semaphore.h>


/*
 * The "alarm" structure now contains the time_t (time since the
 * Epoch, in seconds) for each alarm, so that they can be
 * sorted. Storing the requested number of seconds would not be
 * enough, since the "alarm thread" cannot tell how long it has
 * been on the list.
 */
typedef struct alarm_tag {
        struct alarm_tag  *link;
        int               seconds;
        time_t            time;                         /* seconds from EPOCH */
        char              message[128];

        // Integer used to hold the message number
        int               messageNum;
        /* Integer that keeps track of the alarm request type, either adding a
        *  new alarm (1) or cancelling an alarm from the alarm list (0)*/
        int               alarmRequestType;
        /* Integer that keeps track if an alarm has been changed (the message
        *  and/or time). 1 if it has been changed and 0 if not */
        int               changeTracker;
        /* Integer that works as a flag and is set to 1 if the alarm is new
        *  and 0 if not */
        int               newAlarmFlag;
        /* Integer that holds a value determining if the alarm exists in the
        *  alarm list (1 if it exists, 0 if not) */
        int               alarmExistsFlag;
} alarm_t;

pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t alarm_cond = PTHREAD_MUTEX_INITIALIZER;
alarm_t *alarm_list = NULL;
time_t current_alarm = 0;
// Keeps track of threads using the list
int counter;
// Maintains the head and tail of the list
alarm_t *head, *tail;

sem_t alarm_mutex1;
sem_t alarm_cond1;

// Checks if the alarm exists in the alarm list, used only for adding new alarm
int findTypeA(alarm_t *alarm)
{
        //initial the temp integer is set to zero
        int temp = 0;
        alarm_t *next;
        next = head->link;

        //While the alarm list is not empty, the alarm list is checked to see
        //if the alarm is already in the list
        while (next != tail)
        {
                //Checks if a matching alarm is in the alarm list
                if (next->alarmRequestType == 1
                  && alarm->messageNum == next->messageNum)
                {
                        //temp is change to 1 to indicate a match has been
                        //found
                        temp = 1;
                        break;
                }
                next = next->link;
        }
        //returns 1 if a matching alarm is found and 0 if not
        return temp;
}

// Checks if the alarm exists in the alarm list, used only for cancelling a
// alarm
int findTypeB(alarm_t *alarm)
{
        //initial the temp integer is set to zero
        int temp = 0;
        alarm_t *next;
        next = head->link;

        //While the alarm list is not empty, the alarm list is checked to see
        //if the alarm is already in the list
        while (next != tail)
        {
                //Checks if a matching alarm is in the alarm list
                if (next->alarmRequestType == 0
                  && alarm->messageNum == next->messageNum)
                {
                        //temp is change to 1 to indicate a match has been
                        //found
                        temp = 1;
                        break;
                }
                next = next->link;
        }
        //returns 1 if a matching alarm is found and 0 if not
        return temp;
}

//Method that finds a message with the matching message number and replaces it
//with the new time and new message
void changeAlarm(alarm_t *alarm)
{
        alarm_t *next;
        next = head->link;

        //While the alarm list is not empty, the alarm list is checked to see
        //if the alarm is already in the list
        while (next != tail)
        {
                //Checks if a matching alarm is in the alarm list
                if (next->alarmRequestType == 1
                  && alarm->messageNum == next->messageNum)
                {
                        //Copies the new message time into the old message time
                        next->seconds = alarm->seconds;
                        //Copies the "message" into the old message
                        strcpy(next->message, alarm->message);
                        //Updates the tracker, so we know there has been a
                        //change to the list
                        next->changeTracker = 1;
                        //Exits while loop
                        break;
                }
                next = next->link;
        }
}

/*
 * Insert alarm entry on list, in order of message numbers
 */
void alarm_insert (alarm_t *alarm)
{
        //The int value will contain 1 if the alarm is found and 0 if not
        int statusA = 0;
        int statusB;
        alarm_t *last, *next;

        //Checks if the alarm request is adding to the list
        if(alarm->alarmRequestType == 1)
        {
                //Checks if the alarm is found in the list
                statusA = findTypeA(alarm);
                //If the alarm is found in the list
                if(statusA)
                {
                        //change the alarm to the new alarm with the new
                        //message and time
                        changeAlarm(alarm);
                }
                //If the alarm is not found, it will be inserted into the list
                if (!statusA)
                {
                        last = head;
                        next = head->link;
                        //Loops while the list has not reached the end
                        while (next != tail)
                        {
                                //Makes sure the list is sorted correctly
                                //in order of message numbers
                                if (next->messageNum >= alarm->messageNum)
                                {
                                        //links the new message into the list
                                        alarm->link = next;
                                        last->link = alarm;
                                        //Updates the flag
                                        alarm->alarmExistsFlag = 1;
                                        //Exists the loop early
                                        break;
                                }
                                last = next;
                                next = next->link;
                        }
                        /*
                         * If we reached the end of the list, insert the new
                         * alarm there.
                         */
                        if (next == tail)
                        {
                                //Alarm is being added on list
                                alarm->link = next;
                                last->link = alarm;
                                //Flag is updated
                                alarm->alarmExistsFlag = 1;
                        }

                }
        }
        else
        {
                //Checks if the alarm is found in the list
                statusA = findTypeA(alarm);
                //If the alarm is not found
                if(!statusA)
                {
                      //Prints error message
                      printf("ERROR!!! Alarm With Message Number (%d) "
                        "Does NOT Exist\n", alarm->messageNum);
                }
                //If the alarm is found
                if(statusA)
                {
                        //Checks if alarm is found on the list
                        statusB = findTypeB(alarm);
                        //If alarm is found on the list again
                        if(statusB)
                        {
                                //prints an error message
                                printf("ERROR!!! Multiple(%d)!\n",
                                  alarm->messageNum);
                        }
                        //Otherwise the alarm is inserted onto the list
                        else
                        {
                                last = head;
                                next = head->link;
                                //As long as we are not at the end of the list
                                //keep looping
                                while (next != tail)
                                {
                                        //this if statement ensure the order
                                        //of the list is correct
                                        if (next->messageNum
                                              >= alarm->messageNum)
                                        {
                                                //adding alarm onto list
                                                alarm->link = next;
                                                last->link = alarm;
                                                //The flag is updated
                                                alarm->alarmExistsFlag = 1;
                                                break;
                                        }
                                        last = next;
                                        next = next->link;
                                }
                        }
                }
        }
}
//The periodic_display_thread is responsible for periodically looking up an
//alarm request with a specific Message Number in the alarm list, then printing,
//every Time (seconds)
void *periodic_display_thread (alarm_t *alarm)
{
        int temp = 0;
        //integer variable holding the amount of time in seconds that
        //will be used to pause the loop
        int sleepLength;

        while(1)
        {
                //aquire
                sem_wait(&alarm_mutex1);


                //iterates the counter by 1 each loop
                counter = counter + 1;

                //If the counter is 1 enter
                if (counter == 1)
                {
                        //aquire
                        sem_wait(&alarm_cond1);

                }

                //release
                sem_post(&alarm_mutex1);


                //gets the sleep length time from the alarm field seconds
                sleepLength = alarm->seconds;

                //If the alarm no longer exists
                if(alarm->alarmExistsFlag == 0)
                {
                        //inform the user the alarm no longer exisits
                        printf("DISPLAY THREAD EXITING: Message(%d)\n", alarm->messageNum);

                        //aquire
                        sem_wait(&alarm_mutex1);

                        //decremenet the counter by 1
                        counter = counter - 1;
                        //When counter is zero unlock
                        if (counter == 0)
                        {
                                //release
                                sem_post(&alarm_cond1);

                        }

                        //release
                        sem_post(&alarm_mutex1);

                        return NULL;

                }
                //Checks to see that the alarm has not been changed
                if (alarm->changeTracker == 0)
                {
                        //prints the alarm message number as well as the message
                        printf("Message(%d) %s\n", alarm->messageNum,
                          alarm->message);
                }

                //Checks to see if the alarm message has been changed
                if (alarm->changeTracker == 1 && temp)
                {
                        //if the message has been changed prints the following
                        printf("MESSAGE CHANGED: Message(%d) %s\n",
                          alarm->messageNum, alarm->message);
                }

                //Checks if there has been a change but no flag
                if (alarm->changeTracker == 1 && !temp)
                {
                        //Prints the message
                        printf("Message(%d) %s\n", alarm->messageNum,
                          alarm->message);
                        //Updates the flag
                        temp = 1;
                }

                //aquire
                sem_wait(&alarm_mutex1);


                //decremenet the counter by 1
                counter = counter - 1;

                //Checks if the counter is zero
                if (counter == 0)
                {
                        //release
                        sem_post(&alarm_cond1);

                }

                //release
                sem_post(&alarm_mutex1);


                //Loops waits the specified amount of time before continuing
                sleep(sleepLength);
        }
}

/*
 * The alarm thread's start routine.
 */
void *alarm_thread (void *arg)
{
        alarm_t *alarm;
        struct timespec cond_time;
        time_t now;
        int status, expired, alarmRemove;
        alarm_t *next, *previous;
        pthread_t thread;

        /*
         * Loop forever, processing commands. The alarm thread will
         * be disintegrated when the process exits. Lock the mutex
         * at the start -- it will be unlocked during condition
         * waits, so the main thread can insert alarms.
         */

        while (1)
        {
                //aquire
                sem_wait(&alarm_mutex1);


                //iterates the counter by 1 each loop
                counter = counter + 1;

                //Checks if the counter is equal to 1
                if (counter == 1)
                {
                        //Locks
                        sem_wait(&alarm_cond1);

                }
                //release
                sem_post(&alarm_mutex1);

                alarm = NULL;
                next = head->link;
                //If the end of the list has not been reached
                while (next != tail)
                {
                        //Checks if the flag is 1
                        if(next->newAlarmFlag == 1)
                        {
                                //Updates the flag
                                next->newAlarmFlag = 0;
                                alarm = next;
                                break;
                        }
                        next = next->link;
                }

                //Checks if the is of type A and the alarm is not null
                if (alarm != NULL && alarm->alarmRequestType == 1)
                {
                        //Creates the display thread for this alarm type
                        printf("DISPLAY THREAD CREATED FOR: Message(%d) %s\n",
                          alarm->messageNum, alarm->message);
                        status = pthread_create (&thread, NULL,
                            periodic_display_thread, alarm);
                        if (status != 0)
                                err_abort (status, "Create alarm thread");
                }

                //aquire
                sem_wait(&alarm_mutex1);


                //decremenet the counter by 1
                counter = counter - 1;
                //Checks if the counter is zero
                if (counter == 0)
                {
                        //release
                        sem_post(&alarm_cond1);
                }

                //release
                sem_post(&alarm_mutex1);


                //Checks if the is of type B and the alarm is not null
                //Alarm needs to be removed from the alarm list
                if (alarm != NULL && alarm->alarmRequestType == 0)
                {
                        //aquire
                        sem_wait(&alarm_cond1);


                        next = head->link;
                        previous = head;
                        //If the end of the list has not been reached
                        while (next != tail)
                        {
                                //Once the alarm is found in the list
                                if(next == alarm)
                                {
                                        //Alarm that needs to be removed is
                                        //found in the alarm list
                                        //then the flag is updated
                                        alarmRemove = next->messageNum;
                                        previous->link = next->link;
                                        next->alarmExistsFlag = 0;
                                        break;
                                }
                                previous = next;
                                next = next->link;
                        }
                        next = head->link;
                        previous = head;
                        //If the end of the list has not been reached
                        while (next != tail)
                        {
                                //Alarm is removed from the list
                                if(next->messageNum == alarmRemove)
                                {
                                        next->alarmExistsFlag = 0;
                                        previous->link = next->link;
                                        break;
                                }
                                previous = next;
                                next = next->link;
                        }
                        //Cancel message printed once the cancel request is
                        //recieved and handled
                        printf("CANCEL: Message(%d) %s\n", alarm->messageNum, alarm->message);

                        //aquire
                        sem_post(&alarm_cond1);

                }
        }
}

int main (int argc, char *argv[])
{
        int status, temp;
        char line[128];
        alarm_t *alarm;
        counter = 0;
        pthread_t thread;

        tail = (alarm_t*)malloc(sizeof(alarm_t));
        tail->link = NULL;
        head = (alarm_t*)malloc(sizeof(alarm_t));
        head->link = tail;

        //Creates the thread
        status = pthread_create (&thread, NULL, alarm_thread, NULL);
        if (status != 0)
                err_abort (status, "Create alarm thread");

        while (1)
        {
                temp = 1;
                printf ("Alarm> ");
                if (fgets (line, sizeof (line), stdin) == NULL) exit (0);
                if (strlen (line) <= 1) continue;
                alarm = (alarm_t*)malloc (sizeof (alarm_t));
                if (alarm == NULL)
                        errno_abort ("Allocate alarm");

                /*
                 * Parse input line into seconds (%d), a message number (%d)
                 * and a message (%128[^\n]), consisting of up to 128 characters
                 * separated from the seconds by whitespace.
                 */
                 //Makes sure the input is in the right format
                if (sscanf (line, "%d Message(%d) %128[^\n]", &alarm->seconds,
                    &alarm->messageNum, alarm->message) < 3)
                {

                        if (sscanf (line, "Cancel: Message(%d)", &alarm->messageNum) < 1)
                        {
                                //Prints an error message if the input was
                                //in the wrong format
                                fprintf (stderr, "ERROR!!! Bad Input\n");
                                free (alarm);
                                //Updates the signal
                                temp = 0;
                        }

                        //Otherwise cancels the alarm request if the format of
                        //the input was correct
                        else
                        {
                                //Sets alarm seconds to zero
                                //removes the messages
                                //and sets the request type to cancel
                                alarm->seconds = 0;
                                strcpy(alarm->message,"");
                                alarm->alarmRequestType = 0;
                                //Updates the signal
                                temp = 1;
                        }
                }
                else
                {
                        //Updates the flag
                        alarm->alarmRequestType = 1;
                }

                if (temp == 1)
                {
                        //aquire
                        sem_wait(&alarm_cond1);
                        alarm->time = time (NULL) + alarm->seconds;

                        //Alarm flag and tracking is updated
                        alarm->newAlarmFlag = 1;
                        alarm->changeTracker = 0;

                        /*
                         * Insert the new alarm into the alarm list,
                         * sorted by alarm number.
                         */

                         //release
                        alarm_insert (alarm);
                        sem_post(&alarm_cond1);

                }
        }
}
