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
#include "asd_msg_tests.h"

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <syslog.h>

#include "../asd_msg.h"
#include "../config.h"
#include "../i2c_msg_builder.h"
#include "../logging.h"
#include "../mem_helper.h"
#include "cmocka.h"
#include "i2c_msg_builder_tests.h"

static const char BMC_VERSION[] = "OPENBMC_VERSION:\"wht-0.22-0-ge37eb7922\"";
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

void __wrap_ASD_log_buffer(ASD_LogLevel level, ASD_LogStream stream,
                           ASD_LogOption options, const unsigned char* ptr,
                           size_t len, const char* prefixPtr)
{
    (void)level;
    (void)stream;
    (void)options;
    (void)ptr;
    (void)len;
    (void)prefixPtr;
}

#define MAX_COMMANDS 10000
int malloc_index = 0;
static bool malloc_fail[MAX_COMMANDS];
static bool malloc_fail_mode = false;
void* __real_malloc(size_t size);

int FAKE_FD = 0;
int OS_RELEASE_GOOD = 0;
FILE* __real_fopen(const char* pathname, const char* flags);
FILE* __wrap_fopen(const char* pathname, const char* flags)
{
    if (OS_RELEASE_GOOD == 0)
        return __real_fopen("../../tests/os-release_good", "r");
    else if (OS_RELEASE_GOOD == 1)
        return __real_fopen("../../tests/os-release_bad", "r");
    else
        return NULL;
}

void* __wrap_malloc(size_t size)
{
    if (malloc_fail_mode)
    {
        if (malloc_fail[malloc_index])
        {
            // put the flag back. This behavior can be changed later
            // if needed.
            check_expected(size);
            return NULL;
        }
        malloc_index++;
    }
    return __real_malloc(size);
}

bool IGNORE_CHECKS = false;
int command_index = 0;
STATUS command_result[MAX_COMMANDS];
STATUS FLOCK_RESULT[MAX_COMMANDS];

JTAG_Handler* FAKE_JTAG_HANDLER;
JTAG_Handler* __wrap_JTAGHandler()
{
    return FAKE_JTAG_HANDLER;
}

STATUS __wrap_JTAG_initialize(JTAG_Handler* state, bool sw_mode)
{
    check_expected_ptr(state);
    check_expected(sw_mode);
    return command_result[command_index++];
}

int MEMCPY_SAFE_RESULT = 0;
int __wrap_memcpy_safe(void* dest, size_t destsize, const void* src,
                       size_t count)
{
    if (MEMCPY_SAFE_RESULT == 1)
        return MEMCPY_SAFE_RESULT;
    else
    {
        while (count--)
            *(char*)dest++ = *(const char*)src++;
        return MEMCPY_SAFE_RESULT;
    }
}

void expect_default_bmc_version()
{
    OS_RELEASE_GOOD = 0;
    MEMCPY_SAFE_RESULT = 0;
}

STATUS __wrap_JTAG_deinitialize(JTAG_Handler* state)
{
    check_expected_ptr(state);
    return command_result[command_index++];
}

STATUS JTAG_SET_TAP_STATE_RESULT;
STATUS __wrap_JTAG_set_tap_state(JTAG_Handler* state,
                                 enum jtag_states tap_state)
{
    check_expected_ptr(state);
    check_expected(tap_state);
    return JTAG_SET_TAP_STATE_RESULT;
}

STATUS JTAG_SHIFT_RESULT;
unsigned char* tdi = NULL;
size_t tdo_bytes = 0;
size_t tdo_index = 0;
unsigned char* tdo = NULL;
STATUS __wrap_JTAG_shift(JTAG_Handler* state, unsigned int number_of_bits,
                         unsigned int input_bytes, unsigned char* input,
                         unsigned int output_bytes, unsigned char* output,
                         enum jtag_states end_tap_state)
{
    if (!IGNORE_CHECKS)
    {
        check_expected_ptr(state);
        check_expected(number_of_bits);
        check_expected(input_bytes);
        check_expected_ptr(input);
        check_expected(output_bytes);
        check_expected_ptr(output);
        check_expected(end_tap_state);
    }
    if (input_bytes > 0 && tdi != NULL && input != NULL)
    {
        memcpy(tdi, input, input_bytes);
    }
    if (tdo_bytes > 0 && tdo != NULL && output != NULL)
    {
        memcpy(output, tdo + tdo_index, tdo_bytes);
        tdo_index += tdo_bytes;
    }
    return JTAG_SHIFT_RESULT;
}

STATUS JTAG_SET_JTAG_TCK_RESULT;
STATUS __wrap_JTAG_set_jtag_tck(JTAG_Handler* state, unsigned int tck)
{
    check_expected_ptr(state);
    check_expected(tck);
    return JTAG_SET_JTAG_TCK_RESULT;
}

STATUS JTAG_WAIT_CYCLES_RESULT;
STATUS __wrap_JTAG_wait_cycles(JTAG_Handler* state,
                               unsigned int number_of_cycles)
{
    check_expected_ptr(state);
    check_expected(number_of_cycles);
    return JTAG_WAIT_CYCLES_RESULT;
}

STATUS JTAG_TAP_RESET_RESULT;
STATUS __wrap_JTAG_tap_reset(JTAG_Handler* state)
{
    check_expected_ptr(state);
    return JTAG_TAP_RESET_RESULT;
}

STATUS JTAG_SET_PADDING_RESULT;
STATUS __wrap_JTAG_set_padding(JTAG_Handler* state, JTAGPaddingTypes padding,
                               unsigned int value)
{
    check_expected_ptr(state);
    check_expected(padding);
    check_expected(value);
    return JTAG_SET_PADDING_RESULT;
}

STATUS JTAG_GET_TAP_STATE_RESULT;
enum jtag_states JTAG_STATE = jtag_sel_dr;
STATUS __wrap_JTAG_get_tap_state(JTAG_Handler* state,
                                 enum jtag_states* tap_state)
{
    if (!IGNORE_CHECKS)
    {
        check_expected_ptr(state);
        check_expected_ptr(tap_state);
    }
    *tap_state = JTAG_STATE;
    return JTAG_GET_TAP_STATE_RESULT;
}

STATUS __wrap_JTAG_set_active_chain(JTAG_Handler* state, scanChain chain)
{
    check_expected_ptr(state);
    check_expected(chain);
    return command_result[command_index++];
}

Target_Control_Handle* FAKE_TARGET_HANDLER;
Target_Control_Handle* __wrap_TargetHandler()
{
    return FAKE_TARGET_HANDLER;
}

STATUS __wrap_target_initialize(Target_Control_Handle* state)
{
    check_expected_ptr(state);
    return command_result[command_index++];
}

STATUS __wrap_target_deinitialize(Target_Control_Handle* state)
{
    check_expected_ptr(state);
    return command_result[command_index++];
}

STATUS __wrap_target_write_event_config(Target_Control_Handle* state,
                                        const WriteConfig event_cfg,
                                        const bool enable)
{
    check_expected_ptr(state);
    check_expected(event_cfg);
    check_expected(enable);
    return command_result[command_index++];
}

STATUS __wrap_target_write(Target_Control_Handle* state,
                           Target_Control_GPIOS pin, bool assert)
{
    check_expected_ptr(state);
    check_expected(pin);
    check_expected(assert);
    return command_result[command_index++];
}

bool TARGET_READ_IGNORE = false;
bool TARGET_READ_ASSERTED = false;
int dummy = 0;
STATUS __wrap_target_read(Target_Control_Handle* state,
                          Target_Control_GPIOS pin, bool* asserted)
{
    dummy = 0;
    if (!TARGET_READ_IGNORE)
    {
        check_expected_ptr(state);
        check_expected(pin);
        check_expected_ptr(asserted);
    }
    *asserted = TARGET_READ_ASSERTED;
    return command_result[command_index++];
}

STATUS __wrap_target_wait_PRDY(Target_Control_Handle* state,
                               const uint8_t log2time)
{
    check_expected_ptr(state);
    check_expected(log2time);
    return command_result[command_index++];
}

STATUS __wrap_target_wait_sync(Target_Control_Handle* state,
                               const uint16_t timeout, const uint16_t delay)
{
    check_expected_ptr(state);
    check_expected(timeout);
    check_expected(delay);
    return command_result[command_index++];
}

STATUS __wrap_target_get_fds(Target_Control_Handle* state, target_fdarr_t* fds,
                             int* num_fds)
{
    check_expected_ptr(state);
    check_expected_ptr(fds);
    check_expected_ptr(num_fds);
    return command_result[command_index++];
}

ASD_EVENT TARGET_EVENT_EVENT;
STATUS __wrap_target_event(Target_Control_Handle* state, struct pollfd poll_fd,
                           ASD_EVENT* event)
{
    check_expected_ptr(state);
    check_expected(poll_fd.fd);
    check_expected_ptr(event);
    *event = TARGET_EVENT_EVENT;
    return command_result[command_index++];
}

struct asd_message msg_sent;
STATUS FakeSendFunctionResult = ST_OK;

STATUS FakeSendFunctionPtr(void* state, unsigned char* message, size_t length)
{
    (void)state; /* unused */
    memcpy(&msg_sent.header, message, sizeof(struct message_header));
    memcpy(msg_sent.buffer, message + sizeof(struct message_header),
           length - sizeof(struct message_header));

    return FakeSendFunctionResult;
}

struct asd_message msg_sent;
unsigned char FakeReadFunctionBuffer[MAX_DATA_SIZE];
int FakeReadFunctionNumBytesPerRead = -1;
int FakeReadFunctionNumBytesReadIndex = 0;
bool FakeReadFunctionDataPending = false;
STATUS FakeReadFunctionResult = ST_OK;
STATUS FakeReadFunctionPtr(void* state, void* connection, void* buffer,
                           size_t* num_to_read, bool* data_pending)
{
    check_expected_ptr(state);
    check_expected_ptr(connection);
    check_expected_ptr(buffer);
    check_expected_ptr(num_to_read);
    check_expected_ptr(data_pending);
    *data_pending = FakeReadFunctionDataPending;
    size_t number_to_provide = *num_to_read;
    if (FakeReadFunctionNumBytesPerRead >= 0)
        number_to_provide = (size_t)FakeReadFunctionNumBytesPerRead;

    for (int i = 0; i < (number_to_provide); i++)
        ((unsigned char*)buffer)[i] =
            FakeReadFunctionBuffer[FakeReadFunctionNumBytesReadIndex + i];

    FakeReadFunctionNumBytesReadIndex += number_to_provide;
    *num_to_read -= number_to_provide;
    return FakeReadFunctionResult;
}

STATUS BUS_SELECT_STATUS = ST_OK;
STATUS __wrap_i2c_bus_select(I2C_Handler* state, uint8_t bus)
{
    check_expected_ptr(state);
    check_expected(bus);
    return BUS_SELECT_STATUS;
}

STATUS SET_SCLK_STATUS = ST_OK;
STATUS __wrap_i2c_set_sclk(I2C_Handler* state, uint16_t sclk)
{
    check_expected_ptr(state);
    check_expected(sclk);
    return SET_SCLK_STATUS;
}

STATUS READ_WRITE_STATUS = ST_OK;
uint8_t READ_RESPONSE[256];
STATUS __wrap_i2c_read_write(I2C_Handler* state, void* msg_set)
{
    check_expected_ptr(state);
    check_expected_ptr(msg_set);
    struct i2c_rdwr_ioctl_data* ioctl_data = msg_set;
    struct i2c_msg* msg = ioctl_data->msgs;
    for (int i = 0; i < msg->len; i++)
    {
        msg->buf[i] = READ_RESPONSE[i];
    }
    return READ_WRITE_STATUS;
}

int I2C_MSG_COUNT = 1;
static bool i2c_msg_get_count_mode = false;
STATUS __wrap_i2c_msg_get_count(I2C_Msg_Builder* state, u_int32_t* count)
{
    if (i2c_msg_get_count_mode)
    {
        *count = I2C_MSG_COUNT;
        return ST_OK;
    }
    STATUS status = ST_ERR;
    if (state != NULL && state->msg_set != NULL && count != NULL)
    {
        struct i2c_rdwr_ioctl_data* ioctl_data = state->msg_set;
        *count = ioctl_data->nmsgs;
        status = ST_OK;
    }
    return status;
}

STATUS I2C_MSG_GET_ASD_I2C_MSG_STATUS = ST_OK;
static bool i2c_msg_get_asd_i2c_msg_mode = false;
STATUS __wrap_i2c_msg_get_asd_i2c_msg(I2C_Msg_Builder* state, u_int32_t index,
                                      asd_i2c_msg* msg)
{
    if (i2c_msg_get_asd_i2c_msg_mode)
    {
        msg->length = 0xff;
        return ST_OK;
    }
    STATUS status = ST_ERR;
    if (state != NULL && state->msg_set != NULL && msg != NULL)
    {
        struct i2c_rdwr_ioctl_data* ioctl_data = state->msg_set;
        if (ioctl_data->nmsgs > index)
        {
            struct i2c_msg* i2c_msg1 = ioctl_data->msgs + index;
            status = copy_i2c_to_asd(msg, i2c_msg1);
        }
    }
    if (I2C_MSG_GET_ASD_I2C_MSG_STATUS == ST_OK)
    {
        return status;
    }
    else
    {
        return I2C_MSG_GET_ASD_I2C_MSG_STATUS;
    }
}

STATUS I2C_MSG_RESET_STATUS = ST_OK;
STATUS __wrap_i2c_msg_reset(I2C_Msg_Builder* state)
{
    STATUS status = ST_ERR;
    if (state != NULL)
    {
        struct i2c_rdwr_ioctl_data* ioctl_data = state->msg_set;
        for (int i = 0; i < ioctl_data->nmsgs; i++)
        {
            struct i2c_msg* i2c_msg1 = ioctl_data->msgs + i;
            if (i2c_msg1 != NULL && i2c_msg1->buf != NULL)
            {
                free(i2c_msg1->buf);
                i2c_msg1->buf = NULL;
            }
        }
        ioctl_data->nmsgs = 0;
        if (ioctl_data->msgs)
        {
            free(ioctl_data->msgs);
            ioctl_data->msgs = NULL;
        }
        status = ST_OK;
    }
    if (I2C_MSG_RESET_STATUS == ST_OK)
    {
        return status;
    }
    else
    {
        return I2C_MSG_RESET_STATUS;
    }
}

I2C_Handler* FAKE_I2C_HANDLER;
I2C_Handler* __wrap_I2CHandler()
{
    return FAKE_I2C_HANDLER;
}

STATUS __wrap_i2c_initialize(I2C_Handler* state)
{
    check_expected_ptr(state);
    return command_result[command_index++];
}

STATUS __wrap_i2c_deinitialize(I2C_Handler* state)
{
    check_expected_ptr(state);
    return command_result[command_index++];
}

int __wrap_flock(int fd, int op)
{
    return FLOCK_RESULT[command_index++];
}

config asd_config;
static i2c_config global_i2c_config;
static int setup(void** state)
{
    int cb_state = 1;
    IGNORE_CHECKS = false;
    expect_default_bmc_version();
    FAKE_JTAG_HANDLER = (JTAG_Handler*)malloc(sizeof(JTAG_Handler));
    FAKE_TARGET_HANDLER =
        (struct Target_Control_Handle*)malloc(sizeof(Target_Control_Handle));
    FAKE_I2C_HANDLER = (struct I2C_Handler*)malloc(sizeof(I2C_Handler));
    *state = asd_msg_init(&FakeSendFunctionPtr, &FakeReadFunctionPtr, &cb_state,
                          &asd_config);
    memset(&msg_sent, 0, sizeof(struct asd_message));
    FakeSendFunctionResult = ST_OK;
    ((ASD_MSG*)*state)->handlers_initialized = true;
    ((ASD_MSG*)*state)->i2c_handler->config = &global_i2c_config;
    ((ASD_MSG*)*state)->i2c_handler->config->enable_i2c = false;
    tdo = (unsigned char*)malloc(MAX_DATA_SIZE);
    TARGET_READ_IGNORE = false;
    return 0;
}

