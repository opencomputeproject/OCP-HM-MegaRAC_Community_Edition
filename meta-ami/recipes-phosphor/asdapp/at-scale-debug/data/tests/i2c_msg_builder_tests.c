/*
Copyright (c) 2019, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "i2c_msg_builder_tests.h"

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "../i2c_msg_builder.h"
#include "../logging.h"
#include "cmocka.h"

// static char temporary_log_buffer[512];
void __wrap_ASD_log(ASD_LogLevel level, ASD_LogStream stream,
                    ASD_LogOption options, const char* format, ...)
{
    (void)level;
    (void)stream;
    (void)options;
    (void)format;
    // va_list args;
    // va_start(args, format);
    // vsnprintf(temporary_log_buffer, sizeof(temporary_log_buffer), format,
    // args);
    // fprintf(stderr, "%s\n", temporary_log_buffer);
    // va_end(args);
}

static bool malloc_fail = false;
void* __real_malloc(size_t size);
void* __wrap_malloc(size_t size)
{
    if (malloc_fail)
    {
        // put the flag back. This behavior can be changed later if
        // needed.
        malloc_fail = false;
        check_expected(size);
        return NULL;
    }
    return __real_malloc(size);
}

static int setup(void** state)
{
    I2C_Msg_Builder* handler = I2CMsgBuilder();
    assert_int_equal(i2c_msg_initialize(handler), ST_OK);
    *state = (void*)handler;
    return 0;
}

static int teardown(void** state)
{
    i2c_msg_deinitialize((I2C_Msg_Builder*)*state);
    free(*state);
    // test_free(*state);
    return 0;
}

void I2CMsgBuilder_returns_object_test()
{
    I2C_Msg_Builder* handler;
    handler = I2CMsgBuilder();
    assert_true(handler != NULL);
    assert_true(handler->msg_set == NULL);
    free(handler);
}

void I2CMsgBuilder_malloc_fail_test()
{
    expect_value(__wrap_malloc, size, sizeof(I2C_Msg_Builder));
    malloc_fail = true;
    assert_null(I2CMsgBuilder());
    malloc_fail = false;
}

void i2c_msg_initialize_null_state_returns_error_test()
{
    assert_int_equal(i2c_msg_initialize(NULL), ST_ERR);
}

void i2c_msg_initialize_creates_i2c_rdwr_ioctl_data_test()
{
    I2C_Msg_Builder handler;
    handler.msg_set = NULL;
    assert_int_equal(i2c_msg_initialize(&handler), ST_OK);
    assert_true(handler.msg_set != NULL);
    struct i2c_rdwr_ioctl_data* ioctl_data = handler.msg_set;
    assert_int_equal(ioctl_data->nmsgs, 0);
    assert_true(ioctl_data->msgs == NULL);
    // cleanup
    i2c_msg_deinitialize(&handler);
}

void i2c_msg_deinitialize_null_state_returns_error_test()
{
    assert_int_equal(i2c_msg_deinitialize(NULL), ST_ERR);
}

void i2c_msg_deinitialize_frees_i2c_rdwr_ioctl_data_test()
{
    I2C_Msg_Builder handler;
    assert_int_equal(i2c_msg_initialize(&handler), ST_OK);
    assert_true(handler.msg_set != NULL);
    assert_int_equal(i2c_msg_deinitialize(&handler), ST_OK);
    assert_true(handler.msg_set == NULL);
}

void i2c_msg_add_null_state_returns_error_test()
{
    assert_int_equal(i2c_msg_add(NULL, NULL), ST_ERR);
}

void i2c_msg_add_null_asd_i2c_msg_returns_error_test(void** state)
{
    I2C_Msg_Builder* handler = *state;
    assert_int_equal(i2c_msg_add(handler, NULL), ST_ERR);
}

void getTestMsg(uint8_t address, asd_i2c_msg* msg)
{
    (*msg).address = address;
    (*msg).read = true;
    (*msg).length = 3;
    (*msg).buffer[0] = 3;
    (*msg).buffer[1] = 5;
    (*msg).buffer[2] = 8;
}

void i2c_msg_add_one_message_test(void** state)
{
    I2C_Msg_Builder* handler = *state;
    struct i2c_rdwr_ioctl_data* ioctl_data = handler->msg_set;
    asd_i2c_msg msg;
    getTestMsg(21, &msg);
    assert_int_equal(ioctl_data->nmsgs, 0);
    assert_int_equal(i2c_msg_add(handler, &msg), ST_OK);
    assert_int_equal(ioctl_data->nmsgs, 1);
    struct i2c_msg* actual = &ioctl_data->msgs[0];
    assert_int_equal(msg.address, actual->addr);
    assert_int_equal(msg.length, actual->len);
    assert_true(msg.read == ((actual->flags & I2C_M_RD) == I2C_M_RD));
    for (int i = 0; i < actual->len; i++)
    {
        // printf("checking index: %d   %d==%d\n", i, msg.buffer[i],
        // actual->buf[i]);
        assert_int_equal(msg.buffer[i], actual->buf[i]);
    }
}

void i2c_msg_add_three_messages_test(void** state)
{
    // dont need to check every field, maybe just addresses.
    I2C_Msg_Builder* handler = *state;
    struct i2c_rdwr_ioctl_data* ioctl_data = handler->msg_set;
    asd_i2c_msg msg1, msg2, msg3;
    getTestMsg(1, &msg1);
    getTestMsg(2, &msg2);
    getTestMsg(3, &msg3);

    assert_int_equal(ioctl_data->nmsgs, 0);
    assert_int_equal(i2c_msg_add(handler, &msg1), ST_OK);
    assert_int_equal(ioctl_data->nmsgs, 1);
    assert_int_equal(i2c_msg_add(handler, &msg2), ST_OK);
    assert_int_equal(ioctl_data->nmsgs, 2);
    assert_int_equal(i2c_msg_add(handler, &msg3), ST_OK);
    assert_int_equal(ioctl_data->nmsgs, 3);

    struct i2c_msg* actual = ioctl_data->msgs;
    assert_int_equal(msg1.address, actual->addr);
    actual = ioctl_data->msgs + 1;
    assert_int_equal(msg2.address, actual->addr);
    actual = ioctl_data->msgs + 2;
    assert_int_equal(msg3.address, actual->addr);
}

void i2c_msg_get_count_null_state_returns_error_test()
{
    assert_int_equal(i2c_msg_get_count(NULL, NULL), ST_ERR);
}

void i2c_msg_get_count_null_count_returns_error_test(void** state)
{
    I2C_Msg_Builder* handler = *state;
    assert_int_equal(i2c_msg_get_count(handler, NULL), ST_ERR);
}

void i2c_msg_get_count_three_messages_test(void** state)
{
    I2C_Msg_Builder* handler = *state;
    asd_i2c_msg msg;
    getTestMsg(1, &msg);
    u_int32_t count = 0;

    assert_int_equal(i2c_msg_get_count(handler, &count), ST_OK);
    assert_int_equal(count, 0);

    assert_int_equal(i2c_msg_add(handler, &msg), ST_OK);
    assert_int_equal(i2c_msg_get_count(handler, &count), ST_OK);
    assert_int_equal(count, 1);

    assert_int_equal(i2c_msg_add(handler, &msg), ST_OK);
    assert_int_equal(i2c_msg_get_count(handler, &count), ST_OK);
    assert_int_equal(count, 2);

    assert_int_equal(i2c_msg_add(handler, &msg), ST_OK);
    assert_int_equal(i2c_msg_get_count(handler, &count), ST_OK);
    assert_int_equal(count, 3);
}

void i2c_msg_get_asd_i2c_msg_null_state_returns_error_test()
{
    assert_int_equal(i2c_msg_get_asd_i2c_msg(NULL, 0, NULL), ST_ERR);
}

void i2c_msg_get_asd_i2c_msg_null_asd_i2c_msg_returns_error_test(void** state)
{
    I2C_Msg_Builder* handler = *state;
    assert_int_equal(i2c_msg_get_asd_i2c_msg(handler, 0, NULL), ST_ERR);
}

void i2c_msg_get_asd_i2c_msg_test(void** state)
{
    I2C_Msg_Builder* handler = *state;
    struct i2c_rdwr_ioctl_data* ioctl_data = handler->msg_set;
    asd_i2c_msg msg1, msg2, msg3, actual;
    getTestMsg(1, &msg1);
    getTestMsg(2, &msg2);
    getTestMsg(3, &msg3);

    i2c_msg_add(handler, &msg1);
    i2c_msg_add(handler, &msg2);
    i2c_msg_add(handler, &msg3);
    assert_int_equal(ioctl_data->nmsgs, 3);

    assert_int_equal(i2c_msg_get_asd_i2c_msg(handler, 2, &actual), ST_OK);
    assert_int_equal(msg3.address, actual.address);
    assert_int_equal(msg3.length, actual.length);
    assert_true(msg3.read == actual.read);
    for (int i = 0; i < actual.length; i++)
    {
        // printf("checking index: %d   %d==%d\n", i, msg.buffer[i],
        // actual->buf[i]);
        assert_int_equal(msg3.buffer[i], actual.buffer[i]);
    }
}

void i2c_msg_get_asd_i2c_msg_invalid_index_test(void** state)
{
    I2C_Msg_Builder* handler = *state;
    struct i2c_rdwr_ioctl_data* ioctl_data = handler->msg_set;
    asd_i2c_msg msg1, msg2, msg3, actual;
    getTestMsg(1, &msg1);
    getTestMsg(2, &msg2);
    getTestMsg(3, &msg3);

    i2c_msg_add(handler, &msg1);
    i2c_msg_add(handler, &msg2);
    i2c_msg_add(handler, &msg3);
    assert_int_equal(ioctl_data->nmsgs, 3);

    // ask for index 3, despite only 3 added
    assert_int_equal(i2c_msg_get_asd_i2c_msg(handler, 3, &actual), ST_ERR);
}

void i2c_msg_reset_null_state_returns_error_test()
{
    assert_int_equal(i2c_msg_reset(NULL), ST_ERR);
}

void i2c_msg_reset_test()
{
    I2C_Msg_Builder handler;
    i2c_msg_initialize(&handler);
    struct i2c_rdwr_ioctl_data* ioctl_data = handler.msg_set;
    asd_i2c_msg msg1, msg2, msg3;
    getTestMsg(1, &msg1);
    getTestMsg(2, &msg2);
    getTestMsg(3, &msg3);

    i2c_msg_add(&handler, &msg1);
    i2c_msg_add(&handler, &msg2);
    i2c_msg_add(&handler, &msg3);
    assert_int_equal(ioctl_data->nmsgs, 3);

    assert_int_equal(i2c_msg_reset(&handler), ST_OK);
    assert_int_equal(ioctl_data->nmsgs, 0);
    i2c_msg_deinitialize(&handler);
}

void copy_asd_to_i2c_tests(void** state)
{
    // NULL and invalid tests
    asd_i2c_msg* asd = NULL;
    struct i2c_msg* i2c = NULL;
    assert_int_equal(copy_asd_to_i2c(asd, i2c), ST_ERR);

    // allocate objects and get test message
    asd = malloc(sizeof(asd_i2c_msg));
    i2c = malloc(sizeof(struct i2c_msg));
    getTestMsg(1, asd);

    // positive test cases //
    assert_int_equal(copy_asd_to_i2c(asd, i2c), ST_OK);
}

void copy_i2c_to_asd_tests(void** state)
{
    // NULL and invalid tests
    asd_i2c_msg* asd = NULL;
    struct i2c_msg* i2c = NULL;
    assert_int_equal(copy_i2c_to_asd(asd, i2c), ST_ERR);

    // allocate objects and get test message
    asd = malloc(sizeof(asd_i2c_msg));
    i2c = malloc(sizeof(struct i2c_msg));
    uint8_t buff[] = {0x0, 0x01};
    getTestMsg(1, asd);

    // positive test cases //
    i2c->addr = 1;
    i2c->flags = I2C_M_RD;
    i2c->buf = buff;
    i2c->len = 1;
    assert_int_equal(copy_i2c_to_asd(asd, i2c), ST_OK);
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(I2CMsgBuilder_returns_object_test),
        cmocka_unit_test(I2CMsgBuilder_malloc_fail_test),
        cmocka_unit_test(i2c_msg_initialize_null_state_returns_error_test),
        cmocka_unit_test(i2c_msg_initialize_creates_i2c_rdwr_ioctl_data_test),
        cmocka_unit_test(i2c_msg_deinitialize_null_state_returns_error_test),
        cmocka_unit_test(i2c_msg_deinitialize_frees_i2c_rdwr_ioctl_data_test),
        cmocka_unit_test(i2c_msg_add_null_state_returns_error_test),
        cmocka_unit_test_setup_teardown(
            i2c_msg_add_null_asd_i2c_msg_returns_error_test, setup, teardown),
        cmocka_unit_test_setup_teardown(i2c_msg_add_one_message_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(i2c_msg_add_three_messages_test, setup,
                                        teardown),
        cmocka_unit_test(i2c_msg_get_count_null_state_returns_error_test),
        cmocka_unit_test(i2c_msg_get_count_null_count_returns_error_test),
        cmocka_unit_test_setup_teardown(i2c_msg_get_count_three_messages_test,
                                        setup, teardown),
        cmocka_unit_test(i2c_msg_get_asd_i2c_msg_null_state_returns_error_test),
        cmocka_unit_test_setup_teardown(
            i2c_msg_get_asd_i2c_msg_null_asd_i2c_msg_returns_error_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(i2c_msg_get_asd_i2c_msg_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(
            i2c_msg_get_asd_i2c_msg_invalid_index_test, setup, teardown),
        cmocka_unit_test(i2c_msg_reset_null_state_returns_error_test),
        cmocka_unit_test(i2c_msg_reset_test),
        cmocka_unit_test(copy_asd_to_i2c_tests),
        cmocka_unit_test(copy_i2c_to_asd_tests),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
