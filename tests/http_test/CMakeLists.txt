PROJECT(http_test)
SET (HTTP_TEST_SRC
	http_test.c
	${CMAKE_SOURCE_DIR}/lib/http.c
	${MOCK_SRC_DIR}/Mockdebug.c
	${MOCK_SRC_DIR}/Mockmchar.c
	${MOCK_SRC_DIR}/Mocksocklib.c
	${CMOCK_SRC}
	${UNITY_SRC}
	)
ADD_EXECUTABLE(http_test ${HTTP_TEST_SRC})
TARGET_LINK_LIBRARIES(http_test glib-2.0)

