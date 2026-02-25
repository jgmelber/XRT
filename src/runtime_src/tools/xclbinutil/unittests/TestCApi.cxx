// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

#include "xrt_xclbinutil.h"
#include "globals.h"
#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Handle lifecycle tests
// ---------------------------------------------------------------------------

TEST(CApi, CreateAndFreeHandle) {
  xrtXclbinutilHandle h = xrtXclbinutilCreate();
  ASSERT_NE(h, nullptr) << "xrtXclbinutilCreate() returned NULL";

  int rc = xrtXclbinutilFreeHandle(h);
  EXPECT_EQ(rc, 0) << "xrtXclbinutilFreeHandle() failed on valid handle";
}

TEST(CApi, DoubleFreeReturnsError) {
  xrtXclbinutilHandle h = xrtXclbinutilCreate();
  ASSERT_NE(h, nullptr);

  EXPECT_EQ(xrtXclbinutilFreeHandle(h), 0);
  EXPECT_EQ(xrtXclbinutilFreeHandle(h), -1)
      << "Double-free should return -1";
}

TEST(CApi, FreeNullIsNoOp) {
  EXPECT_EQ(xrtXclbinutilFreeHandle(nullptr), 0)
      << "Freeing NULL should succeed silently";
}

// ---------------------------------------------------------------------------
// Open / Read tests
// ---------------------------------------------------------------------------

TEST(CApi, OpenValidXclbin) {
  fs::path sampleXclbin(TestUtilities::getResourceDir());
  sampleXclbin /= "sample_1_2018.2.xclbin";

  xrtXclbinutilHandle h = xrtXclbinutilOpen(sampleXclbin.string().c_str(), 0);
  ASSERT_NE(h, nullptr) << "xrtXclbinutilOpen() returned NULL for valid xclbin";

  EXPECT_EQ(xrtXclbinutilFreeHandle(h), 0);
}

TEST(CApi, OpenNonexistentFileReturnsNull) {
  xrtXclbinutilHandle h = xrtXclbinutilOpen("/nonexistent/path/to.xclbin", 0);
  EXPECT_EQ(h, nullptr) << "xrtXclbinutilOpen() should return NULL for missing file";
}

TEST(CApi, ReadXclbin) {
  fs::path sampleXclbin(TestUtilities::getResourceDir());
  sampleXclbin /= "sample_1_2018.2.xclbin";

  xrtXclbinutilHandle h = xrtXclbinutilCreate();
  ASSERT_NE(h, nullptr);

  int rc = xrtXclbinutilReadXclbin(h, sampleXclbin.string().c_str(), 0);
  EXPECT_EQ(rc, 0) << "xrtXclbinutilReadXclbin() failed";

  EXPECT_EQ(xrtXclbinutilFreeHandle(h), 0);
}

// ---------------------------------------------------------------------------
// Write / round-trip test
// ---------------------------------------------------------------------------

TEST(CApi, WriteRoundTrip) {
  fs::path sampleXclbin(TestUtilities::getResourceDir());
  sampleXclbin /= "sample_1_2018.2.xclbin";

  // Read an xclbin
  xrtXclbinutilHandle h = xrtXclbinutilOpen(sampleXclbin.string().c_str(), 0);
  ASSERT_NE(h, nullptr);

  // Write it out to a temp file
  fs::path tmpOut = fs::temp_directory_path() / "capi_roundtrip_test.xclbin";
  int rc = xrtXclbinutilWriteXclbin(h, tmpOut.string().c_str(), 1 /* skip uuid */);
  EXPECT_EQ(rc, 0) << "xrtXclbinutilWriteXclbin() failed";

  // Verify the output file exists and is non-empty
  EXPECT_TRUE(fs::exists(tmpOut)) << "Output xclbin was not written";
  EXPECT_GT(fs::file_size(tmpOut), 0u) << "Output xclbin is empty";

  // Read it back
  xrtXclbinutilHandle h2 = xrtXclbinutilOpen(tmpOut.string().c_str(), 0);
  EXPECT_NE(h2, nullptr) << "Could not re-read written xclbin";

  xrtXclbinutilFreeHandle(h2);
  xrtXclbinutilFreeHandle(h);
  fs::remove(tmpOut);
}

// ---------------------------------------------------------------------------
// Section operations
// ---------------------------------------------------------------------------

