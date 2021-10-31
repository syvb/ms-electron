// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_SERVICES_NETWORK_EDGE_HEADER_OBSERVERS_TRV2_TENANT_RESTRICTIONS_HEADER_OBSERVER_H_
#define SRC_SERVICES_NETWORK_EDGE_HEADER_OBSERVERS_TRV2_TENANT_RESTRICTIONS_HEADER_OBSERVER_H_

#include <memory>
#include <string>
#include <vector>

#include "microsoft/src/components/edge_adjust_headers/adjust_trusted_headers_manager.h"

namespace edge {
namespace tenant_restrictions {

namespace trv2 {
class Policy;
}  // namespace trv2

class Trv2HeaderObserver final
    : public ::edge::AdjustTrustedHeadersManager::Observer {
 public:
  Trv2HeaderObserver();
  ~Trv2HeaderObserver() override;
  bool IsFeatureEnabled(const network::NetworkContext* context) override;
  void ResourceRequestInitiated(
      AdjustTrustedHeaders* resource_request) override;

  void Update(const std::string& payload,
              const std::vector<std::string>& hostnames);

 private:
  std::unique_ptr<trv2::Policy> policies_;
};
}  // namespace tenant_restrictions
}  // namespace edge

#endif  // SRC_SERVICES_NETWORK_EDGE_HEADER_OBSERVERS_TRV2_TENANT_RESTRICTIONS_HEADER_OBSERVER_H_
