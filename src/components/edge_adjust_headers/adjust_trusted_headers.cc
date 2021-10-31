// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "microsoft/src/components/edge_adjust_headers/adjust_trusted_headers.h"

#include <utility>

#include "base/callback.h"

namespace edge {

AdjustTrustedHeaders::AdjustTrustedHeaders(const GURL& url) : url_(url) {}

AdjustTrustedHeaders::~AdjustTrustedHeaders() = default;

void AdjustTrustedHeaders::AddCallback(ModifyHeadersCallback callback) {
  callbacks_.emplace_back(std::move(callback));
}

base::Optional<net::HttpRequestHeaders>
AdjustTrustedHeaders::OnBeforeSendHeaders(
    const net::HttpRequestHeaders& headers) {
  base::Optional<net::HttpRequestHeaders> modified_headers;

  const net::HttpRequestHeaders* current_headers = &headers;
  for (const auto& iterator : callbacks_) {
    net::HttpRequestHeaders headers_to_add;
    std::vector<std::string> headers_to_remove;
    iterator.Run(*current_headers, &headers_to_add, &headers_to_remove);
    if (!headers_to_remove.empty() || !headers_to_add.IsEmpty()) {
      if (!modified_headers) {
        modified_headers = headers;
        current_headers = &modified_headers.value();
      }
      modified_headers->MergeFrom(headers_to_add);
      for (auto& header : headers_to_remove) {
        modified_headers->RemoveHeader(header);
      }
    }
  }
  return modified_headers;
}

size_t AdjustTrustedHeaders::GetCallbacksSizeForTesting() {
  return callbacks_.size();
}

}  // namespace edge
