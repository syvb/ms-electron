// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "microsoft/src/services/network/edge_header_observers/trv2/tenant_restrictions_header_observer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "microsoft/src/components/edge_adjust_headers/adjust_trusted_headers.h"
#include "microsoft/src/components/edge_adjust_headers/adjust_trusted_headers_manager.h"
#include "microsoft/src/components/edge_tenant/common/trv2_policy.h"
#include "net/http/http_request_headers.h"

namespace edge {
constexpr const char* kTrv2header = "Sec-Restrict-Tenant-Access-Policy";
namespace tenant_restrictions {
namespace {
void AdjustHeadersCallback(bool add_header,
                           std::string payload,
                           const net::HttpRequestHeaders& headers,
                           net::HttpRequestHeaders* headers_to_modify,
                           std::vector<std::string>* headers_to_remove) {
  // Don't bother to try to remove a header that doesn't exist
  if (add_header || headers.HasHeader(kTrv2header)) {
    if (add_header) {
      headers_to_modify->SetHeader(kTrv2header, payload);
    } else {
      headers_to_remove->push_back(kTrv2header);
    }
  }
}
}  // namespace

Trv2HeaderObserver::Trv2HeaderObserver() = default;
Trv2HeaderObserver::~Trv2HeaderObserver() = default;
bool Trv2HeaderObserver::IsFeatureEnabled(
    const network::NetworkContext* context) {
  // Trv2 is enabled for all contexts if enabled
  return policies_ && policies_->IsActive();
}

void Trv2HeaderObserver::ResourceRequestInitiated(
    AdjustTrustedHeaders* resource_request) {
  DCHECK(policies_);
  bool add_header = policies_->IsUrlWeCareAbout(resource_request->url());
  resource_request->AddCallback(base::BindRepeating(
      &AdjustHeadersCallback, add_header, policies_->payload()));
}

void Trv2HeaderObserver::Update(const std::string& payload,
                                const std::vector<std::string>& hostnames) {
  bool should_be_active = !payload.empty() && !hostnames.empty();
  std::unique_ptr<trv2::Policy> policy =
      std::make_unique<trv2::Policy>(payload, hostnames);
  // Don't replace the policies if we got garbage data for some reason.
  if (policy->IsValid() && (should_be_active == policy->IsActive())) {
    policies_ = std::move(policy);
  }
}

}  // namespace tenant_restrictions
}  // namespace edge
