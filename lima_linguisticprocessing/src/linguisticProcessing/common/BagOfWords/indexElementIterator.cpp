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
/************************************************************************
 *
 * @file       indexElementIterator.cpp
 * @author     Besancon Romaric (romaric.besancon@cea.fr)
 * @date       Tue Feb  7 2006
 * copyright   Copyright (C) 2006-2012 by CEA LIST
 * 
 ***********************************************************************/

#include "bowNamedEntity.h"
#include "bowText.h"
#include "indexElement.h"

#include "linguisticProcessing/common/BagOfWords/BoWRelation.h"
#include "indexElementIterator.h"
#include "common/FsaAccess/AbstractLexiconIdGenerator.h"
#include "defaultIdGenerator.h"
#include "common/Data/strwstrtools.h"
#include "common/MediaticData/EntityType.h"
#include <limits.h>
#include <algorithm>

using namespace std;
using namespace Lima::Common::Misc;

namespace Lima {
namespace Common {
namespace BagOfWords {


class IndexElementIteratorPrivate
{
  friend class IndexElementIterator;
  
  IndexElementIteratorPrivate(const BoWText& bowText,
                       AbstractLexiconIdGenerator* idAccessor=0,
                       const uint64_t maxCompoundSize=0,
                       const uint64_t nbMaxPartialCompounds=1000);
  IndexElementIteratorPrivate(const IndexElementIteratorPrivate& ieip);
  ~IndexElementIteratorPrivate();

  typedef std::deque<IndexElement> IndexElementQueue;

  // members
  BoWText::const_iterator m_iterator;
  BoWText::const_iterator m_iteratorEnd;
  IndexElementQueue m_partQueue;
  uint64_t m_maxSizeQueue;
  uint64_t m_maxCompoundSize;
  AbstractLexiconIdGenerator* m_idGenerator;

  // private functions

  // add in queue
  // (return false if size of queue becomes greater than max)
  bool addInPartQueue(const uint64_t id,
                      const LimaString& word,
                      const uint64_t cat,
                      const uint64_t position,
                      const uint64_t length,
                      const Common::MediaticData::EntityType neType,
                      const uint64_t reType);
  void getPositionLengthList(const std::vector<uint64_t>& structure,
                             Misc::PositionLengthList& poslenlist) const;
  // add in queue: only used for compound elements
  bool addInPartQueue(const IndexElement& newElement);

  void storePartsInQueue(const BoWToken* token, const uint64_t rel);

