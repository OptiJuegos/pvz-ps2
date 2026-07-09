#ifndef SIO_COOKIE_H
#define SIO_COOKIE_H

#include <stdio.h>
#include <kernel.h>

/**
 * @brief initializes both the serial port comunication and the EE_SIO custom stream to print data trough it
 * @param baudrate baudrate of the EE SIO. Unlike the BIOS sio_init() routine, we always base the baud rate on the CPU clock. 38400 is the recommended value
 * @param lcr_ueps  passed as is, so you'll need to use the SIO_LCR_* register values. You can pass 0 for all of the lcr_* params to get the standard 8N1 setting (8 data bits, no parity checking, 1 stop bit).
 * @param lcr_upen  passed as is, so you'll need to use the SIO_LCR_* register values. You can pass 0 for all of the lcr_* params to get the standard 8N1 setting (8 data bits, no parity checking, 1 stop bit).
 * @param lcr_usbl  passed as is, so you'll need to use the SIO_LCR_* register values. You can pass 0 for all of the lcr_* params to get the standard 8N1 setting (8 data bits, no parity checking, 1 stop bit).
 * @param lcr_umode passed as is, so you'll need to use the SIO_LCR_* register values. You can pass 0 for all of the lcr_* params to get the standard 8N1 setting (8 data bits, no parity checking, 1 stop bit).
 * @param hook_stdout wether to also hook EE SIO into stdout
 * @returns 0 on success, nonzero on error. see EE_SIO_START_RETCODES enum for further details
 * @see EE_SIO_START_RETCODES, https://en.cppreference.com/w/c/io/setvbuf
 */
int ee_sio_start(u32 baudrate, u8 lcr_ueps, u8 lcr_upen, u8 lcr_usbl, u8 lcr_umode, int hook_stdout);

/**
 * @brief Return values for ee_sio_start
 * @see ee_sio_start
 */
enum EE_SIO_START_RETCODES
{
    EESIO_SUCESS = 0,          /** everything is OK! */
    EESIO_COOKIE_OPEN_IS_NULL, /** fopencookie() failed to hook the custom stream */
    EESIO_BUFFER_ALLOC_IS_NULL /** the allocation of memfile_cookie.buf failed */
};

/**
 * @brief custom stream to print data to the EE serial port. must call ee_sio_start() first to make it work
 * @note it's recommended to use it with fprintf()
 */
extern FILE *EE_SIO;

#endif
