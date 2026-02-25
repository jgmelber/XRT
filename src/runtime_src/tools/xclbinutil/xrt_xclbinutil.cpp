// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

/**
 * @file xrt_xclbinutil.cpp
 * @brief Implementation of the xclbinutil C API.
 *
 * Each exported function wraps the corresponding XclBin C++ method
 * behind an opaque handle, following the XRT error convention:
 *   try { ... return 0; } catch (...) { xrt_error_msg; return -1; }
 */

#define XRT_API_SOURCE  // for XRT_API_EXPORT visibility

#include "xrt_xclbinutil.h"
#include "XclBinClass.h"
#include "XclBinUtilMain.h"
#include "ParameterSectionData.h"
#include "Section.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>

// ---------------------------------------------------------------------------
// Handle management
// ---------------------------------------------------------------------------
namespace {

std::mutex s_mutex;
std::map<void*, std::unique_ptr<XclBin>> s_handles;

XclBin*
get_xclbin(xrtXclbinutilHandle handle)
{
  // Caller must hold s_mutex or guarantee single-threaded access
  auto it = s_handles.find(handle);
  if (it == s_handles.end())
    return nullptr;
  return it->second.get();
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Handle lifecycle
// ---------------------------------------------------------------------------

xrtXclbinutilHandle
xrtXclbinutilCreate(void)
{
  try {
    auto xclbin = std::make_unique<XclBin>();
    void* handle = xclbin.get();
    std::lock_guard<std::mutex> lock(s_mutex);
    s_handles[handle] = std::move(xclbin);
    return handle;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilCreate() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return nullptr;
}

xrtXclbinutilHandle
xrtXclbinutilOpen(const char* filename, int migrate)
{
  try {
    auto xclbin = std::make_unique<XclBin>();
    xclbin->readXclBinBinary(filename, migrate != 0);
    void* handle = xclbin.get();
    std::lock_guard<std::mutex> lock(s_mutex);
    s_handles[handle] = std::move(xclbin);
    return handle;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilOpen() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return nullptr;
}

int
xrtXclbinutilFreeHandle(xrtXclbinutilHandle handle)
{
  if (!handle)
    return 0;

  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_handles.find(handle);
    if (it == s_handles.end()) {
      std::cerr << "ERROR: xrtXclbinutilFreeHandle() - unknown handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    s_handles.erase(it);
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilFreeHandle() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

// ---------------------------------------------------------------------------
// File I/O
// ---------------------------------------------------------------------------

int
xrtXclbinutilReadXclbin(xrtXclbinutilHandle handle,
                        const char* filename,
                        int migrate)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilReadXclbin() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    xclbin->readXclBinBinary(filename, migrate != 0);
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilReadXclbin() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilWriteXclbin(xrtXclbinutilHandle handle,
                         const char* filename,
                         int skip_uuid)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilWriteXclbin() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    xclbin->writeXclBinBinary(filename, skip_uuid != 0);
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilWriteXclbin() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

// ---------------------------------------------------------------------------
// Section operations
// ---------------------------------------------------------------------------

int
xrtXclbinutilAddSection(xrtXclbinutilHandle handle,
                        const char* section_spec)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilAddSection() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    ParameterSectionData psd(section_spec);
    if (psd.getSectionName().empty() &&
        psd.getFormatType() == Section::FormatType::json) {
      xclbin->addSections(psd);
    } else {
      xclbin->addSection(psd);
    }
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilAddSection() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilAddReplaceSection(xrtXclbinutilHandle handle,
                               const char* section_spec)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilAddReplaceSection() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    ParameterSectionData psd(section_spec);
    xclbin->addReplaceSection(psd);
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilAddReplaceSection() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilAddMergeSection(xrtXclbinutilHandle handle,
                             const char* section_spec)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilAddMergeSection() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    ParameterSectionData psd(section_spec);
    xclbin->addMergeSection(psd);
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilAddMergeSection() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilReplaceSection(xrtXclbinutilHandle handle,
                            const char* section_spec)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilReplaceSection() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    ParameterSectionData psd(section_spec);
    xclbin->replaceSection(psd);
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilReplaceSection() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilRemoveSection(xrtXclbinutilHandle handle,
                           const char* name)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilRemoveSection() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    xclbin->removeSection(name);
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilRemoveSection() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilDumpSection(xrtXclbinutilHandle handle,
                         const char* section_spec)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilDumpSection() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    ParameterSectionData psd(section_spec);
    if (psd.getSectionName().empty() &&
        psd.getFormatType() == Section::FormatType::json) {
      xclbin->dumpSections(psd);
    } else {
      xclbin->dumpSection(psd);
    }
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilDumpSection() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilFindSection(xrtXclbinutilHandle handle,
                         int kind,
                         const char* index)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilFindSection() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    std::string indexName = (index != nullptr) ? index : "";
    auto* section = xclbin->findSection(static_cast<axlf_section_kind>(kind), indexName);
    return (section != nullptr) ? 1 : 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilFindSection() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

// ---------------------------------------------------------------------------
// Metadata key-value operations
// ---------------------------------------------------------------------------

int
xrtXclbinutilSetKeyValue(xrtXclbinutilHandle handle,
                         const char* key_value)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilSetKeyValue() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    xclbin->setKeyValue(key_value);
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilSetKeyValue() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilRemoveKey(xrtXclbinutilHandle handle,
                       const char* key)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilRemoveKey() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    xclbin->removeKey(key);
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilRemoveKey() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

// ---------------------------------------------------------------------------
// Kernel operations
// ---------------------------------------------------------------------------

int
xrtXclbinutilAddPsKernel(xrtXclbinutilHandle handle,
                         const char* spec)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilAddPsKernel() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    xclbin->addPsKernel(spec);
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilAddPsKernel() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilAddKernels(xrtXclbinutilHandle handle,
                        const char* json_file)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilAddKernels() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    xclbin->addKernels(json_file);
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilAddKernels() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

// ---------------------------------------------------------------------------
// Info / Validation
// ---------------------------------------------------------------------------

int
xrtXclbinutilUpdateInterfaceUuid(xrtXclbinutilHandle handle)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilUpdateInterfaceUuid() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    xclbin->updateInterfaceuuid();
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilUpdateInterfaceUuid() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilReportInfoToFile(xrtXclbinutilHandle handle,
                              const char* input_file,
                              const char* output_file,
                              int verbose)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilReportInfoToFile() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }

    std::string inputStr = (input_file != nullptr) ? input_file : "";
    bool bVerbose = (verbose != 0);

    if (output_file == nullptr || output_file[0] == '\0') {
      xclbin->reportInfo(std::cout, inputStr, bVerbose);
    } else {
      std::fstream ofs;
      ofs.open(output_file, std::ios::out | std::ios::binary);
      if (!ofs.is_open()) {
        std::string errMsg = "Unable to open output file: ";
        errMsg += output_file;
        throw std::runtime_error(errMsg);
      }
      xclbin->reportInfo(ofs, inputStr, bVerbose);
      ofs.close();
    }
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilReportInfoToFile() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilReportInfoToBuffer(xrtXclbinutilHandle handle,
                                const char* input_file,
                                int verbose,
                                char* buf,
                                size_t size,
                                size_t* ret_size)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilReportInfoToBuffer() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }

    std::string inputStr = (input_file != nullptr) ? input_file : "";
    bool bVerbose = (verbose != 0);

    std::ostringstream oss;
    xclbin->reportInfo(oss, inputStr, bVerbose);
    std::string report = oss.str();

    // Include space for null terminator
    size_t needed = report.size() + 1;
    if (ret_size)
      *ret_size = needed;

    if (buf != nullptr && size > 0) {
      size_t to_copy = (needed <= size) ? report.size() : (size - 1);
      std::memcpy(buf, report.data(), to_copy);
      buf[to_copy] = '\0';
    }
    return 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilReportInfoToBuffer() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilCheckForValidSection(xrtXclbinutilHandle handle)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilCheckForValidSection() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    return xclbin->checkForValidSection() ? 1 : 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilCheckForValidSection() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

int
xrtXclbinutilCheckForPlatformVbnv(xrtXclbinutilHandle handle)
{
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto* xclbin = get_xclbin(handle);
    if (!xclbin) {
      std::cerr << "ERROR: xrtXclbinutilCheckForPlatformVbnv() - invalid handle" << std::endl;
      errno = EINVAL;
      return -1;
    }
    return xclbin->checkForPlatformVbnv() ? 1 : 0;
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilCheckForPlatformVbnv() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}

// ---------------------------------------------------------------------------
// CLI convenience
// ---------------------------------------------------------------------------

int
xrtXclbinutilExecArgs(int argc, const char** argv)
{
  try {
    return main_(argc, argv);
  }
  catch (const std::exception& ex) {
    std::cerr << "ERROR: xrtXclbinutilExecArgs() - " << ex.what() << std::endl;
    errno = EINVAL;
  }
  return -1;
}
