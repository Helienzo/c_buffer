/**
 * @file:       c_buffer.c
 * @author:     Lucas Wennerholm <lucas.wennerholm@gmail.com>
 * @brief:      Implementation of circular buffer
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
#include "c_buffer.h"
#include "string.h"
#include <stdio.h>

#define MODULO_INC(value, increment, modulus) (((value) + (increment)) % (modulus))
#ifndef LOG
#define LOG(f_, ...) printf((f_), ##__VA_ARGS__)
#endif

static inline size_t MODULO_DEC(size_t value, size_t decrement, size_t modulus)
{
    return (value + modulus - (decrement % modulus)) % modulus;
}

int32_t cBufferInit(cBuffer_t *inst, uint8_t *buffer, size_t buffer_size) {
    if (inst == NULL || buffer == NULL || buffer_size == 0) {
        return C_BUFFER_NULL_ERROR;
    }

    inst->data = buffer;
    inst->size = buffer_size;
    inst->head = 0;
    inst->tail = 0;

    return C_BUFFER_SUCCESS;
}

int32_t cBufferFull(cBuffer_t *inst)
{
    if (inst == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    return (MODULO_INC(inst->head, 1, inst->size) == inst->tail);
}

int32_t cBufferEmpty(cBuffer_t *inst)
{
    if (inst == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    return inst->head == inst->tail;
}

int32_t cBufferAvailableForRead(cBuffer_t* inst)
{
    if (inst == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    if (inst->head < inst->tail) {
        return ((inst->size - inst->tail) + inst->head);
    } else {
        return (inst->head - inst->tail);
    }

    return C_BUFFER_SUCCESS;
}

int32_t cBufferAvailableForWrite(cBuffer_t* inst)
{
    if (inst == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    if (inst->head < inst->tail) {
        return inst->tail - inst->head - 1;
    } else {
        return inst->size - inst->head + inst->tail - 1;
    }
}

int32_t cBufferPrepend(cBuffer_t *inst, uint8_t *data, size_t data_size) {
    if (inst == NULL || data == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    if (data_size == 0) {
        return C_BUFFER_SUCCESS;
    }

    // This cast is safe as the inst null check is allready done
    if ((size_t)cBufferAvailableForWrite(inst) < data_size) {
        return C_BUFFER_INSUFFICIENT;
    }

    // Look for the special case were the buffer is empty
    if (inst->head == inst->tail) {
        // For good reasons we want to reset the buffer when this happens.
        inst->head = 0;
        inst->tail = inst->size - data_size;

        // Copy the data
#ifdef NO_MEMCPY
        uint32_t data_ind = 0;
        for (uint32_t ind = inst->tail; ind < inst->size; ind++) {
            inst->data[ind] = data[data_ind];
            data_ind++;
        }
#else
        // Faster memcpy version
        memcpy(inst->data + inst->tail, data, data_size);
#endif

        return data_size;
    }

    // Check if we need to do a wrap copy
    if (data_size > inst->tail) {
        // First copy from 0 to current tail
        size_t data_ind = data_size - inst->tail;

#ifdef NO_MEMCPY
        for (size_t ind = 0; ind < inst->tail; ind++)
        {
            inst->data[ind] = data[data_ind];
            data_ind++;
        }
#else
        // Faster memcpy version
        memcpy(inst->data, data + data_ind, inst->tail);
#endif

        // Now copy up to the wrap
        size_t new_tail = inst->size - (data_size - inst->tail);

#ifdef NO_MEMCPY
        size_t buffer_ind = new_tail;
        for (size_t ind = 0; ind < data_size - inst->tail; ind++) {
            inst->data[buffer_ind] = data[ind];
            buffer_ind++;
        }
#else
        // The faster memcpy verion of the code
        memcpy(inst->data + new_tail, data, data_size - inst->tail);
#endif

        // Update the tail
        inst->tail = new_tail;
    } else {
#ifdef NO_MEMCPY
        size_t old_tail = inst->tail;
        inst->tail = inst->tail - data_size;

        size_t data_ind = 0;
        for (size_t ind = inst->tail; ind < old_tail; ind++) {
            inst->data[ind] = data[data_ind];
            data_ind++;
        }
#else
        inst->tail = inst->tail - data_size;
        // The faster memcpy version of the code
        memcpy(inst->data + inst->tail, data, data_size);
#endif
    }

    return data_size;
}

int32_t cBufferPrependUint16(cBuffer_t *inst, uint16_t data) {
    uint8_t tmp[2];
    tmp[0] = (uint8_t)(data >> 8);
    tmp[1] = (uint8_t)(data);

    return cBufferPrepend(inst, tmp, sizeof(tmp));
}

int32_t cBufferPrependUint32(cBuffer_t *inst, uint32_t data) {
    uint8_t tmp[4];
    tmp[0] = (uint8_t)(data >> 24);
    tmp[1] = (uint8_t)(data >> 16);
    tmp[2] = (uint8_t)(data >> 8);
    tmp[3] = (uint8_t)(data);

    return cBufferPrepend(inst, tmp, sizeof(tmp));
}

int32_t cBufferPrependByte(cBuffer_t *inst, uint8_t data) {
    if (inst == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    // This cast is safe as the inst null check is allready done
    if ((size_t)cBufferAvailableForWrite(inst) < 1) {
        return C_BUFFER_INSUFFICIENT;
    }

    // Look for the special case were the buffer is empty
    if (inst->head == inst->tail) {
        // For good reasons we want to reset the buffer when this happens.
        inst->head = 0;
        inst->tail = inst->size - 1;
        // Copy the data
        inst->data[inst->tail] = data;
        return 1;
    }

    // Check if we need to do a wrap copy
    if (inst->tail == 0) {
        inst->tail = inst->size - 1;
        inst->data[inst->tail] = data;
    } else {
        inst->tail = inst->tail - 1;
        inst->data[inst->tail] = data;
    }

    return 1;
}

int32_t cBufferAppend(cBuffer_t *inst, uint8_t *data, size_t data_size) {
    if (inst == NULL || data == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    if (data_size == 0) {
        return C_BUFFER_SUCCESS;
    }

    // This cast is safe as the inst null check is allready done
    if ((size_t)cBufferAvailableForWrite(inst) < data_size) {
        return C_BUFFER_INSUFFICIENT;
    }

    // Check if we need to do a wrap copy
    if (inst->head + data_size > inst->size) {
        // Frist copy up to the wrap
#ifdef NO_MEMCPY
        size_t data_ind  = 0;
        for (size_t ind = inst->head; ind < inst->size; ind++) {
            inst->data[ind] = data[data_ind];
            data_ind++;
        }
#else
        memcpy(inst->data + inst->head, data, inst->size - inst->head);
#endif

#ifdef NO_MEMCPY
        // Now copy from the wrap
        size_t buffer_ind = 0;
        for (size_t ind = data_ind; ind < data_size; ind++) {
            inst->data[buffer_ind] = data[ind];
            buffer_ind++;
        }
#else
        memcpy(inst->data, data + (inst->size - inst->head), data_size - (inst->size - inst->head));
#endif

        inst->head = data_size - (inst->size - inst->head);
    } else {
        size_t new_head = inst->head + data_size;
#ifdef NO_MEMCPY
        size_t data_ind = 0;
        for (size_t ind = inst->head; ind < new_head; ind++) {
            inst->data[ind] = data[data_ind];
            data_ind++;
        }
#else
        memcpy(inst->data + inst->head, data, data_size);
#endif
        inst->head = new_head;
    }

    return data_size;
}

int32_t cBufferAppendByte(cBuffer_t *inst, uint8_t data) {
    if (inst == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    // This cast is safe as the inst null check is allready done
    if ((size_t)cBufferAvailableForWrite(inst) < 1) {
        return C_BUFFER_INSUFFICIENT;
    }

    // Look for the special case were the buffer is empty
    if (inst->head == inst->tail) {
        // For good reasons we want to reset the buffer when this happens.
        inst->head = 0;
        inst->tail = 0;
    }

    // Check if we need to do a wrap copy
    inst->data[inst->head] = data;
    inst->head = MODULO_INC(inst->head, 1, inst->size);

    return 1;
}

int32_t cBufferReadAll(cBuffer_t *inst, uint8_t *data, size_t max_read_size) {
    if (inst == NULL || data == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    int32_t num_bytes_in_buffer = cBufferAvailableForRead(inst);

    if (num_bytes_in_buffer < C_BUFFER_SUCCESS) {
        return num_bytes_in_buffer;
    }

    if ((size_t)num_bytes_in_buffer > max_read_size) {
        return C_BUFFER_INSUFFICIENT;
    }

    // Check if there is a wrap in buffer
    if (inst->head < inst->tail) {
        // First read the data up to the wrap
        size_t bytes_in_first = inst->size - inst->tail;
#ifdef NO_MEMCPY
        size_t data_ind = 0;
        for (size_t ind = inst->tail; ind < inst->size; ind++) {
            data[data_ind] = inst->data[ind];
            data_ind++;
        }
#else
        memcpy(data, inst->data + inst->tail, bytes_in_first);
#endif

       // Then read the remaining data after the wrap
#ifdef NO_MEMCPY
        for (size_t ind = 0; ind < num_bytes_in_buffer - bytes_in_first; ind++) {
            data[data_ind] = inst->data[ind];
            data_ind++;
        }
#else
        memcpy(data + bytes_in_first, inst->data, num_bytes_in_buffer - bytes_in_first);
#endif
    } else {
        // No data wrap, just read the data into the buffer
#ifdef NO_MEMCPY
        size_t buffer_ind = inst->tail;
        for (size_t ind = 0; ind < num_bytes_in_buffer; ind++) {
            data[ind] = inst->data[buffer_ind];
            buffer_ind++;
        }
#else
        // Faster memcpy version
        memcpy(data, inst->data + inst->tail, num_bytes_in_buffer);
#endif
    }

    // Reset the buffer pointers
    inst->head = 0;
    inst->tail = 0;

    return num_bytes_in_buffer;
}

uint8_t cBufferReadByte(cBuffer_t *inst) {
    if (inst == NULL) {
        return 0;
    }

    // Protect from empty buffers
    if (cBufferEmpty(inst)) {
        LOG("Reading from empty buffer!\n");
        return 0;
    }

    // Get the next data
    uint8_t data = inst->data[inst->tail];

    inst->tail = MODULO_INC(inst->tail, 1, inst->size);

    return data;
}

int32_t cBufferReadBytes(cBuffer_t *inst, uint8_t *data, size_t read_size) {
    if (inst == NULL || data == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    int32_t num_bytes_in_buffer = cBufferAvailableForRead(inst);

    if (num_bytes_in_buffer < C_BUFFER_SUCCESS) {
        return num_bytes_in_buffer;
    }

    if (read_size > (size_t)num_bytes_in_buffer) {
        return C_BUFFER_MISMATCH;
    }


    // Check if there is a wrap in buffer
    if (inst->head < inst->tail) {
        size_t bytes_in_first = inst->size - inst->tail;
        if (read_size <= bytes_in_first) {
            // All requested data is in the first block.
            // First read the data up to the wrap
#ifdef NO_MEMCPY
            size_t data_ind = 0;
            for (size_t ind = inst->tail; ind < inst->size; ind++) {
                data[data_ind] = inst->data[ind];
                data_ind++;
            }
#else
            memcpy(data, inst->data + inst->tail, bytes_in_first);
#endif
        } else {
            // Data is divided before and after wrap
#ifdef NO_MEMCPY
            size_t data_ind = 0;
            for (size_t ind = inst->tail; ind < inst->size; ind++) {
                data[data_ind] = inst->data[ind];
                data_ind++;
            }
#else
            memcpy(data, inst->data + inst->tail, bytes_in_first);
#endif

        // Then read the remaining data after the wrap
#ifdef NO_MEMCPY
            for (size_t ind = 0; ind < read_size - bytes_in_first; ind++) {
                data[data_ind] = inst->data[ind];
                data_ind++;
            }
#else
            memcpy(data + bytes_in_first, inst->data, read_size - bytes_in_first);
#endif
        }
    } else {
        // No data wrap, just read the data into the buffer
#ifdef NO_MEMCPY
        size_t buffer_ind = inst->tail;
        for (size_t ind = 0; ind < read_size ; ind++) {
            data[ind] = inst->data[buffer_ind];
            buffer_ind++;
        }
#else
        // Faster memcpy version
        memcpy(data, inst->data + inst->tail, read_size);
#endif
    }


    inst->tail = MODULO_INC(inst->tail, read_size, inst->size);

    return read_size;
}

int32_t cBufferClear(cBuffer_t *inst) {
    if (inst == NULL) {
        return C_BUFFER_NULL_ERROR;
    }
    inst->head = 0;
    inst->tail = 0;

    return C_BUFFER_SUCCESS;
}

int32_t cBufferContiguate(cBuffer_t* inst)
{
    if (inst == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    // Check if there is a wrap in the buffer, or if it is empty
    if (cBufferEmpty(inst)) {
        // Make sure that tail points to the start of the buffer
        inst->head = 0;
        inst->tail = 0;
    } else if (inst->head < inst->tail && inst->head != 0) {
        int32_t num_of_bytes = cBufferAvailableForRead(inst);

        uint8_t* last_element  = &inst->data[inst->size - 1];
        uint8_t* first_element = &inst->data[0];
        uint8_t* tail_element  = &inst->data[inst->tail];

        // Rotate the circular buffer to remove the wrap
        uint8_t* next = tail_element;

        while (first_element != next) {
            uint8_t temp = *first_element;
            *first_element = *next;
            *next = temp;

            first_element++;
            if (next == last_element) {
                next = tail_element;
            } else {
                next++;
            }

            if (first_element == tail_element) {
                tail_element = next;
            }
        }

        // Update the tail and head variables
        inst->tail = 0;
        inst->head = num_of_bytes;
    } else {
        return C_BUFFER_SUCCESS;
    }

    return C_BUFFER_SUCCESS;
}

int32_t cBufferIsContigous(cBuffer_t* inst) {
    if (inst == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    // Check if there is a wrap in the buffer
    if (inst->head < inst->tail && inst->head != 0) {
        return C_BUFFER_WRAPED;
    }

    return C_BUFFER_SUCCESS;
}

uint8_t *cBufferGetReadPointer(cBuffer_t* inst) {
    if (inst == NULL) {
        return NULL;
    }

    // Protect from buffers with wraps
    if (inst->head < inst->tail && inst->head != 0) {
        return NULL;
    }

    return &inst->data[inst->tail];
}

uint8_t *cBufferGetWritePointer(cBuffer_t* inst) {
    if (inst == NULL) {
        return NULL;
    }

    return &inst->data[inst->head];
}

int32_t cBufferEmptyWrite(cBuffer_t* inst, size_t num_bytes) {
    if (inst == NULL) {
        return C_BUFFER_NULL_ERROR;
    }

    inst->head = MODULO_INC(inst->head, num_bytes, inst->size);

    return num_bytes;
}