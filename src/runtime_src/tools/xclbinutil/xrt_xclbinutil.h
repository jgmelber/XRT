// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

/**
 * @file xrt_xclbinutil.h
 * @brief C API for xclbinutil operations
 *
 * This header provides a C API layer for invoking xclbinutil operations
 * programmatically without forking a subprocess. The API wraps the
 * existing XclBin C++ class behind an opaque-handle interface following
 * XRT C API conventions.
 */

#ifndef XRT_XCLBINUTIL_H
#define XRT_XCLBINUTIL_H

#include "xrt/detail/config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque handle to an xclbinutil session (wraps XclBin C++ object).
 */
typedef void* xrtXclbinutilHandle;

// --------------------------------------------------------------------------
// Handle lifecycle
// --------------------------------------------------------------------------

/**
 * @brief Create a new empty xclbinutil handle.
 *
 * Creates a default in-memory xclbin image with no sections.
 *
 * @return Valid handle on success, NULL on failure.
 */
XRT_API_EXPORT
xrtXclbinutilHandle
xrtXclbinutilCreate(void);

/**
 * @brief Create a handle and immediately read an xclbin file.
 *
 * Equivalent to xrtXclbinutilCreate() followed by
 * xrtXclbinutilReadXclbin().
 *
 * @param filename  Path to the xclbin file to read.
 * @param migrate   If non-zero, migrate the xclbin forward to the
 *                  current binary format.
 * @return Valid handle on success, NULL on failure.
 */
XRT_API_EXPORT
xrtXclbinutilHandle
xrtXclbinutilOpen(const char* filename, int migrate);

/**
 * @brief Destroy an xclbinutil handle and free its resources.
 *
 * @param handle  Handle previously returned by xrtXclbinutilCreate()
 *                or xrtXclbinutilOpen(). Passing NULL is a no-op.
 * @return 0 on success, -1 on error (e.g. unknown handle).
 */
XRT_API_EXPORT
int
xrtXclbinutilFreeHandle(xrtXclbinutilHandle handle);

// --------------------------------------------------------------------------
// File I/O
// --------------------------------------------------------------------------

