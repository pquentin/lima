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
 * @file       recognizerData.h
 * @author     besancon (besanconr@zoe.cea.fr)
 * @date       Tue Jan 25 2005
 * @version    $Id$
 * copyright   Copyright (C) 2005-2012 by CEA LIST
 * Project     s2lp
 *
 * @brief      this class contains data useful for recognizer application
 *
 *
 ***********************************************************************/

#ifndef RECOGNIZERDATA_H
#define RECOGNIZERDATA_H

#include "AutomatonExport.h"
#include "common/ProcessUnitFramework/AnalysisContent.h"
#include "linguisticProcessing/core/Automaton/recognizerMatch.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/AnalysisGraph.h"

namespace Lima {
namespace LinguisticProcessing {
namespace ApplyRecognizer {

#define APPRLOGINIT  LOGINIT("LP::ApplyRecognizer")

// class to store results of the application of the recognizer
class LIMA_AUTOMATON_EXPORT RecognizerResultData :
  public AnalysisData,
  public std::vector<std::vector< Automaton::RecognizerMatch > >
{
 public:
  RecognizerResultData(const std::string& sourceGraph);
  RecognizerResultData(const RecognizerResultData&);
  ~RecognizerResultData();
  RecognizerResultData& operator = (const RecognizerResultData&);

  void insert(const Automaton::RecognizerMatch& m,
              const uint64_t sentenceId=0);

  inline const std::string& getGraphId() const { return m_graphId; }

 private:
  std::string m_graphId;
};

// class to store information used during the application of the
// Recognizer (not used afterwards)
class LIMA_AUTOMATON_EXPORT RecognizerData : public AnalysisData
{
 public:
  RecognizerData();
  virtual ~RecognizerData();

  // deal with removing vertices
  // (when alternative create by the rules is the only one kept)
  bool matchOnRemovedVertices(const Automaton::RecognizerMatch& result) const;
  void storeVerticesToRemove(const Automaton::RecognizerMatch& result,
                             LinguisticGraph* graph);
  void clearVerticesToRemove() { m_verticesToRemove.clear(); }
  void removeVertices(AnalysisContent& analysis) const;

  void setEdgeToBeRemoved(AnalysisContent& analysis, LinguisticGraphEdge e);
  void clearEdgesToRemove() {m_edgesToRemove.clear();}
  void removeEdges(AnalysisContent& analysis);
  bool isEdgeToBeRemoved(LinguisticGraphVertex s, LinguisticGraphVertex t) const;
  /** @brief remove edges linked to vertices that have no path between @p from
   * and @p to excepted those in @p storedEdges
   * @pre All vertices that have to be kept must be reachable from @p from and @p to
   * should be reachable from them
   * @param analysis @b IN/OUT <I>AnalysisContent&</I> the analysis containing
   * the graph to modify
   * @param from @b IN <I>LinguisticGraphVertex</I> the vertex from which to
   * start the search
   * @param to @b IN <I>LinguisticGraphVertex</I> the vertex on which to
   * finish the search
   * @param storedEdges @b IN <I>std::set< LinguisticGraphEdge >&</I>The edges
   * to keep anyway
  */
  void clearUnreachableVertices(AnalysisContent& analysis,
                                LinguisticGraphVertex from,
                                LinguisticGraphVertex to,
                                std::set< std::pair<LinguisticGraphVertex, LinguisticGraphVertex > >& storedEdges
                               );
  void clearUnreachableVertices(AnalysisContent& analysis,
                                LinguisticGraphVertex from);
  
  void setResultData(RecognizerResultData* data);
  void deleteResultData();
  void addResult(const Automaton::RecognizerMatch& result);
  void nextSentence();
  bool hasStoredResults() const { return (! m_resultData->empty()); }

  inline const std::string& getGraphId() const { return m_resultData->getGraphId(); }

  inline const std::set< LinguisticGraphVertex >& getNextVertices() const { return m_nextVertices; }
  inline std::set< LinguisticGraphVertex >& getNextVertices() { return m_nextVertices; }
  inline void setNextVertex(LinguisticGraphVertex v) { m_nextVertices.insert(v); }

private:
  
  RecognizerData(const RecognizerData&);
  RecognizerData& operator = (const RecognizerData&);

  // dictionary code (needed to create alternative vertex (deal with
  // linguistic properties)
  std::set<LinguisticGraphVertex> m_verticesToRemove;
  std::set< std::pair<LinguisticGraphVertex, LinguisticGraphVertex> > m_edgesToRemove;
  RecognizerResultData* m_resultData;
  uint64_t m_currentSentence;
  std::set< LinguisticGraphVertex > m_nextVertices;
};

} // end namespace
} // end namespace
} // end namespace

#endif
