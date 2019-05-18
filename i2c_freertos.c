/*******************************************************
 * i2c_freertos.c
 * 
 * Created on: Mar 10, 2019 
 * 
 *******************************************************/

#include <interfaces/i2c_freertos.h>
#include "obc_i2c.h"
#include "FreeRTOS.h"
#include "rtos_queue.h"
#include "rtos_task.h"
#include "orca_i2c.h"

#define COMMAND_QUEUE_SIZE 0 + TEMP_READ_RECEIVE_QUEUE_SIZE + TEMP_WRITE_RECEIVE_QUEUE_SIZE // + EXAMPLE_RECEIVE_QUEUE_SIZE

volatile TaskHandle_t WORKER_HANDLE = NULL;

/**
 * send_command_to_i2c_worker
 *
 * Queues a command in the command queue and updates the priority of the worker task.
 * 
 *  command: the command to be queued
 *  priority: priority of the task that created the command
 *
 */
void send_command_to_i2c_worker(command_t command, UBaseType_t priority)
{
    xQueueSend(command_queue, &command, portMAX_DELAY);
    if (WORKER_HANDLE != NULL)
    {
        vTaskPrioritySet(WORKER_HANDLE, priority);
    }

    return;
}

/**
 * init_i2c_command_and_receive_queues
 *
 * Creates the command-queue and all the receive-queues.
 * Failing to create a receive queue is a partial failure mode:
 * The I2C Task continues to process commands from the task but it won't attempt to
 * write the result of the execution of those commands to the tasks receive queue.
 * 
 *  worker_task_handle: the task handle of the worker task
 * 
 *  returns: whether it successfully created the command queue.
 *      - If it fails to create the command queue it returns FAILED_TO_INITIALIZE_COMMAND_QUEUE.
 *      - otherwise it returns SUCCESS
 *
 */
int16_t init_i2c_command_and_receive_queues(TaskHandle_t worker_task_handle) {
    WORKER_HANDLE = worker_task_handle;

    command_queue = xQueueCreateStatic(COMMAND_QUEUE_SIZE, sizeof(command_t));

    if (!command_queue) {
        return FAILED_TO_INITIALIZE_COMMAND_QUEUE;
    }

    // >> MODIFY_HERE
    // example_receive_queue = xQueueCreateStatic(EXAMPLE_RECEIVE_QUEUE_SIZE, sizeof(result_t));
    temp_read_recieve_queue = xQueueCreateStatic(TEMP_READ_RECEIVE_QUEUE_SIZE, sizeof(result_t));
    temp_write_recieve_queue = xQueueCreateStatic(TEMP_WRITE_RECEIVE_QUEUE_SIZE, sizeof(result_t));

    return SUCCESS;
}

void vI2CWorkerTask(void* pvParameters) {
    command_t command;
    int16_t errcode;

    for (;;) {
        // TODO: make this an atomic operation
        if (uxQueueSpacesAvailable(command_queue) == COMMAND_QUEUE_SIZE) {
            vTaskPrioritySet(NULL, WORKER_DEFAULT_PRIORITY);
        }
        xQueueReceive(command_queue, (void*)&command, portMAX_DELAY); // Wait for a command

        if (command.wr == true) {
            if (command.size) {
                errcode = write_one_byte(command.destination, *command.data);
            } else {
                errcode = write_two_bytes(command.destination, command.data);
            }
        } else {
            if (command.size) {
                errcode = read_n_bytes(command.destination, 1, command.data);
            } else {
                errcode = read_n_bytes(command.destination, 2, command.data);
            }
        }

        if (errcode != I2C_OK) {
            notify_sender(&command, errcode);
            continue;
        }

        notify_sender(&command, SUCCESS);
    }
}

/**
 * notify_sender
 *
 * A helper function to write the result of the execution of a command to the receive queue associated with the task that sent the command.
 *
 *  command:    the command that was dequeued from the command queue
 *  error:      the result of execution of the command
 *
 */
void notify_sender(command_t* command, result_t error) {
    // Only send the result to a receive queue if the queue was successfully created (its handle is not null) in the `init` function.

    // >> MODIFY_HERE
    // if (example_receive_queue && command->receive_queue_id == EXAMPLE_RECEIVE_QUEUE_ID) {
    //     xQueueSend(example_receive_queue, (void*)&error, portMAX_DELAY);
    // }
    if (temp_read_recieve_queue && command->receive_queue_id == TEMP_READ_RECEIVE_QUEUE_ID) {
        xQueueSend(temp_read_recieve_queue, (void*)&error, portMAX_DELAY);
    }
    if (temp_write_recieve_queue && command->receive_queue_id == TEMP_WRITE_RECEIVE_QUEUE_ID) {
        xQueueSend(temp_write_recieve_queue, (void*)&error, portMAX_DELAY);
    }
}
