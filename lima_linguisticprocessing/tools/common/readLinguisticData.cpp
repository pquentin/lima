/*
    Copyright 2002-2013 CEA LIST

    This file is part of LIMA.

    LIMA is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LIMA is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with LIMA.  If not, see <http://www.gnu.org/licenses/>
*/
/***************************************************************************
 *   Copyright (C) 2004 by CEA - LIST                                      *
 *                                                                         *
 ***************************************************************************/
#include "common/LimaCommon.h"
// #include "common/linguisticData/linguisticData.h"
#include "common/XMLConfigurationFiles/xmlConfigurationFileParser.h"
#include "common/MediaticData/mediaticData.h"

#include <string>
#include <vector>
#include <iostream>

#include <QtCore/QCoreApplication>

using namespace Lima::Common::MediaticData;
using namespace std;

void usage(int argc, char* argv[]);

int main(int argc,char* argv[])
{
  QCoreApplication a(argc, argv);
  QsLogging::initQsLog();
  
  std::string resourcesPath=string(getenv("LIMA_RESOURCES"));
  std::string configDir=string(getenv("LIMA_CONF"));
  std::string configFile=string("lima-common.xml");

  std::deque<std::string> langs;

  if (argc>1)
  {
    for (int i = 1 ; i < argc; i++)
    {
      QString arg = QString::fromUtf8(argv[i]);
      int pos = -1;
      if ( arg[0] == '-' )
      {
        if (arg == "--help")
          usage(argc, argv);
        else if ( (arg.contains("--config-file=")) )
          configFile = arg.mid(pos+14).toUtf8().data();
        else if ( (arg.contains("--config-dir=")) )
          configDir = arg.mid(pos+13).toUtf8().data();
        else if ( (arg.contains("--resources-dir=")) )
          resourcesPath = arg.mid(pos+16).toUtf8().data();
        else usage(argc, argv);
      }
      else
      {
        langs.push_back(arg.toUtf8().data());
      }
    }
  }

  MediaticData::changeable().init(resourcesPath,configDir,configFile,langs);

}

void usage(int argc, char *argv[])
{
  LIMA_UNUSED(argc);
  std::cout << "usage: " << argv[0] << " [OPTIONS] [lang1 [lang2 [...]]] " << std::endl;
  std::cout << "\t--resources-dir=</path/to/the/resources> Optional. Default is $LIMA_RESOURCES" << std::endl;
  std::cout << "\t--config-dir=</path/to/the/configuration/directory> Optional. Default is $LIMA_CONF" << std::endl;
  std::cout << "\t--config-file=<configuration/file/name>\tOptional. Default is lima-common.xml" << std::endl;
  std::cout << "\twhere langs are languages to load. If no languages, load all." << std::endl;
  exit(0);
}