static int teardown(void** state)
{
    expect_any(__wrap_JTAG_deinitialize, state);
    expect_any(__wrap_target_deinitialize, state);
    expect_any(__wrap_i2c_deinitialize, state);
    asd_msg_free(*state);
    free(*state);
    FAKE_JTAG_HANDLER = NULL;
    FAKE_TARGET_HANDLER = NULL;
    FAKE_I2C_HANDLER = NULL;
    free(tdo);
    return 0;
}

unsigned char data_buffer[MAX_DATA_SIZE];
unsigned char data_buffer[MAX_DATA_SIZE];
void get_fake_message(uint32_t type, struct asd_message* msg)
{
    memset(&msg->header, 0, sizeof(struct message_header));
    msg->header.type = (uint8_t)type;
    for (int i = 0; i < MAX_DATA_SIZE; i++)
        msg->buffer[i] = data_buffer[i];
}

void asd_msg_init_invalid_param_test(void** state)
{
    (void)state; /* unused */
    int fake_cb = 1;
    config config;
    assert_null(asd_msg_init(NULL, &FakeReadFunctionPtr, &fake_cb, &config));
    assert_null(asd_msg_init(&FakeSendFunctionPtr, &FakeReadFunctionPtr,
                             &fake_cb, NULL));
    assert_null(asd_msg_init(&FakeSendFunctionPtr, &FakeReadFunctionPtr, NULL,
                             &config));
    assert_null(asd_msg_init(&FakeSendFunctionPtr, NULL, NULL, &config));
}

void asd_msg_init_malloc_failure_test(void** state)
{
    (void)state; /* unused */
    expect_value(__wrap_malloc, size, sizeof(ASD_MSG));
    malloc_fail[0] = true;
    config config;
    int fake_state = 1;
    malloc_fail_mode = true;
    assert_null(asd_msg_init(&FakeSendFunctionPtr, &FakeReadFunctionPtr,
                             &fake_state, &config));
    malloc_fail_mode = false;
}

void asd_msg_jtag_init_failed_test(void** state)
{
    (void)state; /* unused */
    config config;
    int fake_state = 1;

    FAKE_JTAG_HANDLER = NULL;
    FAKE_TARGET_HANDLER =
        (struct Target_Control_Handle*)malloc(sizeof(Target_Control_Handle));
    FAKE_I2C_HANDLER = (struct I2C_Handler*)malloc(sizeof(I2C_Handler));
    assert_null(asd_msg_init(&FakeSendFunctionPtr, &FakeReadFunctionPtr,
                             &fake_state, &config));
    FAKE_JTAG_HANDLER = (JTAG_Handler*)malloc(sizeof(JTAG_Handler));
    FAKE_TARGET_HANDLER = NULL;
    FAKE_I2C_HANDLER = NULL;
    assert_null(asd_msg_init(&FakeSendFunctionPtr, &FakeReadFunctionPtr,
                             &fake_state, &config));
    FAKE_JTAG_HANDLER = NULL;
}

void asd_msg_init_test(void** state)
{
    (void)state; /* unused */
    config config;
    int cb_state = 1;
    expect_default_bmc_version();
    FAKE_JTAG_HANDLER = (JTAG_Handler*)malloc(sizeof(JTAG_Handler));
    FAKE_TARGET_HANDLER =
        (struct Target_Control_Handle*)malloc(sizeof(Target_Control_Handle));
    FAKE_I2C_HANDLER = (struct I2C_Handler*)malloc(sizeof(I2C_Handler));
    ASD_MSG* sdk = asd_msg_init(&FakeSendFunctionPtr, &FakeReadFunctionPtr,
                                &cb_state, &config);

    assert_non_null(sdk->should_remote_log);
    assert_non_null(sdk->send_remote_logging_message);
    assert_non_null(sdk->send_function);
    assert_non_null(sdk->read_function);
    assert_non_null(sdk->asd_cfg);
    assert_false(sdk->handlers_initialized);
    assert_non_null(sdk->callback_state);
    assert_int_equal(READ_STATE_INITIAL, sdk->in_msg.read_state);
    assert_int_equal(0, sdk->in_msg.read_index);

    expect_any(__wrap_JTAG_deinitialize, state);
    expect_any(__wrap_target_deinitialize, state);
    expect_any(__wrap_i2c_deinitialize, state);

    asd_msg_free(sdk);
    free(sdk);
    FAKE_JTAG_HANDLER = NULL;
    FAKE_TARGET_HANDLER = NULL;
    FAKE_I2C_HANDLER = NULL;
}

void asd_msg_init_only_one_instance_test(void** state)
{
    (void)state; /* unused */
    config config;
    int cb_state = 1;
    expect_default_bmc_version();
    FAKE_JTAG_HANDLER = (JTAG_Handler*)malloc(sizeof(JTAG_Handler));
    FAKE_TARGET_HANDLER =
        (struct Target_Control_Handle*)malloc(sizeof(Target_Control_Handle));
    FAKE_I2C_HANDLER = (struct I2C_Handler*)malloc(sizeof(I2C_Handler));
    ASD_MSG* sdk = asd_msg_init(&FakeSendFunctionPtr, &FakeReadFunctionPtr,
                                &cb_state, &config);

    // this one should fail
    assert_null(asd_msg_init(&FakeSendFunctionPtr, &FakeReadFunctionPtr,
                             &cb_state, &config));

    expect_any(__wrap_JTAG_deinitialize, state);
    expect_any(__wrap_target_deinitialize, state);
    expect_any(__wrap_i2c_deinitialize, state);

    asd_msg_free(sdk);
    free(sdk);
    FAKE_JTAG_HANDLER = NULL;
}

void asd_msg_free_invalid_param_test(void** state)
{
    (void)state; /* unused */
    assert_int_equal(asd_msg_free(NULL), ST_ERR);
}

void asd_msg_free_jtag_deinit_fail_test(void** state)
{
    (void)state; /* unused */
    config config;
    int fake_state = 1;
    expect_default_bmc_version();
    FAKE_JTAG_HANDLER = (JTAG_Handler*)malloc(sizeof(JTAG_Handler));
    FAKE_TARGET_HANDLER =
        (struct Target_Control_Handle*)malloc(sizeof(Target_Control_Handle));
    FAKE_I2C_HANDLER = (struct I2C_Handler*)malloc(sizeof(I2C_Handler));
    ASD_MSG* actual = asd_msg_init(&FakeSendFunctionPtr, &FakeReadFunctionPtr,
                                   &fake_state, &config);
    command_index = 0;
    command_result[0] = ST_ERR;
    command_result[1] = ST_ERR;
    command_result[2] = ST_ERR;

    expect_any(__wrap_JTAG_deinitialize, state);
    expect_any(__wrap_target_deinitialize, state);
    expect_any(__wrap_i2c_deinitialize, state);
    assert_int_equal(asd_msg_free(actual), ST_ERR);
    free(actual);
    FAKE_JTAG_HANDLER = NULL;
}

void asd_msg_free_target_deinit_fail_test(void** state)
{
    (void)state; /* unused */
    config config;
    int fake_state = 1;
    expect_default_bmc_version();
    FAKE_JTAG_HANDLER = (JTAG_Handler*)malloc(sizeof(JTAG_Handler));
    FAKE_TARGET_HANDLER =
        (struct Target_Control_Handle*)malloc(sizeof(Target_Control_Handle));
    FAKE_I2C_HANDLER = (struct I2C_Handler*)malloc(sizeof(I2C_Handler));
    ASD_MSG* actual = asd_msg_init(&FakeSendFunctionPtr, &FakeReadFunctionPtr,
                                   &fake_state, &config);
    command_result[0] = ST_OK;
    command_result[1] = ST_ERR;
    command_result[2] = ST_OK;
    command_index = 0;

    expect_any(__wrap_JTAG_deinitialize, state);
    expect_any(__wrap_target_deinitialize, state);
    expect_any(__wrap_i2c_deinitialize, state);
    assert_int_equal(asd_msg_free(actual), ST_ERR);
    free(actual);
}

void asd_msg_free_test(void** state)
{
    (void)state; /* unused */
    config config;
    int fake_state = 1;
    expect_default_bmc_version();
    FAKE_JTAG_HANDLER = (JTAG_Handler*)malloc(sizeof(JTAG_Handler));
    FAKE_TARGET_HANDLER =
        (struct Target_Control_Handle*)malloc(sizeof(Target_Control_Handle));
    FAKE_I2C_HANDLER = (struct I2C_Handler*)malloc(sizeof(I2C_Handler));
    ASD_MSG* actual = asd_msg_init(&FakeSendFunctionPtr, &FakeReadFunctionPtr,
                                   &fake_state, &config);
    command_result[0] = ST_OK;
    command_result[1] = ST_OK;
    command_result[2] = ST_OK;
    command_index = 0;

    expect_any(__wrap_JTAG_deinitialize, state);
    expect_any(__wrap_target_deinitialize, state);
    expect_any(__wrap_i2c_deinitialize, state);
    assert_int_equal(asd_msg_free(actual), ST_OK);
    free(actual);
}

void asd_msg_on_msg_recv_invalid_param_test(void** state)
{
    (void)state; /* unused */
    // should have no failed 'expects'
    asd_msg_on_msg_recv(NULL);
}

void asd_msg_on_msg_recv_encrypted_check_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.enc_bit = 1;
    MEMCPY_SAFE_RESULT = 0;
    asd_msg_on_msg_recv(sdk);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_MSG_CRYPY_NOT_SUPPORTED);
}

void asd_msg_on_msg_recv_send_error_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.enc_bit = 1; // error should be generated from this.
    FakeSendFunctionResult = ST_ERR;
    // there is no error returned from this, but just make sure the code
    // doesnt blow up.
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_MSG_CRYPY_NOT_SUPPORTED);
}

void asd_msg_on_msg_recv_agent_control_num_messages_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = NUM_IN_FLIGHT_MESSAGES_SUPPORTED_CMD;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.buffer[0], NUM_IN_FLIGHT_MESSAGES_SUPPORTED_CMD);
    assert_int_equal(msg_sent.buffer[1], NUM_IN_FLIGHT_BUFFERS_TO_USE);
    assert_int_equal(msg_sent.header.size_lsb, 2);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_agent_control_result_send_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = NUM_IN_FLIGHT_MESSAGES_SUPPORTED_CMD;
    FakeSendFunctionResult = ST_ERR;
    // there is no error returned from this, but just make sure the code
    // doesnt blow up.
    asd_msg_on_msg_recv(*state);
}

void asd_msg_on_msg_recv_agent_control_max_data_size_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = MAX_DATA_SIZE_CMD;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.buffer[0], MAX_DATA_SIZE_CMD);
    assert_int_equal(msg_sent.buffer[1], (MAX_DATA_SIZE)&0xFF);
    assert_int_equal(msg_sent.buffer[2], (MAX_DATA_SIZE >> 8) & 0xFF);
    assert_int_equal(msg_sent.header.size_lsb, 3);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_agent_control_supported_jtag_chains_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = SUPPORTED_JTAG_CHAINS_CMD;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.buffer[0], SUPPORTED_JTAG_CHAINS_CMD);
    assert_int_equal(msg_sent.buffer[1], SUPPORTED_JTAG_CHAINS);
    assert_int_equal(msg_sent.header.size_lsb, 2);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_agent_control_supported_i2c_buses_i2c_disabled_test(
    void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = SUPPORTED_I2C_BUSES_CMD;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.buffer[0], SUPPORTED_I2C_BUSES_CMD);
    assert_int_equal(msg_sent.buffer[1], 0);
    assert_int_equal(msg_sent.header.size_lsb, 2);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_agent_control_supported_i2c_buses_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t expected_bus = 6;
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = SUPPORTED_I2C_BUSES_CMD;
    sdk->i2c_handler->config->enable_i2c = true;
    sdk->i2c_handler->config->default_bus = expected_bus;
    sdk->asd_cfg->i2c.allowed_buses[expected_bus] = true;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.buffer[0], SUPPORTED_I2C_BUSES_CMD);
    assert_int_equal(msg_sent.buffer[1], 1);
    assert_int_equal(msg_sent.buffer[2], expected_bus);
    assert_int_equal(msg_sent.header.size_lsb, 3);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_agent_control_downstream_version_test(void** state)
{
    ASD_MSG* sdk = (*state);
    char version[] = "ASD_BMC_v1.4.3_<wht-0.38-22-gf0d9505>";
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = OBTAIN_DOWNSTREAM_VERSION_CMD;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.buffer[0], OBTAIN_DOWNSTREAM_VERSION_CMD);
    assert_memory_equal(msg_sent.buffer + 1, version, sizeof(version));
    assert_int_equal(msg_sent.header.size_lsb, sizeof(version));
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_agent_control_missing_byte_failure_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = AGENT_CONFIGURATION_CMD;
    sdk->in_msg.msg.header.size_lsb = 0;
    // *** we are missing the 1st data byte. it should not crash ***
    asd_msg_on_msg_recv(sdk);

    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = AGENT_CONFIG_TYPE_LOGGING;
    // *** we are missing the 2nd data byte. it should not crash ***
    asd_msg_on_msg_recv(sdk);

    sdk->in_msg.msg.buffer[0] = AGENT_CONFIG_TYPE_GPIO;
    asd_msg_on_msg_recv(sdk);

    sdk->in_msg.msg.buffer[0] = AGENT_CONFIG_TYPE_JTAG_SETTINGS;
    asd_msg_on_msg_recv(sdk);
}

void asd_msg_on_msg_recv_i2c_init_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(I2C_TYPE, &sdk->in_msg.msg);
    sdk->handlers_initialized = false;
    sdk->i2c_handler->config->enable_i2c = true;
    command_index = 0;
    command_result[0] = ST_OK;
    command_result[1] = ST_ERR;
    expect_any(__wrap_JTAG_initialize, state);
    expect_any(__wrap_JTAG_initialize, sw_mode);
    expect_any(__wrap_i2c_initialize, state);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.size_lsb, 0);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_INIT_I2C_HANDLER);
}

void asd_msg_on_msg_recv_agent_control_logging_config_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    unsigned char expected = 3;
    sdk->in_msg.msg.header.cmd_stat = AGENT_CONFIGURATION_CMD;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = AGENT_CONFIG_TYPE_LOGGING;
    sdk->in_msg.msg.buffer[1] = expected;
    asd_msg_on_msg_recv(sdk);
    assert_int_equal(msg_sent.buffer[0], AGENT_CONFIGURATION_CMD);
    assert_int_equal(msg_sent.header.size_lsb, 1);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
    assert_int_equal(sdk->asd_cfg->remote_logging.value, expected);
}

void asd_msg_on_msg_recv_agent_control_gpio_config_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    unsigned char expected = 5;
    sdk->in_msg.msg.header.cmd_stat = AGENT_CONFIGURATION_CMD;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = AGENT_CONFIG_TYPE_GPIO;
    sdk->in_msg.msg.buffer[1] = expected;
    asd_msg_on_msg_recv(sdk);
    assert_int_equal(msg_sent.buffer[0], AGENT_CONFIGURATION_CMD);
    assert_int_equal(msg_sent.header.size_lsb, 1);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
    // gpio not yet supported. At this point we wont assert anything else.
}

