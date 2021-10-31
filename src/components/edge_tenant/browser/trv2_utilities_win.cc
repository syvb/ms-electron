// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "microsoft/src/components/edge_tenant/browser/trv2_utilities_win.h"

#include <memory>
#include <utility>

#include "base/guid.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "microsoft/src/base/manual_delay_load.h"
#include "microsoft/src/base/win/scoped_co_mem.h"
#include "microsoft/src/components/edge_tenant/browser/trv2_plugin_win.h"

namespace {
constexpr const wchar_t* kPolicyRegistryKey =
    L"SOFTWARE\\Policies\\Microsoft\\Windows\\TenantRestrictions\\Payload";
constexpr const wchar_t* kPayloadCloudId = L"cloudid";
constexpr const wchar_t* kPayloadTenantId = L"tenantid";
constexpr const wchar_t* kPayloadPolicyId = L"policyid";
}  // namespace

namespace edge {
namespace tenant_restrictions {
namespace trv2 {
namespace test {
const wchar_t* GetPolicyRegistryKey() {
  return kPolicyRegistryKey;
}
const wchar_t* GetPayloadCloudId() {
  return kPayloadCloudId;
}
const wchar_t* GetPayloadTenantId() {
  return kPayloadTenantId;
}
const wchar_t* GetPayloadPolicyId() {
  return kPayloadPolicyId;
}

}  // namespace test

namespace win {

Utilities::Utilities() = default;
Utilities::~Utilities() = default;

base::win::RegKey Utilities::GetPolicyRegKey() {
  return base::win::RegKey(HKEY_LOCAL_MACHINE, kPolicyRegistryKey, KEY_READ);
}

bool Utilities::HasPayload() {
  base::win::RegKey policy_key = GetPolicyRegKey();
  if (!policy_key.Valid()) {
    return false;
  }
  return policy_key.HasValue(kPayloadTenantId) &&
         policy_key.HasValue(kPayloadPolicyId);
}

// Payload format:
// "<cloud id>:<tenant id>:<policy id>" or  "<tenant id>:<policy id>"
// If payload is malformed or policy keys are non existing,
// this returns an empty string
std::string Utilities::GetPayload() {
  constexpr size_t kMaxCloudIdLength = 38;
  base::win::RegKey policy_key = GetPolicyRegKey();
  if (!policy_key.Valid()) {
    return std::string();
  }

  std::vector<std::wstring> payload_vector;
  std::wstring temp;

  // Cloud id
  if (policy_key.ReadValue(kPayloadCloudId, &temp) == ERROR_SUCCESS) {
    if (temp.length() > kMaxCloudIdLength) {
      return std::string();
    }
    if (!temp.empty()) {
      payload_vector.push_back(std::move(temp));
    }
  }

  // Tenant id
  if (policy_key.ReadValue(kPayloadTenantId, &temp) != ERROR_SUCCESS) {
    return std::string();
  }
  // Save the admin / dev from themselves (trim whitespace)
  base::TrimWhitespace(temp, base::TrimPositions::TRIM_ALL);
  if (!base::IsValidGUID(base::WideToUTF16(temp))) {
    return std::string();
  }
  payload_vector.push_back(std::move(temp));

  // Policy id
  if (policy_key.ReadValue(kPayloadPolicyId, &temp) != ERROR_SUCCESS) {
    return std::string();
  }
  // Save the admin / dev from themselves (trim whitespace)
  base::TrimWhitespace(temp, base::TrimPositions::TRIM_ALL);
  if (!base::IsValidGUID(base::WideToUTF16(temp))) {
    return std::string();
  }
  payload_vector.push_back(std::move(temp));

  std::wstring payload = base::JoinString(payload_vector, L":");
  return base::WideToUTF8(payload);
}

PluginListType Utilities::GetHostList() {
  microsoft::base::win::ScopedCoMem<LPWSTR> hostnames;
  UINT32 hostnames_count = 0;
  microsoft::base::win::ScopedCoMem<LPWSTR> wildcard_hostnames;
  UINT32 wildcard_hostnames_count = 0;
  HRESULT hr = Plugin::GetPlugin()->CallGetTenantRestrictionsHostname(
      &hostnames, &hostnames_count, &wildcard_hostnames,
      &wildcard_hostnames_count);
  if (FAILED(hr)) {
    // we shouldn't have gotten anything populated (use trv1 domains)
    return PluginListType{{"login.microsoft.com", false},
                          {"login.microsoftonline.com", false},
                          {"login.windows.net", false}};
  }
  std::vector<microsoft::base::win::ScopedCoMem<wchar_t>> hostname_vector;
  hostname_vector.reserve(hostnames_count + wildcard_hostnames_count);
  PluginListType host_list;
  host_list.reserve(hostnames_count + wildcard_hostnames_count);
  for (uint32_t i = 0; i < hostnames_count; i++) {
    hostname_vector.emplace_back(
        microsoft::base::win::ScopedCoMem<wchar_t>(hostnames.get()[i]));
    Policy::HostNode node{base::WideToUTF8(hostnames.get()[i]), false};
    if (node.IsValid()) {
      host_list.push_back(std::move(node));
    }
  }
  for (uint32_t i = 0; i < wildcard_hostnames_count; i++) {
    hostname_vector.emplace_back(microsoft::base::win::ScopedCoMem<wchar_t>(
        wildcard_hostnames.get()[i]));
    Policy::HostNode node{base::WideToUTF8(wildcard_hostnames.get()[i]), true};
    if (node.IsValid()) {
      host_list.push_back(std::move(node));
    }
  }

  // delete all the host names
  hostname_vector.clear();
  // delete the array of pointers to hostnames
  hostnames.Reset();
  // delete the array of pointers to wildcard hostnames
  wildcard_hostnames.Reset();

  return host_list;
}

Utilities* Utilities::GetUtilities(Utilities* new_utilities) {
  static std::unique_ptr<Utilities> utilities = std::make_unique<Utilities>();
  if (new_utilities) {
    utilities.reset(new_utilities);
  }
  return utilities.get();
}
bool Utilities::WindowsSupportsTrv2() {
  return Plugin::GetPlugin()->WindowsSupportsTrv2();
}
}  // namespace win
}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge
