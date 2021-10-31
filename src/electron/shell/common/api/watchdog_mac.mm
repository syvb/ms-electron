// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/electron/shell/common/api/watchdog.h"

#import <Foundation/Foundation.h>

namespace microsoft {

void Watchdog::RaiseException() {
  [NSException raise:@"WatchdogException" format:@"Deadlock detected!"];
}

}  // namespace microsoft
