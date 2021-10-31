// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_COMPONENTS_EDGE_ADJUST_HEADERS_ADJUST_TRUSTED_HEADERS_H_
#define SRC_COMPONENTS_EDGE_ADJUST_HEADERS_ADJUST_TRUSTED_HEADERS_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/optional.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"

namespace edge {

class AdjustTrustedHeaders {
 public:
  using ModifyHeadersCallback =
      base::RepeatingCallback<void(const net::HttpRequestHeaders&,
                                   net::HttpRequestHeaders*,
                                   std::vector<std::string>*)>;

  explicit AdjustTrustedHeaders(const GURL& url);
  ~AdjustTrustedHeaders();
  void AddCallback(ModifyHeadersCallback callback);
  base::Optional<net::HttpRequestHeaders> OnBeforeSendHeaders(
      const net::HttpRequestHeaders& headers);
  const GURL& url() { return url_; }
  bool Active() { return !callbacks_.empty(); }
  size_t GetCallbacksSizeForTesting();
  friend class AdjustHeadersTest;

 private:
  std::vector<ModifyHeadersCallback> callbacks_;
  const GURL url_;
};

}  // namespace edge

#endif  // SRC_COMPONENTS_EDGE_ADJUST_HEADERS_ADJUST_TRUSTED_HEADERS_H_
