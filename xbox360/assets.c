#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assets/smw_assets.h"
#include "util.h"
#include "xsdk/log.h"

const uint8 *g_asset_ptrs[kNumberOfAssets];
uint32 g_asset_sizes[kNumberOfAssets];

static uint32_t ReadLe32(const uint8_t *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

bool SmwXenonLoadAssets(void) {
  size_t length = 0;
  uint8_t *data = ReadWholeFile("smw_assets.dat", &length);
  if (!data) {
    xsdk_log_write("smw: assets missing at game:\\smw_assets.dat; place beside smw_xbox360.xex");
    return false;
  }

  static const uint8_t signature[] = { kAssets_Sig };
  if (length < 88u + (size_t)kNumberOfAssets * 4u ||
      memcmp(data, signature, sizeof signature) != 0 ||
      ReadLe32(data + 80) != kNumberOfAssets) {
    xsdk_log_write("smw: invalid smw_assets.dat header or resource count");
    free(data);
    return false;
  }

  uint64_t offset = 88u + (uint64_t)kNumberOfAssets * 4u + ReadLe32(data + 84);
  for (uint32_t i = 0; i < kNumberOfAssets; i++) {
    const uint32_t size = ReadLe32(data + 88u + (size_t)i * 4u);
    offset = (offset + 3u) & ~(uint64_t)3u;
    if (offset + size > length) {
      xsdk_log_write("smw: truncated smw_assets.dat at resource %u", (unsigned)i);
      free(data);
      memset(g_asset_ptrs, 0, sizeof g_asset_ptrs);
      memset(g_asset_sizes, 0, sizeof g_asset_sizes);
      return false;
    }
    g_asset_ptrs[i] = data + (size_t)offset;
    g_asset_sizes[i] = size;
    offset += size;
  }

  xsdk_log_write("smw: assets loaded bytes=%u resources=%u", (unsigned)length, (unsigned)kNumberOfAssets);
  return true;
}

MemBlk FindInAssetArray(int asset, int idx) {
  if (asset < 0 || asset >= kNumberOfAssets)
    return (MemBlk){ 0 };
  return FindIndexInMemblk((MemBlk){ g_asset_ptrs[asset], g_asset_sizes[asset] }, idx);
}

const uint8 *FindPtrInAsset(int asset, uint32 addr) {
  if (asset < 0 || asset >= kNumberOfAssets)
    return NULL;
  return FindAddrInMemblk((MemBlk){ g_asset_ptrs[asset], g_asset_sizes[asset] }, addr);
}
