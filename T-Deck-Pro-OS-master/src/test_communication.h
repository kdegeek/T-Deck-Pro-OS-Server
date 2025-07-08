/**
 * @file test_communication.h
 * @brief Communication stack test application header
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#ifndef TEST_COMMUNICATION_H
#define TEST_COMMUNICATION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Test communication interfaces availability
 */
void test_communication_interfaces(void);

/**
 * @brief Test message sending functionality
 */
void test_message_sending(void);

/**
 * @brief Test message receiving functionality
 */
void test_message_receiving(void);

/**
 * @brief Test interface switching functionality
 */
void test_interface_switching(void);

/**
 * @brief Test communication statistics
 */
void test_communication_statistics(void);

/**
 * @brief Run comprehensive communication tests
 */
void run_communication_tests(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_COMMUNICATION_H