#include "hid.h"
#define _WIN32_DCOM
#include <comdef.h>
#include <wbemidl.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

// Apple Studio Display
static const char vidStr[] = "VID_05AC";
static const char pidStr[] = "PID_1114";
static const char interfaceStr[] = "MI_07";
static const char collectionStr[] = "Col";

static HANDLE hDeviceObject = INVALID_HANDLE_VALUE;
static PHIDP_PREPARSED_DATA preparsedData = nullptr;

static struct {
  USHORT reportLength;
  USAGE usagePage;
  USAGE usage;
  UCHAR reportId;
} inputCaps, featureCaps;

int hid_init () {
  if (hDeviceObject != INVALID_HANDLE_VALUE) return -1;

  SP_DEVINFO_DATA deviceInfoData;
  SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
  GUID hidGuid;
  DWORD type, size;
  char propBuf[500];

  HidD_GetHidGuid(&hidGuid);

  HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&hidGuid, nullptr, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if (hDevInfoSet == INVALID_HANDLE_VALUE) return -2;

  for (DWORD memberIndex = 0; ; memberIndex++) {
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    // Get SP_DEVINFO_DATA for this member.
    if (!SetupDiEnumDeviceInfo(hDevInfoSet, memberIndex, &deviceInfoData)) {
      DWORD err = GetLastError();
      SetupDiDestroyDeviceInfoList(hDevInfoSet);
      if (err == ERROR_NO_MORE_ITEMS) break;
      return -3;
    }

    // Get required size for device property
    memset(propBuf, 0, sizeof(propBuf));
    if (!SetupDiGetDeviceRegistryProperty(hDevInfoSet, &deviceInfoData, SPDRP_HARDWAREID, &type, reinterpret_cast<PBYTE>(propBuf), sizeof(propBuf), &size)) {
      SetupDiDestroyDeviceInfoList(hDevInfoSet);
      return -4;
    }

    if (!strstr(propBuf, vidStr) ||
      !strstr(propBuf, pidStr) ||
      !strstr(propBuf, interfaceStr) ||
      strstr(propBuf, collectionStr))
      continue;

    // std::printf("Device found: %s\n", propBuf);

    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    SetupDiEnumDeviceInterfaces(hDevInfoSet, nullptr, &hidGuid, memberIndex, &deviceInterfaceData);

    memset(propBuf, 0, sizeof(propBuf));
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceDetailData = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(propBuf);
    deviceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    if (!SetupDiGetDeviceInterfaceDetail(hDevInfoSet, &deviceInterfaceData, deviceDetailData, sizeof(propBuf), &size, nullptr)) {
      SetupDiDestroyDeviceInfoList(hDevInfoSet);
      return -5;
    }

    SetupDiDestroyDeviceInfoList(hDevInfoSet);

    hDeviceObject = CreateFile(deviceDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (hDeviceObject == INVALID_HANDLE_VALUE) return -6;

    if (!HidD_GetPreparsedData(hDeviceObject, &preparsedData)) {
      hid_deinit();
      return -7;
    }

    memset(propBuf, 0, sizeof(propBuf));
    PHIDP_CAPS caps = reinterpret_cast<PHIDP_CAPS>(propBuf);
    if (HidP_GetCaps(preparsedData, caps) != HIDP_STATUS_SUCCESS) {
      hid_deinit();
      return -8;
    }

    inputCaps.reportLength = caps->InputReportByteLength;
    featureCaps.reportLength = caps->FeatureReportByteLength;

    memset(propBuf, 0, sizeof(propBuf));
    PHIDP_VALUE_CAPS valueCaps = reinterpret_cast<PHIDP_VALUE_CAPS>(propBuf);

    USHORT valueCapsLength = 2;
    HidP_GetValueCaps(HidP_Input, valueCaps, &valueCapsLength, preparsedData);
    if (valueCapsLength <= 0 || valueCaps[0].ReportCount != 1) {
      hid_deinit();
      return -9;
    }

    inputCaps.reportId = valueCaps[0].ReportID;
    inputCaps.usage = valueCaps[0].NotRange.Usage;
    inputCaps.usagePage = valueCaps[0].UsagePage;

    memset(propBuf, 0, sizeof(propBuf));
    valueCapsLength = 2;
    HidP_GetValueCaps(HidP_Feature, valueCaps, &valueCapsLength, preparsedData);
    if (valueCapsLength <= 0 || valueCaps[0].ReportCount != 1) {
      hid_deinit();
      return -10;
    }

    featureCaps.reportId = valueCaps[0].ReportID;
    featureCaps.usage = valueCaps[0].NotRange.Usage;
    featureCaps.usagePage = valueCaps[0].UsagePage;

    return 0;
  }
  return -11; // Device not found
}

int hid_getBrightness (ULONG *val) {
  if (hDeviceObject == INVALID_HANDLE_VALUE) return -1;

  uint8_t dataBuf[100];
  memset(dataBuf, 0, sizeof(dataBuf));
  dataBuf[0] = inputCaps.reportId;
  if (!HidD_GetInputReport(hDeviceObject, dataBuf, sizeof(dataBuf))) return -2;

  NTSTATUS status = HidP_GetUsageValue(HidP_Input, inputCaps.usagePage, 0, inputCaps.usage, val, preparsedData, reinterpret_cast<PCHAR>(dataBuf), inputCaps.reportLength);
  if (status != HIDP_STATUS_SUCCESS) return -3;

  return 0;
}

int hid_setBrightness (ULONG val) {
  if (hDeviceObject == INVALID_HANDLE_VALUE) return -1;

  uint8_t dataBuf[100];
  memset(dataBuf, 0, sizeof(dataBuf));
  dataBuf[0] = featureCaps.reportId;
  if (!HidD_GetFeature(hDeviceObject, dataBuf, sizeof(dataBuf))) return -2;

  NTSTATUS status = HidP_SetUsageValue(HidP_Feature, featureCaps.usagePage, 0, featureCaps.usage, val, preparsedData, reinterpret_cast<PCHAR>(dataBuf), featureCaps.reportLength);
  if (status != HIDP_STATUS_SUCCESS) return -3;

  if (!HidD_SetFeature(hDeviceObject, dataBuf, featureCaps.reportLength)) return -4;

  return 0;
}

void hid_deinit () {
  if (preparsedData) {
    HidD_FreePreparsedData(preparsedData);
    preparsedData = nullptr;
  }

  if (hDeviceObject != INVALID_HANDLE_VALUE) {
    CloseHandle(hDeviceObject);
    hDeviceObject = INVALID_HANDLE_VALUE;
  }
}
