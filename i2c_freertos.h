/*******************************************************
 * i2c_freertos.h
 * 
 * Created on: Mar 10, 2019 
 *  
 * Each client task is associated with a receive-queue from where it receives
 * the result of the execution of the commands it has send to the command-queue.
 * 
 * When sending a command the client task specifies the queue it wants to receive the result
 * from later (here EXAMPLE_RECEIVE_QUEUE_ID).
 * It can then block on the receive-queue (here example_recieve_queue) it has specified
 * waiting for the result of execution.
 * A client should not send more commands than there is free space in it receive queue.
 * 
 * To work with the command queue, you can follow the example below and update any where in the
 * i2c_freertos.h and i2c_freertos.c files where the key word `>> MODIFY_HERE` is added.
 * 
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * 
 * #define EXAMPLE_TASK_PRIORITY 4
 * #define SOME_STACK_SIZE 300
 * 
 *  void client_example_task(void* arg) {
 *      result_t result;
 *	    uint8_t success_count = 0;
 *	    uint8_t failure_count = 0;
 *	    uint8_t data = 0;
 *
 *      command_t command = {0, EXAMPLE_RECEIVE_QUEUE_ID, 0, 1, &data};
 *
 *      for (;;) {
 *	        sendCommandToI2CWorker(command, EXAMPLE_TASK_PRIORITY);
 *
 *		    xQueueReceive(example_receive_queue, (void *)&result, portMAX_DELAY);
 *
 *		    if (result != SUCCESS) {
 *			    success_count += 1;
 *		    } else {
 *			    failure_count += 1;
 *		    }
 *
 *		    vTaskDelay(10000 / portTICK_RATE_MS);
 *	    }
 *  }
 * 
 *  void app_main() {
 *      // create the worker task
 *      TaskHandle_t worker_task_handle;
 *      xTaskCreate(vI2CWorkerTask, "worker_task", SOME_STACK_SIZE, NULL, WORKER_DEFAULT_PRIORITY, &worker_task_handle);
 *      // initialize the queues
 *      int16_t errcode = initI2CCommandAndReceiveQueues(worker_task_handle);
 *      if (errcode != SUCCESS)
 *          return;
 *      // create the worker and then clients
 *      xTaskCreate(client_example_task, "example_task", SOME_STACK_SIZE, NULL, EXAMPLE_TASK_PRIORITY, NULL);
 *      // ...
 *  }
 * 
 *******************************************************/

#include "FreeRTOS.h"
#include "rtos_queue.h"
#include "rtos_task.h"

// >> MODIFY_HERE
// #define EXAMPLE_RECEIVE_QUEUE_ID 1
// #define EXAMPLE_RECEIVE_QUEUE_SIZE 1

#define TEMP_READ_RECEIVE_QUEUE_ID 2
#define TEMP_READ_RECEIVE_QUEUE_SIZE 1

#define TEMP_WRITE_RECEIVE_QUEUE_ID 3
#define TEMP_WRITE_RECEIVE_QUEUE_SIZE 1

#define WORKER_DEFAULT_PRIORITY 1

// Error Codes
#define SUCCESS 0
#define FAILED_TO_INITIALIZE_COMMAND_QUEUE -30088

// >> MODIFY_HERE
// extern QueueHandle_t example_receive_queue;
extern QueueHandle_t temp_read_recieve_queue;
extern QueueHandle_t temp_write_recieve_queue;
extern QueueHandle_t command_queue;

typedef int16_t result_t; // the result of execution of a command passed to the command queue

typedef struct {
    uint8_t cmd;              // Sent to the slave to select a register/memory
    uint8_t receive_queue_id; // Where the I2C Task sends the result of executing the command
    uint8_t destination;      // The slave address
    bool wr;                  // Whether we want to read or write [true: write, false: read]
    bool size;                // Size of the data in bytes [true: 8bit, false: 16bit]
    uint8_t *data;            // A pointer to the actual data
} command_t;

void send_command_to_i2c_worker(command_t command, UBaseType_t priority);
int16_t init_i2c_command_and_receive_queues(TaskHandle_t worker_task_handle);
void vI2CWorkerTask(void* pvParameters);
