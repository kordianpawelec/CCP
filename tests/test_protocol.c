#include "cci_protocol.h"

#include <assert.h>
#include <stdio.h>


static void test_build_frame(void)
{
    /*
     * TODO
     */
}


static void test_invalid_payload_length(void)
{
    /*
     * TODO
     */
}


static void test_register_payload(void)
{
    /*
     * TODO
     */
}


static void test_send_payload(void)
{
    /*
     * TODO
     */
}


int main(void)
{
    test_build_frame();
    test_invalid_payload_length();

    test_register_payload();
    test_send_payload();

    printf("protocol tests passed\n");

    return 0;
}