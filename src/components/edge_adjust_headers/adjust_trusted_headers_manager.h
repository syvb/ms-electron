// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_COMPONENTS_EDGE_ADJUST_HEADERS_ADJUST_TRUSTED_HEADERS_MANAGER_H_
#define SRC_COMPONENTS_EDGE_ADJUST_HEADERS_ADJUST_TRUSTED_HEADERS_MANAGER_H_

#include <memory>

#include "base/observer_list.h"

namespace network {
class NetworkContext;
}  // namespace network

class GURL;

namespace edge {

class AdjustTrustedHeaders;

// Class that managed features that allow Edge to add headers for all
// http requests. Features register with the manager as an observer and are
// notified when an http request starts. An AdjustTrustedHeaders class is
// created for the http request and the observer can get the url and other
// information about the request at this time. Then the observer can add a
// a callback to be called when (if) the http request reaches
// OnBeforeSendHeaders. This callback will allow modification of the http
// headers at this time. This is the last time (excepting cors/other security
// checks) that the headers will be modified before being sent out.

// All observers must be removed before destruction.

class AdjustTrustedHeadersManager {
 public:
  AdjustTrustedHeadersManager();
  ~AdjustTrustedHeadersManager();

  class Observer : public base::CheckedObserver {
   public:
    virtual bool IsFeatureEnabled(const network::NetworkContext* context) = 0;
    virtual void ResourceRequestInitiated(
        AdjustTrustedHeaders* resource_request) = 0;
  };

  void Register(Observer* delegate);
  void Unregister(Observer* delegate);

  std::unique_ptr<AdjustTrustedHeaders> MakeCallbackForRequest(
      const network::NetworkContext* context,
      const GURL& url);

  friend class AdjustHeadersManagerTest;

 private:
  base::ObserverList<Observer, true /* check empty */> observers_;
};

}  // namespace edge

#endif  // SRC_COMPONENTS_EDGE_ADJUST_HEADERS_ADJUST_TRUSTED_HEADERS_MANAGER_H_
