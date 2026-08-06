#include "LTE/Array.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Location.h"
#include "LTE/OS.h"

static Function const Directory_List_Registration = Function_Bind(
  "Directory_List",
  "Return a list of all files and directories in 'path'",
  [](String const& path) -> Vector<String>
  {
  Vector<String> files = OS_ListDir(path);
  files.remove(".");
  files.remove("..");
  return files;
  },
  "path");
static int const Directory_List_Alias = Function_Alias("Directory_List", "ListDir");

static Function const OS_IsDirectory_Registration = Function_Bind(
  "OS_IsDirectory",
  "Return whether 'path' is a directory",
  [](String const& path) -> bool
  {
  return OS_IsDir(path);
  },
  "path");
static int const OS_IsDirectory_Alias = Function_Alias("OS_IsDirectory", "IsDirectory");

static Function const OS_IsFile_Registration = Function_Bind(
  "OS_IsFile",
  "Return whether 'path' is a file",
  [](String const& path) -> bool
  {
  return OS_IsFile(path);
  },
  "path");
static int const OS_IsFile_Alias = Function_Alias("OS_IsFile", "IsFile");

static Function const File_Read_Registration = Function_Bind(
  "File_Read",
  "Return the contents of the file located at 'path'",
  [](String const& path) -> String
  {
  Location location = Location_File(path);
  if (!location->Exists())
    return "";
  return location->ReadAscii();
  },
  "path");

static Function const File_Write_Registration = Function_Bind(
  "File_Write",
  "Write 'contents' to the file at 'path'. Returns true on success.",
  [](String const& path, String const& contents) -> bool
  {
  Location location = Location_File(path);
  Array<uchar> data((size_t)contents.length(), (uchar const*)contents.c_str());
  return location->Write(data);
  },
  "path", "contents");
static int const File_Write_Alias = Function_Alias("File_Write", "WriteFile");
