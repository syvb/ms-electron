// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "microsoft/src/components/edge_tenant/common/trv2_policy.h"

#include <algorithm>
#include <utility>

#include "base/callback.h"
#include "base/guid.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "url/gurl.h"

namespace edge {
namespace tenant_restrictions {
namespace trv2 {
namespace {
bool ValidateHostname(const std::string& host) {
  std::string add_scheme = std::string("http://") + host;
  GURL url(add_scheme);
  return url.is_valid();
}

bool ValidatePayload(const std::string& payload) {
  // We're only going to validate length here. Full validation of
  // the payload is done when reading from the registry and on the server.

  // Max length is 3 guids + 2 separator characters
  constexpr uint32_t max_length = (36 * 3) + 2;
  if (payload.length() > max_length) {
    return false;
  }
  return true;
}
}  // namespace

bool Policy::HostNode::operator<(const HostNode& rhs) const {
  base::StringPiece lhs_host(hostname);
  base::StringPiece rhs_host(rhs.hostname);
  int comp_val = lhs_host.compare(rhs_host);
  if (comp_val == 0) {
    // Hosts are equal. if lhs has a wildcard, it should be "less than".
    // If both have a wildcard or neither has a wildcard they are equal.
    // If rhs has a wildcard and lhs does not, lhs is greater then lhs.
    return isWildCard && !rhs.isWildCard;
  }
  return comp_val < 0;
}

bool Policy::HostNode::Match(const std::string& host) const {
  // Direct compare or wildcard match
  return (!isWildCard && host == hostname) ||
         (isWildCard && base::EndsWith(base::StringPiece(host), hostname,
                                       base::CompareCase::SENSITIVE));
}

bool Policy::HostNode::IsValid() const {
  return ValidateHostname(hostname);
}

Policy::~Policy() = default;
Policy::Policy(const std::string& payload,
               const std::vector<std::string>& hosts) {
  if (!ValidatePayload(payload)) {
    return;
  }
  header_payload_ = payload;
  InitHosts(UnpackHosts(hosts));
}

// Requires pre-validated parameters
Policy::Policy(const std::string& payload, const std::vector<HostNode>& hosts)
    : header_payload_(payload) {
  DCHECK(ValidatePayload(payload));
  DCHECK(!(std::any_of(hosts.cbegin(), hosts.cend(),
                       [](const HostNode& node) { return !node.IsValid(); })));
  InitHosts(hosts);
}

Policy::Policy(Policy&& rhs) = default;
Policy& Policy::operator=(Policy&& rhs) = default;

void Policy::InitHosts(std::vector<HostNode> hosts) {
  std::sort(hosts.begin(), hosts.end());
  // Remove duplicates
  hosts.erase(std::unique(hosts.begin(), hosts.end()), hosts.end());
  hosts_ = std::move(hosts);
}

std::vector<Policy::HostNode> Policy::UnpackHosts(
    const std::vector<std::string>& hosts) {
  std::vector<HostNode> host_list;
  for (const std::string& host : hosts) {
    HostNode node;
    if (base::StartsWith(host, "*", base::CompareCase::SENSITIVE)) {
      node = {host.substr(1), true};
    } else {
      node = {host, false};
    }
    if (!node.IsValid()) {
      continue;
    }
    host_list.emplace_back(std::move(node));
  }
  return host_list;
}

std::vector<std::string> Policy::PackHosts() const {
  std::vector<std::string> hosts;
  for (const auto& node : hosts_) {
    if (node.isWildCard) {
      hosts.push_back("*" + node.hostname);
    } else {
      hosts.push_back(node.hostname);
    }
  }
  return hosts;
}

bool Policy::IsUrlWeCareAbout(const GURL& url) const {
  if (url.SchemeIsHTTPOrHTTPS() && url.has_host()) {
    std::string hostname = url.host();
    return std::any_of(
        hosts_.cbegin(), hosts_.cend(),
        [&hostname](const HostNode& node) { return node.Match(hostname); });
  }
  return false;
}

}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge
