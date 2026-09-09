#include "cci_state.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>


static void test_register_client(void)
{
    /*
     * TODO
     */
}


static void test_duplicate_active_registration(void)
{
    /*
     * TODO
     */
}


static void test_send_fetch_ack(void)
{
    /*
     * TODO
     */
}


static void test_disconnect_redelivery(void)
{
    Client bob = {}
    Message *msg = 
}


static void test_mailbox_full(void)
{
    /*
     * TODO
     */
}


static void test_invalid_ack(void)
{
    /*
     * TODO
     */
}


int main(void)
{
    test_register_client();
    test_duplicate_active_registration();

    test_send_fetch_ack();
    test_disconnect_redelivery();

    test_mailbox_full();
    test_invalid_ack();

    printf("state tests passed\n");

    return 0;
}