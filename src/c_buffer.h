/**
 * @file:       c_buffer.h
 * @author:     Lucas Wennerholm <lucas.wennerholm@gmail.com>
 * @brief:      Header file for a circular buffer
 *
 * @license: MIT License
 *
 * Copyright (c) 2024 Lucas Wennerholm
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#ifndef C_BUFFER_H
#define C_BUFFER_H

#include <stdint.h>
#include <stddef.h>

// The available number of bytes in the C buffer will be one less than the size of the array
#define C_BUFFER_ARRAY_OVERHEAD 1

/**
 * This module manages connections data streams.
 */

typedef enum {
    C_BUFFER_WRAPED       = 1,
    C_BUFFER_SUCCESS      = 0,
    C_BUFFER_NULL_ERROR   = -301,
    C_BUFFER_INSUFFICIENT = -302,
    C_BUFFER_MISMATCH     = -303,
} cBufferErr_t;

typedef struct {
    uint8_t *data;
    size_t  size;
    uint32_t head;
    uint32_t tail;
} cBuffer_t;

/**
 * Initialize the buffer
 * Note: The available size in the buffer will be one less than input array
 * Input: Pointer to buffer instance
 * Input: Pointer to data array
 * Input: Size of the data array
 * Returns: cBufferErr_t
 */
int32_t cBufferInit(cBuffer_t *inst, uint8_t *buffer, size_t buffer_size);
 
/**
 * Check if the buffer is empty
 * Input: Pointer to buffer instance
 * Returns: cBufferErr_t or 1 if empty
 */
int32_t cBufferEmpty(cBuffer_t *inst);

/**
 * Check if the buffer is full
 * Input: Pointer to buffer instance
 * Returns: cBufferErr_t or 1 if full
 */
int32_t cBufferFull(cBuffer_t *inst);

/**
 * Get number of bytes available for read
 * Input: Pointer to buffer instance
 * Returns: Number of bytes available or cBufferErr_t
 */
int32_t cBufferAvailableForRead(cBuffer_t* inst);

/**
 * Get number of bytes available for write
 * Input: Pointer to buffer instance
 * Returns: Number of bytes available or cBufferErr_t
 */
int32_t cBufferAvailableForWrite(cBuffer_t* inst);

/**
 * Write the new data at the start of the buffer
 * Input: Pointer to buffer instance
 * Input: Pointer to data to write
 * Input: Size of data to write
 * Returns: cBufferErr_t or num bytes written
 */
int32_t cBufferPrepend(cBuffer_t *inst, uint8_t *data, size_t data_size);

/**
 * Write a uint32 at the start of the buffer in big endian format
 * Input: Pointer to buffer instance
 * Input: Data to write
 * Returns: cBufferErr_t or num bytes written
 */
int32_t cBufferPrependUint32(cBuffer_t *inst, uint32_t data);

/**
 * Write a uint16 at the start of the buffer in big endian format
 * Input: Pointer to buffer instance
 * Input: Data to write
 * Returns: cBufferErr_t or num bytes written
 */
int32_t cBufferPrependUint16(cBuffer_t *inst, uint16_t data);

/**
 * Write a single byte at the start of the buffer
 * Input: Pointer to buffer instance
 * Input: Byte to write
 * Returns: cBufferErr_t or 1 if one byte was written
 */
int32_t cBufferPrependByte(cBuffer_t *inst, uint8_t data);

/**
 * Write the new data at the end of the buffer
 * Input: Pointer to buffer instance
 * Input: Pointer to data to write
 * Input: Size of data to write
 * Returns: cBufferErr_t or 1 if full
 */
int32_t cBufferAppend(cBuffer_t *inst, uint8_t *data, size_t data_size);

/**
 * Write the new data at the end of the buffer
 * Input: Pointer to buffer instance
 * Input: Byte to write
 * Returns: cBufferErr_t or 1 if one byte was written
 */
int32_t cBufferAppendByte(cBuffer_t *inst, uint8_t data);

/**
 * Read data from the buffer
 * Input: Pointer to buffer instance
 * Input: Pointer to data to read into
 * Input: Maximum amount amount of data that can be read
 * Returns: cBufferErr_t or num of bytes read
 */
int32_t cBufferReadAll(cBuffer_t *inst, uint8_t *data, size_t max_read_size);

/**
 * Read the next byte from the buffer
 * Note: Using this function on emtpy buffers will return 0
 * Input: Pointer to buffer instance
 * Returns: the next data in the buffer
 */
uint8_t cBufferReadByte(cBuffer_t *inst);

/**
 * Read the next byte from the buffer
 * Note: Using this function on emtpy buffers will return 0
 * Input: Pointer to buffer instance
 * Input: Pointer to data to read into
 * Input: Number of bytes to read
 * Returns: cBufferErr_t or num of bytes read
 */
int32_t cBufferReadBytes(cBuffer_t *inst, uint8_t *data, size_t read_size);

/**
 * Clear a buffer, this resets the head and tail to first element of buffer
 * Input: Pointer to buffer instance
 * Returns: cBufferErr_t
 */
int32_t cBufferClear(cBuffer_t *inst);

/**
 * Rotate the buffer to make sure the data is available in continous memory
 * Input: Pointer to buffer instance
 * Returns: cBufferErr_t
 */
int32_t cBufferContiguate(cBuffer_t* inst);

/**
 * Check if the data is available in continous memory
 * Input: Pointer to buffer instance
 * Returns: cBufferErr_t, SUCCESS indicates that the data is contigous
 */
int32_t cBufferIsContigous(cBuffer_t* inst);

/**
 * Get pointer to the current first element in the buffer
 * Input: Pointer to buffer instance
 * Returns: Pointer to element or null if invalid or wrap in buffer
 */
uint8_t *cBufferGetReadPointer(cBuffer_t* inst);

/**
 * Get pointer to the current next write element in the buffer
 * Note: There is no garantuee that the data is continous
 * Input: Pointer to buffer instance
 * Returns: Pointer to element
 */
uint8_t *cBufferGetWritePointer(cBuffer_t* inst);

/**
 * Increment the amount of data in the buffer without writing anything to the buffer
 * Note: There is no garantuee that the data is continous
 * Input: Pointer to buffer instance
 * Input: Number of bytes to append to head
 * Returns: cBufferErr_t
 */
int32_t cBufferEmptyWrite(cBuffer_t* inst, size_t num_bytes);

/**
 * Decrement the amount of data in the buffer without copying any data
 * Input: Pointer to buffer instance
 * Input: Number of bytes to remove from to tail
 * Returns: cBufferErr_t
 */
int32_t cBufferEmptyRead(cBuffer_t* inst, size_t num_bytes);

#endif /* C_BUFFER_H */