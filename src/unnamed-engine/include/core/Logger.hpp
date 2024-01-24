//
//  ScriptingManager.hpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 25/12/23.
//

#pragma once

#define SET_TEXT_COLOR(COLOR, STREAM) fprintf(STREAM, "\033[" COLOR "m")
#define RESET_COLOR(STREAM) fprintf(STREAM, "\033[0m")

#define _LOG_LN(FORMAT, ...)                                                                                           \
    fprintf(stdout, FORMAT, ##__VA_ARGS__);                                                                            \
    fprintf(stdout, "\n\r")

#define _LOG_ERROR_LN(FORMAT, ...)                                                                                     \
    fprintf(stderr, FORMAT, ##__VA_ARGS__);                                                                            \
    fprintf(stderr, "\n\r")

#define LOG_INFO_LN(FORMAT, ...)                                                                                       \
    SET_TEXT_COLOR("1;32", stdout);                                                                                    \
    fprintf(stdout, "[Info] - ");                                                                                      \
    _LOG_LN(FORMAT, ##__VA_ARGS__);                                                                                    \
    RESET_COLOR(stdout)

#define LOG_WARN_LN(FORMAT, ...)                                                                                       \
    SET_TEXT_COLOR("1;33", stdout);                                                                                    \
    fprintf(stdout, "[Warn] - ");                                                                                      \
    _LOG_LN(FORMAT, ##__VA_ARGS__);                                                                                    \
    RESET_COLOR(stdout)

#if DEBUG
#define LOG_DEBUG_LN(FORMAT, ...)                                                                                      \
    SET_TEXT_COLOR("1;34", stdout);                                                                                    \
    fprintf(stdout, "[Debug] - ");                                                                                     \
    _LOG_LN(FORMAT, ##__VA_ARGS__);                                                                                    \
    RESET_COLOR(stdout)
#else
#define LOG_DEBUG_LN(FORMAT, ...)
#endif
#define LOG_ERROR_LN(FORMAT, ...)                                                                                      \
    SET_TEXT_COLOR("1;31", stderr);                                                                                    \
    fprintf(stderr, "[Error] - ");                                                                                     \
    _LOG_ERROR_LN(FORMAT, ##__VA_ARGS__);                                                                              \
    RESET_COLOR(stderr)
