// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/electron/shell/browser/send_to_sync_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/strings/stringprintf.h"
#include "microsoft/src/electron/shell/browser/message_wrapper.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/common/gin_helper/trackable_object.h"

namespace microsoft {

SendToSyncHandler::SendToSyncHandler(electron::api::WebContents* web_contents)
    : web_contents_(web_contents) {
  DCHECK(web_contents_);
}

// Complex class/struct needs an explicit out-of-line destructor.
SendToSyncHandler::~SendToSyncHandler() = default;

void SendToSyncHandler::OnConnectToRenderer(
    mojo::AssociatedRemote<electron::mojom::ElectronRenderer>* ep,
    int32_t destination_web_contents_id,
    electron::mojom::ElectronBrowser::ConnectToRendererCallback callback,
    bool success) {
  LOG(INFO) << "Connected to main frame renderer, requesting WebContents id: "
            << web_contents_->ID()
            << " target WebContents id: " << destination_web_contents_id;
  std::move(callback).Run(web_contents_->ID());
}

void SendToSyncHandler::ConnectToRenderer(
    int32_t web_contents_id,
    mojo::PendingReceiver<electron::mojom::RendererService> receiver,
    electron::mojom::ElectronBrowser::ConnectToRendererCallback callback) {
  LOG(INFO) << "ConnectToRenderer for target WebContents id: "
            << web_contents_id;

  auto* dest_web_contents =
      gin_helper::TrackableObject<electron::api::WebContents>::FromWeakMapID(
          web_contents_->isolate(), web_contents_id);

  if (!dest_web_contents) {
    LOG(ERROR) << "Requested WebContents with id not found: "
               << web_contents_id;
    std::move(callback).Run(0);
    return;
  }

  if (web_contents_->ID() == web_contents_id ||
      dest_web_contents == web_contents_) {
    LOG(ERROR) << "Connection request to originating WebContents not allowed. "
                  "Requesting WebContents id is same as target WebContents: "
               << web_contents_id;
    std::move(callback).Run(0);
    return;
  }

  auto* frame_host = dest_web_contents->web_contents()->GetMainFrame();
  if (!frame_host) {
    LOG(ERROR) << "RenderFrameHost not found for WebContents with id: "
               << web_contents_id;
    std::move(callback).Run(0);
    return;
  }

  LOG(INFO) << "Attempting to connect to main frame renderer, requesting "
               "WebContents: "
            << web_contents_->ID()
            << " target WebContents: " << dest_web_contents->ID();

  // This dance with `base::Owned` is to ensure that the interface stays alive
  // until the callback is called. Otherwise it would be closed at the end of
  // this function.
  auto electron_renderer = std::make_unique<
      mojo::AssociatedRemote<electron::mojom::ElectronRenderer>>();
  frame_host->GetRemoteAssociatedInterfaces()->GetInterface(
      electron_renderer.get());
  auto* raw_ptr = electron_renderer.get();
  (*raw_ptr)->ConnectToRenderer(
      web_contents_->ID(), std::move(receiver),
      base::BindOnce(&SendToSyncHandler::OnConnectToRenderer,
                     weak_factory_.GetWeakPtr(),
                     base::Owned(std::move(electron_renderer)), web_contents_id,
                     std::move(callback)));
}

void SendToSyncHandler::MessageToSync(bool internal,
                                      int32_t web_contents_id,
                                      const std::string& channel,
                                      blink::CloneableMessage args,
                                      MessageToSyncCallback callback) {
  auto wrapper =
      std::make_unique<MessageWrapper>(std::move(callback), web_contents_id);

  auto* dest_web_contents =
      gin_helper::TrackableObject<electron::api::WebContents>::FromWeakMapID(
          web_contents_->isolate(), web_contents_id);

  if (!dest_web_contents) {
    wrapper->SendError(base::StringPrintf(
        "Destination WebContents with ID %d not found", web_contents_id));
    return;
  }

  if (dest_web_contents->IsCrashed()) {
    wrapper->SendError(base::StringPrintf(
        "Destination WebContents with ID %d is crashed", web_contents_id));
    return;
  }

  auto* frame_host = dest_web_contents->web_contents()->GetMainFrame();
  if (!frame_host) {
    wrapper->SendError(base::StringPrintf(
        "RenderFrameHost not found for WebContents with ID %d",
        web_contents_id));
    return;
  }

  // This dance with `base::Owned` is to ensure that the interface stays alive
  // until the callback is called. Otherwise it would be closed at the end of
  // this function.
  auto electron_renderer = std::make_unique<
      mojo::AssociatedRemote<electron::mojom::ElectronRenderer>>();
  frame_host->GetRemoteAssociatedInterfaces()->GetInterface(
      electron_renderer.get());
  auto* raw_ptr = electron_renderer.get();
  (*raw_ptr)->MessageSync(
      internal, channel, std::move(args), web_contents_->ID(),
      base::BindOnce(
          [](mojo::AssociatedRemote<electron::mojom::ElectronRenderer>* ep,
             std::unique_ptr<MessageWrapper> wrapper,
             blink::CloneableMessage result, bool success) {
            if (success) {
              wrapper->SendReply(std::move(result));
            } else {
              wrapper.reset();
            }
          },
          base::Owned(std::move(electron_renderer)), std::move(wrapper)));
}

}  // namespace microsoft
