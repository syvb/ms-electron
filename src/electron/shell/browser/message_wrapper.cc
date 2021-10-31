// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/electron/shell/browser/message_wrapper.h"

#include <utility>

#include "base/strings/stringprintf.h"

namespace microsoft {

MessageWrapper::MessageWrapper(MessageToSyncCallback callback,
                               int32_t web_contents_id)
    : callback_(std::move(callback)), web_contents_id_(web_contents_id) {}

MessageWrapper::~MessageWrapper() {
  SendError(
      base::StringPrintf("WebContents %d failed to reply", web_contents_id_));
}

bool MessageWrapper::SendReply(blink::CloneableMessage result) {
  if (!callback_)
    return false;

  std::move(*callback_).Run(std::string(), std::move(result));
  callback_.reset();

  return true;
}

bool MessageWrapper::SendError(const std::string& error) {
  if (!callback_)
    return false;

  std::move(*callback_).Run(error, blink::CloneableMessage());
  callback_.reset();

  return true;
}

}  // namespace microsoft
