/*#include <fstream>
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
#define MAX_FILE_IINDEX 1024*/

#include "main.hpp"
/*
static int MaxFlag=0;
static int MaxSelFlag=0;*/

bool customsort(string s1, string s2)
{
	int i , j;
	size_t pos1, pos2;
	
	pos1 = s1.find(logfileformat);
	pos2 = s2.find(logfileformat);
	if (pos1 != std::string::npos)
	{
		s1.erase(pos1,logfileformat.length());
		i=std::stoi(s1);
	}
	else
	{
		i=0;
	}
	if (pos2 != std::string::npos)
	{
		s2.erase(pos2,logfileformat.length());
		j=std::stoi(s2);
	}
	else
	{
		j=0;
	}
	if ( i==j)
		return i > j;
	else
		return i < j;
}

bool getSELLogFiles(std::vector<std::string>& selLogFiles, int* filecount)
{
    // Loop through the directory looking for ipmi_sel log files
    for (const std::filesystem::directory_entry& dirEnt :
         std::filesystem::directory_iterator(selLogDir))
    {
        std::string filename = dirEnt.path().filename();
        if (filename.find(redfishLogFilename) != std::string::npos)
        {
			
            // If we find an ipmi_sel log file, save the path
            selLogFiles.emplace_back(selLogDir /
                                     filename);
			*filecount=*filecount+1;
        }
    }
    // As the log files rotate, they are appended with a ".#" that is higher for
    // the older logs. Since we don't expect more than 10 log files, we
    // can just sort the list to get them in order from newest to oldest
    std::sort(selLogFiles.begin(), selLogFiles.end(),customsort);

    return !selLogFiles.empty();
}

