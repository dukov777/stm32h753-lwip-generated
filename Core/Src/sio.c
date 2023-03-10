#include "stm32h7xx_hal.h"
#include "usart.h"

#include "sio.h"
#include "cmsis_os2.h"
#include "lwip.h"

#define SLIP_UART &huart2

typedef struct
{
    UART_HandleTypeDef *huart;
    osMessageQueueId_t queue;
} sio_descriptor_t;

sio_descriptor_t __sio_descriptor;

#define SLIP_INPUT_QUEUE_SIZE 256

uint8_t __input;

typedef enum
{
    Data = 0,
    Error
} _event_t;

typedef struct
{
    uint8_t data;
    _event_t event_type;
} _sio_message_t;

/**
 * Opens a serial device for communication.
 *
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t sio_open(u8_t devnum)
{
    __sio_descriptor.huart = SLIP_UART;

    __sio_descriptor.queue = osMessageQueueNew(SLIP_INPUT_QUEUE_SIZE, sizeof(_sio_message_t), NULL);
    if (__sio_descriptor.queue == NULL)
    {
        return NULL;
    }

    HAL_StatusTypeDef status = HAL_UART_Receive_IT(__sio_descriptor.huart, &__input, sizeof(__input));
    if (status != HAL_OK)
    {
        return NULL;
    }

    return &__sio_descriptor;
}

/**
 * Sends a single character to the serial device.
 *
 * @param c character to send
 * @param fd serial device handle
 *
 * @note This function will block until the character can be sent.
 */
void sio_send(u8_t c, sio_fd_t fd)
{
    sio_write(fd, (u8_t*) &c, 1);
}

/**
 * Receives a single character from the serial device.
 *
 * @param fd serial device handle
 *
 * @note This function will block until a character is received.
 */
u8_t sio_recv(sio_fd_t fd)
{
    uint8_t data;

    uint32_t status = sio_read(fd, &data, 1);
    if (status == 0)
    {
        //something really bad has happen. this function does not return errror result so we will go to system error hadler.
        Error_Handler();
    }

    return data;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    _sio_message_t data_msg = { .event_type = Data };
    data_msg.data = __input;
    osStatus_t status = osMessageQueuePut(__sio_descriptor.queue, &data_msg, osPriorityNormal, 0);
    if (status != osOK)
    {
        // something really bad has happen. this
        // function does not return error result so we will go to system error handler.
        Error_Handler();
    }
    HAL_StatusTypeDef hal_status = HAL_UART_Receive_IT(huart, &__input, sizeof(__input));
    if (hal_status != HAL_OK)
    {
        // something really bad has happen. this
        // function does not return error result so we will go to system error handler.
        Error_Handler();
    }
}

/**
 * Reads from the serial device.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 *
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */

u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
    _sio_message_t data_msg = { .event_type = Data };
    sio_descriptor_t *desc = (sio_descriptor_t*) fd;
    uint32_t bytes_to_read = len;

    for (uint32_t idx = 0; idx < bytes_to_read; idx++)
    {
        osStatus_t status = osMessageQueueGet(desc->queue, &data_msg, NULL, osWaitForever);
        if (status != osOK || data_msg.event_type == Error)
        {
            // message gueue failed and we return error.
            return 0;
        }
        data[idx] = data_msg.data;
    }

    return bytes_to_read;
}

/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
    //not implemented and not used
    Error_Handler();
    return 0;
}

/**
 * Writes to the serial device.
 *
 * @param fd serial device handle
 * @param data pointer to data to send
 * @param len length (in bytes) of data to send
 * @return number of bytes actually sent
 *
 * @note This function will block until all data can be sent.
 */
u32_t sio_write(sio_fd_t fd, u8_t *data, u32_t len)
{
    sio_descriptor_t *desc = (sio_descriptor_t*) fd;

    HAL_UART_Transmit(desc->huart, data, len, osWaitForever);

    return 0;
}

/**
 * Aborts a blocking sio_read() call.
 *
 * @param fd serial device handle
 */
void sio_read_abort(sio_fd_t fd)
{
    _sio_message_t error_msg = { .event_type = Error };
    osStatus_t status = osMessageQueuePut(__sio_descriptor.queue, &error_msg, osPriorityNormal, 0);
    if (status != osOK)
    {
        // something really bad has happen. this
        // function does not return error result so we will go to system error handler.
        Error_Handler();
    }
}