void asd_msg_on_msg_recv_agent_control_jtag_driver_sw_mode_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    unsigned char expected = 0;
    sdk->in_msg.msg.header.cmd_stat = AGENT_CONFIGURATION_CMD;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = AGENT_CONFIG_TYPE_JTAG_SETTINGS;
    sdk->in_msg.msg.buffer[1] = expected;
    asd_msg_on_msg_recv(sdk);
    assert_int_equal(msg_sent.buffer[0], AGENT_CONFIGURATION_CMD);
    assert_int_equal(msg_sent.header.size_lsb, 1);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
    assert_int_equal(sdk->asd_cfg->jtag.mode, JTAG_DRIVER_MODE_SOFTWARE);
}

void asd_msg_on_msg_recv_agent_control_jtag_driver_hw_mode_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    unsigned char expected = 1;
    sdk->in_msg.msg.header.cmd_stat = AGENT_CONFIGURATION_CMD;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = AGENT_CONFIG_TYPE_JTAG_SETTINGS;
    sdk->in_msg.msg.buffer[1] = expected;
    asd_msg_on_msg_recv(sdk);
    assert_int_equal(msg_sent.buffer[0], AGENT_CONFIGURATION_CMD);
    assert_int_equal(msg_sent.header.size_lsb, 1);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
    assert_int_equal(sdk->asd_cfg->jtag.mode, JTAG_DRIVER_MODE_HARDWARE);
}

void asd_msg_on_msg_recv_agent_control_jtag_single_chain_mode_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    unsigned char expected = 0;
    sdk->in_msg.msg.header.cmd_stat = AGENT_CONFIGURATION_CMD;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = AGENT_CONFIG_TYPE_JTAG_SETTINGS;
    sdk->in_msg.msg.buffer[1] = expected;
    asd_msg_on_msg_recv(sdk);
    assert_int_equal(msg_sent.buffer[0], AGENT_CONFIGURATION_CMD);
    assert_int_equal(msg_sent.header.size_lsb, 1);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
    assert_int_equal(sdk->jtag_chain_mode, JTAG_CHAIN_SELECT_MODE_SINGLE);
}

void asd_msg_on_msg_recv_agent_control_jtag_multi_chain_mode_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    unsigned char expected = 2;
    sdk->in_msg.msg.header.cmd_stat = AGENT_CONFIGURATION_CMD;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = AGENT_CONFIG_TYPE_JTAG_SETTINGS;
    sdk->in_msg.msg.buffer[1] = expected;
    asd_msg_on_msg_recv(sdk);
    assert_int_equal(msg_sent.buffer[0], AGENT_CONFIGURATION_CMD);
    assert_int_equal(msg_sent.header.size_lsb, 1);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
    assert_int_equal(sdk->jtag_chain_mode, JTAG_CHAIN_SELECT_MODE_MULTI);
}

void asd_msg_on_msg_recv_agent_control_unsupported_command_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 78; // unknown command
    asd_msg_on_msg_recv(*state);
    // unsupported agent control commands are simply ignored. success
    // returned.
    assert_int_equal(msg_sent.buffer[0], 78);
    assert_int_equal(msg_sent.header.size_lsb, 1);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_send_response_failure_test(void** state)
{
    ASD_MSG* sdk = (*state);
    FakeSendFunctionResult = ST_ERR; // this will cause send to fail
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    unsigned char expected = 1;
    sdk->in_msg.msg.header.cmd_stat = AGENT_CONFIGURATION_CMD;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = AGENT_CONFIG_TYPE_JTAG_SETTINGS;
    sdk->in_msg.msg.buffer[1] = expected;
    asd_msg_on_msg_recv(sdk);
    // with this error, it just logs and moves on.
    // Well just ensure it doesnt crash...
}

void asd_msg_on_msg_recv_jtag_init_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->handlers_initialized = false;
    command_index = 0;
    command_result[0] = ST_ERR;
    expect_any(__wrap_JTAG_initialize, state);
    expect_any(__wrap_JTAG_initialize, sw_mode);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.size_lsb, 0);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_INIT_JTAG_HANDLER);
}

void asd_msg_on_msg_recv_target_init_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->handlers_initialized = false;
    command_index = 0;
    command_result[0] = ST_OK;
    command_result[1] = ST_ERR;
    command_result[2] = ST_OK;
    expect_any(__wrap_JTAG_initialize, state);
    expect_any(__wrap_JTAG_initialize, sw_mode);
    expect_any(__wrap_target_initialize, state);
    expect_any(__wrap_JTAG_deinitialize, state);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.size_lsb, 0);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_XDP_PRESENT);
}

void asd_msg_on_msg_recv_target_init_failed_cleanup_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->handlers_initialized = false;
    command_index = 0;
    command_result[0] = ST_OK;
    command_result[1] = ST_ERR;
    command_result[2] = ST_ERR;
    expect_any(__wrap_JTAG_initialize, state);
    expect_any(__wrap_JTAG_initialize, sw_mode);
    expect_any(__wrap_target_initialize, state);
    expect_any(__wrap_JTAG_deinitialize, state);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.size_lsb, 0);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_XDP_PRESENT);
}

void asd_msg_on_msg_recv_jtag_init_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->handlers_initialized = false;
    command_index = 0;
    expect_any(__wrap_JTAG_initialize, state);
    expect_any(__wrap_JTAG_initialize, sw_mode);
    expect_any(__wrap_target_initialize, state);
    command_result[0] = ST_OK;
    command_result[1] = ST_OK;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.size_lsb, 0);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_unsupported_type_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(66, &sdk->in_msg.msg); // random unsupported type
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.size_lsb, 0);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_MSG_NOT_SUPPORTED);
}

void asd_msg_on_msg_recv_unsupported_cmd_stat_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 66; // unknown cmd_stat
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.size_lsb, 0);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_MSG_NOT_SUPPORTED);
}