/**
 * @brief Read an xclbin binary file into the handle.
 *
 * @param handle    Valid xclbinutil handle.
 * @param filename  Path to the xclbin file to read.
 * @param migrate   If non-zero, migrate the xclbin forward.
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilReadXclbin(xrtXclbinutilHandle handle,
                        const char* filename,
                        int migrate);

/**
 * @brief Write the in-memory xclbin image to a file.
 *
 * @param handle     Valid xclbinutil handle.
 * @param filename   Output file path.
 * @param skip_uuid  If non-zero, do not update the xclbin UUID.
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilWriteXclbin(xrtXclbinutilHandle handle,
                         const char* filename,
                         int skip_uuid);

// --------------------------------------------------------------------------
// Section operations
// --------------------------------------------------------------------------

/**
 * @brief Add a section to the xclbin.
 *
 * @param handle        Valid xclbinutil handle.
 * @param section_spec  Section specification in the format
 *                      @c \<section\>:\<format\>:\<file\>
 *                      If section name is empty and format is JSON,
 *                      multiple sections are added from the JSON.
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilAddSection(xrtXclbinutilHandle handle,
                        const char* section_spec);

/**
 * @brief Add a section, or replace it if it already exists.
 *
 * @param handle        Valid xclbinutil handle.
 * @param section_spec  Section specification: @c \<section\>:\<format\>:\<file\>
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilAddReplaceSection(xrtXclbinutilHandle handle,
                               const char* section_spec);

/**
 * @brief Add a section, or merge with an existing one.
 *
 * @param handle        Valid xclbinutil handle.
 * @param section_spec  Section specification: @c \<section\>:\<format\>:\<file\>
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilAddMergeSection(xrtXclbinutilHandle handle,
                             const char* section_spec);

/**
 * @brief Replace an existing section in the xclbin.
 *
 * @param handle        Valid xclbinutil handle.
 * @param section_spec  Section specification: @c \<section\>:\<format\>:\<file\>
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilReplaceSection(xrtXclbinutilHandle handle,
                            const char* section_spec);

/**
 * @brief Remove a section from the xclbin by name.
 *
 * @param handle  Valid xclbinutil handle.
 * @param name    Section name to remove (e.g. "BITSTREAM").
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilRemoveSection(xrtXclbinutilHandle handle,
                           const char* name);

/**
 * @brief Dump a section from the xclbin to a file.
 *
 * @param handle        Valid xclbinutil handle.
 * @param section_spec  Section specification: @c \<section\>:\<format\>:\<file\>
 *                      If section name is empty and format is JSON,
 *                      multiple sections are dumped.
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilDumpSection(xrtXclbinutilHandle handle,
                         const char* section_spec);

/**
 * @brief Check whether a section of a given kind exists.
 *
 * @param handle  Valid xclbinutil handle.
 * @param kind    Section kind value (axlf_section_kind enum).
 * @param index   Index name for indexed sections (NULL or "" for none).
 * @return 1 if found, 0 if not found, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilFindSection(xrtXclbinutilHandle handle,
                         int kind,
                         const char* index);

// --------------------------------------------------------------------------
// Metadata key-value operations
// --------------------------------------------------------------------------

/**
 * @brief Set a key-value pair in the xclbin metadata.
 *
 * @param handle     Valid xclbinutil handle.
 * @param key_value  Key-value specification: @c [USER|SYS]:\<key\>:\<value\>
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilSetKeyValue(xrtXclbinutilHandle handle,
                         const char* key_value);

/**
 * @brief Remove a key from the xclbin metadata.
 *
 * @param handle  Valid xclbinutil handle.
 * @param key     Key specification to remove.
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilRemoveKey(xrtXclbinutilHandle handle,
                       const char* key);

// --------------------------------------------------------------------------
// Kernel operations
// --------------------------------------------------------------------------

/**
 * @brief Add a PS kernel to the xclbin.
 *
 * @param handle  Valid xclbinutil handle.
 * @param spec    PS kernel spec:
 *                @c [mem_banks]:[symbol_name]:[instances]:\<path_to_shared_library\>
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilAddPsKernel(xrtXclbinutilHandle handle,
                         const char* spec);

/**
 * @brief Add fixed kernels from a JSON description file.
 *
 * @param handle     Valid xclbinutil handle.
 * @param json_file  Path to the JSON kernel description file.
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilAddKernels(xrtXclbinutilHandle handle,
                        const char* json_file);

// --------------------------------------------------------------------------
// Info / Validation
// --------------------------------------------------------------------------

/**
 * @brief Update the interface UUID in the xclbin.
 *
 * @param handle  Valid xclbinutil handle.
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilUpdateInterfaceUuid(xrtXclbinutilHandle handle);

/**
 * @brief Report xclbin information and write to a file.
 *
 * @param handle       Valid xclbinutil handle.
 * @param input_file   Original input file path (for display purposes,
 *                     may be NULL or "").
 * @param output_file  Path to write the report. Use NULL or "" for stdout.
 * @param verbose      If non-zero, produce verbose output.
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilReportInfoToFile(xrtXclbinutilHandle handle,
                              const char* input_file,
                              const char* output_file,
                              int verbose);

/**
 * @brief Report xclbin information into a caller-provided buffer.
 *
 * Call with @p buf = NULL and @p size = 0 to query the required size
 * (returned in @p ret_size).  Then call again with a sufficiently
 * large buffer.
 *
 * @param handle      Valid xclbinutil handle.
 * @param input_file  Original input file path (for display, may be NULL).
 * @param verbose     If non-zero, produce verbose output.
 * @param buf         Output buffer (may be NULL for size query).
 * @param size        Size of @p buf in bytes.
 * @param ret_size    Receives the number of bytes needed/written
 *                    (may be NULL).
 * @return 0 on success, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilReportInfoToBuffer(xrtXclbinutilHandle handle,
                                const char* input_file,
                                int verbose,
                                char* buf,
                                size_t size,
                                size_t* ret_size);

/**
 * @brief Check whether the xclbin contains at least one valid section
 *        recognized by the Linux @c file command.
 *
 * @param handle  Valid xclbinutil handle.
 * @return 1 if valid section found, 0 if not, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilCheckForValidSection(xrtXclbinutilHandle handle);

/**
 * @brief Check whether the xclbin contains platform VBNV information.
 *
 * @param handle  Valid xclbinutil handle.
 * @return 1 if platform VBNV found, 0 if not, -1 on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilCheckForPlatformVbnv(xrtXclbinutilHandle handle);

// --------------------------------------------------------------------------
// CLI convenience
// --------------------------------------------------------------------------

/**
 * @brief Execute xclbinutil as if invoked from the command line.
 *
 * This wraps the internal @c main_() entry point so that the full CLI
 * can be driven programmatically.
 *
 * @param argc  Argument count (same semantics as @c main()).
 * @param argv  Argument vector (same semantics as @c main()).
 * @return 0 on success, non-zero on error.
 */
XRT_API_EXPORT
int
xrtXclbinutilExecArgs(int argc, const char** argv);

#ifdef __cplusplus
}
#endif

#endif // XRT_XCLBINUTIL_H
