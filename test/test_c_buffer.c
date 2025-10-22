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

    /********* Test 6: Contiguate - Empty Buffer *********/
    {
        cBuffer_t cb_test;
        uint8_t testBuffer[SMALL_BUFFER_SIZE];
        ret = cBufferInit(&cb_test, testBuffer, SMALL_BUFFER_SIZE);
        assert(ret == C_BUFFER_SUCCESS);

        ret = cBufferContiguate(&cb_test);
        assert(ret == C_BUFFER_SUCCESS);
        assert(cb_test.head == 0);
        assert(cb_test.tail == 0);
        printf("Test 6: Contiguate on empty buffer successful.\n");
    }

    /********* Test 7: Contiguate - Already Contiguous *********/
    {
        cBuffer_t cb_test;
        uint8_t testBuffer[SMALL_BUFFER_SIZE];
        ret = cBufferInit(&cb_test, testBuffer, SMALL_BUFFER_SIZE);
        assert(ret == C_BUFFER_SUCCESS);

        const char *data = "HELLO";
        ret = cBufferAppend(&cb_test, (uint8_t*)data, strlen(data));
        assert(ret == (int32_t)strlen(data));

        // Already contiguous (tail=0, head=5)
        ret = cBufferContiguate(&cb_test);
        assert(ret == C_BUFFER_SUCCESS);

        uint8_t readOut[SMALL_BUFFER_SIZE];
        ret = cBufferReadAll(&cb_test, readOut, SMALL_BUFFER_SIZE);
        readOut[ret] = '\0';
        assert(strcmp((char*)readOut, "HELLO") == 0);
        printf("Test 7: Contiguate on already contiguous buffer successful.\n");
    }

    /********* Test 8: Contiguate - Overlap Case (tail_bytes < head_bytes) *********/
    {
        cBuffer_t cb_test;
        uint8_t testBuffer[SMALL_BUFFER_SIZE];  // 10 bytes
        ret = cBufferInit(&cb_test, testBuffer, SMALL_BUFFER_SIZE);
        assert(ret == C_BUFFER_SUCCESS);
        // Create wrapped state with overlap condition: tail=8, head=6
        // This gives us: tail_bytes=2 (data at [8,9]), head_bytes=6 (data at [0-5])
        // Overlap case because tail_bytes < head_bytes
        memset(testBuffer, 0, SMALL_BUFFER_SIZE);
        cb_test.data[8] = 'X';
        cb_test.data[9] = 'Y';
        cb_test.data[0] = '1';
        cb_test.data[1] = '2';
        cb_test.data[2] = '3';
        cb_test.data[3] = '4';
        cb_test.data[4] = '5';
        cb_test.data[5] = '6';
        cb_test.tail = 8;
        cb_test.head = 6;

        available = cBufferAvailableForRead(&cb_test);
        assert(available == 8);
        printf("Test 8: Created overlap condition: tail_bytes=2, head_bytes=6.\n");

        ret = cBufferContiguate(&cb_test);
        assert(ret == C_BUFFER_SUCCESS);
        assert(cb_test.tail == 0);
        assert(cb_test.head == 8);

        uint8_t overlapOut[SMALL_BUFFER_SIZE];
        ret = cBufferReadAll(&cb_test, overlapOut, SMALL_BUFFER_SIZE);
        overlapOut[ret] = '\0';
        printf("Test 8: After contiguate (overlap), data: \"%s\" (expected \"XY123456\").\n", overlapOut);
        assert(strcmp((char*)overlapOut, "XY123456") == 0);
    }

    /********* Test 9: Contiguate - Fast Path (tail_bytes > head_bytes) *********/
    {
        cBuffer_t cb_test;
        uint8_t testBuffer[SMALL_BUFFER_SIZE];
        ret = cBufferInit(&cb_test, testBuffer, SMALL_BUFFER_SIZE);
        assert(ret == C_BUFFER_SUCCESS);
        printf("Test 9: Testing fast path (tail_bytes > head_bytes).\n");

        // Direct setup: tail=6, head=2
        cb_test.tail = 6;
        cb_test.head = 2;
        // Data at [6,7,8,9] = "ABCD" (4 bytes at end)
        // Data at [0,1] = "XY" (2 bytes at start)
        testBuffer[6] = 'A';
        testBuffer[7] = 'B';
        testBuffer[8] = 'C';
        testBuffer[9] = 'D';
        testBuffer[0] = 'X';
        testBuffer[1] = 'Y';
        // tail_bytes = 10-6 = 4
        // head_bytes = 2
        // NO OVERLAP: tail_bytes (4) > head_bytes (2)

        available = cBufferAvailableForRead(&cb_test);
        assert(available == 6);
        printf("Test 9: Created fast path condition: tail_bytes=4, head_bytes=2.\n");

        ret = cBufferContiguate(&cb_test);
        assert(ret == C_BUFFER_SUCCESS);
        assert(cb_test.tail == 0);
        assert(cb_test.head == 6);

        uint8_t fastOut[SMALL_BUFFER_SIZE];
        ret = cBufferReadAll(&cb_test, fastOut, SMALL_BUFFER_SIZE);
        fastOut[ret] = '\0';
        printf("Test 9: After contiguate (fast path), data: \"%s\" (expected \"ABCDXY\").\n", fastOut);
        assert(strcmp((char*)fastOut, "ABCDXY") == 0);
    }

    /********* Test 10: Contiguate - Very Small Buffer (< CBUFFER_TEMP_SIZE) *********/
    {
        #define TINY_BUFFER_SIZE 6
        cBuffer_t cb_test;
        uint8_t testBuffer[TINY_BUFFER_SIZE];
        ret = cBufferInit(&cb_test, testBuffer, TINY_BUFFER_SIZE);
        assert(ret == C_BUFFER_SUCCESS);
        printf("Test 10: Testing very small buffer (size=%d < temp_size=8).\n", TINY_BUFFER_SIZE);

        // Create wrapped state: tail=4, head=2
        cb_test.tail = 4;
        cb_test.head = 2;
        // Data at [4,5] = "AB" (2 bytes at end)
        // Data at [0,1] = "XY" (2 bytes at start)
        testBuffer[4] = 'A';
        testBuffer[5] = 'B';
        testBuffer[0] = 'X';
        testBuffer[1] = 'Y';

        available = cBufferAvailableForRead(&cb_test);
        assert(available == 4);

        ret = cBufferContiguate(&cb_test);
        assert(ret == C_BUFFER_SUCCESS);
        assert(cb_test.tail == 0);
        assert(cb_test.head == 4);

        uint8_t tinyOut[TINY_BUFFER_SIZE];
        ret = cBufferReadAll(&cb_test, tinyOut, TINY_BUFFER_SIZE);
        tinyOut[ret] = '\0';
        printf("Test 10: After contiguate (tiny buffer), data: \"%s\" (expected \"ABXY\").\n", tinyOut);
        assert(strcmp((char*)tinyOut, "ABXY") == 0);
    }

    /********* Test 11: Contiguate - Single Byte Sections *********/
    {
        cBuffer_t cb_test;
        uint8_t testBuffer[SMALL_BUFFER_SIZE];
        ret = cBufferInit(&cb_test, testBuffer, SMALL_BUFFER_SIZE);
        assert(ret == C_BUFFER_SUCCESS);
        printf("Test 11: Testing single byte sections.\n");

        // Create: tail=9, head=1 (1 byte at end, 1 byte at start)
        cb_test.tail = 9;
        cb_test.head = 1;
        testBuffer[9] = 'Z';
        testBuffer[0] = 'A';

        available = cBufferAvailableForRead(&cb_test);
        assert(available == 2);

        ret = cBufferContiguate(&cb_test);
        assert(ret == C_BUFFER_SUCCESS);
        assert(cb_test.tail == 0);
        assert(cb_test.head == 2);

        uint8_t singleOut[SMALL_BUFFER_SIZE];
        ret = cBufferReadAll(&cb_test, singleOut, SMALL_BUFFER_SIZE);
        singleOut[ret] = '\0';
        printf("Test 11: After contiguate (single bytes), data: \"%s\" (expected \"ZA\").\n", singleOut);
        assert(strcmp((char*)singleOut, "ZA") == 0);
    }

    /********* Test 12: Contiguate - Nearly Full Buffer with Overlap *********/
    {
        cBuffer_t cb_test;
        uint8_t testBuffer[SMALL_BUFFER_SIZE];  // 10 bytes
        ret = cBufferInit(&cb_test, testBuffer, SMALL_BUFFER_SIZE);
        assert(ret == C_BUFFER_SUCCESS);
        printf("Test 12: Testing nearly full buffer with overlap.\n");

        // Create wrapped state: tail=3, head=2
        // This gives us: tail_bytes=7 (data at [3,4,5,6,7,8,9]), head_bytes=2 (data at [0,1])
        // Total used = 9 bytes out of 10 (nearly full)
        // But this is NOT overlap since tail_bytes > head_bytes

        // Let's create: tail=9, head=8
        // tail_bytes=1 (data at [9]), head_bytes=8 (data at [0-7])
        // Total used = 9 bytes (nearly full), overlap case since tail_bytes < head_bytes
        memset(testBuffer, 0, SMALL_BUFFER_SIZE);
        cb_test.data[9] = 'Z';
        cb_test.data[0] = '1';
        cb_test.data[1] = '2';
        cb_test.data[2] = '3';
        cb_test.data[3] = '4';
        cb_test.data[4] = '5';
        cb_test.data[5] = '6';
        cb_test.data[6] = '7';
        cb_test.data[7] = '8';
        cb_test.tail = 9;
        cb_test.head = 8;

        available = cBufferAvailableForRead(&cb_test);
        assert(available == 9);
        printf("Test 12: Created nearly full overlap: tail_bytes=1, head_bytes=8, total=9/%d.\n", SMALL_BUFFER_SIZE);

        ret = cBufferContiguate(&cb_test);
        assert(ret == C_BUFFER_SUCCESS);
        assert(cb_test.tail == 0);
        assert(cb_test.head == 9);

        uint8_t fullOut[SMALL_BUFFER_SIZE];
        ret = cBufferReadAll(&cb_test, fullOut, SMALL_BUFFER_SIZE);
        fullOut[ret] = '\0';
        printf("Test 12: After contiguate (nearly full), data: \"%s\" (expected \"Z12345678\").\n", fullOut);
        assert(strcmp((char*)fullOut, "Z12345678") == 0);
    }

    printf("=== All tests passed! ===\n");
    return 0;
}