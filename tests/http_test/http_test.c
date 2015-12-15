#include <string.h>
#include <unity.h>

#include "errors.h"
#include "http.h"

#include "Mockdebug.h"
#include "Mockmchar.h"
#include "Mocksocklib.h"


static URLINFO urlinfo;

void setUp(void)
{
    Mockdebug_Init();
    Mockmchar_Init();
    Mocksocklib_Init();

    memset(&urlinfo, 0xCC, sizeof(urlinfo));
}

void tearDown(void)
{
    Mockdebug_Verify();
    Mockmchar_Verify();
    Mocksocklib_Verify();
}


static void test_parse_url_simple(void)
{
    debug_printf_Ignore();

    TEST_ASSERT_EQUAL(SR_SUCCESS, http_parse_url("http://host.com/path", &urlinfo));
    TEST_ASSERT_EQUAL_STRING("host.com", urlinfo.host);
    TEST_ASSERT_EQUAL_STRING("/path", urlinfo.path);
    TEST_ASSERT_EQUAL(80, urlinfo.port);
    TEST_ASSERT_EQUAL_STRING("", urlinfo.username);
    TEST_ASSERT_EQUAL_STRING("", urlinfo.password);
}


static void test_parse_url_with_path_and_port(void)
{
    debug_printf_Ignore();

    TEST_ASSERT_EQUAL(SR_SUCCESS, http_parse_url("http://host.com:8888/path", &urlinfo));
    TEST_ASSERT_EQUAL_STRING("host.com", urlinfo.host);
    TEST_ASSERT_EQUAL_STRING("/path", urlinfo.path);
    TEST_ASSERT_EQUAL(8888, urlinfo.port);
}


static void test_parse_url_with_port(void)
{
    debug_printf_Ignore();

    TEST_ASSERT_EQUAL(SR_SUCCESS, http_parse_url("http://host.com:8888", &urlinfo));
    TEST_ASSERT_EQUAL_STRING("host.com", urlinfo.host);
    TEST_ASSERT_EQUAL_STRING("/", urlinfo.path);
    TEST_ASSERT_EQUAL(8888, urlinfo.port);
}


static void test_parse_url_with_credentials(void)
{
    debug_printf_Ignore();

    TEST_ASSERT_EQUAL(SR_SUCCESS, http_parse_url("http://username:password@host.com/path", &urlinfo));
    TEST_ASSERT_EQUAL_STRING("host.com", urlinfo.host);
    TEST_ASSERT_EQUAL_STRING("/path", urlinfo.path);
    TEST_ASSERT_EQUAL(80, urlinfo.port);
    TEST_ASSERT_EQUAL_STRING("username", urlinfo.username);
    TEST_ASSERT_EQUAL_STRING("password", urlinfo.password);
}


/** for some servers the colon ist part of the uri */
static void test_parse_url_with_colon_in_uri(void)
{
    debug_printf_Ignore();

    const char * url = "http://host.com/path?value=foo:bar";
    TEST_ASSERT_EQUAL(SR_SUCCESS, http_parse_url(url, &urlinfo));
    TEST_ASSERT_EQUAL_STRING("host.com", urlinfo.host);
    TEST_ASSERT_EQUAL_STRING("/path?value=foo:bar", urlinfo.path);
    TEST_ASSERT_EQUAL(80, urlinfo.port);
    TEST_ASSERT_EQUAL_STRING("", urlinfo.username);
    TEST_ASSERT_EQUAL_STRING("", urlinfo.password);
}


static void test_parse_url_complex(void)
{
    debug_printf_Ignore();

    TEST_ASSERT_EQUAL(SR_SUCCESS, http_parse_url("http://username:password@host.com:1234/path?value=foo:bar", &urlinfo));
    TEST_ASSERT_EQUAL_STRING("host.com", urlinfo.host);
    TEST_ASSERT_EQUAL_STRING("/path?value=foo:bar", urlinfo.path);
    TEST_ASSERT_EQUAL(1234, urlinfo.port);
    TEST_ASSERT_EQUAL_STRING("username", urlinfo.username);
    TEST_ASSERT_EQUAL_STRING("password", urlinfo.password);
}


int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_parse_url_simple);
    RUN_TEST(test_parse_url_with_path_and_port);
    RUN_TEST(test_parse_url_with_port);
    RUN_TEST(test_parse_url_with_credentials);
    RUN_TEST(test_parse_url_with_colon_in_uri);
    RUN_TEST(test_parse_url_complex);

    return UNITY_END();
}