  bool addPartElementsInQueue(const BoWToken* token,
                              std::pair<std::vector<uint64_t>, uint64_t> & ids_rels,
                              const uint64_t rel);
  bool addCombinedPartsInQueue(const std::vector<std::pair<std::vector<uint64_t>, uint64_t> >& partIds_Rels,
                               const uint64_t head,
                               const Common::MediaticData::EntityType neType,
                               std::pair<std::vector<uint64_t>, uint64_t>& ids_rels,
                               std::vector<uint64_t>& structure,
                               std::vector<uint64_t>& relations,
                               const uint64_t i);

};

IndexElementIteratorPrivate::IndexElementIteratorPrivate(const BoWText& bowText,
                     AbstractLexiconIdGenerator* idGenerator,
                     const uint64_t maxCompoundSize,
                     const uint64_t nbMaxPartialCompounds):
m_iterator(bowText.begin()),
m_iteratorEnd(bowText.end()),
m_partQueue(),
m_maxSizeQueue(nbMaxPartialCompounds),
m_maxCompoundSize(maxCompoundSize),
m_idGenerator(idGenerator)
{
  if (m_idGenerator==0) {
    m_idGenerator=new DefaultIdGenerator(
       AbstractLexiconIdGeneratorInformer::getInstance() );
  }
  if (m_maxCompoundSize==0) {
    // if 0, no limitation => set it to UINT_MAX to avoid testing 0
    // each time
    m_maxCompoundSize=UINT_MAX;
  }
}

IndexElementIteratorPrivate::IndexElementIteratorPrivate(const IndexElementIteratorPrivate& ieip):
m_iterator(ieip.m_iterator),
m_iteratorEnd(ieip.m_iteratorEnd),
m_partQueue(ieip.m_partQueue),
m_maxSizeQueue(ieip.m_maxSizeQueue),
m_maxCompoundSize(ieip.m_maxCompoundSize)
{
  m_idGenerator=new DefaultIdGenerator(
    AbstractLexiconIdGeneratorInformer::getInstance() );
  *m_idGenerator = *ieip.m_idGenerator;
}

IndexElementIteratorPrivate::~IndexElementIteratorPrivate()
{
  delete m_idGenerator;
}


//***********************************************************************
// constructors and destructors
IndexElementIterator::IndexElementIterator(const BoWText& bowText,
                     AbstractLexiconIdGenerator* idGenerator,
                     const uint64_t maxCompoundSize,
                     const uint64_t nbMaxPartialCompounds):
    m_d(new IndexElementIteratorPrivate(bowText, idGenerator, maxCompoundSize, nbMaxPartialCompounds))
{
}

IndexElementIterator::IndexElementIterator(const IndexElementIterator& iei):
    m_d(new IndexElementIteratorPrivate(*iei.m_d))
{
}

IndexElementIterator::~IndexElementIterator()
{
  delete m_d;
}

//***********************************************************************

bool IndexElementIterator::isAtEnd() const
{
  return (m_d->m_iterator == m_d->m_iteratorEnd);
}

//**********************************************************************
// get current element ("dereference" iterator)
//**********************************************************************
// getting parts is done in this function (rather than in ++ function):
// which means that is a ++ is done before calling a getElement on 
// a complex token, no parts will be explored
IndexElement IndexElementIterator::getElement()
{
  if (m_d->m_partQueue.empty())
  {
    if (m_d->m_iterator==m_d->m_iteratorEnd)
    { // at end
      return IndexElement(0,LimaString()); // empty element has id 0
    }
    else
    {
      switch ((*m_d->m_iterator)->getType())
      {
      case BOW_TOKEN:
      {
        uint64_t id=m_d->m_idGenerator->getId((*m_d->m_iterator)->getIndexString());
        return IndexElement(id,(*m_d->m_iterator)->getLemma(),
                            (*m_d->m_iterator)->getCategory(),
                            (*m_d->m_iterator)->getPosition(),
                            (*m_d->m_iterator)->getLength());
      }
      case BOW_TERM:
      case BOW_NAMEDENTITY:
        // element itself will be stored in queue as part
        m_d->storePartsInQueue(*m_d->m_iterator,0);
        return m_d->m_partQueue.front();
      case BOW_NOTYPE:
        ;
      }
    }
  }
  else {
    return m_d->m_partQueue.front();
  }
  return IndexElement(0,LimaString()); // empty element has id 0
}

//**********************************************************************
// operator ++
//**********************************************************************
IndexElementIterator& IndexElementIterator::operator++() 
{
  if (m_d->m_partQueue.empty()) {
    if (m_d->m_iterator!=m_d->m_iteratorEnd) {
      m_d->m_iterator++;
    }
  }
  else {
    m_d->m_partQueue.pop_front();
    if (m_d->m_partQueue.empty()) { // finished for the parts of this token
        m_d->m_iterator++;
    }
  }
  return *this;
}

// postfix ++ operator
IndexElementIterator IndexElementIterator::operator++(int) {
  IndexElementIterator it = *this;
  ++(*this);
  return it;
}

//**********************************************************************
// helper functions for iterator
//**********************************************************************
bool IndexElementIteratorPrivate::addInPartQueue(const uint64_t id,
               const LimaString& word,
               const uint64_t cat,
               const uint64_t position,
               const uint64_t length,
               const Common::MediaticData::EntityType neType,
               const uint64_t reType)
{
  if (m_partQueue.size() >= m_maxSizeQueue) {
    BOWLOGINIT;
    LWARN << "size of queue exceeded" << LENDL; 
    return false;
  }
  
  m_partQueue.push_back(IndexElement(id,word,cat,position,length,neType,reType));
//   BOWLOGINIT;
//   LDEBUG << "add in part queue " << id << ":" 
//          << Common::Misc::limastring2utf8stdstring(word)
//          << ";size of queue=" << m_partQueue.size()
//          << LENDL;
  return true;
}

void IndexElementIteratorPrivate::getPositionLengthList(const std::vector<uint64_t>& structure,
                      PositionLengthList& poslenlist) const
{
  // update position/length list for structure
  // use previous elements in queue
  std::vector<uint64_t>::const_iterator 
    it=structure.begin(),it_end=structure.end();
  for (std::deque<IndexElement>::const_iterator 
         elt=m_partQueue.begin(),elt_end=m_partQueue.end();
       elt!=elt_end; elt++) {
    if ((*elt).getId()==*it) {
      const PositionLengthList& p=(*elt).getPositionLengthList();
      poslenlist.insert(poslenlist.end(),p.begin(),p.end());
      it++;
      if (it==it_end) {
        break;
      }
    }
  }
  // sort positions
  std::sort(poslenlist.begin(),poslenlist.end());
}


bool IndexElementIteratorPrivate::addInPartQueue(const IndexElement& newElement)
{
  if (m_partQueue.size() >= m_maxSizeQueue) {
    BOWLOGINIT;
    LWARN << "size of queue exceeded" << LENDL; 
    return false;
  }
  
  m_partQueue.push_back(newElement);
//   BOWLOGINIT;
//   if (logger.isDebugEnabled()) {
//     ostringstream oss;
//     for (vector<uint64_t>::const_iterator it=structure.begin(),
//            it_end=structure.end(); it!=it_end; it++) {
//       oss << *it << ";";
//     }
//     LDEBUG << "add in part queue " << id << ":" 
//            << oss.str()
//            << ";size of queue=" << m_partQueue.size()
//            << LENDL;
//   }
  return true;
}


void IndexElementIteratorPrivate::storePartsInQueue(const BoWToken* token,const uint64_t rel)
{
  pair<vector<uint64_t>, uint64_t> tokenIds;
  if (!addPartElementsInQueue(token,tokenIds,rel)) {
    BOWLOGINIT;
    LWARN << "Token contain too many subparts (some are ignored): " 
      << Common::Misc::limastring2utf8stdstring(token->getLemma()) << LENDL;
  }
}

bool IndexElementIteratorPrivate::addPartElementsInQueue(const BoWToken* token,
                       pair<vector<uint64_t>, uint64_t>& ids_rel,
                       uint64_t rel) 
{
//   BOWLOGINIT;
//    LDEBUG << "addPartElementsInQueue:" << Common::Misc::limastring2utf8stdstring(token->getLemma()) << ", rel=" << rel << LENDL;

  Common::MediaticData::EntityType neType;
  

  switch (token->getType())
  {
  case BOW_TOKEN:
  {
    // simple token : get Id and push in parts
    uint64_t id=m_idGenerator->getId(token->getIndexString());
    ids_rel=make_pair(vector<uint64_t>(1,id),rel);

    LimaString lemma=token->getLemma();
    if (lemma.size()==0) {
        lemma=token->getInflectedForm();
    }
    
    return addInPartQueue(id,lemma,
                          token->getCategory(),
                          token->getPosition(),
                          token->getLength(),
                          neType,
                          rel);
  }
  case BOW_NAMEDENTITY: 
    neType=static_cast<const BoWNamedEntity*>(token)->getNamedEntityType();
    break;
  case BOW_TERM:
    break;
  case BOW_NOTYPE:
    break;
  }

  // is a complex token
  const BoWComplexToken* complexToken=
    static_cast<const BoWComplexToken*>(token);
  
  if (complexToken==0) {
    BOWLOGINIT;
    LERROR << "failed to convert BoWText element in complex token" << LENDL;
    return false;
  }

  if (complexToken->size() == 1) { 
    // only one part, do not get into it
    // (for instance, named entity with one element)
    // push simple token in parts 
    uint64_t id=m_idGenerator->getId(token->getIndexString());
    ids_rel=make_pair(vector<uint64_t>(1,id),rel);

    LimaString lemma=token->getLemma();
    if (lemma.size()==0) {
        lemma=token->getInflectedForm();
    }
    return addInPartQueue(id,lemma,
                          token->getCategory(),
                          token->getPosition(),
                          token->getLength(),
                          neType,
                          rel);
  }

  ids_rel=make_pair(vector<uint64_t>(0),rel);
  uint64_t nbParts=complexToken->getParts().size();
  uint64_t head=complexToken->getHead();
  vector<pair<vector<uint64_t>, uint64_t> > partIdsRels(nbParts);
  for (uint64_t i=0; i<nbParts; i++) {
    pair<vector<uint64_t>, uint64_t>& thisPartIdsRels=partIdsRels[i];
    uint64_t relType;
    BoWRelation * relation=(complexToken->getParts()[i]).getBoWRelation();
    if (relation !=0 ) relType=relation->getSynType(); else  relType=0; 
    if (!addPartElementsInQueue(complexToken->getParts()[i].getBoWToken(),thisPartIdsRels,relType)) {
      return false;
    }
    if (i==head) {
      // add ids of the head
        ids_rel.first.insert(ids_rel.first.end(),thisPartIdsRels.first.begin(),thisPartIdsRels.first.end());
    }
  }
  // add ids for combined parts
  vector<uint64_t> structure; //current structure in recursive function
  vector<uint64_t> relations; //current relations in recursive function
  if (!addCombinedPartsInQueue(partIdsRels,head,neType,ids_rel,structure,relations,0)) {
    return false;
  }
  return true;
}

/** 
 * this function is recursive to build all composed elements that contains
 * the head and all or parts of the extensions, for all possible values (ids) 
 * of head and extensions
 * 
 * @param partIdsRels : the possible ids of each part, plus one relation per part
 * @param head : the position of the head in the parts
 * @param ids : the id list in which new ids are added for combined element
 * @param structure : the current structure
 * @param i : the current part looked at
 * 
 * @return 
 */
bool IndexElementIteratorPrivate::addCombinedPartsInQueue(const std::vector<std::pair<std::vector<uint64_t>, uint64_t> >& partIdsRels,
                        const uint64_t head,
                        const Common::MediaticData::EntityType neType,
                        std::pair<std::vector<uint64_t>, uint64_t>& ids_rel,
                        std::vector<uint64_t>& structure,
                        std::vector<uint64_t>& relations,
                        const uint64_t i)
{
//    BOWLOGINIT;
//    if (logger.isDebugEnabled()) {
//      ostringstream oss;
//      for (vector<pair<uint64_t,uint64_t> >::const_iterator it=structure.begin(),
//             it_end=structure.end(); it!=it_end; it++) {
//        oss << (*it).first << "/" << (*it).second << ";";
//      }
//      LDEBUG << "addCombinedPartsInQueue: nb parts=" << partIdsRels.size() 
//             << ", head=" << head << ", current=" << i << ",structure=" << oss.str() << LENDL;
//    }
  
  if (i>=partIdsRels.size()) {
    if (structure.size() == 1) {
      //just the head: is already in queue
      return true;
    }
    // build indexElement before getting the id : allow to have the
    // true size of compound (trick: use PositionLengthList to have
    // the size: number of leaves of the structure), and to avoid
    // compute the id if size is more than maxCompoundSize
    IndexElement compoundElement(0,structure,relations,neType,0); // relType is not used
    getPositionLengthList(structure,compoundElement.getPositionLengthList());
    if (compoundElement.getPositionLengthList().size() > m_maxCompoundSize) {
      // compound larger than allowed, do not add it in parts, but
      // return true anyway (false is reserved for queue size
      // overflow)
      return true;
    }
    // at end of parts => add current structure
    
    uint64_t id=m_idGenerator->getId(structure);
//    BOWLOGINIT;
//    LDEBUG << "IndexElementIterator: get id from generator " << id << LENDL;
    compoundElement.setId(id);
    if (!addInPartQueue(compoundElement)) {
      return false;
    }
    ids_rel.first.push_back(id);
    return true;
  }

  // add possible at end of structure and recursive call
  for (std::vector<uint64_t>::const_iterator 
         it=partIdsRels[i].first.begin(),it_end=partIdsRels[i].first.end();
       it!=it_end; it++) {
    structure.push_back(*it);
    relations.push_back(partIdsRels[i].second);
    if (!addCombinedPartsInQueue(partIdsRels,head,neType,ids_rel,structure,relations,i+1)) {
      return false;
    }
    structure.pop_back();
    relations.pop_back();
  }
  // if head, stop here: current iterator is head, hence always added
  // otherwise, recursive call without current iterator (that is an
  // extension)
  if (i!=head) {
    if (!addCombinedPartsInQueue(partIdsRels,head,neType,ids_rel,structure,relations,i+1)) {
      return false;
    }
  }
  return true;
}

} // end namespace
} // end namespace
} // end namespace
