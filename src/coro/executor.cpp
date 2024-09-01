/**
 * @file executor.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/coro/def.h"
#include "xsl/logctl.h"
#include "xsl/coro/executor.h"

#include <thread>
XSL_CORO_NB
void NoopExecutor::schedule(move_only_function<void()> &&func) { func(); }

void NewThreadExecutor::schedule(move_only_function<void()> &&func) {
  LOG5("new task scheduled");
  std::thread(std::move(func)).detach();
}

XSL_CORO_NE