void asd_msg_on_msg_recv_process_jtag_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb =
        0x1f; // invalid size should generate jtag processing error
    sdk->in_msg.msg.header.size_lsb = 0xff;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_event_cfg_no_data_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = WRITE_EVENT_CONFIG;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_event_cfg_unkown_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WRITE_EVENT_CONFIG;
    sdk->in_msg.msg.buffer[1] = 55;
    asd_msg_on_msg_recv(*state);
    // these failures are ignored
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_event_cfg_failure_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WRITE_EVENT_CONFIG;
    sdk->in_msg.msg.buffer[1] = WRITE_CONFIG_MIN + 1;

    expect_any(__wrap_target_write_event_config, state);
    expect_value(__wrap_target_write_event_config, event_cfg,
                 WRITE_CONFIG_MIN + 1);
    expect_value(__wrap_target_write_event_config, enable, false);
    command_result[0] = ST_ERR;
    command_index = 0;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_event_cfg_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WRITE_EVENT_CONFIG;
    sdk->in_msg.msg.buffer[1] = WRITE_CONFIG_MIN + 1;

    expect_any(__wrap_target_write_event_config, state);
    expect_value(__wrap_target_write_event_config, event_cfg,
                 WRITE_CONFIG_MIN + 1);
    expect_value(__wrap_target_write_event_config, enable, false);
    command_result[0] = ST_OK;
    command_index = 0;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_cfg_tck_64_test(void** state)
{
    uint8_t prescaleVal = 0, divisorVal = 0;
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = JTAG_FREQ;
    sdk->in_msg.msg.buffer[1] =
        (unsigned char)(prescaleVal << 5 | divisorVal & 0x1f);
    JTAG_SET_JTAG_TCK_RESULT = ST_OK;
    expect_any(__wrap_JTAG_set_jtag_tck, state);
    expect_value(__wrap_JTAG_set_jtag_tck, tck, 64);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_cfg_tck_1_test(void** state)
{
    uint8_t prescaleVal = 0, divisorVal = 1;
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = JTAG_FREQ;
    sdk->in_msg.msg.buffer[1] =
        (unsigned char)(prescaleVal << 5 | divisorVal & 0x1f);
    JTAG_SET_JTAG_TCK_RESULT = ST_OK;
    expect_any(__wrap_JTAG_set_jtag_tck, state);
    expect_value(__wrap_JTAG_set_jtag_tck, tck, 1);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_cfg_tck_2_test(void** state)
{
    uint8_t prescaleVal = 1, divisorVal = 1;
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = JTAG_FREQ;
    sdk->in_msg.msg.buffer[1] =
        (unsigned char)(prescaleVal << 5 | divisorVal & 0x1f);
    JTAG_SET_JTAG_TCK_RESULT = ST_OK;
    expect_any(__wrap_JTAG_set_jtag_tck, state);
    expect_value(__wrap_JTAG_set_jtag_tck, tck, 2);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_cfg_tck_4_test(void** state)
{
    uint8_t prescaleVal = 2, divisorVal = 1;
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = JTAG_FREQ;
    sdk->in_msg.msg.buffer[1] =
        (unsigned char)(prescaleVal << 5 | divisorVal & 0x1f);
    JTAG_SET_JTAG_TCK_RESULT = ST_OK;
    expect_any(__wrap_JTAG_set_jtag_tck, state);
    expect_value(__wrap_JTAG_set_jtag_tck, tck, 4);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_cfg_tck_8_test(void** state)
{
    uint8_t prescaleVal = 3, divisorVal = 1;
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = JTAG_FREQ;
    sdk->in_msg.msg.buffer[1] =
        (unsigned char)(prescaleVal << 5 | divisorVal & 0x1f);
    JTAG_SET_JTAG_TCK_RESULT = ST_OK;
    expect_any(__wrap_JTAG_set_jtag_tck, state);
    expect_value(__wrap_JTAG_set_jtag_tck, tck, 8);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_cfg_tck_invalid_prescale_test(void** state)
{
    uint8_t prescaleVal = 0xff, divisorVal = 1;
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = JTAG_FREQ;
    sdk->in_msg.msg.buffer[1] =
        (unsigned char)(prescaleVal << 5 | divisorVal & 0x1f);
    JTAG_SET_JTAG_TCK_RESULT = ST_OK;
    expect_any(__wrap_JTAG_set_jtag_tck, state);
    // prescale should revert to 1
    expect_value(__wrap_JTAG_set_jtag_tck, tck, 1);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_cfg_tck_set_failed_test(void** state)
{
    uint8_t prescaleVal = 0, divisorVal = 1;
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = JTAG_FREQ;
    sdk->in_msg.msg.buffer[1] =
        (unsigned char)(prescaleVal << 5 | divisorVal & 0x1f);
    JTAG_SET_JTAG_TCK_RESULT = ST_ERR;
    expect_any(__wrap_JTAG_set_jtag_tck, state);
    expect_value(__wrap_JTAG_set_jtag_tck, tck, 1);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_cfg_jtag_tck_invalid_packet_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = JTAG_FREQ;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_cfg_dr_prefix_invalid_packet_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = DR_PREFIX;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_cfg_dr_prefix_set_padding_failed_test(
    void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    uint8_t expected = 2;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = DR_PREFIX;
    sdk->in_msg.msg.buffer[1] = expected;
    JTAG_SET_PADDING_RESULT = ST_ERR;
    expect_any(__wrap_JTAG_set_padding, state);
    expect_value(__wrap_JTAG_set_padding, padding, JTAGPaddingTypes_DRPost);
    expect_value(__wrap_JTAG_set_padding, value, expected);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_cfg_dr_prefix_set_padding_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    uint8_t expected = 2;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = DR_PREFIX;
    sdk->in_msg.msg.buffer[1] = expected;
    JTAG_SET_PADDING_RESULT = ST_OK;
    expect_any(__wrap_JTAG_set_padding, state);
    expect_value(__wrap_JTAG_set_padding, padding, JTAGPaddingTypes_DRPost);
    expect_value(__wrap_JTAG_set_padding, value, expected);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_cfg_dr_postfix_invalid_packet_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = DR_POSTFIX;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_cfg_dr_postfix_set_padding_failed_test(
    void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    uint8_t expected = 44;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = DR_POSTFIX;
    sdk->in_msg.msg.buffer[1] = expected;
    JTAG_SET_PADDING_RESULT = ST_ERR;
    expect_any(__wrap_JTAG_set_padding, state);
    expect_value(__wrap_JTAG_set_padding, padding, JTAGPaddingTypes_DRPre);
    expect_value(__wrap_JTAG_set_padding, value, expected);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_cfg_dr_postfix_set_padding_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    uint8_t expected = 44;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = DR_POSTFIX;
    sdk->in_msg.msg.buffer[1] = expected;
    JTAG_SET_PADDING_RESULT = ST_OK;
    expect_any(__wrap_JTAG_set_padding, state);
    expect_value(__wrap_JTAG_set_padding, padding, JTAGPaddingTypes_DRPre);
    expect_value(__wrap_JTAG_set_padding, value, expected);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_cfg_ir_prefix_invalid_packet_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = IR_PREFIX;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_cfg_ir_prefix_set_padding_failed_test(
    void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    uint8_t expected = 33;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 3;
    sdk->in_msg.msg.buffer[0] = IR_PREFIX;
    sdk->in_msg.msg.buffer[1] = expected;
    sdk->in_msg.msg.buffer[2] = expected;
    JTAG_SET_PADDING_RESULT = ST_ERR;
    expect_any(__wrap_JTAG_set_padding, state);
    expect_value(__wrap_JTAG_set_padding, padding, JTAGPaddingTypes_IRPost);
    expect_value(__wrap_JTAG_set_padding, value, (expected << 8) | expected);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_cfg_ir_prefix_set_padding_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    uint8_t expected = 33;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 3;
    sdk->in_msg.msg.buffer[0] = IR_PREFIX;
    sdk->in_msg.msg.buffer[1] = expected;
    sdk->in_msg.msg.buffer[2] = expected;
    JTAG_SET_PADDING_RESULT = ST_OK;
    expect_any(__wrap_JTAG_set_padding, state);
    expect_value(__wrap_JTAG_set_padding, padding, JTAGPaddingTypes_IRPost);
    expect_value(__wrap_JTAG_set_padding, value, (expected << 8) | expected);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_cfg_ir_postfix_invalid_packet_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = IR_POSTFIX;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_cfg_ir_postfix_set_padding_failed_test(
    void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    uint8_t expected = 33;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 3;
    sdk->in_msg.msg.buffer[0] = IR_POSTFIX;
    sdk->in_msg.msg.buffer[1] = expected;
    sdk->in_msg.msg.buffer[2] = expected;
    JTAG_SET_PADDING_RESULT = ST_ERR;
    expect_any(__wrap_JTAG_set_padding, state);
    expect_value(__wrap_JTAG_set_padding, padding, JTAGPaddingTypes_IRPre);
    expect_value(__wrap_JTAG_set_padding, value, (expected << 8) | expected);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_cfg_ir_postfix_set_padding_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    uint8_t expected = 33;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 3;
    sdk->in_msg.msg.buffer[0] = IR_POSTFIX;
    sdk->in_msg.msg.buffer[1] = expected;
    sdk->in_msg.msg.buffer[2] = expected;
    JTAG_SET_PADDING_RESULT = ST_OK;
    expect_any(__wrap_JTAG_set_padding, state);
    expect_value(__wrap_JTAG_set_padding, padding, JTAGPaddingTypes_IRPre);
    expect_value(__wrap_JTAG_set_padding, value, (expected << 8) | expected);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_cfg_prdy_timeout_invalid_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = PRDY_TIMEOUT;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_cfg_set_prdy_timeout_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    uint8_t expected = 33;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = PRDY_TIMEOUT;
    sdk->in_msg.msg.buffer[1] = expected;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
    assert_int_equal(expected, sdk->prdy_timeout);
}

void asd_msg_on_msg_recv_write_pins_no_data_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = WRITE_PINS;
    // it's not implemented yet so just make sure this succeeds
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_pins_failure_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WRITE_PINS;
    sdk->in_msg.msg.buffer[1] = PIN_PWRGOOD;

    expect_any(__wrap_target_write, state);
    expect_value(__wrap_target_write, pin, PIN_PWRGOOD);
    expect_value(__wrap_target_write, assert, false);
    command_result[0] = ST_ERR;
    command_index = 0;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_pins_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WRITE_PINS;
    sdk->in_msg.msg.buffer[1] = PIN_PWRGOOD;

    expect_any(__wrap_target_write, state);
    expect_value(__wrap_target_write, pin, PIN_PWRGOOD);
    expect_value(__wrap_target_write, assert, false);
    command_result[0] = ST_OK;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_chain_select_invalid_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WRITE_PINS;
    // we only support chains 0 and 1
    sdk->in_msg.msg.buffer[1] = SCAN_CHAIN_SELECT + 2;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_chain_select_target_write_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WRITE_PINS;
    sdk->in_msg.msg.buffer[1] = SCAN_CHAIN_SELECT + SCAN_CHAIN_1;
    command_index = 0;

    expect_any(__wrap_target_write, state);
    expect_value(__wrap_target_write, pin, PIN_TCK_MUX_SELECT);
    expect_value(__wrap_target_write, assert, true);
    command_result[0] = ST_ERR;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_chain_select_set_active_chain_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WRITE_PINS;
    sdk->in_msg.msg.buffer[1] = SCAN_CHAIN_SELECT + SCAN_CHAIN_1;
    command_index = 0;

    expect_any(__wrap_target_write, state);
    expect_value(__wrap_target_write, pin, PIN_TCK_MUX_SELECT);
    expect_value(__wrap_target_write, assert, true);
    command_result[0] = ST_OK;

    expect_any(__wrap_JTAG_set_active_chain, state);
    expect_value(__wrap_JTAG_set_active_chain, chain, SCAN_CHAIN_1);
    command_result[1] = ST_ERR;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_chain_select_set_multichain_null_data_test(
    void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WRITE_PINS;
    sdk->in_msg.msg.buffer[1] = SCAN_CHAIN_SELECT;
    sdk->jtag_chain_mode = JTAG_CHAIN_SELECT_MODE_MULTI;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_chain_select_set_multichain_unsuported(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 5;
    sdk->in_msg.msg.buffer[0] = WRITE_PINS;
    sdk->in_msg.msg.buffer[1] = SCAN_CHAIN_SELECT + 2;
    sdk->in_msg.msg.buffer[2] = 0xFF;
    sdk->in_msg.msg.buffer[3] = 0xFF;
    sdk->in_msg.msg.buffer[4] = 0xFF;
    sdk->jtag_chain_mode = JTAG_CHAIN_SELECT_MODE_MULTI;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_unexpected_write_pins_index_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WRITE_PINS;
    sdk->in_msg.msg.buffer[1] = 191;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_chain_select_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WRITE_PINS;
    sdk->in_msg.msg.buffer[1] = SCAN_CHAIN_SELECT + SCAN_CHAIN_1;
    command_index = 0;

    expect_any(__wrap_target_write, state);
    expect_value(__wrap_target_write, pin, PIN_TCK_MUX_SELECT);
    expect_value(__wrap_target_write, assert, true);
    command_result[0] = ST_OK;

    expect_any(__wrap_JTAG_set_active_chain, state);
    expect_value(__wrap_JTAG_set_active_chain, chain, SCAN_CHAIN_1);
    command_result[1] = ST_OK;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_read_status_no_data_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = READ_STATUS_MIN;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_read_status_response_buffer_full_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0x10;
    sdk->in_msg.msg.header.size_lsb = 0x0a;
    int message_size = get_message_size(&sdk->in_msg.msg);
    int result_index = 0;

    TARGET_READ_IGNORE = true;

    unsigned char num_bits = 64;
    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    JTAG_SHIFT_RESULT = ST_OK;
    tdo_bytes = 8;
    tdo_index = 0;
    for (int j = 0; j < tdo_bytes; j++)
        tdo[j] = 1;

    for (int i = 0; i < message_size; i++)
    {
        if (i == 0)
        {
            // Its not possible to cause this error with read_status
            // calls alone. So we will send one read_scan call
            // to put us into a situation where more data is being
            // requested, than was sent.
            sdk->in_msg.msg.buffer[i++] = (unsigned char)(READ_SCAN_MIN);

            expect_any(__wrap_JTAG_get_tap_state, state);
            expect_any(__wrap_JTAG_get_tap_state, tap_state);

            expect_any(__wrap_JTAG_shift, state);
            expect_value(__wrap_JTAG_shift, number_of_bits, num_bits);
            expect_any(__wrap_JTAG_shift, input_bytes);
            expect_any(__wrap_JTAG_shift, input);
            expect_any(__wrap_JTAG_shift, output_bytes);
            expect_any(__wrap_JTAG_shift, output);
            expect_any(__wrap_JTAG_shift, end_tap_state);

            sdk->in_msg.msg.buffer[i] = TAP_STATE_MIN + jtag_cap_ir;
            expect_any(__wrap_JTAG_set_tap_state, state);
            expect_any(__wrap_JTAG_set_tap_state, tap_state);
            JTAG_SET_TAP_STATE_RESULT = ST_OK;
        }
        else
        {
            sdk->in_msg.msg.buffer[i++] = READ_STATUS_MIN + 1;
            sdk->in_msg.msg.buffer[i] = PIN_PWRGOOD;
            command_result[result_index++] = ST_OK;
        }
    }
    command_index = 0;

    dummy = 0;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_read_status_target_read_failure_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = READ_STATUS_MIN + 1;
    sdk->in_msg.msg.buffer[1] = PIN_PWRGOOD;

    expect_any(__wrap_target_read, state);
    expect_value(__wrap_target_read, pin, PIN_PWRGOOD);
    expect_any(__wrap_target_read, asserted);
    command_result[0] = ST_ERR;
    command_index = 0;
    TARGET_READ_ASSERTED = true;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_read_status_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = READ_STATUS_MIN + 1;
    sdk->in_msg.msg.buffer[1] = PIN_PWRGOOD;

    expect_any(__wrap_target_read, state);
    expect_value(__wrap_target_read, pin, PIN_PWRGOOD);
    expect_any(__wrap_target_read, asserted);
    command_result[0] = ST_OK;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_wait_cycles_invalid_packet_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = WAIT_CYCLES_TCK_DISABLE;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_wait_cycles_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    uint8_t expected = 12;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WAIT_CYCLES_TCK_ENABLE;
    sdk->in_msg.msg.buffer[1] = expected;

    expect_any(__wrap_JTAG_wait_cycles, state);
    expect_value(__wrap_JTAG_wait_cycles, number_of_cycles, expected);
    JTAG_WAIT_CYCLES_RESULT = ST_ERR;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_wait_cycles_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WAIT_CYCLES_TCK_ENABLE;
    sdk->in_msg.msg.buffer[1] = 0; // 256 is expressed as 0

    expect_any(__wrap_JTAG_wait_cycles, state);
    expect_value(__wrap_JTAG_wait_cycles, number_of_cycles, 256);
    JTAG_WAIT_CYCLES_RESULT = ST_OK;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_wait_prdy_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    uint8_t expected = 4;
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = WAIT_PRDY;
    sdk->prdy_timeout = expected;

    expect_any(__wrap_target_wait_PRDY, state);
    expect_value(__wrap_target_wait_PRDY, log2time, expected);
    command_result[0] = ST_ERR;
    command_index = 0;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_wait_prdy_test(void** state)
{
    ASD_MSG* sdk = (*state);
    uint8_t expected = 24;
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = WAIT_PRDY;
    sdk->prdy_timeout = expected;

    expect_any(__wrap_target_wait_PRDY, state);
    expect_value(__wrap_target_wait_PRDY, log2time, expected);
    command_result[0] = ST_OK;
    command_index = 0;

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_clear_timeout_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = CLEAR_TIMEOUT;
    // it's not implemented yet so just make sure this succeeds
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_tap_reset_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = TAP_RESET;

    expect_any(__wrap_JTAG_tap_reset, state);
    JTAG_TAP_RESET_RESULT = ST_ERR;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_tap_reset_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = TAP_RESET;

    expect_any(__wrap_JTAG_tap_reset, state);
    JTAG_TAP_RESET_RESULT = ST_OK;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_wait_sync_invalid_packet_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WAIT_SYNC;
    sdk->in_msg.msg.buffer[1] = 1; // missing 3 bytes
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_wait_sync_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    uint16_t timeout = 12526;
    uint16_t delay = 2034;
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 5;
    sdk->in_msg.msg.buffer[0] = WAIT_SYNC;
    sdk->in_msg.msg.buffer[1] = (unsigned char)(timeout & 0xFF);
    sdk->in_msg.msg.buffer[2] = (unsigned char)((timeout & 0xFF00) >> 8);
    sdk->in_msg.msg.buffer[3] = (unsigned char)(delay & 0xFF);
    sdk->in_msg.msg.buffer[4] = (unsigned char)((delay & 0xFF00) >> 8);

    expect_any(__wrap_target_wait_sync, state);
    expect_value(__wrap_target_wait_sync, timeout, timeout);
    expect_value(__wrap_target_wait_sync, delay, delay);
    command_result[0] = ST_ERR;
    command_index = 0;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_wait_sync_timeout_test(void** state)
{
    ASD_MSG* sdk = (*state);
    uint16_t timeout = 12526;
    uint16_t delay = 2034;
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 5;
    sdk->in_msg.msg.buffer[0] = WAIT_SYNC;
    sdk->in_msg.msg.buffer[1] = (unsigned char)(timeout & 0xFF);
    sdk->in_msg.msg.buffer[2] = (unsigned char)((timeout & 0xFF00) >> 8);
    sdk->in_msg.msg.buffer[3] = (unsigned char)(delay & 0xFF);
    sdk->in_msg.msg.buffer[4] = (unsigned char)((delay & 0xFF00) >> 8);

    expect_any(__wrap_target_wait_sync, state);
    expect_value(__wrap_target_wait_sync, timeout, timeout);
    expect_value(__wrap_target_wait_sync, delay, delay);
    command_result[0] = ST_TIMEOUT;
    command_index = 0;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_wait_sync_test(void** state)
{
    ASD_MSG* sdk = (*state);
    uint16_t timeout = 12526;
    uint16_t delay = 2034;
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 5;
    sdk->in_msg.msg.buffer[0] = WAIT_SYNC;
    sdk->in_msg.msg.buffer[0] = WAIT_SYNC;
    sdk->in_msg.msg.buffer[1] = (unsigned char)(timeout & 0xFF);
    sdk->in_msg.msg.buffer[2] = (unsigned char)((timeout & 0xFF00) >> 8);
    sdk->in_msg.msg.buffer[3] = (unsigned char)(delay & 0xFF);
    sdk->in_msg.msg.buffer[4] = (unsigned char)((delay & 0xFF00) >> 8);

    expect_any(__wrap_target_wait_sync, state);
    expect_value(__wrap_target_wait_sync, timeout, timeout);
    expect_value(__wrap_target_wait_sync, delay, delay);
    command_result[0] = ST_OK;
    command_index = 0;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_set_tap_state_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = TAP_STATE_MIN + jtag_ex1_dr;

    expect_any(__wrap_JTAG_set_tap_state, state);
    expect_value(__wrap_JTAG_set_tap_state, tap_state, jtag_ex1_dr);
    JTAG_SET_TAP_STATE_RESULT = ST_ERR;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_set_tap_state_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = TAP_STATE_MIN + jtag_cap_ir;

    expect_any(__wrap_JTAG_set_tap_state, state);
    expect_value(__wrap_JTAG_set_tap_state, tap_state, jtag_cap_ir);
    JTAG_SET_TAP_STATE_RESULT = ST_OK;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_write_scan_invalid_packet_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] =
        WRITE_SCAN_MIN + 22; // add invalid length not in packet

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_scan_determine_shift_end_state_failed_test(
    void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = WRITE_SCAN_MIN + 1;
    sdk->in_msg.msg.buffer[1] = 1;

    JTAG_GET_TAP_STATE_RESULT = ST_ERR;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_scan_jtag_shift_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    unsigned char expected_num_bits = 1;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] =
        (unsigned char)(WRITE_SCAN_MIN + expected_num_bits);
    sdk->in_msg.msg.buffer[1] = 1;

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    JTAG_SHIFT_RESULT = ST_ERR;
    expect_any(__wrap_JTAG_shift, state);
    expect_value(__wrap_JTAG_shift, number_of_bits, expected_num_bits);
    expect_any(__wrap_JTAG_shift, input_bytes);
    expect_any(__wrap_JTAG_shift, input);
    expect_value(__wrap_JTAG_shift, output_bytes, 0);
    expect_any(__wrap_JTAG_shift, output);
    expect_any(__wrap_JTAG_shift, end_tap_state);

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_write_scan_jtag_shift_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    unsigned char expected_num_bits = 1;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] =
        (unsigned char)(WRITE_SCAN_MIN + expected_num_bits);
    sdk->in_msg.msg.buffer[1] = 1;

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    JTAG_SHIFT_RESULT = ST_OK;
    expect_any(__wrap_JTAG_shift, state);
    expect_value(__wrap_JTAG_shift, number_of_bits, expected_num_bits);
    expect_any(__wrap_JTAG_shift, input_bytes);
    expect_any(__wrap_JTAG_shift, input);
    expect_value(__wrap_JTAG_shift, output_bytes, 0);
    expect_any(__wrap_JTAG_shift, output);
    expect_any(__wrap_JTAG_shift, end_tap_state);

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_read_scan_determine_shift_end_state_failed_test(
    void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = READ_SCAN_MIN + 1;

    JTAG_GET_TAP_STATE_RESULT = ST_ERR;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_read_scan_jtag_shift_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    unsigned char expected_num_bits = 1;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] =
        (unsigned char)(READ_SCAN_MIN + expected_num_bits);

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    JTAG_SHIFT_RESULT = ST_ERR;
    expect_any(__wrap_JTAG_shift, state);
    expect_value(__wrap_JTAG_shift, number_of_bits, expected_num_bits);
    expect_any(__wrap_JTAG_shift, input_bytes);
    expect_any(__wrap_JTAG_shift, input);
    expect_any(__wrap_JTAG_shift, output_bytes);
    expect_any(__wrap_JTAG_shift, output);
    expect_any(__wrap_JTAG_shift, end_tap_state);

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_read_scan_jtag_shift_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    unsigned char expected_num_bits = 1;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] =
        (unsigned char)(READ_SCAN_MIN + expected_num_bits);

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    JTAG_SHIFT_RESULT = ST_OK;
    tdo_bytes = 1;
    tdo_index = 0;
    tdo[0] = 1;
    expect_any(__wrap_JTAG_shift, state);
    expect_value(__wrap_JTAG_shift, number_of_bits, expected_num_bits);
    expect_any(__wrap_JTAG_shift, input_bytes);
    expect_any(__wrap_JTAG_shift, input);
    expect_any(__wrap_JTAG_shift, output_bytes);
    expect_any(__wrap_JTAG_shift, output);
    expect_any(__wrap_JTAG_shift, end_tap_state);

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
    assert_int_equal(msg_sent.header.size_lsb, 2);
    assert_int_equal(msg_sent.buffer[0],
                     (unsigned char)(READ_SCAN_MIN + expected_num_bits));
    assert_int_equal(msg_sent.buffer[1], 1);
}

void asd_msg_on_msg_recv_read_scan_response_buffer_full_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    unsigned char num_bits = 64;
    unsigned char num_bits_sdk_encoded = 0;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 1;
    sdk->in_msg.msg.header.size_lsb = 201;
    int message_size = get_message_size(&sdk->in_msg.msg);

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    JTAG_SHIFT_RESULT = ST_OK;
    tdo_bytes = 8;
    tdo_index = 0;
    for (int j = 0; j < tdo_bytes; j++)
        tdo[j] = 1;

    IGNORE_CHECKS = true;
    for (int i = 0; i < message_size; i++)
    {
        sdk->in_msg.msg.buffer[i] =
            (unsigned char)(READ_SCAN_MIN + num_bits_sdk_encoded);
    }

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_read_scan_jtag_shift_64_bits_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    unsigned char expected_num_bits = 64; // 0 is 64
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = (unsigned char)(READ_SCAN_MIN + 0);

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    JTAG_SHIFT_RESULT = ST_OK;
    tdo_bytes = 1;
    tdo_index = 0;
    tdo[0] = 1;
    expect_any(__wrap_JTAG_shift, state);
    expect_value(__wrap_JTAG_shift, number_of_bits, expected_num_bits);
    expect_any(__wrap_JTAG_shift, input_bytes);
    expect_any(__wrap_JTAG_shift, input);
    expect_any(__wrap_JTAG_shift, output_bytes);
    expect_any(__wrap_JTAG_shift, output);
    expect_any(__wrap_JTAG_shift, end_tap_state);

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
    assert_int_equal(msg_sent.header.size_lsb, 9);
    assert_int_equal(msg_sent.buffer[0], (unsigned char)(READ_SCAN_MIN + 0));
    assert_int_equal(msg_sent.buffer[1], 1);
}

void asd_msg_on_msg_recv_read_write_scan_invalid_packet_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] =
        READ_WRITE_SCAN_MIN + 44; // add invalid length not in packet

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_read_write_scan_determine_shift_end_state_failed_test(
    void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] = READ_WRITE_SCAN_MIN + 1;
    sdk->in_msg.msg.buffer[1] = 1;

    JTAG_GET_TAP_STATE_RESULT = ST_ERR;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_read_write_scan_jtag_shift_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    unsigned char expected_num_bits = 1;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] =
        (unsigned char)(READ_WRITE_SCAN_MIN + expected_num_bits);
    sdk->in_msg.msg.buffer[1] = 1;

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    JTAG_SHIFT_RESULT = ST_ERR;
    expect_any(__wrap_JTAG_shift, state);
    expect_value(__wrap_JTAG_shift, number_of_bits, expected_num_bits);
    expect_any(__wrap_JTAG_shift, input_bytes);
    expect_any(__wrap_JTAG_shift, input);
    expect_any(__wrap_JTAG_shift, output_bytes);
    expect_any(__wrap_JTAG_shift, output);
    expect_any(__wrap_JTAG_shift, end_tap_state);

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_read_write_scan_jtag_shift_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    unsigned char expected_num_bits = 1;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] =
        (unsigned char)(READ_WRITE_SCAN_MIN + expected_num_bits);
    sdk->in_msg.msg.buffer[1] = 1;

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    JTAG_SHIFT_RESULT = ST_OK;
    tdo_bytes = 1;
    tdo_index = 0;
    tdo[0] = 1;
    expect_any(__wrap_JTAG_shift, state);
    expect_value(__wrap_JTAG_shift, number_of_bits, expected_num_bits);
    expect_any(__wrap_JTAG_shift, input_bytes);
    expect_any(__wrap_JTAG_shift, input);
    expect_any(__wrap_JTAG_shift, output_bytes);
    expect_any(__wrap_JTAG_shift, output);
    expect_any(__wrap_JTAG_shift, end_tap_state);

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
    assert_int_equal(msg_sent.header.size_lsb, 2);
    assert_int_equal(msg_sent.buffer[0],
                     (unsigned char)(READ_WRITE_SCAN_MIN + expected_num_bits));
    assert_int_equal(msg_sent.buffer[1], 1);
}

void asd_msg_on_msg_recv_read_write_scan_response_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    unsigned char expected_num_bits = 1;
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.buffer[0] =
        (unsigned char)(READ_WRITE_SCAN_MIN + expected_num_bits);
    sdk->in_msg.msg.buffer[1] = 1;

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    JTAG_SHIFT_RESULT = ST_OK;
    tdo_bytes = 1;
    tdo_index = 0;
    tdo[0] = 1;
    expect_any(__wrap_JTAG_shift, state);
    expect_value(__wrap_JTAG_shift, number_of_bits, expected_num_bits);
    expect_any(__wrap_JTAG_shift, input_bytes);
    expect_any(__wrap_JTAG_shift, input);
    expect_any(__wrap_JTAG_shift, output_bytes);
    expect_any(__wrap_JTAG_shift, output);
    expect_any(__wrap_JTAG_shift, end_tap_state);

    FakeSendFunctionResult = ST_ERR;
    asd_msg_on_msg_recv(*state);
}

// sorry this is such a long test.
// it is basically mimicking a response that is too large for the rw scan
// in the future, it may be easier to programmatically set the
// max response size to something smaller for testing so that not so much effort
// needs to go into the test
void asd_msg_on_msg_recv_read_write_scan_response_buffer_full_test(void** state)
{
    ASD_MSG* sdk = (*state);
    unsigned char num_bits = 64;
    unsigned char num_bits_sdk_encoded = 0;
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 1;
    sdk->in_msg.msg.header.size_lsb = 210;
    int message_size = get_message_size(&sdk->in_msg.msg);

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    JTAG_SHIFT_RESULT = ST_OK;
    tdo_bytes = 8;
    tdo_index = 0;
    for (int j = 0; j < tdo_bytes; j++)
        tdo[j] = 1;

    IGNORE_CHECKS = true;
    for (int i = 0; i < message_size; i++)
    {
        if ((i + 10) < message_size)
        {
            sdk->in_msg.msg.buffer[i] =
                (unsigned char)(READ_SCAN_MIN + num_bits_sdk_encoded);
        }
        else
        {
            JTAG_SET_TAP_STATE_RESULT = ST_OK;
            sdk->in_msg.msg.buffer[i++] =
                TAP_STATE_MIN + jtag_rti; // random other jtag state to get
                                          // past check end state checks
            sdk->in_msg.msg.buffer[i++] =
                (unsigned char)(READ_WRITE_SCAN_MIN + num_bits_sdk_encoded);
            sdk->in_msg.msg.buffer[i++] = 1; // tdi
            sdk->in_msg.msg.buffer[i++] = 1; // tdi
            sdk->in_msg.msg.buffer[i++] = 1; // tdi
            sdk->in_msg.msg.buffer[i++] = 1; // tdi
            sdk->in_msg.msg.buffer[i++] = 1; // tdi
            sdk->in_msg.msg.buffer[i++] = 1; // tdi
            sdk->in_msg.msg.buffer[i++] = 1; // tdi
            sdk->in_msg.msg.buffer[i] = 1;   // tdi
        }
    }

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void asd_msg_on_msg_recv_unknown_command_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.buffer[0] = 0x1f; // unknown command

    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_JTAG_MSG);
}

void send_error_message_invalid_param_test(void** state)
{
    ASD_MSG* sdk = (*state);
    struct asd_message msg;
    send_error_message(NULL, &msg, 0);
    send_error_message(sdk, NULL, 0);
}

void should_remote_log_test(void** state)
{
    ASD_MSG* sdk = (*state);
    sdk->asd_cfg->remote_logging.logging_level = IPC_LogType_Off;
    assert_false(sdk->should_remote_log(ASD_LogLevel_Error, ASD_LogStream_All));

    sdk->asd_cfg->remote_logging.logging_level = IPC_LogType_Error;
    assert_false(sdk->should_remote_log(ASD_LogLevel_Trace, ASD_LogStream_All));

    sdk->asd_cfg->remote_logging.logging_level = ASD_LogLevel_Warning;
    assert_false(sdk->should_remote_log(ASD_LogLevel_Trace, ASD_LogStream_All));

    sdk->asd_cfg->remote_logging.logging_level = ASD_LogLevel_Info;
    assert_false(sdk->should_remote_log(ASD_LogLevel_Trace, ASD_LogStream_All));

    sdk->asd_cfg->remote_logging.logging_level = ASD_LogLevel_Debug;
    assert_false(sdk->should_remote_log(ASD_LogLevel_Trace, ASD_LogStream_All));

    sdk->asd_cfg->remote_logging.logging_level = ASD_LogLevel_Trace;
    assert_true(sdk->should_remote_log(ASD_LogLevel_Trace, ASD_LogStream_All));
}

void send_remote_log_message_invalid_param_test(void** state)
{
    ASD_MSG* sdk = (*state);
    sdk->send_remote_logging_message(ASD_LogLevel_Error, ASD_LogStream_All,
                                     NULL);
}

void send_remote_log_message_test(void** state)
{
    ASD_MSG* sdk = (*state);
    char* msg = "this is a test msg";
    sdk->send_remote_logging_message(ASD_LogLevel_Error, ASD_LogStream_All,
                                     msg);

    assert_int_equal(msg_sent.buffer[0], AGENT_CONFIGURATION_CMD);
    assert_int_equal(msg_sent.buffer[1], 4);
    // check the actual log message is present
    assert_string_equal(&msg_sent.buffer[2], msg);
    assert_int_equal(msg_sent.header.size_lsb, 2 + strlen(msg));
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void send_remote_log_message_concatenated_test(void** state)
{
    ASD_MSG* sdk = (*state);
    char data = '0';
    int cmd_hdr_sz = 2;
    char msg[MAX_DATA_SIZE * 2]; // going to be too big
    char expected[MAX_DATA_SIZE];
    for (int i = 0; i < sizeof(msg); i++)
    {
        msg[i] = data;
        if (i < MAX_DATA_SIZE - cmd_hdr_sz)
            expected[i] = data;
        else if (i < MAX_DATA_SIZE)
            expected[i] = 0;
    }
    sdk->send_remote_logging_message(ASD_LogLevel_Error, ASD_LogStream_All,
                                     msg);

    assert_int_equal(msg_sent.buffer[0], AGENT_CONFIGURATION_CMD);
    assert_int_equal(msg_sent.buffer[1], 4);
    // check the actual log message is present
    assert_string_equal(&msg_sent.buffer[cmd_hdr_sz], expected);
    u_int32_t buffer_length = (u_int32_t)MAX_DATA_SIZE;
    assert_int_equal(msg_sent.header.size_lsb,
                     lsb_from_msg_size(buffer_length));
    assert_int_equal(msg_sent.header.size_msb,
                     msb_from_msg_size(buffer_length));
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void determine_shift_end_state_invalid_param_test(void** state)
{
    ASD_MSG* sdk = *state;
    struct packet_data data;
    enum jtag_states end_state;

    assert_int_equal(ST_ERR,
                     determine_shift_end_state(NULL, 0, &data, &end_state));
    assert_int_equal(ST_ERR,
                     determine_shift_end_state(sdk, 0, NULL, &end_state));
    assert_int_equal(ST_ERR, determine_shift_end_state(sdk, 0, &data, NULL));
}

void determine_shift_end_state_get_jtag_state_failed_test(void** state)
{
    ASD_MSG* sdk = *state;
    struct packet_data data;
    enum jtag_states end_state;

    JTAG_GET_TAP_STATE_RESULT = ST_ERR;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    assert_int_equal(ST_ERR,
                     determine_shift_end_state(sdk, 0, &data, &end_state));
}

void determine_shift_end_state_end_of_packet_test(void** state)
{
    ASD_MSG* sdk = *state;
    sdk->asd_cfg->jtag.mode = JTAG_DRIVER_MODE_SOFTWARE;
    struct packet_data data;
    enum jtag_states end_state = jtag_sel_dr;
    enum jtag_states expected_state = jtag_sel_ir;

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    JTAG_STATE = expected_state;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    assert_int_equal(ST_OK,
                     determine_shift_end_state(sdk, 0, &data, &end_state));
    assert_int_equal(expected_state, end_state);
}

void determine_shift_end_state_hw_mode_jtag_pau_dr_test(void** state)
{
    ASD_MSG* sdk = *state;
    sdk->asd_cfg->jtag.mode = JTAG_DRIVER_MODE_HARDWARE;
    struct packet_data data;
    enum jtag_states end_state = jtag_shf_dr;
    enum jtag_states expected_state = jtag_pau_dr;

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    JTAG_STATE = expected_state;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    unsigned char test_data[2] = {TAP_STATE_MIN + jtag_pau_ir,
                                  TAP_STATE_MIN + expected_state};
    data.next_data = (unsigned char*)&test_data;
    data.total = 2;
    data.used = 0;

    assert_int_equal(ST_OK,
                     determine_shift_end_state(sdk, 0, &data, &end_state));
    assert_int_equal(expected_state, end_state);
    // ensure data struct is untouched
    assert_int_equal(2, data.total);
    assert_int_equal(0, data.used);
}

void determine_shift_end_state_hw_mode_jtag_pau_ir_test(void** state)
{
    ASD_MSG* sdk = *state;
    sdk->asd_cfg->jtag.mode = JTAG_DRIVER_MODE_HARDWARE;
    struct packet_data data;
    enum jtag_states end_state = jtag_shf_dr;
    enum jtag_states expected_state = jtag_pau_ir;

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    JTAG_STATE = expected_state;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    unsigned char test_data[2] = {TAP_STATE_MIN + jtag_pau_ir,
                                  TAP_STATE_MIN + expected_state};
    data.next_data = (unsigned char*)&test_data;
    data.total = 2;
    data.used = 0;

    assert_int_equal(ST_OK,
                     determine_shift_end_state(sdk, 0, &data, &end_state));
    assert_int_equal(expected_state, end_state);
    // ensure data struct is untouched
    assert_int_equal(2, data.total);
    assert_int_equal(0, data.used);
}

void determine_shift_end_state_next_is_tap_state_cmd_test(void** state)
{
    ASD_MSG* sdk = *state;
    struct packet_data data;
    enum jtag_states end_state = jtag_shf_dr;
    enum jtag_states expected_state = jtag_pau_dr;

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    JTAG_STATE = expected_state;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    unsigned char test_data[1] = {TAP_STATE_MIN + jtag_pau_dr};
    data.next_data = (unsigned char*)&test_data;
    data.total = 1;
    data.used = 0;

    assert_int_equal(ST_OK,
                     determine_shift_end_state(sdk, 0, &data, &end_state));
    assert_int_equal(expected_state, end_state);
    // ensure data struct is untouched
    assert_int_equal(1, data.total);
    assert_int_equal(0, data.used);
}

void determine_shift_end_state_next_is_not_read_scan_test(void** state)
{
    ASD_MSG* sdk = *state;
    struct packet_data data;
    enum jtag_states end_state = jtag_shf_dr;
    enum jtag_states expected_state = jtag_pau_dr;

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    JTAG_STATE = expected_state;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    unsigned char test_data[1] = {(READ_SCAN_MAX + 1)}; // not a read scan
    data.next_data = (unsigned char*)&test_data;
    data.total = 1;
    data.used = 0;

    assert_int_equal(ST_ERR, determine_shift_end_state(sdk, ScanType_Read,
                                                       &data, &end_state));

    // ensure data struct is untouched
    assert_int_equal(1, data.total);
    assert_int_equal(0, data.used);
}

void determine_shift_end_state_next_is_not_write_scan_test(void** state)
{
    ASD_MSG* sdk = *state;
    struct packet_data data;
    enum jtag_states end_state = jtag_shf_dr;
    enum jtag_states expected_state = jtag_pau_dr;

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    JTAG_STATE = expected_state;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    unsigned char test_data[1] = {(WRITE_SCAN_MAX + 1)}; // not a read scan
    data.next_data = (unsigned char*)&test_data;
    data.total = 1;
    data.used = 0;

    assert_int_equal(ST_ERR, determine_shift_end_state(sdk, ScanType_Write,
                                                       &data, &end_state));

    // ensure data struct is untouched
    assert_int_equal(1, data.total);
    assert_int_equal(0, data.used);
}

void determine_shift_end_state_next_is_not_readwrite_scan_test(void** state)
{
    ASD_MSG* sdk = *state;
    struct packet_data data;
    enum jtag_states end_state = jtag_shf_dr;
    enum jtag_states expected_state = jtag_pau_dr;

    JTAG_GET_TAP_STATE_RESULT = ST_OK;
    JTAG_STATE = expected_state;
    expect_any(__wrap_JTAG_get_tap_state, state);
    expect_any(__wrap_JTAG_get_tap_state, tap_state);

    unsigned char test_data[1] = {(READ_WRITE_SCAN_MIN - 1)}; // not a read scan
    data.next_data = (unsigned char*)&test_data;
    data.total = 1;
    data.used = 0;

    assert_int_equal(ST_ERR, determine_shift_end_state(sdk, ScanType_ReadWrite,
                                                       &data, &end_state));

    // ensure data struct is untouched
    assert_int_equal(1, data.total);
    assert_int_equal(0, data.used);
}

void asd_msg_read_invalid_params_test(void** state)
{
    ASD_MSG* sdk = *state;
    bool pending = false;
    char fake_conn[9];

    assert_int_equal(ST_ERR, asd_msg_read(NULL, &fake_conn, &pending));
    assert_int_equal(ST_ERR, asd_msg_read(sdk, NULL, &pending));
    assert_int_equal(ST_ERR, asd_msg_read(sdk, &fake_conn, NULL));
}

void asd_msg_read_header_read_failure_test(void** state)
{
    ASD_MSG* sdk = *state;
    bool pending = false;
    char fake_conn[9];

    expect_any(FakeReadFunctionPtr, state);
    expect_any(FakeReadFunctionPtr, connection);
    expect_any(FakeReadFunctionPtr, buffer);
    expect_any(FakeReadFunctionPtr, num_to_read);
    expect_any(FakeReadFunctionPtr, data_pending);
    FakeReadFunctionResult = ST_ERR;

    assert_int_equal(ST_ERR, asd_msg_read(sdk, &fake_conn, &pending));
}

void asd_msg_read_invalid_size_test(void** state)
{
    ASD_MSG* sdk = *state;
    bool pending = false;
    char fake_conn[9];

    expect_any(FakeReadFunctionPtr, state);
    expect_any(FakeReadFunctionPtr, connection);
    FakeReadFunctionBuffer[0] = 0xff;
    FakeReadFunctionBuffer[1] = 0xff;
    FakeReadFunctionBuffer[2] = 0xff;
    FakeReadFunctionBuffer[3] = 0xff;
    expect_any(FakeReadFunctionPtr, buffer);
    expect_any(FakeReadFunctionPtr, num_to_read);
    FakeReadFunctionDataPending = false;
    expect_any(FakeReadFunctionPtr, data_pending);
    FakeReadFunctionResult = ST_OK;
    FakeReadFunctionNumBytesPerRead = -1;
    FakeReadFunctionNumBytesReadIndex = 0;

    assert_int_equal(ST_ERR, asd_msg_read(sdk, &fake_conn, &pending));
}

void asd_msg_read_header_not_complete_test(void** state)
{
    ASD_MSG* sdk = *state;
    bool pending = false;
    char fake_conn[9];

    // we will read 4 times
    expect_any_count(FakeReadFunctionPtr, state, 4);
    expect_any_count(FakeReadFunctionPtr, connection, 4);
    FakeReadFunctionBuffer[0] = 0x00;
    FakeReadFunctionBuffer[1] = 0x00;
    FakeReadFunctionBuffer[2] = 0x00;
    FakeReadFunctionBuffer[3] = 0x00;
    expect_any_count(FakeReadFunctionPtr, buffer, 4);
    expect_any_count(FakeReadFunctionPtr, num_to_read, 4);
    FakeReadFunctionDataPending = false;
    expect_any_count(FakeReadFunctionPtr, data_pending, 4);
    FakeReadFunctionResult = ST_OK;
    FakeReadFunctionNumBytesPerRead = 1;
    FakeReadFunctionNumBytesReadIndex = 0;

    assert_int_equal(ST_OK, asd_msg_read(sdk, &fake_conn, &pending));
    assert_int_equal(ST_OK, asd_msg_read(sdk, &fake_conn, &pending));
    assert_int_equal(ST_OK, asd_msg_read(sdk, &fake_conn, &pending));
    assert_int_equal(ST_OK, asd_msg_read(sdk, &fake_conn, &pending));
}

void asd_msg_read_header_only_success_test(void** state)
{
    ASD_MSG* sdk = *state;
    bool pending = false;
    char fake_conn[9];

    expect_any(FakeReadFunctionPtr, state);
    expect_any(FakeReadFunctionPtr, connection);
    FakeReadFunctionBuffer[0] = 0;
    FakeReadFunctionBuffer[1] = 0;
    FakeReadFunctionBuffer[2] = 0;
    FakeReadFunctionBuffer[3] = 0;
    expect_any(FakeReadFunctionPtr, buffer);
    expect_any(FakeReadFunctionPtr, num_to_read);
    FakeReadFunctionDataPending = false;
    expect_any(FakeReadFunctionPtr, data_pending);
    FakeReadFunctionResult = ST_OK;
    FakeReadFunctionNumBytesPerRead = -1;
    FakeReadFunctionNumBytesReadIndex = 0;

    assert_int_equal(ST_OK, asd_msg_read(sdk, &fake_conn, &pending));
}

void asd_msg_read_header_and_buffer_success_test(void** state)
{
    ASD_MSG* sdk = *state;
    bool pending = false;
    char fake_conn[9];

    // header read
    expect_any(FakeReadFunctionPtr, state);
    expect_any(FakeReadFunctionPtr, connection);
    FakeReadFunctionBuffer[0] = AGENT_CONTROL_TYPE;
    FakeReadFunctionBuffer[1] = 2; // 2 byte buffer
    FakeReadFunctionBuffer[2] = 0;
    FakeReadFunctionBuffer[3] = AGENT_CONFIGURATION_CMD;
    expect_any(FakeReadFunctionPtr, buffer);
    expect_any(FakeReadFunctionPtr, num_to_read);
    FakeReadFunctionDataPending = false;
    expect_any(FakeReadFunctionPtr, data_pending);
    FakeReadFunctionResult = ST_OK;
    FakeReadFunctionNumBytesPerRead = -1;
    FakeReadFunctionNumBytesReadIndex = 0;
    assert_int_equal(ST_OK, asd_msg_read(sdk, &fake_conn, &pending));

    // buffer read
    expect_any_count(FakeReadFunctionPtr, state, 2);
    expect_any_count(FakeReadFunctionPtr, connection, 2);
    FakeReadFunctionBuffer[0] = AGENT_CONFIG_TYPE_JTAG_SETTINGS;
    FakeReadFunctionBuffer[1] = 1; // set to hw jtag mode.
    expect_any_count(FakeReadFunctionPtr, buffer, 2);
    expect_any_count(FakeReadFunctionPtr, num_to_read, 2);
    FakeReadFunctionDataPending = false;
    expect_any_count(FakeReadFunctionPtr, data_pending, 2);
    FakeReadFunctionResult = ST_OK;
    // set to 1 byte read to ensure retry works
    FakeReadFunctionNumBytesPerRead = 1;
    FakeReadFunctionNumBytesReadIndex = 0;
    // first 1 byte read
    assert_int_equal(ST_OK, asd_msg_read(sdk, &fake_conn, &pending));
    // second 1 byte read
    assert_int_equal(ST_OK, asd_msg_read(sdk, &fake_conn, &pending));

    assert_int_equal(JTAG_DRIVER_MODE_HARDWARE, sdk->asd_cfg->jtag.mode);
}

void asd_msg_read_header_and_buffer_data_pending_success_test(void** state)
{
    ASD_MSG* sdk = *state;
    bool pending = false;
    char fake_conn[9];
    sdk->asd_cfg->jtag.mode = JTAG_DRIVER_MODE_SOFTWARE;

    // header and buffer reads
    expect_any_count(FakeReadFunctionPtr, state, 2);
    expect_any_count(FakeReadFunctionPtr, connection, 2);
    expect_any_count(FakeReadFunctionPtr, buffer, 2);
    expect_any_count(FakeReadFunctionPtr, num_to_read, 2);
    FakeReadFunctionDataPending = false;
    expect_any_count(FakeReadFunctionPtr, data_pending, 2);
    FakeReadFunctionBuffer[0] = AGENT_CONTROL_TYPE;
    FakeReadFunctionBuffer[1] = 2; // 2 byte buffer
    FakeReadFunctionBuffer[2] = 0;
    FakeReadFunctionBuffer[3] = AGENT_CONFIGURATION_CMD;
    FakeReadFunctionBuffer[4] = AGENT_CONFIG_TYPE_JTAG_SETTINGS;
    FakeReadFunctionBuffer[5] = 1; // set to hw jtag mode.
    FakeReadFunctionDataPending = true;
    FakeReadFunctionResult = ST_OK;
    FakeReadFunctionNumBytesPerRead = -1;
    FakeReadFunctionNumBytesReadIndex = 0;

    // read whole packet
    assert_int_equal(ST_OK, asd_msg_read(sdk, &fake_conn, &pending));

    assert_int_equal(JTAG_DRIVER_MODE_HARDWARE, sdk->asd_cfg->jtag.mode);
}

void asd_msg_read_break_from_asd_msg_on_msg_recv_test(void** state)
{
    ASD_MSG* sdk = *state;
    bool pending = false;
    char fake_conn[9];
    expect_any(FakeReadFunctionPtr, state);
    expect_any(FakeReadFunctionPtr, connection);
    expect_any(FakeReadFunctionPtr, buffer);
    expect_any(FakeReadFunctionPtr, num_to_read);
    expect_any(FakeReadFunctionPtr, data_pending);
    FakeReadFunctionResult = ST_OK;
    sdk->in_msg.read_state = READ_STATE_BUFFER;
    get_fake_message(1, &sdk->in_msg.msg);
    sdk->handlers_initialized = false;
    command_index = 0;
    command_result[0] = ST_ERR;
    command_result[1] = ST_ERR;
    expect_any(__wrap_JTAG_initialize, state);
    expect_any(__wrap_JTAG_initialize, sw_mode);
    assert_int_equal(ST_ERR, asd_msg_read(sdk, &fake_conn, &pending));
}

void asd_msg_read_bad_state_test(void** state)
{
    ASD_MSG* sdk = *state;
    bool pending = false;
    char fake_conn[9];

    sdk->in_msg.read_state = 99; // some invalid state.

    assert_int_equal(ST_ERR, asd_msg_read(sdk, &fake_conn, &pending));
}

void send_response_invalid_response_test(void** state)
{
    ASD_MSG* sdk = *state;
    struct asd_message message;

    assert_int_equal(ST_ERR, send_response(NULL, &message));
    assert_int_equal(ST_ERR, send_response(sdk, NULL));
    message.header.size_lsb = 0xff;
    message.header.size_msb = 0x1f;
    assert_int_equal(ST_ERR, send_response(sdk, &message));
}

void asd_msg_get_fds_invalid_params_test(void** state)
{
    ASD_MSG* sdk = *state;
    target_fdarr_t fds;
    int num_fds;

    assert_int_equal(ST_ERR, asd_msg_get_fds(NULL, &fds, &num_fds));
    assert_int_equal(ST_ERR, asd_msg_get_fds(sdk, NULL, &num_fds));
    assert_int_equal(ST_ERR, asd_msg_get_fds(sdk, &fds, NULL));
}

void asd_msg_get_fds_failure_test(void** state)
{
    ASD_MSG* sdk = *state;
    target_fdarr_t fds;
    int num_fds;

    expect_any(__wrap_target_get_fds, state);
    expect_any(__wrap_target_get_fds, fds);
    expect_any(__wrap_target_get_fds, num_fds);
    command_result[0] = ST_ERR;
    command_index = 0;

    assert_int_equal(ST_ERR, asd_msg_get_fds(sdk, &fds, &num_fds));
}

void asd_msg_get_fds_test(void** state)
{
    ASD_MSG* sdk = *state;
    target_fdarr_t fds;
    int num_fds;

    expect_any(__wrap_target_get_fds, state);
    expect_any(__wrap_target_get_fds, fds);
    expect_any(__wrap_target_get_fds, num_fds);
    command_result[0] = ST_OK;
    command_index = 0;

    assert_int_equal(ST_OK, asd_msg_get_fds(sdk, &fds, &num_fds));
}

void asd_msg_event_invalid_params_test(void** state)
{
    struct pollfd poll_fd;
    poll_fd.fd = 1;
    assert_int_equal(ST_ERR, asd_msg_event(NULL, poll_fd));
}

void asd_msg_event_target_event_error_test(void** state)
{
    ASD_MSG* sdk = *state;
    struct pollfd expected_fd;
    expected_fd.fd = 87;

    expect_any(__wrap_target_event, state);
    expect_value(__wrap_target_event, poll_fd.fd, expected_fd.fd);
    expect_any(__wrap_target_event, event);
    command_result[0] = ST_ERR;
    command_index = 0;

    assert_int_equal(ST_ERR, asd_msg_event(sdk, expected_fd));
}

void asd_msg_event_ASD_EVENT_XDP_PRESENT_test(void** state)
{
    ASD_MSG* sdk = *state;
    struct pollfd expected_fd;
    expected_fd.fd = 87;

    expect_any(__wrap_target_event, state);
    expect_value(__wrap_target_event, poll_fd.fd, expected_fd.fd);
    expect_any(__wrap_target_event, event);
    TARGET_EVENT_EVENT = ASD_EVENT_XDP_PRESENT;
    command_result[0] = ST_OK;
    command_index = 0;

    assert_int_equal(ST_ERR, asd_msg_event(sdk, expected_fd));
}

void asd_msg_event_ASD_EVENT_PRDY_EVENT_test(void** state)
{
    ASD_MSG* sdk = *state;
    struct pollfd expected_fd;
    expected_fd.fd = 87;

    expect_any(__wrap_target_event, state);
    expect_value(__wrap_target_event, poll_fd.fd, expected_fd.fd);
    expect_any(__wrap_target_event, event);
    TARGET_EVENT_EVENT = ASD_EVENT_PRDY_EVENT;
    command_result[0] = ST_OK;
    command_index = 0;

    assert_int_equal(ST_OK, asd_msg_event(sdk, expected_fd));

    assert_int_equal(msg_sent.buffer[0], ASD_EVENT_PRDY_EVENT);
    assert_int_equal(msg_sent.header.size_lsb, 1);
    assert_int_equal(msg_sent.header.size_msb, 0);
    assert_int_equal(msg_sent.header.type, JTAG_TYPE);
    assert_int_equal(msg_sent.header.tag, BROADCAST_MESSAGE_ORIGIN_ID);
    assert_int_equal(msg_sent.header.origin_id, BROADCAST_MESSAGE_ORIGIN_ID);
}

void asd_msg_event_send_failed_test(void** state)
{
    ASD_MSG* sdk = *state;
    struct pollfd expected_fd;
    expected_fd.fd = 87;

    expect_any(__wrap_target_event, state);
    expect_value(__wrap_target_event, poll_fd.fd, expected_fd.fd);
    expect_any(__wrap_target_event, event);
    TARGET_EVENT_EVENT = ASD_EVENT_PRDY_EVENT;
    command_result[0] = ST_OK;
    command_index = 0;

    FakeSendFunctionResult = ST_ERR;

    assert_int_equal(ST_ERR, asd_msg_event(sdk, expected_fd));
}

void process_i2c_messages_null_param_checks_test(void** state)
{
    ASD_MSG* sdk = *state;
    struct asd_message msg;
    assert_int_equal(process_i2c_messages(sdk, NULL), ST_ERR);
    assert_int_equal(process_i2c_messages(NULL, &msg), ST_ERR);
}

void process_i2c_messages_msg_builder_fail_test(void** state)
{
    ASD_MSG* sdk = *state;
    malloc_fail[0] = true;
    expect_value(__wrap_malloc, size, sizeof(I2C_Msg_Builder));
    malloc_fail_mode = true;
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_ERR);
    malloc_fail_mode = false;
}

void process_i2c_messages_invalid_in_msg_test(void** state)
{
    ASD_MSG* sdk = *state;
    // use a size that will cause get msg size to fail
    sdk->in_msg.msg.header.size_lsb = 0xff;
    sdk->in_msg.msg.header.size_msb = 0x1f;

    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_ERR);
}

void process_i2c_msg_handles_bus_select_command_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t expected_bus = 6;
    sdk->in_msg.msg.buffer[0] = I2C_WRITE_CFG_BUS_SELECT;
    sdk->in_msg.msg.buffer[1] = expected_bus;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.header.size_msb = 0;
    expect_value(__wrap_i2c_bus_select, state, sdk->i2c_handler);
    expect_value(__wrap_i2c_bus_select, bus, expected_bus);
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_OK);
    assert_int_equal(sdk->out_msg.header.size_lsb, 0);
    assert_int_equal(sdk->out_msg.header.size_msb, 0);
}

void process_i2c_msg_handles_invalid_bus_select_command_test(void** state)
{
    ASD_MSG* sdk = *state;
    sdk->in_msg.msg.buffer[0] = I2C_WRITE_CFG_BUS_SELECT;
    // missing bus byte, just 1 byte
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.header.size_msb = 0;
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_ERR);
}

void process_i2c_msg_handles_bus_select_command_error_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t expected_bus = 6;
    sdk->in_msg.msg.buffer[0] = I2C_WRITE_CFG_BUS_SELECT;
    sdk->in_msg.msg.buffer[1] = expected_bus;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.header.size_msb = 0;

    expect_value(__wrap_i2c_bus_select, state, sdk->i2c_handler);
    expect_value(__wrap_i2c_bus_select, bus, expected_bus);
    BUS_SELECT_STATUS = ST_ERR;
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_ERR);
}

