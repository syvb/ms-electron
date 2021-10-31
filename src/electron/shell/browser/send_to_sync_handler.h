// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_BROWSER_SEND_TO_SYNC_HANDLER_H_
#define SRC_ELECTRON_SHELL_BROWSER_SEND_TO_SYNC_HANDLER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "shell/common/api/api.mojom.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/messaging/cloneable_message.h"

namespace electron {
namespace api {
class WebContents;
}
}  // namespace electron

namespace microsoft {

class SendToSyncHandler {
 public:
  explicit SendToSyncHandler(electron::api::WebContents* web_contents);
  ~SendToSyncHandler();

  using MessageToSyncCallback =
      electron::mojom::ElectronBrowser::MessageToSyncCallback;

  void MessageToSync(bool internal,
                     int32_t web_contents_id,
                     const std::string& channel,
                     blink::CloneableMessage args,
                     MessageToSyncCallback callback);

  void ConnectToRenderer(
      int32_t web_contents_id,
      mojo::PendingReceiver<electron::mojom::RendererService>,
      electron::mojom::ElectronBrowser::ConnectToRendererCallback callback);

 private:
  void OnConnectToRenderer(
      mojo::AssociatedRemote<electron::mojom::ElectronRenderer>* ep,
      int32_t dest_web_contents_id,
      electron::mojom::ElectronBrowser::ConnectToRendererCallback callback,
      bool success);

  electron::api::WebContents* web_contents_ = nullptr;
  base::WeakPtrFactory<SendToSyncHandler> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(SendToSyncHandler);
};

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_BROWSER_SEND_TO_SYNC_HANDLER_H_
