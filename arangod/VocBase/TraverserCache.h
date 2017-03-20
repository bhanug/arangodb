////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2017-2017 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Michael Hackstein
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_VOC_BASE_TRAVERSER_CACHE_H
#define ARANGOD_VOC_BASE_TRAVERSER_CACHE_H 1

#include "Basics/Common.h"

namespace arangodb {
class ManagedDocumentResult;

namespace cache {
class Cache;
class Finding;
}

namespace transaction {
class Methods;
}

namespace velocypack {
class Builder;
class Slice;
}

namespace aql {
  struct AqlValue;
}
  
namespace traverser {
class TraverserCache {

  public:
   TraverserCache(transaction::Methods* trx, ManagedDocumentResult* mmdr);

   ~TraverserCache();

   //////////////////////////////////////////////////////////////////////////////
   /// @brief Inserts the real document stored within the token
   ///        into the given builder.
   ///        The document will be taken from the hash-cache.
   ///        If it is not cached it will be looked up in the StorageEngine
   //////////////////////////////////////////////////////////////////////////////

   void insertIntoResult(arangodb::velocypack::Slice const& idString,
                         arangodb::velocypack::Builder& builder);
  
   aql::AqlValue fetchAqlResult(arangodb::velocypack::Slice const& idString);

   //////////////////////////////////////////////////////////////////////////////
   /// @brief Throws the document referenced by the token into the filter
   ///        function and returns it result.
   ///        The document will be taken from the hash-cache.
   ///        If it is not cached it will be looked up in the StorageEngine
   //////////////////////////////////////////////////////////////////////////////

   bool validateFilter(
       arangodb::velocypack::Slice const& idString,
       std::function<bool(arangodb::velocypack::Slice const&)> filterFunc);
  
  size_t getAndResetReadDocuments() {
    size_t tmp = _readDocuments;
    _readDocuments = 0;
    return tmp;
  }

  private:

   //////////////////////////////////////////////////////////////////////////////
   /// @brief Lookup a document by token in the cache.
   ///        As long as finding is retained it is guaranteed that the result
   ///        stays valid. Finding should not be retained very long, if it is
   ///        needed for longer, copy the value.
   //////////////////////////////////////////////////////////////////////////////
   cache::Finding lookup(arangodb::velocypack::Slice const& idString);

   //////////////////////////////////////////////////////////////////////////////
   /// @brief Lookup a document from the database and insert it into the cache.
   ///        The Slice returned here is only valid until the NEXT call of this
   ///        function.
   //////////////////////////////////////////////////////////////////////////////

   arangodb::velocypack::Slice lookupInCollection(
       arangodb::velocypack::Slice const& idString);

  private:

   //////////////////////////////////////////////////////////////////////////////
   /// @brief The hash-cache that saves documents found in the Database
   //////////////////////////////////////////////////////////////////////////////
   std::shared_ptr<arangodb::cache::Cache> _cache;

   //////////////////////////////////////////////////////////////////////////////
   /// @brief Reusable ManagedDocumentResult that temporarily takes
   ///        responsibility for one document.
   //////////////////////////////////////////////////////////////////////////////
   ManagedDocumentResult *_mmdr;

   //////////////////////////////////////////////////////////////////////////////
   /// @brief Transaction to access data, This class is NOT responsible for it.
   //////////////////////////////////////////////////////////////////////////////
   arangodb::transaction::Methods* _trx;
  
   size_t _readDocuments;
  std::set<std::string> _found;
};

}
}

#endif