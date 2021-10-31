// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "microsoft/src/components/edge_adjust_headers/adjust_trusted_headers_manager.h"

#include <algorithm>
#include <memory>

#include "microsoft/src/components/edge_adjust_headers/adjust_trusted_headers.h"
#include "url/gurl.h"

namespace edge {

AdjustTrustedHeadersManager::AdjustTrustedHeadersManager() = default;
AdjustTrustedHeadersManager::~AdjustTrustedHeadersManager() = default;

void AdjustTrustedHeadersManager::Register(Observer* observer) {
  observers_.AddObserver(observer);
}

void AdjustTrustedHeadersManager::Unregister(Observer* observer) {
  observers_.RemoveObserver(observer);
}

std::unique_ptr<AdjustTrustedHeaders>
AdjustTrustedHeadersManager::MakeCallbackForRequest(
    const network::NetworkContext* context,
    const GURL& url) {
  if (!observers_.might_have_observers()) {
    return nullptr;
  }
  auto resource_request = std::make_unique<AdjustTrustedHeaders>(url);
  std::for_each(observers_.begin(), observers_.end(),
                [&resource_request, context](Observer& observer) {
                  if (observer.IsFeatureEnabled(context)) {
                    observer.ResourceRequestInitiated(resource_request.get());
                  }
                });
  if (resource_request->Active()) {
    return resource_request;
  }
  return nullptr;
}

}  // namespace edge
