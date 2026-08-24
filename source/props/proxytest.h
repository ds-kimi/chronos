#pragma once

#include "core/chronos.h"

class CSendProxyRecipients;

namespace Chronos
{

// A datatable prop is only followed when its proxy hands back the same address
// the engine passed in, meaning the sub-table is stored inline. Proxies that
// redirect to another object would give offsets into the wrong allocation.
bool IsInlineProxy( SendProp *prop, const void *structBase );

}
