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
/**
  * @file          MediaticData.cpp
  * @author        Gael de Chalendar <Gael.de-Chalendar@cea.fr> 

  *                Copyright (C) 2002-2012 by CEA LIST
  * @date          Started on Mon dec, 2 2002
  */

#include "mediaticData.h"

// ---------------------------------------------------------------------------
//  Local includes
// ---------------------------------------------------------------------------
#include "common/LimaCommon.h"
#include "common/QsLog/QsLog.h"
#include "common/XMLConfigurationFiles/xmlConfigurationFileExceptions.h"
#include "common/Data/readwritetools.h"
#include "common/misc/DoubleAccessObjectToIdMap.h"
//#include "common/misc/strwstrtools.h"
//#include "common/misc/strx.h"
//#include "common/misc/traceUtils.h"


// ---------------------------------------------------------------------------
//  Standard libraries includes
// ---------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <fstream>

using namespace std;

using namespace Lima::Common::XMLConfigurationFiles;
using namespace Lima::Common::Misc;
using namespace Lima::Common::MediaticData;

namespace Lima
{
namespace Common
{
using namespace XMLConfigurationFiles;
//using namespace Misc;

namespace MediaticData
{

class MediaticDataPrivate
{
  friend class MediaticData;

private:
  MediaticDataPrivate();
  virtual ~MediaticDataPrivate();
  
protected:

    virtual void initMedias(
        XMLConfigurationFiles::XMLConfigurationFileParser& configParser,
        const std::deque< std::string >& meds);

    void initReleaseStringsPool(
        XMLConfigurationFiles::XMLConfigurationFileParser& configParser);

    void initRelations(
        XMLConfigurationFiles::XMLConfigurationFileParser& configParser);

    std::deque< std::string > m_medias;

private:
    static const LimaString s_entityTypeNameSeparator;

    std::map< std::string, MediaId > m_mediasIds;
    std::map< MediaId, std::string > m_mediasSymbol;
    std::map< MediaId, std::string > m_mediaDefinitionFiles;
    std::map< MediaId, MediaData* > m_mediasData;

    // entity types
    typedef Common::Misc::DoubleAccessObjectToIdMap<LimaString,EntityGroupId> EntityGroupMap;
    typedef Common::Misc::DoubleAccessObjectToIdMap<LimaString,EntityTypeId> EntityTypeMap;
    EntityGroupMap m_entityGroups;
    std::vector<EntityTypeMap*> m_entityTypes;

    std::map< std::string, uint8_t > m_relTypes;
    std::map< uint8_t, std::string > m_relTypesNum;

    static std::string s_undefinedRelation;


    std::map<MediaId,FsaStringsPool*> m_stringsPool;
    bool m_releaseStringsPool;

    std::string m_resourcesPath;
    std::string m_configPath;

