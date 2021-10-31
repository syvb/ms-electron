// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_BROWSER_MESSAGE_WRAPPER_H_
#define SRC_ELECTRON_SHELL_BROWSER_MESSAGE_WRAPPER_H_

#include <string>

#include "base/optional.h"
#include "shell/common/api/api.mojom.h"

namespace microsoft {

class MessageWrapper {
 public:
  using MessageToSyncCallback =
      electron::mojom::ElectronBrowser::MessageToSyncCallback;

  MessageWrapper(MessageToSyncCallback callback, int32_t web_contents_id);
  ~MessageWrapper();

  bool SendReply(blink::CloneableMessage result);
  bool SendError(const std::string& error);

 private:
  base::Optional<MessageToSyncCallback> callback_;
  int32_t web_contents_id_ = 0;

  DISALLOW_COPY_AND_ASSIGN(MessageWrapper);
};

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_BROWSER_MESSAGE_WRAPPER_H_
