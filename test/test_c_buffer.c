#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "c_buffer.h"

#define MAIN_BUFFER_SIZE 16
#define SMALL_BUFFER_SIZE 10

int main(void) {
    int32_t ret;
    int32_t available;
    uint8_t out[MAIN_BUFFER_SIZE];
    cBuffer_t cb;
    uint8_t buffer[MAIN_BUFFER_SIZE];

    printf("=== Circular Buffer Test Suite ===\n");

    /********* Test 1: Initialization and Append/ReadAll *********/
    ret = cBufferInit(&cb, buffer, MAIN_BUFFER_SIZE);
    assert(ret == C_BUFFER_SUCCESS);
    printf("Test 1: Initialization successful.\n");

    const char *testStr = "Hello";
    size_t testStrLen = strlen(testStr);

    ret = cBufferAppend(&cb, (uint8_t*)testStr, testStrLen);
    assert(ret == (int32_t)testStrLen);
    available = cBufferAvailableForRead(&cb);
    assert(available == (int32_t)testStrLen);
    printf("Test 1: Appended \"%s\" (%zu bytes).\n", testStr, testStrLen);

    ret = cBufferReadAll(&cb, out, MAIN_BUFFER_SIZE);
    assert(ret == (int32_t)testStrLen);
    out[ret] = '\0';
    printf("Test 1: ReadAll returned \"%s\".\n", out);
    assert(strcmp((char*)out, testStr) == 0);

    /********* Test 2: Prepend *********/
    ret = cBufferClear(&cb);
    assert(ret == C_BUFFER_SUCCESS);

    const char *testStr2 = "World";
    size_t testStr2Len = strlen(testStr2);
    ret = cBufferPrepend(&cb, (uint8_t*)testStr2, testStr2Len);
    assert(ret == (int32_t)testStr2Len);
    available = cBufferAvailableForRead(&cb);
    assert(available == (int32_t)testStr2Len);
    printf("Test 2: Prepend \"%s\" successful.\n", testStr2);

    ret = cBufferReadAll(&cb, out, MAIN_BUFFER_SIZE);
    assert(ret == (int32_t)testStr2Len);
    out[ret] = '\0';
    printf("Test 2: After Prepend, ReadAll returned \"%s\".\n", out);
    assert(strcmp((char*)out, testStr2) == 0);

    /********* Test 3: Wrap-Around Append and ReadAll *********/
    // Use a smaller buffer to force wrap-around.
    cBuffer_t cb_small;
    uint8_t smallBuffer[SMALL_BUFFER_SIZE];
    ret = cBufferInit(&cb_small, smallBuffer, SMALL_BUFFER_SIZE);
    assert(ret == C_BUFFER_SUCCESS);
    printf("Test 3: Initialized small buffer (%d bytes).\n", SMALL_BUFFER_SIZE);

    const char *wrapStr1 = "ABCDE";  // 5 bytes
    ret = cBufferAppend(&cb_small, (uint8_t*)wrapStr1, strlen(wrapStr1));
    assert(ret == (int32_t)strlen(wrapStr1));

    // Read 2 bytes to move the tail forward.
    uint8_t byte;
    byte = cBufferReadByte(&cb_small);
    assert(byte == 'A');
    byte = cBufferReadByte(&cb_small);
    assert(byte == 'B');

    // Now the available bytes are 3 ("CDE")
    const char *wrapStr2 = "XYZ";  // 3 bytes; this should wrap to the beginning.
    ret = cBufferAppend(&cb_small, (uint8_t*)wrapStr2, strlen(wrapStr2));
    assert(ret == (int32_t)strlen(wrapStr2));

    available = cBufferAvailableForRead(&cb_small);
    // Expected available = remaining from first append (3) + newly appended (3) = 6.
    assert(available == 6);
    printf("Test 3: After wrap-around append, available bytes: %d.\n", available);

    uint8_t smallOut[SMALL_BUFFER_SIZE];
    ret = cBufferReadAll(&cb_small, smallOut, SMALL_BUFFER_SIZE);
    assert(ret == 6);
    smallOut[ret] = '\0';
    printf("Test 3: ReadAll returned \"%s\" (expected \"CDEXYZ\").\n", smallOut);
    assert(strcmp((char*)smallOut, "CDEXYZ") == 0);

    /********* Test 4: ReadBytes (Partial Read) *********/
    ret = cBufferClear(&cb);
    const char *pattern = "123456789";  // 9 bytes
    ret = cBufferAppend(&cb, (uint8_t*)pattern, strlen(pattern));
    assert(ret == (int32_t)strlen(pattern));
    printf("Test 4: Appended pattern \"%s\".\n", pattern);

    // Read first 4 bytes using ReadBytes.
    uint8_t subset[10];
    ret = cBufferReadBytes(&cb, subset, 4);
    assert(ret == 4);
    subset[4] = '\0';
    printf("Test 4: ReadBytes (4 bytes) returned \"%s\".\n", subset);
    assert(strcmp((char*)subset, "1234") == 0);

    available = cBufferAvailableForRead(&cb);
    assert(available == 5);  // 9 - 4 = 5 remaining.
    ret = cBufferReadAll(&cb, out, MAIN_BUFFER_SIZE);
    assert(ret == 5);
    out[ret] = '\0';
    printf("Test 4: After ReadBytes, ReadAll returned \"%s\" (expected \"56789\").\n", out);
    assert(strcmp((char*)out, "56789") == 0);

    /********* Test 5: Contiguate *********/
    {
        cBuffer_t cb_small;
        uint8_t smallBuffer[SMALL_BUFFER_SIZE];
        ret = cBufferInit(&cb_small, smallBuffer, SMALL_BUFFER_SIZE);
        assert(ret == C_BUFFER_SUCCESS);
        printf("Test 5: Initialized small buffer for contiguate test.\n");

        // Append 7 bytes so that we fill the buffer partially.
        const char *data1 = "ABCDEFG";  // 7 bytes.
        ret = cBufferAppend(&cb_small, (uint8_t*)data1, strlen(data1));
        assert(ret == (int32_t)strlen(data1));

        // Read 4 bytes to move the tail forward.
        for (int i = 0; i < 4; i++) {
            byte = cBufferReadByte(&cb_small);
        }
        // Now available should be 3 bytes.
        // Append 3 more bytes to force a wrap-around.
        const char *data2 = "XYZ";
        ret = cBufferAppend(&cb_small, (uint8_t*)data2, strlen(data2));
        assert(ret == (int32_t)strlen(data2));

        available = cBufferAvailableForRead(&cb_small);
        printf("Test 5: Before contiguate, available bytes: %d.\n", available);

        ret = cBufferContiguate(&cb_small);
        assert(ret == C_BUFFER_SUCCESS);
        uint8_t *read_ptr = cBufferGetReadPointer(&cb_small);
        assert(read_ptr != NULL);
        printf("Test 5: After contiguate, data is: ");
        for (size_t i = 0; i < (size_t)available; i++) {
            printf("%c", read_ptr[i]);
        }
        printf(" (expected \"EFGXYZ\")\n");

        /* Optionally, read the contiguous data into an output buffer and compare.
           The expected result here is a concatenation of the unread data from the
           original data ("ABCDEFG") after reading 4 bytes ("ABCD" read), which leaves
           "EFG", then appended "XYZ" => "EFGXYZ". */
        ret = cBufferReadAll(&cb_small, smallOut, SMALL_BUFFER_SIZE);
        smallOut[ret] = '\0';
        assert(strcmp((char*)smallOut, "EFGXYZ") == 0);
        printf("Test 5: ReadAll after contiguate returned \"%s\".\n", smallOut);
    }

    printf("=== All tests passed! ===\n");
    return 0;
}