    static const std::string s_nocateg;

};
  
const LimaString MediaticDataPrivate::s_entityTypeNameSeparator=Common::Misc::utf8stdstring2limastring(".");


std::string MediaticDataPrivate::s_undefinedRelation("Unknown");
const std::string MediaticDataPrivate::s_nocateg=SYMBOLIC_NONE;


MediaticData::MediaticData() :
    Singleton<MediaticData>(),
    m_d(new MediaticDataPrivate())
{
}

MediaticData::MediaticData(const MediaticData& md) :
    Singleton<MediaticData>(),
    m_d(new MediaticDataPrivate(*md.m_d))
{
}

MediaticData::~MediaticData()
{
  delete m_d;
}

const std::map< std::string, MediaId >& MediaticData::getMediasIds() const
{
    return m_d->m_mediasIds;
}

const std::string& MediaticData::media(MediaId media) const
{
    return getMediaId(media);
}

MediaId MediaticData::media(const std::string& media) const
{
    return getMediaId(media);
}

const std::deque<std::string>& MediaticData::getMedias() const
{
    return m_d->m_medias;
}

/*********************************************************************
  * Configuration functions
  ********************************************************************/

bool MediaticData::releaseStringsPool() const
{
    return m_d->m_releaseStringsPool;
}

const std::string& MediaticData::getResourcesPath() const
{
    return m_d->m_resourcesPath;
}

const std::string& MediaticData::getConfigPath() const
{
    return m_d->m_configPath;
}

void MediaticData::init(
  const std::string& resourcesPath,
  const std::string& configPath,
  const std::string& configFile,
  const std::deque< std::string >& meds)
{

//  TimeUtils::updateCurrentTime();
  MDATALOGINIT;
  LINFO << "MediaticData::init " << resourcesPath.c_str() << " " << configPath.c_str() << " " << configFile.c_str();
  //LINFO << "Mediatic data initialization";

  m_d->m_resourcesPath=resourcesPath;
  m_d->m_configPath=configPath;

  //LINFO << "initialize XMLParser";
  initXMLParser();
  //LINFO << "parse configuration file: " << configPath << "/" << configFile;
  Common::XMLConfigurationFiles::XMLConfigurationFileParser configuration(configPath + "/" + configFile);

  LINFO << "MediaticData::init for ";
  for (std::deque< std::string >::const_iterator it = meds.begin(); it != meds.end(); it++)
    LINFO << "    " << (*it).c_str();

  //    initHomoSyntagmaticChainsAndRelationsTypes(*configParser);
  LDEBUG << "initialize global parameters";
 m_d->initReleaseStringsPool(configuration);

  initEntityTypes(configuration);

  m_d->initRelations(configuration);
  
  /**
    * initialize active medias
    */

  m_d->initMedias(configuration, meds);
  
  m_d->m_mediasData.clear();
  for (map<string,MediaId>::const_iterator it=m_d->m_mediasIds.begin();
       it!=m_d->m_mediasIds.end();
       it++)
  {
    initMediaData(it->second);
  }

  //LINFO << "Mediatic data initialization finished";
//  TimeUtils::logElapsedTime("MediaticDataInit");
}

/** @return the string value of the given numerical media ID */
const std::string& MediaticData::getMediaId(MediaId idNum) const
{
  map<MediaId,string>::const_iterator it=m_d->m_mediasSymbol.find(idNum);
  if (it==m_d->m_mediasSymbol.end())
  {
//     MDATALOGINIT;
//     LERROR << "No media id for " ;//<< (int)idNum;
    throw MediaNotInitialized(idNum);
  }
  return it->second;
}

/** @return the numerical value of the given string media id (3 chars code) */
MediaId MediaticData::getMediaId(const std::string& stringId) const
{
  std::map< std::string, MediaId >::const_iterator it = m_d->m_mediasIds.find(stringId);
  if (it == m_d->m_mediasIds.end())
  {
    MDATALOGINIT;
    LERROR << "MediaId for string " << stringId.c_str() << " is not initialized ! ";
    throw MediaNotInitialized(stringId);
  }
  return it->second;
}

const MediaData& MediaticData::mediaData(MediaId media) const
{
  map<MediaId,MediaData*>::const_iterator it = m_d->m_mediasData.find(media);
  if (it == m_d->m_mediasData.end())
  {
    MDATALOGINIT;
    LERROR << "Media data for id " << (int)media << " is not initialized ! ";
    throw MediaNotInitialized(media);
  }
  return *(it->second);
}

const MediaData& MediaticData::mediaData(const std::string& med) const
{
  MediaId medId=getMediaId(med);
  return mediaData(medId);
}

MediaData& MediaticData::mediaData(MediaId media)
{
  map<MediaId,MediaData*>::iterator it = m_d->m_mediasData.find(media);
  if (it == m_d->m_mediasData.end())
  {
    MDATALOGINIT;
    LERROR << "Media data for id " << (int)media << " is not initialized ! ";
    throw MediaNotInitialized(media);
  }
  return *(it->second);
}

void MediaticData::initXMLParser()
{
//   MDATALOGINIT;
  //LINFO << "XMLParser initialization";

}

void MediaticDataPrivate::initMedias(
  XMLConfigurationFileParser& configParser,
  const std::deque< std::string >& meds)
{
  MDATALOGINIT;
  LDEBUG << "MediaticDataPrivate::initMedias";
  //LINFO << "initializes available medias list";
  if (meds.size()==0)
  {
    try
    {
      m_medias =
        configParser.getModuleGroupListValues("common", "mediaDeclaration", "available");
    }
    catch (NoSuchList& )
    {
      LERROR << "missing 'medias/declaration/available' list in configuration file";
      throw InvalidConfiguration();
    }
  }
  else
  {
    m_medias = meds;
  }

  for (deque<string>::iterator it=m_medias.begin();
       it!=m_medias.end();
       it++)
  {
    try
    {
      MediaId id = static_cast<MediaId>(std::atoi(configParser.getModuleGroupParamValue("common","mediasIds",*it).c_str()));
      LINFO << "media '" << (*it).c_str() << "' has id " << (int)id;
      LDEBUG << (void*)this << " initialize string pool";
      m_stringsPool.insert(std::make_pair(id, new FsaStringsPool()));
  
      m_mediasIds[*it]=id;
      m_mediasSymbol[id]=*it;

      string deffile=configParser.getModuleGroupParamValue("common","mediaDefinitionFiles",*it);
      m_mediaDefinitionFiles[id]= m_configPath+"/"+deffile;
      
    }
    catch (NoSuchList& )
    {
      LERROR << "missing id or definition file for media " << (*it).c_str();
      throw InvalidConfiguration();
    }
  }
}

void MediaticData::initMediaData(MediaId med)
{
  MDATALOGINIT;
  LDEBUG << "MediaticData::initMediaData '" << (int)med << "'";
  map<MediaId,string>::const_iterator it=m_d->m_mediaDefinitionFiles.find(med);
  if (it==m_d->m_mediaDefinitionFiles.end())
  {
    LERROR << "No media definition file for med id " << med;
    throw InvalidConfiguration();
  }
  LDEBUG << "MediaticData::initMediaData Parse MediaConfigurationFile " << (it->second).c_str();
  XMLConfigurationFileParser parser(it->second);

  LDEBUG << "MediaticData::initMediaData Class: " << parser.getModuleGroupParamValue("MediaData","Class","class").c_str();
  MediaData* ldata = MediaData::Factory::getFactory(parser.getModuleGroupParamValue("MediaData","Class","class"))->create(parser.getModuleGroupConfiguration("MediaData","Class"),0);

//   MediaData* ldata=new MediaData();
  m_d->m_mediasData[med]=ldata;
  ldata->initialize(med,m_d->m_resourcesPath,parser);
}



// uint32_t MediaticData::
// getEntityType(const std::string& entityTypeGroup,
//               const MediaId& media,
//               const std::string& name) const
// {
//   return mediaData(media).getEntityCommonType(entityTypeGroup,name);
// }

/** @return the string value of the given relation numerical value */
const std::string& MediaticData::getRelation(uint8_t relation) const
{
  std::map< uint8_t, std::string >::const_iterator it = m_d->m_relTypesNum.find(relation);
  if (it == m_d->m_relTypesNum.end())
  {
    MDATALOGINIT;
    LWARN << "ask getRelation for undefined relation " << (int)relation;
    return m_d->s_undefinedRelation;
  }
  return ( (*it).second );
}

/** @return the numerical value of the given relation name */
uint8_t MediaticData::getRelation(const std::string& relation) const
{
  std::map< std::string, uint8_t >::const_iterator it = m_d->m_relTypes.find(relation);
  if (it == m_d->m_relTypes.end())
  {
    MDATALOGINIT;
    LWARN << "ask getRelation for undefined relation " << relation.c_str();
    return 0;
  }
  return ( (*it).second );
}

void MediaticDataPrivate::initRelations(
  XMLConfigurationFiles::XMLConfigurationFileParser& configParser)
{
  MDATALOGINIT;
  //LINFO << "intialize Relations";
  m_relTypes[s_undefinedRelation]=0;
  m_relTypesNum[0]=s_undefinedRelation;
  
  try {
    const map<string,string>& rels=configParser.getModuleConfiguration("common").getGroupNamed("semanticRelations").getMapAtKey("declaration");
    for (map<string,string>::const_iterator it=rels.begin();
         it!=rels.end();
         it++)
         {
            uint8_t relId=atoi(it->second.c_str());
            LDEBUG << "read relation " << it->first.c_str() << " -> " << (int)relId;
            m_relTypes[it->first]=relId;
            m_relTypesNum[relId]=it->first;
         }
  
  } catch (NoSuchGroup& ) {
    LERROR << "No group 'semanticRelations' in 'common' module of S2-common configuration file";
    throw InvalidConfiguration();
  } catch (NoSuchMap& ) {
    LERROR << "No map 'declaration' in 'semanticRelations' group of S2-common configuration file";
    throw InvalidConfiguration();
  }
}

void MediaticDataPrivate::initReleaseStringsPool(
  XMLConfigurationFiles::XMLConfigurationFileParser& configParser)
{
  MDATALOGINIT;
  LINFO << "initializes the release of strings pool on each text: ";
  m_releaseStringsPool = false;
  try
  {
    std::string release =
      configParser.getModuleGroupParamValue("common", "stringPool", "release") ;
    std::istringstream releaseS(release);
    releaseS >> std::boolalpha >> m_releaseStringsPool;
    LINFO << int(m_releaseStringsPool);
  }
  catch (const NoSuchParam& )
  {
    LWARN << "ReleaseStringsPool parameter not found. Using " << m_releaseStringsPool;
  }
}


//***********************************************************************
// entity types initialization

// internal output function for debug
void printEntities(QsLogging::Logger& logger,
                   const DoubleAccessObjectToIdMap<LimaString,EntityGroupId>& groups,
                   const std::vector<DoubleAccessObjectToIdMap<LimaString,EntityTypeId>* >& types)
{
  const DoubleAccessObjectToIdMap<LimaString,EntityGroupId>::AccessMap& g=groups.getAccessMap();
  for (DoubleAccessObjectToIdMap<LimaString,EntityGroupId>::AccessMap::const_iterator it=g.begin(),
         it_end=g.end(); it!=it_end; it++) {
    LDEBUG << *((*it).first)
       << "(" << (*it).first << ")"
       << "->" << (*it).second;
    if ((*it).second < types.size()) {
      const DoubleAccessObjectToIdMap<LimaString,EntityTypeId>::AccessMap& t=types[(*it).second]->getAccessMap();
      for (DoubleAccessObjectToIdMap<LimaString,EntityTypeId>::AccessMap::const_iterator it2=t.begin(),
             it2_end=t.end(); it2!=it2_end; it2++) {
        LDEBUG 
           << "   " << *((*it2).first)
           << "(" << (*it2).first << ")"
           << "->" << (*it2).second;
      }
    }
  }
  // reverse maps
  const std::vector<const LimaString*>& rg=groups.getReverseAccessMap();
  for (uint32_t i(0);i<rg.size(); i++) {
    LDEBUG 
       << "reverse " << i << "->" << rg[i];
    if (rg[i]!=0) {
      const std::vector<const LimaString*>& rt=types[i]->getReverseAccessMap();
      for (uint32_t j(0);j<rt.size(); j++) {
        LDEBUG
           << "    reverse " << j << "->" << rt[j];
      }
    }
  }
}

void MediaticData::initEntityTypes(XMLConfigurationFileParser& configParser)
{
  MDATALOGINIT;
  LINFO << "initEntityTypes: intialize entities";

  // look at all groups : ModuleConfigurationStructure is a map
  try {
    ModuleConfigurationStructure& moduleConf=configParser.getModuleConfiguration("entities");

    for (ModuleConfigurationStructure::iterator it=moduleConf.begin(),
           it_end=moduleConf.end(); it!=it_end; it++) {
      LDEBUG << "initEntityTypes: looking at group " << (*it).first.c_str();
      
      LimaString groupName=Common::Misc::utf8stdstring2limastring((*it).first);
      EntityGroupId groupId=addEntityGroup(groupName);
      LDEBUG << "initEntityTypes: id is " << groupId;
      
      GroupConfigurationStructure& groupConf=(*it).second;
      
      deque<string>& entityList=groupConf.getListsValueAtKey("entityList");
      for (deque<string>::const_iterator ent=entityList.begin(),
             ent_end=entityList.end(); ent!=ent_end; ent++) {
        
        LimaString entityName=Common::Misc::utf8stdstring2limastring(*ent);
        LDEBUG << "initEntityTypes: add entityType " << (*ent).c_str() << " in group "
               << groupName;
        EntityType type=addEntity(groupId,entityName);
        LDEBUG << "initEntityTypes: type is " << type;
      }
    }
  }
  catch(NoSuchModule& ) {
    MDATALOGINIT;
    LWARN << "no module 'entities' in entity types configuration";
  }
  catch(NoSuchGroup& ) {
    MDATALOGINIT;
    LERROR << "missing group in entity types configuration";
    throw InvalidConfiguration();
  }
  catch(NoSuchList& ) {
    MDATALOGINIT;
    LERROR << "missing list 'entityList' in entity types configuration";
    throw InvalidConfiguration();
  }
  if (logger.loggingLevel()<=QsLogging::DebugLevel) {
    printEntities(logger,m_d->m_entityGroups,m_d->m_entityTypes);
  }
}

EntityGroupId MediaticData::addEntityGroup(const LimaString& groupName)
{
  EntityGroupId groupId= m_d->m_entityGroups.insert(groupName);
  // insert may have created new element or not
  if (static_cast<uint32_t>(groupId) >= m_d->m_entityTypes.size()) {
    m_d->m_entityTypes.push_back(new MediaticDataPrivate::EntityTypeMap());
  }
  return groupId;
}

EntityType MediaticData::addEntity(EntityGroupId groupId, const LimaString& entityName)
{
  if (static_cast<uint32_t>(groupId)>=m_d->m_entityTypes.size()) {
    MDATALOGINIT;
    LERROR << "unknown entity group id " << groupId;
    throw LimaException();
  }
  EntityTypeId typeId= m_d->m_entityTypes[groupId]->insert(entityName);
  return EntityType(typeId,groupId);
}

EntityType MediaticData::addEntity(const LimaString& groupName,
          const LimaString& entityName)
{
  EntityGroupId groupId=getEntityGroupId(groupName);
  return addEntity(groupId,entityName);
}

// entity types accessors
EntityType MediaticData::getEntityType(const LimaString& entityName) const
{
  int i=entityName.indexOf(m_d->s_entityTypeNameSeparator);
  if (i==-1) {
    MDATALOGINIT;
    LERROR << "missing group name in entity name " << entityName;
    throw LimaException();
  }
  LimaString groupName = entityName.left(i);
  LimaString name = entityName.mid(i+m_d->s_entityTypeNameSeparator.length());
  return getEntityType(getEntityGroupId(groupName),name);
}

EntityType MediaticData::getEntityType(const EntityGroupId groupId,
              const LimaString& entityName) const
{
  if (static_cast<uint32_t>(groupId)>=m_d->m_entityTypes.size()) {
    MDATALOGINIT;
    LERROR << "unknown entity group id " << groupId;
    throw LimaException();
  }
  try {
    return EntityType(m_d->m_entityTypes[groupId]->get(entityName),groupId);
  }
  catch(LimaException& ) {
    MDATALOGINIT;
    LWARN << "Unknown entity type "
          << entityName;
    throw;
  }
}
 
EntityGroupId MediaticData::getEntityGroupId(const LimaString& groupName) const
{
  try {
    return m_d->m_entityGroups.get(groupName);
  }
  catch(LimaException& ) {
    MDATALOGINIT;
    LERROR << "Unknown entity group "
           << groupName;
    throw;
  }
}

LimaString MediaticData::getEntityName(const EntityType& type) const
{
  MDATALOGINIT;
  LDEBUG << "MediaticData::getEntityName("  << type << ")";
  if (logger.loggingLevel()<=QsLogging::DebugLevel) {
    printEntities(logger,m_d->m_entityGroups,m_d->m_entityTypes);
  }
  if (type.getGroupId()==0) {
    MDATALOGINIT;
    LERROR << "invalid entity group id " << type.getGroupId() << " in entity " /*<< type */<< LENDL;
    throw LimaException();
  }
  if (static_cast<uint32_t>(type.getGroupId())>=m_d->m_entityTypes.size()) {
    MDATALOGINIT;
    LERROR << "unknown entity group id " << type.getGroupId() << " in entity " /*<< type */<< LENDL;
    throw LimaException();
  }
  try {
    // return m_entityTypes[type.getGroupId()]->get(type.getTypeId());
    return
      m_d->m_entityGroups.get(type.getGroupId())+
      m_d->s_entityTypeNameSeparator+
      m_d->m_entityTypes[type.getGroupId()]->get(type.getTypeId());
  }
  catch(LimaException& ) {
    MDATALOGINIT;
    LERROR << "Cannot find name of entity type "
           /*<< type */<< LENDL;
    throw;
  }
}

const LimaString& MediaticData::getEntityGroupName(EntityGroupId id) const
{
  try {
    return m_d->m_entityGroups.get(id);
  }
  catch(LimaException& ) {
    MDATALOGINIT;
    LERROR << "Cannot find name of entity group "
           << id;
    throw;
  }
}

void MediaticData::writeEntityTypes(std::ostream& file) const
{
  MDATALOGINIT;

  const DoubleAccessObjectToIdMap<LimaString,EntityGroupId>::AccessMap& groups=m_d->m_entityGroups.getAccessMap();
  Misc::writeCodedInt(file,groups.size());
  for (DoubleAccessObjectToIdMap<LimaString,EntityGroupId>::AccessMap::const_iterator 
         it=groups.begin(),it_end=groups.end();it!=it_end; it++) {
    LDEBUG << "writeEntityTypes: write group id " << (*it).second;
    Misc::writeCodedInt(file,(*it).second);
    LDEBUG << "writeEntityTypes: write group name " << *((*it).first);
    Misc::writeUTF8StringField(file,*((*it).first));
    LDEBUG  << "writeEntityTypes: after group name file at " << file.tellp();
    // write entities for this group
    const DoubleAccessObjectToIdMap<LimaString,EntityTypeId>::AccessMap& entities=m_d->m_entityTypes[(*it).second]->getAccessMap();
    LDEBUG << "writeEntityTypes: write nb entities: " << entities.size();
    Misc::writeCodedInt(file,entities.size());
    LDEBUG  << "writeEntityTypes: after write nb entities, file at " << file.tellp();
    for (DoubleAccessObjectToIdMap<LimaString,EntityTypeId>::AccessMap::const_iterator 
           it2=entities.begin(),it2_end=entities.end();it2!=it2_end; it2++) {
      LDEBUG << "writeEntityTypes: write entity id " << (*it2).second;
      Misc::writeCodedInt(file,(*it2).second);
      LDEBUG  << "writeEntityTypes: after write entity id file at " << file.tellp();
      LDEBUG << "writeEntityTypes: write entity name " << *((*it2).first);
      Misc::writeUTF8StringField(file,*((*it2).first));
    LDEBUG  << "writeEntityTypes: after write entity name file at " << file.tellp();
    }
  }
  if (logger.loggingLevel()<=QsLogging::DebugLevel) {
    printEntities(logger,m_d->m_entityGroups,m_d->m_entityTypes);
  }
}

void MediaticData::readEntityTypes(std::istream& file,
                std::map<EntityGroupId,EntityGroupId>& entityGroupIdMapping,
                std::map<EntityType,EntityType>& entityTypeMapping)
{
  MDATALOGINIT;
  uint64_t size=Misc::readCodedInt(file);
  // read group names
  for (uint64_t i(0); i<size; i++) {
    EntityGroupId groupId= static_cast<EntityGroupId>(Misc::readCodedInt(file));
    LDEBUG << "readEntityTypes: read group id " << groupId;
    LimaString groupName;
    Misc::readUTF8StringField(file,groupName);
    LDEBUG << "readEntityTypes: read group name " << groupName;
    EntityGroupId newGroupId=addEntityGroup(groupName);
    entityGroupIdMapping[groupId]=newGroupId;
    LDEBUG << "readEntityTypes: added group id mapping " << groupId << "->" << newGroupId;

    // read entities for this group
    uint64_t nbEntities=Misc::readCodedInt(file);
    
    for (uint64_t j(0);j<nbEntities; j++) {
      EntityTypeId typeId = static_cast<EntityTypeId>(Misc::readCodedInt(file));
      LDEBUG << "readEntityTypes: read entity id " << typeId;
      LimaString entityName;
      Misc::readUTF8StringField(file,entityName);
      LDEBUG << "readEntityTypes: read entity name " << entityName;
      if (logger.loggingLevel()<=QsLogging::DebugLevel) {
        printEntities(logger,m_d->m_entityGroups,m_d->m_entityTypes);
      }
      EntityType oldTypeId(typeId,groupId);
      EntityType newTypeId=addEntity(newGroupId,entityName);
      LDEBUG << "readEntityTypes: added entity type mapping " << oldTypeId << "->" << newTypeId;
      LDEBUG << "before insert " << entityTypeMapping.size();
      entityTypeMapping.insert(make_pair(oldTypeId,newTypeId));
      LDEBUG << "after insert " << entityTypeMapping.size();
      if (logger.loggingLevel()<=QsLogging::DebugLevel) {
        LDEBUG << "readEntityTypes: type mapping is";
        std::ostringstream oss;
        for (std::map<EntityType,EntityType>::const_iterator
               it=entityTypeMapping.begin(),it_end=entityTypeMapping.end();
         it!=it_end; it++) {
          oss << (*it).first << " -> " << (*it).second << std::endl;
        }
        LDEBUG << oss.str();
      }
    }
  }
  if (logger.loggingLevel()<=QsLogging::DebugLevel) {
    printEntities(logger,m_d->m_entityGroups,m_d->m_entityTypes);
  }
}

const FsaStringsPool& MediaticData::stringsPool(MediaId med) const
{
  map<MediaId,FsaStringsPool*>::const_iterator it=m_d->m_stringsPool.find(med);
  if (it == m_d->m_stringsPool.end())
  {
    MDATALOGINIT;
    LERROR << "no available string pool for media " << (int)med;
    throw MediaNotInitialized(med);
  }
  return *(it->second);
}

FsaStringsPool& MediaticData::stringsPool(MediaId med)
{
  map<MediaId,FsaStringsPool*>::const_iterator it=m_d->m_stringsPool.find(med);
  if (it == m_d->m_stringsPool.end())
  {
    MDATALOGINIT;
    LERROR << "no available string pool for media " << (int)med;
    throw MediaNotInitialized(med);
  }
  return *(it->second);

  
}


MediaticDataPrivate::MediaticDataPrivate() :
    m_mediasIds(),
    m_mediasSymbol(),
    m_mediaDefinitionFiles(),
    m_mediasData(),
    m_entityGroups(),
    m_entityTypes(),
    m_relTypes(),
    m_relTypesNum(),
    m_stringsPool(),
    m_releaseStringsPool(false),
    m_resourcesPath(),
    m_configPath()
{
  // null first element
  m_entityTypes.push_back( static_cast<EntityTypeMap*>(0));
}

MediaticDataPrivate::~MediaticDataPrivate()
{
  m_mediasIds.clear();
  for (map<MediaId, MediaData*>::const_iterator it=m_mediasData.begin();
       it!=m_mediasData.end();
       it++)
  {
    delete it->second;
  }
}

const LimaString& MediaticData::getEntityTypeNameSeparator() const
{
  return m_d->s_entityTypeNameSeparator;
}

} // closing namespace MediaticData
} // closing namespace Common
} // closing namespace Lima