int main()
{
	int filecount=0,index=0,linecount=0,i=0;
	std::string entry;
	std::vector<std::string> selLogFiles;
	std::string newfile;
	char buf[64*1024];
	std::fstream logstream;
	std::fstream Writefile;
    struct stat stats;
	std::string tempbuf;
	std::string indexstr;
	int indexFlag=0;
	int totalrecords=0;
	size_t pos;
	fstream LastSelIdFile;
	int LastSelId = 1;

	memset(buf, '\0', sizeof(buf) );
	
	while(fgets(buf, sizeof(buf), stdin) != NULL)
	{

			//Reinitializing variables
			filecount=0;linecount=0;totalrecords=0;index=0;
			pos=0; selLogFiles.clear();	indexstr.clear(); tempbuf.clear();
			
			if (access(selbusyflag, F_OK) == 0)
			{
				fputs("OK",stdout);
				break;
				return 0;
			}
			
			//Get the file list
			getSELLogFiles(selLogFiles, &filecount);
			
			//Reinitialize MaxFlag with latest file list while reboot
			if (( MaxFlag == 0 ) && (filecount > 1))
			{
				
				tempbuf.clear();				
				tempbuf = selLogFiles[filecount-1];
				pos = tempbuf.find(logfileformat);
				if (pos != std::string::npos)
				{
					tempbuf.erase(pos,logfileformat.length());
				}
				index=std::stoi(tempbuf);
				index=index+1;
					
				if (index > MAX_FILE_IINDEX) 
				{
					MaxSelFlag=0;
					for ( i=1 ; i < filecount -1 ; i++)
					{
						tempbuf.clear();
						tempbuf = selLogFiles[i];
						pos = tempbuf.find(logfileformat);
						if (pos != std::string::npos)
						{
							tempbuf.erase(pos,logfileformat.length());
						}
						index=std::stoi(tempbuf); 
						if (index > OBMC_MAX_SEL_RECORDS/RECORDS_PER_FILE)						
							break;
					}
					MaxFlag = i-1;
					
				}
			}
			//Reinitialize flag value
			if (MaxFlag == OBMC_MAX_SEL_RECORDS/RECORDS_PER_FILE)
				MaxFlag = 0;
			
			//Write into log file for the first time
			if(filecount == 0)
			{
				logstream.open(logfilename.c_str(), ios::out);
				if(!logstream)
					return -1;
				else
				{
					logstream << buf;
					logstream.seekp(0,std::ios::end);
					logstream.close();
				}


				LastSelIdFile.open(LAST_SEL_ID_FILE, ios::out);
                                if( !LastSelIdFile ) { // file couldn't be opened
					std::cerr << "Error: file could not be opened" << endl;
                                }

				LastSelId = 1;
				LastSelIdFile << LastSelId << endl;

                                LastSelIdFile.close();					
			}
			else
			{
				//Read latest File and get the count
				std::ifstream logStream(logfilename.c_str());
				if (!logStream.is_open())
					return -1;
				while(std::getline(logStream, entry))
					linecount++;
				logStream.close();
				
				//Get total records
				totalrecords = linecount;
				totalrecords += (filecount -1) * RECORDS_PER_FILE;
				
				
				
				//Remove oldest file
				if (( totalrecords == OBMC_MAX_SEL_RECORDS + RECORDS_PER_FILE ) || ( MaxSelFlag == 1 && totalrecords == OBMC_MAX_SEL_RECORDS + RECORDS_PER_FILE-2))
                {
					tempbuf.clear();
					
					if (MaxFlag == 0)
						tempbuf=selLogFiles[1];
					else
						tempbuf = selLogFiles[MaxFlag+1];
					Writefile.open(selrotateflag, ios::out);
					Writefile.close();
                    remove(tempbuf.c_str());
					remove(selrotateflag);
                }
				
				//Rotate the file;
				if((linecount == RECORDS_PER_FILE)|| ( MaxSelFlag == 1 && linecount == RECORDS_PER_FILE -2))
				{
					if (filecount > 1 )
					{
						tempbuf.clear();				
						tempbuf = selLogFiles[filecount-1];
						pos = tempbuf.find(logfileformat);
						if (pos != std::string::npos)
						{
							tempbuf.erase(pos,logfileformat.length());
						}
						index=std::stoi(tempbuf);
						index=index+1;
						if (index > MAX_FILE_IINDEX)
						{
							MaxSelFlag=0;
							MaxFlag = MaxFlag+1;
							index=MaxFlag;
							
						}
					}
					else
					{
						index=filecount;
					}
					if( index == MAX_FILE_IINDEX -1 )
					{
						MaxSelFlag=1;
					}
					indexstr = std::to_string(index);
                	newfile = logfileformat;
		            newfile = newfile + indexstr;
					Writefile.open(selrotateflag, ios::out);
					Writefile.close();
					rename(logfilename.c_str(),newfile.c_str());
					remove(selrotateflag);
				}
				
				//write logs into file
				if (stat(logfilename.c_str(), &stats) == 0)
                {
					logstream.open(logfilename.c_str(), ios::app);
                }
                else
                {
                    logstream.open(logfilename.c_str(), ios::out);
                }
                if(!logstream)
                    return -1;
                else
                {
                    logstream << buf;
					logstream.seekp(0,std::ios::end);
                    logstream.close();


		    LastSelIdFile.open(LAST_SEL_ID_FILE, ios::in);

                    if( !LastSelIdFile ) { // file couldn't be opened
                          std::cerr << "Error: file could not be opened" << endl;
                    }

		    /* Retrieve the older SEL ID */
    	            LastSelIdFile >> LastSelId;

    		    LastSelIdFile.close();

		    LastSelId++;

    	            LastSelIdFile.open(LAST_SEL_ID_FILE, ios::out);

		    if(LastSelId == MaxSelId )
                            LastSelId = 1;

		    /* Update the latest SEL ID */
    		    LastSelIdFile << LastSelId;

		    LastSelIdFile.close();
                }
				
				
			}
	}
	fputs("OK",stdout);
	return 0;
}