void process_i2c_msg_handles_set_sclk_command_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint16_t expected_sclk = 300;
    sdk->in_msg.msg.buffer[0] = I2C_WRITE_CFG_SCLK;
    sdk->in_msg.msg.buffer[1] = (unsigned char)(expected_sclk & 0xff);
    sdk->in_msg.msg.buffer[2] = (unsigned char)(expected_sclk >> 8);
    sdk->in_msg.msg.header.size_lsb = 3;
    sdk->in_msg.msg.header.size_msb = 0;
    expect_value(__wrap_i2c_set_sclk, state, sdk->i2c_handler);
    expect_value(__wrap_i2c_set_sclk, sclk, expected_sclk);
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_OK);
}

void process_i2c_msg_handles_invalid_set_sclk_command_test(void** state)
{
    ASD_MSG* sdk = *state;
    sdk->in_msg.msg.buffer[0] = I2C_WRITE_CFG_SCLK;
    // missing bus byte, just 1 byte
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.header.size_msb = 0;
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_ERR);
}

void process_i2c_msg_handles_set_sclk_command_error_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint16_t expected_sclk = 300;
    sdk->in_msg.msg.buffer[0] = (unsigned char)I2C_WRITE_CFG_SCLK;
    sdk->in_msg.msg.buffer[1] = (unsigned char)(expected_sclk & 0xff);
    sdk->in_msg.msg.buffer[2] = (unsigned char)(expected_sclk >> 8);
    sdk->in_msg.msg.header.size_lsb = 3;
    sdk->in_msg.msg.header.size_msb = 0;

    expect_value(__wrap_i2c_set_sclk, state, sdk->i2c_handler);
    expect_value(__wrap_i2c_set_sclk, sclk, expected_sclk);
    SET_SCLK_STATUS = ST_ERR;
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_ERR);
}

