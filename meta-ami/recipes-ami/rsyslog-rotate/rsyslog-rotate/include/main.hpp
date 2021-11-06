#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <unistd.h>
#include <map>
#include <string>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "projdef.h"

using namespace std;

static const std::filesystem::path selLogDir = "/var/sellog";
static const std::string redfishLogFilename = "redfish";
std::string logfileformat = "/var/sellog/redfish.";
std::string logfilename = "/var/sellog/redfish";
const char *selbusyflag = "/tmp/sel_busy";
const char *selrotateflag ="/tmp/selrotate";

#define RECORDS_PER_FILE 64
//#define MAX_RECORDS 4096
#define MAX_FILE_IINDEX 1024

#define LAST_SEL_ID_FILE "/var/sellog/LastSELID"

static int MaxFlag=0;
static int MaxSelFlag=0;
static int MaxSelId = 65535;