TEST(CApi, AddFindRemoveSection) {
  fs::path sampleXclbin(TestUtilities::getResourceDir());
  sampleXclbin /= "sample_1_2018.2.xclbin";

  xrtXclbinutilHandle h = xrtXclbinutilOpen(sampleXclbin.string().c_str(), 0);
  ASSERT_NE(h, nullptr);

  // CLEARING_BITSTREAM should not exist initially
  // CLEARING_BITSTREAM = 2
  int found = xrtXclbinutilFindSection(h, 2 /* CLEARING_BITSTREAM */, nullptr);
  EXPECT_EQ(found, 0) << "CLEARING_BITSTREAM should not exist initially";

  // Add a CLEARING_BITSTREAM section using the sample xclbin as raw data
  std::string addSpec = std::string("CLEARING_BITSTREAM:RAW:") + sampleXclbin.string();
  int rc = xrtXclbinutilAddSection(h, addSpec.c_str());
  EXPECT_EQ(rc, 0) << "xrtXclbinutilAddSection() failed";

  // Now it should be found
  found = xrtXclbinutilFindSection(h, 2 /* CLEARING_BITSTREAM */, nullptr);
  EXPECT_EQ(found, 1) << "CLEARING_BITSTREAM should exist after add";

  // Remove it
  rc = xrtXclbinutilRemoveSection(h, "CLEARING_BITSTREAM");
  EXPECT_EQ(rc, 0) << "xrtXclbinutilRemoveSection() failed";

  // Should no longer exist
  found = xrtXclbinutilFindSection(h, 2 /* CLEARING_BITSTREAM */, nullptr);
  EXPECT_EQ(found, 0) << "CLEARING_BITSTREAM should not exist after remove";

  xrtXclbinutilFreeHandle(h);
}

// ---------------------------------------------------------------------------
// Metadata key-value tests
// ---------------------------------------------------------------------------

TEST(CApi, SetAndRemoveKeyValue) {
  xrtXclbinutilHandle h = xrtXclbinutilCreate();
  ASSERT_NE(h, nullptr);

  int rc = xrtXclbinutilSetKeyValue(h, "USER:test_key:test_value");
  EXPECT_EQ(rc, 0) << "xrtXclbinutilSetKeyValue() failed";

  rc = xrtXclbinutilRemoveKey(h, "USER:test_key");
  EXPECT_EQ(rc, 0) << "xrtXclbinutilRemoveKey() failed";

  xrtXclbinutilFreeHandle(h);
}

// ---------------------------------------------------------------------------
// ReportInfo to buffer
// ---------------------------------------------------------------------------

TEST(CApi, ReportInfoToBuffer) {
  fs::path sampleXclbin(TestUtilities::getResourceDir());
  sampleXclbin /= "sample_1_2018.2.xclbin";

  xrtXclbinutilHandle h = xrtXclbinutilOpen(sampleXclbin.string().c_str(), 0);
  ASSERT_NE(h, nullptr);

  // Query required size
  size_t needed = 0;
  int rc = xrtXclbinutilReportInfoToBuffer(h, sampleXclbin.string().c_str(),
                                           0, nullptr, 0, &needed);
  EXPECT_EQ(rc, 0) << "ReportInfoToBuffer size query failed";
  EXPECT_GT(needed, 0u) << "Report should be non-empty";

  // Fill buffer
  std::vector<char> buf(needed);
  size_t written = 0;
  rc = xrtXclbinutilReportInfoToBuffer(h, sampleXclbin.string().c_str(),
                                       0, buf.data(), buf.size(), &written);
  EXPECT_EQ(rc, 0) << "ReportInfoToBuffer fill failed";
  EXPECT_EQ(written, needed) << "Size mismatch on second call";

  // Verify it contains something meaningful
  std::string report(buf.data());
  EXPECT_FALSE(report.empty()) << "Report string is empty";

  xrtXclbinutilFreeHandle(h);
}

// ---------------------------------------------------------------------------
// ExecArgs
// ---------------------------------------------------------------------------

TEST(CApi, ExecArgsVersion) {
  const char* argv[] = {"xclbinutil", "--version"};
  int rc = xrtXclbinutilExecArgs(2, argv);
  EXPECT_EQ(rc, 0) << "ExecArgs --version should succeed";
}

// ---------------------------------------------------------------------------
// Invalid handle tests
// ---------------------------------------------------------------------------

TEST(CApi, InvalidHandleReturnsError) {
  void* bad = reinterpret_cast<void*>(static_cast<uintptr_t>(0xDEADBEEF));

  EXPECT_EQ(xrtXclbinutilReadXclbin(bad, "foo.xclbin", 0), -1);
  EXPECT_EQ(xrtXclbinutilWriteXclbin(bad, "foo.xclbin", 0), -1);
  EXPECT_EQ(xrtXclbinutilAddSection(bad, "FOO:RAW:bar"), -1);
  EXPECT_EQ(xrtXclbinutilRemoveSection(bad, "FOO"), -1);
  EXPECT_EQ(xrtXclbinutilFindSection(bad, 0, nullptr), -1);
  EXPECT_EQ(xrtXclbinutilSetKeyValue(bad, "USER:k:v"), -1);
  EXPECT_EQ(xrtXclbinutilRemoveKey(bad, "USER:k"), -1);
  EXPECT_EQ(xrtXclbinutilUpdateInterfaceUuid(bad), -1);
  EXPECT_EQ(xrtXclbinutilReportInfoToFile(bad, "", "", 0), -1);
  EXPECT_EQ(xrtXclbinutilReportInfoToBuffer(bad, "", 0, nullptr, 0, nullptr), -1);
  EXPECT_EQ(xrtXclbinutilCheckForValidSection(bad), -1);
  EXPECT_EQ(xrtXclbinutilCheckForPlatformVbnv(bad), -1);
  EXPECT_EQ(xrtXclbinutilFreeHandle(bad), -1);
}