void process_i2c_msg_handles_read_command_short_packet_error_test(void** state)
{
    ASD_MSG* sdk = *state;
    sdk->in_msg.msg.buffer[0] = (unsigned char)I2C_READ_MIN;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.header.size_msb = 0;
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_ERR);
}

void process_i2c_msg_handles_read_command_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t read_length = 2;
    uint8_t address = 22;
    uint8_t force_stop = 1;
    uint8_t cmd_byte = (unsigned char)I2C_READ_MIN + read_length;
    sdk->in_msg.msg.buffer[0] = cmd_byte;
    sdk->in_msg.msg.buffer[1] = (address << 1) + force_stop;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.header.size_msb = 0;
    READ_RESPONSE[0] = 55;
    READ_RESPONSE[1] = 77;
    expect_value(__wrap_i2c_read_write, state, sdk->i2c_handler);
    expect_any(__wrap_i2c_read_write, msg_set);
    READ_WRITE_STATUS = ST_OK;
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_OK);
    // 2 for cmd and ACKs byte
    assert_int_equal(sdk->out_msg.header.size_lsb, 2 + read_length);
    assert_int_equal(sdk->out_msg.header.size_msb, 0);
    assert_int_equal(sdk->out_msg.buffer[0], cmd_byte);
    // address ack + read ack
    assert_int_equal(sdk->out_msg.buffer[1], I2C_ADDRESS_ACK + read_length);
    // 1st byte read
    assert_int_equal(sdk->out_msg.buffer[2], READ_RESPONSE[0]);
    // 2nd byte read
    assert_int_equal(sdk->out_msg.buffer[3], READ_RESPONSE[1]);
}

void process_i2c_msg_handles_i2c_read_write_failed_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t read_length = 2;
    uint8_t address = 22;
    uint8_t force_stop = 1;
    uint8_t cmd_byte = (unsigned char)I2C_READ_MIN + read_length;
    sdk->in_msg.msg.buffer[0] = cmd_byte;
    sdk->in_msg.msg.buffer[1] = (address << 1) + force_stop;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.header.size_msb = 0;
    expect_value(__wrap_i2c_read_write, state, sdk->i2c_handler);
    expect_any(__wrap_i2c_read_write, msg_set);
    READ_WRITE_STATUS = ST_ERR;
    // a i2c read write failure is treated as a NAK
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_OK);
}

void process_i2c_msg_handles_write_command_short_packet_error_test(void** state)
{
    ASD_MSG* sdk = *state;
    sdk->in_msg.msg.buffer[0] = (unsigned char)I2C_WRITE_MIN;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.header.size_msb = 0;
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_ERR);
}

void process_i2c_msg_handles_write_command_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t write_length = 2;
    uint8_t address = 22;
    uint8_t force_stop = 1;
    uint8_t cmd_byte = (unsigned char)I2C_WRITE_MIN + write_length;
    sdk->in_msg.msg.buffer[0] = cmd_byte;
    sdk->in_msg.msg.buffer[1] = (address << 1) + force_stop;
    sdk->in_msg.msg.buffer[2] = 11;
    sdk->in_msg.msg.buffer[3] = 22;
    sdk->in_msg.msg.header.size_lsb = 4;
    sdk->in_msg.msg.header.size_msb = 0;
    expect_value(__wrap_i2c_read_write, state, sdk->i2c_handler);
    expect_any(__wrap_i2c_read_write, msg_set);
    READ_WRITE_STATUS = ST_OK;
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_OK);
    // 2 for cmd and ACKs byte
    assert_int_equal(sdk->out_msg.header.size_lsb, 2);
    assert_int_equal(sdk->out_msg.header.size_msb, 0);
    assert_int_equal(sdk->out_msg.buffer[0], cmd_byte);
    // address ack + write ack
    assert_int_equal(sdk->out_msg.buffer[1], I2C_ADDRESS_ACK + write_length);
}

void process_i2c_msg_handles_unknown_command_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t unknown_cmd_byte = 0;
    sdk->in_msg.msg.buffer[0] = unknown_cmd_byte;
    sdk->in_msg.msg.header.size_lsb = 1;
    sdk->in_msg.msg.header.size_msb = 0;

    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_ERR);
}

void process_i2c_msg_handles_multiple_commands_test(void** state)
{
    // first well fo a set clock rate command
    ASD_MSG* sdk = *state;
    uint16_t expected_sclk = 300;
    sdk->in_msg.msg.buffer[0] = (unsigned char)I2C_WRITE_CFG_SCLK;
    sdk->in_msg.msg.buffer[1] = (unsigned char)(expected_sclk & 0xff);
    sdk->in_msg.msg.buffer[2] = (unsigned char)(expected_sclk >> 8);
    sdk->in_msg.msg.header.size_lsb = 3;
    sdk->in_msg.msg.header.size_msb = 0;
    expect_value(__wrap_i2c_set_sclk, state, sdk->i2c_handler);
    expect_value(__wrap_i2c_set_sclk, sclk, expected_sclk);

    // then well do a read command
    uint8_t address = 22;
    uint8_t force_stop = 0;
    uint8_t read_length = 2;
    uint8_t read_cmd_byte = (unsigned char)I2C_READ_MIN + read_length;
    sdk->in_msg.msg.buffer[3] = read_cmd_byte;
    sdk->in_msg.msg.buffer[4] = (address << 1) + force_stop;
    sdk->in_msg.msg.header.size_lsb += 2;
    READ_RESPONSE[0] = 55;
    READ_RESPONSE[1] = 77;
    expect_value(__wrap_i2c_read_write, state, sdk->i2c_handler);
    expect_any(__wrap_i2c_read_write, msg_set);
    SET_SCLK_STATUS = ST_OK;
    BUS_SELECT_STATUS = ST_OK;
    READ_WRITE_STATUS = ST_OK;

    // then well do a write command
    uint8_t write_length = 4;
    uint8_t write_cmd_byte = (unsigned char)I2C_WRITE_MIN + write_length;
    sdk->in_msg.msg.buffer[5] = write_cmd_byte;
    sdk->in_msg.msg.buffer[6] = (address << 1) + force_stop;
    sdk->in_msg.msg.buffer[7] = 11;
    sdk->in_msg.msg.buffer[8] = 22;
    sdk->in_msg.msg.buffer[9] = 33;
    sdk->in_msg.msg.buffer[10] = 44;
    sdk->in_msg.msg.header.size_lsb += 6;

    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_OK);

    assert_int_equal(sdk->out_msg.header.size_lsb, 6);
    assert_int_equal(sdk->out_msg.header.size_msb, 0);

    // check the response for the read command
    assert_int_equal(sdk->out_msg.buffer[0], read_cmd_byte);
    // address ack + read ac
    assert_int_equal(sdk->out_msg.buffer[1], I2C_ADDRESS_ACK + read_length);
    assert_int_equal(sdk->out_msg.buffer[2], READ_RESPONSE[0]);
    assert_int_equal(sdk->out_msg.buffer[3], READ_RESPONSE[1]);

    // check the response for the write command
    assert_int_equal(sdk->out_msg.buffer[4], write_cmd_byte);
    // address ack + write ack
    assert_int_equal(sdk->out_msg.buffer[5], I2C_ADDRESS_ACK + write_length);
}

void process_i2c_msg_handles_i2c_msg_initialize_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t read_length = 2;
    uint8_t address = 22;
    uint8_t force_stop = 1;
    uint8_t cmd_byte = (unsigned char)I2C_READ_MIN + read_length;
    sdk->in_msg.msg.buffer[0] = cmd_byte;
    sdk->in_msg.msg.buffer[1] = (address << 1) + force_stop;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.header.size_msb = 0;
    READ_WRITE_STATUS = ST_ERR;
    malloc_fail[0] = false;
    malloc_fail[1] = true;
    expect_value(__wrap_malloc, size, sizeof(struct i2c_rdwr_ioctl_data));
    malloc_fail_mode = true;
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_ERR);
    malloc_fail_mode = false;
}

void do_read_command_i2c_msg_add_failed_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t read_length = 2;
    get_fake_message(I2C_TYPE, &sdk->in_msg.msg);
    uint8_t address = 22;
    bool force_stop = true;
    I2C_Msg_Builder* builder = NULL;
    uint8_t cmd_byte = (unsigned char)I2C_READ_MIN + read_length;
    sdk->in_msg.msg.buffer[0] = cmd_byte;
    sdk->in_msg.msg.buffer[1] = (address << 1) + force_stop;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.header.size_msb = 0;
    int size = get_message_size(&(sdk->in_msg.msg));
    struct packet_data packet;
    packet.next_data = (unsigned char*)&(sdk->in_msg.msg).buffer;
    packet.used = 0;
    packet.total = (unsigned int)size;
    assert_int_equal(do_read_command(0x01, builder, &packet, &force_stop),
                     ST_ERR);
    assert_int_equal(do_read_command(0x01, builder, &packet, NULL), ST_ERR);
}

void do_write_command_i2c_msg_add_failed_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t write_length = 2;
    uint8_t address = 22;
    bool force_stop = true;
    I2C_Msg_Builder* builder = NULL;
    uint8_t cmd_byte = (unsigned char)I2C_WRITE_MIN + write_length;
    sdk->in_msg.msg.buffer[0] = cmd_byte;
    sdk->in_msg.msg.buffer[1] = (address << 1) + force_stop;
    sdk->in_msg.msg.buffer[2] = 11;
    sdk->in_msg.msg.buffer[3] = 22;
    sdk->in_msg.msg.header.size_lsb = 4;
    sdk->in_msg.msg.header.size_msb = 0;
    READ_WRITE_STATUS = ST_OK;
    int size = get_message_size(&(sdk->in_msg.msg));
    struct packet_data packet;
    packet.next_data = (unsigned char*)&(sdk->in_msg.msg).buffer;
    packet.used = 0;
    packet.total = (unsigned int)size;
    assert_int_equal(do_write_command(0x01, builder, &packet, &force_stop),
                     ST_ERR);
    assert_int_equal(do_write_command(0x01, builder, &packet, NULL), ST_ERR);
}

void do_write_command_get_backet_data_failed_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t write_length = 2;
    uint8_t address = 22;
    bool force_stop = true;
    I2C_Msg_Builder* builder = NULL;
    uint8_t cmd_byte = (unsigned char)I2C_WRITE_MIN + write_length;
    sdk->in_msg.msg.buffer[0] = cmd_byte;
    sdk->in_msg.msg.buffer[1] = (address << 1) + force_stop;
    sdk->in_msg.msg.buffer[2] = 11;
    sdk->in_msg.msg.buffer[3] = 22;
    sdk->in_msg.msg.header.size_lsb = 4;
    sdk->in_msg.msg.header.size_msb = 0;
    READ_WRITE_STATUS = ST_OK;
    int size = get_message_size(&(sdk->in_msg.msg));
    struct packet_data packet;
    packet.next_data = (unsigned char*)&(sdk->in_msg.msg).buffer;
    packet.used = 3;
    packet.total = (unsigned int)size;
    assert_int_equal(do_write_command(0x01, builder, &packet, &force_stop),
                     ST_ERR);
}

void build_responses_i2c_msg_get_asd_i2c_msg_failed_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t read_length = 2;
    uint8_t address = 22;
    uint8_t force_stop = 1;
    uint8_t cmd_byte = (unsigned char)I2C_READ_MIN + read_length;
    sdk->in_msg.msg.buffer[0] = cmd_byte;
    sdk->in_msg.msg.buffer[1] = (address << 1) + force_stop;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.header.size_msb = 0;
    READ_RESPONSE[0] = 55;
    READ_RESPONSE[1] = 77;
    expect_value(__wrap_i2c_read_write, state, sdk->i2c_handler);
    expect_any(__wrap_i2c_read_write, msg_set);
    READ_WRITE_STATUS = ST_OK;
    I2C_MSG_GET_ASD_I2C_MSG_STATUS = ST_ERR;
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_ERR);
    I2C_MSG_GET_ASD_I2C_MSG_STATUS = ST_OK;
}

void process_i2c_msg_handles_i2c_msg_reset_failed_test(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t read_length = 2;
    uint8_t address = 22;
    uint8_t force_stop = 1;
    uint8_t cmd_byte = (unsigned char)I2C_READ_MIN + read_length;
    sdk->in_msg.msg.buffer[0] = cmd_byte;
    sdk->in_msg.msg.buffer[1] = (address << 1) + force_stop;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.header.size_msb = 0;
    READ_RESPONSE[0] = 55;
    READ_RESPONSE[1] = 77;
    expect_value(__wrap_i2c_read_write, state, sdk->i2c_handler);
    expect_any(__wrap_i2c_read_write, msg_set);
    READ_WRITE_STATUS = ST_OK;
    I2C_MSG_RESET_STATUS = ST_ERR;
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_ERR);
    I2C_MSG_RESET_STATUS = ST_OK;
}

void asd_msg_on_msg_recv_process_i2c_messages_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(I2C_TYPE, &sdk->in_msg.msg);
    FakeSendFunctionResult = ST_ERR;
    // there is no error returned from this, but just make sure the code
    // doesnt blow up.
    sdk->i2c_handler->config->enable_i2c = false;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_I2C_MSG_NOT_SUPPORTED);
}

void asd_msg_on_msg_recv_process_msg_process_i2c_msg_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(I2C_TYPE, &sdk->in_msg.msg);
    FakeSendFunctionResult = ST_ERR;
    // there is no error returned from this, but just make sure the code
    // doesnt blow up.
    sdk->i2c_handler->config->enable_i2c = true;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_I2C_MSG);
}

void asd_msg_on_msg_recv_process_msg_process_i2c_msg_failed_lock_test(
    void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(I2C_TYPE, &sdk->in_msg.msg);
    FakeSendFunctionResult = ST_ERR;
    // there is no error returned from this, but just make sure the code
    // doesnt blow up.
    sdk->i2c_handler->config->enable_i2c = true;
    command_index = 0;
    FLOCK_RESULT[0] = -1;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_PROCESS_I2C_LOCK);
}

void asd_msg_on_msg_recv_process_msg_i2c_msg_failed_unlock_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(I2C_TYPE, &sdk->in_msg.msg);
    FakeSendFunctionResult = ST_ERR;
    // there is no error returned from this, but just make sure the code
    // doesnt blow up.
    sdk->i2c_handler->config->enable_i2c = true;
    command_index = 0;
    FLOCK_RESULT[0] = 0;
    FLOCK_RESULT[1] = -1;
    process_message(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_FAILURE_REMOVE_I2C_LOCK);
}

void process_i2c_msg_handles_build_responses_failed_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(I2C_TYPE, &sdk->in_msg.msg);
    assert_int_equal(build_responses(sdk, &I2C_MSG_COUNT, NULL, true), ST_ERR);
}

void process_i2c_msg_handles_build_responses_buffer_full_failed_test(
    void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(I2C_TYPE, &sdk->in_msg.msg);
    FakeSendFunctionResult = ST_ERR;
    I2C_MSG_COUNT = 5000;
    I2C_Msg_Builder* builder = I2CMsgBuilder();
    I2C_MSG_RESET_STATUS = ST_OK;
    i2c_msg_get_count_mode = true;
    i2c_msg_get_asd_i2c_msg_mode = true;
    assert_int_equal(build_responses(sdk, &I2C_MSG_COUNT, builder, true),
                     ST_ERR);
    i2c_msg_get_count_mode = false;
    i2c_msg_get_asd_i2c_msg_mode = false;
}

void process_i2c_msg_handles_build_responses_buffer_full_test(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(I2C_TYPE, &sdk->in_msg.msg);
    FakeSendFunctionResult = ST_ERR;
    I2C_MSG_COUNT = 16; // 16 I2C Messages and each message is 0xff length
    I2C_Msg_Builder* builder = I2CMsgBuilder();
    I2C_MSG_RESET_STATUS = ST_OK;
    i2c_msg_get_count_mode = true;
    i2c_msg_get_asd_i2c_msg_mode = true;
    FakeSendFunctionResult = ST_OK;
    assert_int_equal(build_responses(sdk, &I2C_MSG_COUNT, builder, true),
                     ST_OK);
    i2c_msg_get_count_mode = false;
    i2c_msg_get_asd_i2c_msg_mode = false;
    FakeSendFunctionResult = ST_ERR;
}

void i2c_lock_unlock_test(void** state)
{
    ASD_MSG* sdk = *state;
    get_fake_message(I2C_TYPE, &sdk->in_msg.msg);
    sdk->i2c_handler->config->enable_i2c = true;
    FLOCK_RESULT[0] = 0;
    FLOCK_RESULT[1] = 0;
    process_message(sdk);
    command_index = 0;
    assert_int_equal(flock_i2c(sdk, LOCK_EX), ST_OK);
    assert_int_equal(flock_i2c(sdk, LOCK_UN), ST_OK);
    FLOCK_RESULT[0] = -1;
    FLOCK_RESULT[1] = -1;
    process_message(sdk);
    command_index = 0;
    assert_int_equal(flock_i2c(sdk, LOCK_EX), ST_ERR);
    assert_int_equal(flock_i2c(sdk, LOCK_UN), ST_ERR);
}

void asd_msg_on_msg_recv_agent_control_memcpy_safe_failure1(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = OBTAIN_DOWNSTREAM_VERSION_CMD;
    MEMCPY_SAFE_RESULT = 1;
    asd_msg_on_msg_recv(*state);
    assert_memory_not_equal(msg_sent.buffer + 1, asd_version,
                            sizeof(asd_version));
}

void asd_msg_on_msg_recv_agent_control_memcpy_safe_failure2(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(AGENT_CONTROL_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.enc_bit = 1;
    struct asd_message error_message;
    MEMCPY_SAFE_RESULT = 1;
    asd_msg_on_msg_recv(sdk);
    assert_memory_not_equal(&error_message.header, &sdk->in_msg.msg.header,
                            sizeof(struct message_header));
}

void asd_msg_on_msg_recv_agent_control_memcpy_safe_failure3(void** state)
{
    ASD_MSG* sdk = (*state);
    char* msg = "this is a test msg";
    MEMCPY_SAFE_RESULT = 1;
    sdk->send_remote_logging_message(ASD_LogLevel_Error, ASD_LogStream_All,
                                     msg);
    assert_int_not_equal(msg_sent.buffer[0], AGENT_CONFIGURATION_CMD);
    assert_int_not_equal(msg_sent.buffer[1], 4);
    // check the actual log message is present
    assert_string_not_equal(&msg_sent.buffer[2], msg);
}

void asd_msg_on_msg_recv_agent_control_memcpy_safe_failure4(void** state)
{
    ASD_MSG* sdk = (*state);
    get_fake_message(JTAG_TYPE, &sdk->in_msg.msg);
    sdk->in_msg.msg.header.cmd_stat = 0;
    sdk->in_msg.msg.header.size_msb = 0;
    sdk->in_msg.msg.header.size_lsb = 5;
    sdk->in_msg.msg.buffer[0] = WRITE_PINS;
    sdk->in_msg.msg.buffer[1] = SCAN_CHAIN_SELECT + 2;
    sdk->in_msg.msg.buffer[2] = 0xFF;
    sdk->in_msg.msg.buffer[3] = 0xFF;
    sdk->in_msg.msg.buffer[4] = 0xFF;
    sdk->jtag_chain_mode = JTAG_CHAIN_SELECT_MODE_MULTI;
    asd_msg_on_msg_recv(*state);
    assert_int_equal(msg_sent.header.cmd_stat, ASD_SUCCESS);
}

void asd_msg_on_msg_recv_agent_control_memcpy_safe_failure5(void** state)
{
    ASD_MSG* sdk = *state;
    uint8_t expected_bus = 6;
    sdk->in_msg.msg.buffer[0] = I2C_WRITE_CFG_BUS_SELECT;
    sdk->in_msg.msg.buffer[1] = expected_bus;
    sdk->in_msg.msg.header.size_lsb = 2;
    sdk->in_msg.msg.header.size_msb = 0;
    expect_value(__wrap_i2c_bus_select, state, sdk->i2c_handler);
    expect_value(__wrap_i2c_bus_select, bus, expected_bus);
    assert_int_equal(process_i2c_messages(sdk, &sdk->in_msg.msg), ST_OK);
}

void read_openbmc_version_null_failure_test(void** state)
{
    ASD_MSG* sdk = (ASD_MSG*)malloc(sizeof(ASD_MSG));
    OS_RELEASE_GOOD = 2;
    assert_int_equal(ST_ERR, read_openbmc_version(sdk));
}

void read_openbmc_version_read_minus_test(void** state)
{
    ASD_MSG* sdk = (ASD_MSG*)malloc(sizeof(ASD_MSG));
    OS_RELEASE_GOOD = 1;
    assert_int_equal(ST_ERR, read_openbmc_version(sdk));
}

void read_openbmc_version_read_memcpy_safe_failed_test(void** state)
{
    ASD_MSG* sdk = (ASD_MSG*)malloc(sizeof(ASD_MSG));
    OS_RELEASE_GOOD = 0;
    MEMCPY_SAFE_RESULT = 1;
    assert_int_equal(ST_ERR, read_openbmc_version(sdk));
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(asd_msg_init_invalid_param_test),
        cmocka_unit_test(asd_msg_init_malloc_failure_test),
        cmocka_unit_test(asd_msg_jtag_init_failed_test),
        cmocka_unit_test(asd_msg_init_test),
        cmocka_unit_test(asd_msg_init_only_one_instance_test),
        cmocka_unit_test(asd_msg_free_invalid_param_test),
        cmocka_unit_test(asd_msg_free_jtag_deinit_fail_test),
        cmocka_unit_test(asd_msg_free_target_deinit_fail_test),
        cmocka_unit_test(asd_msg_free_test),
        cmocka_unit_test(asd_msg_on_msg_recv_invalid_param_test),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_memcpy_safe_failure1, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_memcpy_safe_failure2, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_memcpy_safe_failure3, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_memcpy_safe_failure4, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_memcpy_safe_failure5, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_encrypted_check_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_send_error_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_num_messages_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_result_send_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_downstream_version_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_missing_byte_failure_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_logging_config_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_i2c_init_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_gpio_config_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_jtag_driver_sw_mode_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_jtag_driver_hw_mode_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_jtag_single_chain_mode_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_jtag_multi_chain_mode_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_chain_select_set_multichain_unsuported, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_unsupported_command_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_max_data_size_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_supported_jtag_chains_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_supported_i2c_buses_i2c_disabled_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_agent_control_supported_i2c_buses_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_send_response_failure_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_jtag_init_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_target_init_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_target_init_failed_cleanup_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(asd_msg_on_msg_recv_jtag_init_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_unsupported_type_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_unsupported_cmd_stat_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_process_jtag_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_event_cfg_no_data_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_event_cfg_unkown_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_event_cfg_failure_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_event_cfg_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_tck_64_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_tck_1_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_tck_2_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_tck_4_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_tck_8_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_tck_invalid_prescale_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_tck_set_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_jtag_tck_invalid_packet_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_dr_prefix_invalid_packet_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_dr_prefix_set_padding_failed_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_dr_prefix_set_padding_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_dr_postfix_invalid_packet_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_dr_postfix_set_padding_failed_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_dr_postfix_set_padding_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_ir_prefix_invalid_packet_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_ir_prefix_set_padding_failed_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_ir_prefix_set_padding_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_ir_postfix_invalid_packet_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_ir_postfix_set_padding_failed_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_ir_postfix_set_padding_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_prdy_timeout_invalid_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_cfg_set_prdy_timeout_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_pins_no_data_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_pins_failure_test, setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_on_msg_recv_write_pins_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_chain_select_invalid_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_unexpected_write_pins_index_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(asd_msg_on_msg_recv_chain_select_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_status_no_data_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_chain_select_target_write_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_chain_select_set_active_chain_failed_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_chain_select_set_multichain_null_data_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_status_response_buffer_full_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_status_target_read_failure_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(asd_msg_on_msg_recv_read_status_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_wait_cycles_invalid_packet_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_wait_cycles_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_on_msg_recv_wait_cycles_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_wait_prdy_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_on_msg_recv_wait_prdy_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_on_msg_recv_clear_timeout_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_tap_reset_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_on_msg_recv_tap_reset_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_wait_sync_invalid_packet_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_wait_sync_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_wait_sync_timeout_test, setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_on_msg_recv_wait_sync_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_set_tap_state_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_on_msg_recv_set_tap_state_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_scan_invalid_packet_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_scan_determine_shift_end_state_failed_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_scan_jtag_shift_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_write_scan_jtag_shift_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_write_scan_invalid_packet_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_scan_determine_shift_end_state_failed_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_scan_jtag_shift_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_scan_jtag_shift_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_scan_response_buffer_full_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_scan_jtag_shift_64_bits_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_write_scan_determine_shift_end_state_failed_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_write_scan_jtag_shift_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_write_scan_jtag_shift_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_write_scan_response_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_read_write_scan_response_buffer_full_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_unknown_command_test, setup, teardown),
        cmocka_unit_test_setup_teardown(send_error_message_invalid_param_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(should_remote_log_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(
            send_remote_log_message_invalid_param_test, setup, teardown),
        cmocka_unit_test_setup_teardown(send_remote_log_message_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(
            send_remote_log_message_concatenated_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            determine_shift_end_state_invalid_param_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            determine_shift_end_state_get_jtag_state_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            determine_shift_end_state_end_of_packet_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            determine_shift_end_state_hw_mode_jtag_pau_dr_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            determine_shift_end_state_hw_mode_jtag_pau_ir_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            determine_shift_end_state_next_is_tap_state_cmd_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            determine_shift_end_state_next_is_not_read_scan_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            determine_shift_end_state_next_is_not_write_scan_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            determine_shift_end_state_next_is_not_readwrite_scan_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(asd_msg_read_invalid_params_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(asd_msg_read_header_read_failure_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_read_invalid_size_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(asd_msg_read_header_not_complete_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_read_header_only_success_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_read_header_and_buffer_success_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_read_header_and_buffer_data_pending_success_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_read_break_from_asd_msg_on_msg_recv_test, setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_read_bad_state_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(send_response_invalid_response_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_get_fds_invalid_params_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_get_fds_failure_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(asd_msg_get_fds_test, setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_event_invalid_params_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_event_target_event_error_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_event_ASD_EVENT_XDP_PRESENT_test, setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_event_ASD_EVENT_PRDY_EVENT_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(asd_msg_event_send_failed_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_messages_null_param_checks_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_messages_msg_builder_fail_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_messages_invalid_in_msg_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_bus_select_command_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_invalid_bus_select_command_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_bus_select_command_error_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_set_sclk_command_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_invalid_set_sclk_command_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_set_sclk_command_error_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_read_command_short_packet_error_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_read_command_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_i2c_read_write_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_write_command_short_packet_error_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_write_command_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_unknown_command_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_multiple_commands_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_i2c_msg_initialize_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_i2c_msg_reset_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_build_responses_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_build_responses_buffer_full_failed_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            process_i2c_msg_handles_build_responses_buffer_full_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(do_read_command_i2c_msg_add_failed_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            do_write_command_i2c_msg_add_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            do_write_command_get_backet_data_failed_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            build_responses_i2c_msg_get_asd_i2c_msg_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_process_i2c_messages_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_process_msg_process_i2c_msg_failed_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_process_msg_process_i2c_msg_failed_lock_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            asd_msg_on_msg_recv_process_msg_i2c_msg_failed_unlock_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(i2c_lock_unlock_test, setup, teardown),
        cmocka_unit_test(read_openbmc_version_null_failure_test),
        cmocka_unit_test(read_openbmc_version_read_minus_test),
        cmocka_unit_test(read_openbmc_version_read_memcpy_safe_failed_test)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
